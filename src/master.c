/*
 * MarketPulse - Real-Time Stock Monitoring and Insight Engine
 * Master Process - Multi-Level Process Architecture Controller
 * 
 * System Programming Concepts:
 * - fork() - Create child processes
 * - waitpid() - Monitor child processes
 * - kill() - Send signals to processes
 * - select() - I/O multiplexing
 * - Signal handling - SIGCHLD, SIGUSR1, SIGUSR2, SIGTERM
 * 
 * Process Hierarchy:
 *   Master (this)
 *     ├── Worker 1 (AAPL)
 *     ├── Worker 2 (MSFT)
 *     ├── Worker 3 (GOOGL)
 *     └── ...
 */

#include "marketpulse.h"
#include "ipc.h"
#include "daemon.h"
#include "logger.h"
#include "config.h"
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>

/* Worker process information */
typedef struct {
    pid_t pid;
    char symbol[16];
    int pipe_fd[2];         /* Pipe for communication */
    int status;             /* 0=stopped, 1=running, 2=error */
    int restart_count;
    time_t last_restart;
} WorkerInfo;

/* Master process state */
static struct {
    SharedMemory *shm;
    int semid;
    WorkerInfo workers[MAX_WORKERS];
    int worker_count;
    volatile int running;
    volatile int reload_config;
    volatile int debug_mode;
    AppConfig config;
} master_state;

/* Signal flags */
static volatile sig_atomic_t got_sigchld = 0;
static volatile sig_atomic_t got_sigusr1 = 0;
static volatile sig_atomic_t got_sigusr2 = 0;
static volatile sig_atomic_t got_sigterm = 0;

/* Forward declarations */
static void master_signal_handler(int signum);
static pid_t spawn_worker(const char *symbol, int index);
static void reap_children(void);
static void restart_dead_workers(void);
static int check_worker_health(void);

/* External declarations */
extern int fetch_stock_quote(const char *symbol, char *response, size_t response_size);
extern int parse_stock_quote(const char *json, StockData *stock);

/*
 * Master signal handler
 */
static void master_signal_handler(int signum) {
    switch (signum) {
        case SIGCHLD:
            got_sigchld = 1;
            break;
        case SIGUSR1:
            got_sigusr1 = 1;  /* Reload config */
            break;
        case SIGUSR2:
            got_sigusr2 = 1;  /* Toggle debug */
            break;
        case SIGTERM:
        case SIGINT:
            got_sigterm = 1;
            break;
    }
}

/*
 * Setup signal handlers for master process
 */
static void setup_master_signals(void) {
    struct sigaction sa;
    
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = master_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    sigaction(SIGCHLD, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    
    /* Ignore SIGPIPE */
    signal(SIGPIPE, SIG_IGN);
}

/*
 * Worker process main function
 * This runs in the child process after fork()
 */
static void worker_main(const char *symbol, int worker_index) {
    SharedMemory *shm;
    int semid;
    char response[MAX_BUFFER_SIZE];
    StockData stock;
    SharedStockData shared_stock;
    int consecutive_errors = 0;
    
    /* Attach to shared memory */
    shm = shm_attach_worker();
    if (shm == NULL) {
        fprintf(stderr, "Worker %s: Failed to attach shared memory\n", symbol);
        exit(1);
    }
    semid = get_global_semid();
    
    /* Register worker */
    shm_register_worker(shm, semid, getpid(), symbol);
    
    log_info("Worker started: PID=%d, Symbol=%s", getpid(), symbol);
    
    /* Main worker loop */
    while (!shm->shutdown_flag) {
        /* Update heartbeat */
        shm_update_worker_heartbeat(shm, semid, getpid());
        
        /* Fetch stock data */
        if (fetch_stock_quote(symbol, response, sizeof(response)) == 0) {
            if (parse_stock_quote(response, &stock) == 0) {
                /* Convert to shared format */
                memset(&shared_stock, 0, sizeof(shared_stock));
                strncpy(shared_stock.symbol, symbol, sizeof(shared_stock.symbol) - 1);
                shared_stock.current_price = stock.current_price;
                shared_stock.previous_close = stock.previous_close;
                shared_stock.change = stock.change;
                shared_stock.change_percent = stock.change_percent;
                shared_stock.high = stock.high;
                shared_stock.low = stock.low;
                shared_stock.open_price = stock.open;
                shared_stock.valid = 1;
                shared_stock.worker_pid = getpid();
                
                /* Update shared memory */
                shm_update_stock(shm, semid, &shared_stock);
                stats_increment_requests(shm, semid, 1);
                
                consecutive_errors = 0;
            } else {
                consecutive_errors++;
                stats_increment_requests(shm, semid, 0);
            }
        } else {
            consecutive_errors++;
            stats_increment_requests(shm, semid, 0);
        }
        
        /* Check for too many errors */
        if (consecutive_errors >= 5) {
            log_error("Worker %s: Too many consecutive errors, exiting", symbol);
            break;
        }
        
        /* Sleep before next fetch */
        sleep(master_state.config.refresh_interval > 0 ? 
              master_state.config.refresh_interval : DEFAULT_REFRESH_INTERVAL);
    }
    
    /* Cleanup */
    shm_unregister_worker(shm, semid, getpid());
    log_info("Worker stopped: PID=%d, Symbol=%s", getpid(), symbol);
    
    exit(0);
}

/*
 * Spawn a worker process for a stock symbol
 */
static pid_t spawn_worker(const char *symbol, int index) {
    pid_t pid;
    
    pid = fork();
    
    if (pid == -1) {
        log_error("Failed to fork worker for %s: %s", symbol, strerror(errno));
        return -1;
    }
    
    if (pid == 0) {
        /* Child process */
        worker_main(symbol, index);
        exit(0);  /* Should not reach here */
    }
    
    /* Parent process */
    master_state.workers[index].pid = pid;
    strncpy(master_state.workers[index].symbol, symbol, 
            sizeof(master_state.workers[index].symbol) - 1);
    master_state.workers[index].status = 1;
    master_state.workers[index].last_restart = time(NULL);
    
    log_info("Spawned worker: PID=%d, Symbol=%s", pid, symbol);
    
    return pid;
}

/*
 * Reap terminated child processes
 */
static void reap_children(void) {
    pid_t pid;
    int status;
    int i;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        /* Find worker and mark as stopped */
        for (i = 0; i < master_state.worker_count; i++) {
            if (master_state.workers[i].pid == pid) {
                master_state.workers[i].status = 0;
                master_state.workers[i].pid = 0;
                
                if (WIFEXITED(status)) {
                    log_info("Worker %s (PID %d) exited with status %d",
                             master_state.workers[i].symbol, pid, WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    log_warn("Worker %s (PID %d) killed by signal %d",
                             master_state.workers[i].symbol, pid, WTERMSIG(status));
                }
                break;
            }
        }
    }
}

/*
 * Restart dead workers (fault tolerance)
 */
static void restart_dead_workers(void) {
    int i;
    time_t now = time(NULL);
    
    for (i = 0; i < master_state.worker_count; i++) {
        if (master_state.workers[i].status == 0 && 
            master_state.workers[i].symbol[0] != '\0') {
            
            /* Rate limit restarts (max 1 per 10 seconds) */
            if (now - master_state.workers[i].last_restart < 10) {
                continue;
            }
            
            /* Max restart attempts */
            if (master_state.workers[i].restart_count >= 5) {
                log_error("Worker %s exceeded max restart attempts",
                          master_state.workers[i].symbol);
                continue;
            }
            
            log_info("Restarting worker %s (attempt %d)",
                     master_state.workers[i].symbol,
                     master_state.workers[i].restart_count + 1);
            
            spawn_worker(master_state.workers[i].symbol, i);
            master_state.workers[i].restart_count++;
        }
    }
}

/*
 * Check worker health via heartbeats
 */
static int check_worker_health(void) {
    pid_t dead_pids[MAX_WORKERS];
    int dead_count;
    int i, j;
    
    dead_count = shm_get_dead_workers(master_state.shm, master_state.semid, 
                                       dead_pids, MAX_WORKERS);
    
    for (i = 0; i < dead_count; i++) {
        /* Find and kill unresponsive worker */
        for (j = 0; j < master_state.worker_count; j++) {
            if (master_state.workers[j].pid == dead_pids[i]) {
                log_warn("Worker %s (PID %d) unresponsive, killing",
                         master_state.workers[j].symbol, dead_pids[i]);
                kill(dead_pids[i], SIGKILL);
                break;
            }
        }
    }
    
    return dead_count;
}

/*
 * Handle SIGUSR1 - Reload configuration
 */
static void handle_reload_config(void) {
    log_info("Reloading configuration...");
    
    /* Reload config file */
    if (config_load(CONFIG_FILE, &master_state.config) == 0) {
        log_info("Configuration reloaded successfully");
        
        /* Update shared memory flags */
        master_state.shm->reload_config_flag = 0;
        
        /* Send message to workers */
        IPCMessage msg;
        msg.type = MSG_CONFIG_RELOAD;
        msg.sender_pid = getpid();
        msg.timestamp = time(NULL);
        msg_send(master_state.shm, master_state.semid, &msg);
    } else {
        log_error("Failed to reload configuration");
    }
}

/*
 * Handle SIGUSR2 - Toggle debug mode
 */
static void handle_toggle_debug(void) {
    master_state.debug_mode = !master_state.debug_mode;
    master_state.shm->debug_mode = master_state.debug_mode;
    
    log_info("Debug mode %s", master_state.debug_mode ? "enabled" : "disabled");
    
    if (master_state.debug_mode) {
        shm_print_status(master_state.shm);
    }
}

/*
 * Graceful shutdown
 */
static void shutdown_workers(void) {
    int i;
    
    log_info("Initiating graceful shutdown...");
    
    /* Set shutdown flag */
    master_state.shm->shutdown_flag = 1;
    
    /* Send SIGTERM to all workers */
    for (i = 0; i < master_state.worker_count; i++) {
        if (master_state.workers[i].pid > 0) {
            log_info("Sending SIGTERM to worker %s (PID %d)",
                     master_state.workers[i].symbol, master_state.workers[i].pid);
            kill(master_state.workers[i].pid, SIGTERM);
        }
    }
    
    /* Wait for workers to exit (with timeout) */
    int timeout = 10;
    while (timeout > 0) {
        reap_children();
        
        /* Check if all workers stopped */
        int all_stopped = 1;
        for (i = 0; i < master_state.worker_count; i++) {
            if (master_state.workers[i].pid > 0) {
                all_stopped = 0;
                break;
            }
        }
        
        if (all_stopped) break;
        
        sleep(1);
        timeout--;
    }
    
    /* Force kill remaining workers */
    for (i = 0; i < master_state.worker_count; i++) {
        if (master_state.workers[i].pid > 0) {
            log_warn("Force killing worker %s (PID %d)",
                     master_state.workers[i].symbol, master_state.workers[i].pid);
            kill(master_state.workers[i].pid, SIGKILL);
        }
    }
    
    /* Final reap */
    reap_children();
}

/*
 * Master process main loop
 * Uses select() for event-driven architecture
 */
int master_run(char symbols[][MAX_SYMBOL_LENGTH], int symbol_count) {
    int i;
    struct timeval tv;
    fd_set readfds;
    int max_fd = 0;
    time_t last_health_check = 0;
    
    /* Initialize master state */
    memset(&master_state, 0, sizeof(master_state));
    master_state.running = 1;
    
    /* Load configuration */
    config_set_defaults(&master_state.config);
    config_load(CONFIG_FILE, &master_state.config);
    
    /* Initialize shared memory */
    master_state.shm = shm_init_master();
    if (master_state.shm == NULL) {
        log_error("Failed to initialize shared memory");
        return -1;
    }
    master_state.semid = get_global_semid();
    
    /* Setup signal handlers */
    setup_master_signals();
    
    log_info("Master process started: PID=%d", getpid());
    log_info("Monitoring %d stocks", symbol_count);
    
    /* Add stocks to shared memory and spawn workers */
    for (i = 0; i < symbol_count && i < MAX_WORKERS; i++) {
        shm_add_stock(master_state.shm, master_state.semid, symbols[i]);
        spawn_worker(symbols[i], i);
        master_state.worker_count++;
        
        /* Small delay between spawns */
        usleep(100000);
    }
    
    /* Main event loop using select() */
    while (master_state.running && !got_sigterm) {
        /* Handle signals */
        if (got_sigchld) {
            got_sigchld = 0;
            reap_children();
            restart_dead_workers();
        }
        
        if (got_sigusr1) {
            got_sigusr1 = 0;
            handle_reload_config();
        }
        
        if (got_sigusr2) {
            got_sigusr2 = 0;
            handle_toggle_debug();
        }
        
        /* Setup select() */
        FD_ZERO(&readfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        /* Wait for events */
        int ret = select(max_fd + 1, &readfds, NULL, NULL, &tv);
        
        if (ret == -1) {
            if (errno == EINTR) continue;
            log_error("select() failed: %s", strerror(errno));
            break;
        }
        
        /* Periodic health check */
        time_t now = time(NULL);
        if (now - last_health_check >= 30) {
            check_worker_health();
            last_health_check = now;
        }
        
        /* Process messages from workers */
        IPCMessage msg;
        while (msg_receive(master_state.shm, master_state.semid, &msg) == 0) {
            switch (msg.type) {
                case MSG_ALERT_TRIGGER:
                    log_info("ALERT: %s crossed threshold! Price: $%.2f",
                             msg.symbol, msg.value);
                    stats_increment_alerts(master_state.shm, master_state.semid);
                    break;
                    
                case MSG_WORKER_ERROR:
                    log_error("Worker error: %s - %s", msg.symbol, msg.text);
                    break;
                    
                default:
                    break;
            }
        }
    }
    
    /* Shutdown */
    shutdown_workers();
    
    /* Cleanup shared memory */
    shm_cleanup(master_state.shm);
    
    log_info("Master process exiting");
    
    return 0;
}

/*
 * Start master in daemon mode
 */
int master_start_daemon(char symbols[][MAX_SYMBOL_LENGTH], int symbol_count) {
    pid_t pid;
    
    /* First fork */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    if (pid > 0) {
        /* Parent exits */
        printf("MarketPulse daemon started with PID %d\n", pid);
        exit(0);
    }
    
    /* Create new session */
    if (setsid() < 0) {
        perror("setsid");
        return -1;
    }
    
    /* Second fork (prevent acquiring terminal) */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    if (pid > 0) {
        exit(0);
    }
    
    /* Change working directory */
    chdir(WORKING_DIR);
    
    /* Close standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    /* Redirect to /dev/null */
    open("/dev/null", O_RDONLY);  /* stdin */
    open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);  /* stdout */
    dup2(1, 2);  /* stderr = stdout */
    
    /* Create PID file */
    create_pid_file(getpid());
    
    /* Initialize logger for daemon mode */
    logger_init(LOG_FILE, LOG_INFO);
    
    /* Run master */
    return master_run(symbols, symbol_count);
}

/*
 * Get master process status
 */
void master_print_status(void) {
    if (master_state.shm != NULL) {
        shm_print_status(master_state.shm);
    }
}