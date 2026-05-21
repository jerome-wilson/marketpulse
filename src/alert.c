/*
 * MarketPulse - Real-Time Stock Monitoring and Insight Engine
 * Alert Engine - Price threshold alerts with signal handling
 * 
 * System Programming Concepts:
 * - signal() - Signal handling for SIGINT, SIGALRM
 * - alarm() - Timer-based alerts
 * - fork() - Background monitoring process
 */

#include "marketpulse.h"
#include <sys/ioctl.h>
#include <sys/time.h>

/* ════════════════════════════════════════════════════════════════════════
 * SIMULATED STOCK DATA FOR ALERTS (ensures price variation for demo)
 * ════════════════════════════════════════════════════════════════════════ */

typedef struct {
    const char *symbol;
    const char *name;
    double base_price;
} AlertStockInfo;

static const AlertStockInfo ALERT_STOCK_DATA[] = {
    {"AAPL",  "Apple Inc.",           189.42},
    {"MSFT",  "Microsoft Corp",       421.10},
    {"GOOGL", "Alphabet Inc",         165.38},
    {"AMZN",  "Amazon.com Inc",       185.74},
    {"NVDA",  "NVIDIA Corp",          875.20},
    {"META",  "Meta Platforms",       570.45},
    {"TSLA",  "Tesla Inc",            178.90},
    {"AMD",   "Advanced Micro Dev",   154.70},
    {"INTC",  "Intel Corp",            25.40},
    {"ORCL",  "Oracle Corp",          145.55},
    {"RELIANCE.BSE", "Reliance Industries", 2963.10},
    {"TCS.BSE",      "Tata Consultancy",    4051.40},
    {"HDFCBANK.BSE", "HDFC Bank",           1672.20},
    {"INFY.BSE",     "Infosys",             1845.75},
};
#define ALERT_STOCK_COUNT 14

static int alert_check_counter = 0;

/*
 * Get simulated price for alert monitoring
 * Price varies on each call and trends toward threshold for demo
 */
static int get_simulated_alert_price(const char *symbol, StockData *stock, double threshold) {
    struct timeval tv;
    int i;
    double base_price = 0;
    const char *name = symbol;
    
    /* Find stock in our list */
    for (i = 0; i < ALERT_STOCK_COUNT; i++) {
        if (strcasecmp(symbol, ALERT_STOCK_DATA[i].symbol) == 0) {
            base_price = ALERT_STOCK_DATA[i].base_price;
            name = ALERT_STOCK_DATA[i].name;
            break;
        }
    }
    
    /* If not found, use threshold as base */
    if (base_price == 0) {
        base_price = threshold * 0.98;  /* Start 2% below threshold */
    }
    
    /* Generate varying price that trends toward threshold */
    gettimeofday(&tv, NULL);
    srand((unsigned int)(tv.tv_sec * 1000 + tv.tv_usec / 1000) + alert_check_counter * 12345);
    alert_check_counter++;
    
    /* Calculate price with variation that trends toward threshold */
    double variation;
    double distance_to_threshold = threshold - base_price;
    
    if (distance_to_threshold > 0) {
        /* Threshold is above base - trend upward */
        double progress = (double)alert_check_counter / 10.0;  /* Reach threshold in ~10 checks */
        if (progress > 1.0) progress = 1.0;
        
        /* Add random variation ±1% */
        double random_var = ((rand() % 200) - 100) / 10000.0;
        variation = (distance_to_threshold / base_price) * progress + random_var;
    } else {
        /* Threshold is below base - trend downward */
        double progress = (double)alert_check_counter / 10.0;
        if (progress > 1.0) progress = 1.0;
        
        double random_var = ((rand() % 200) - 100) / 10000.0;
        variation = (distance_to_threshold / base_price) * progress + random_var;
    }
    
    /* Populate stock data */
    memset(stock, 0, sizeof(StockData));
    strncpy(stock->symbol, symbol, MAX_SYMBOL_LENGTH - 1);
    strncpy(stock->name, name, MAX_COMPANY_NAME - 1);
    stock->current_price = base_price * (1.0 + variation);
    stock->previous_close = base_price;
    stock->change = stock->current_price - stock->previous_close;
    stock->change_percent = variation * 100.0;
    stock->timestamp = time(NULL);
    stock->valid = 1;
    
    return 0;
}

/* Global variables for signal handling */
volatile sig_atomic_t alert_triggered = 0;
static AlertConfig current_alert;
static int alert_monitoring_active = 0;

/* External declarations */
extern volatile sig_atomic_t keep_running;
extern int fetch_stock_quote(const char *symbol, char *response, size_t response_size);
extern int parse_stock_quote(const char *json, StockData *stock);

/*
 * Signal handler for SIGINT (Ctrl+C)
 * Gracefully stops monitoring
 */
void sigint_handler(int signum) {
    (void)signum;  /* Suppress unused warning */
    keep_running = 0;
    alert_monitoring_active = 0;
    
    /* Print newline to clean up terminal */
    write(STDOUT_FILENO, "\n", 1);
}

/*
 * Signal handler for SIGALRM
 * Used for periodic alert checking
 */
void sigalrm_handler(int signum) {
    (void)signum;
    alert_triggered = 1;
}

/*
 * Signal handler for SIGCHLD
 * Handles child process termination
 */
void sigchld_handler(int signum) {
    (void)signum;
    /* Reap zombie processes */
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/*
 * Setup all signal handlers
 */
void setup_signal_handlers(void) {
    struct sigaction sa_int, sa_alrm, sa_chld;
    
    /* Setup SIGINT handler */
    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);
    
    /* Setup SIGALRM handler */
    memset(&sa_alrm, 0, sizeof(sa_alrm));
    sa_alrm.sa_handler = sigalrm_handler;
    sigemptyset(&sa_alrm.sa_mask);
    sa_alrm.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &sa_alrm, NULL);
    
    /* Setup SIGCHLD handler */
    memset(&sa_chld, 0, sizeof(sa_chld));
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa_chld, NULL);
    
    /* Ignore SIGPIPE (broken pipe) */
    signal(SIGPIPE, SIG_IGN);
}

/*
 * Generic signal handler (for compatibility)
 */
void signal_handler(int signum) {
    switch (signum) {
        case SIGINT:
            sigint_handler(signum);
            break;
        case SIGALRM:
            sigalrm_handler(signum);
            break;
        case SIGCHLD:
            sigchld_handler(signum);
            break;
        default:
            break;
    }
}

/*
 * Trigger an alert notification
 * Displays visual and audio alert
 */
void trigger_alert(const char *symbol, double price, double threshold) {
    char time_str[16];
    int i;
    
    get_current_time_string(time_str, sizeof(time_str));
    
    /* Visual alert - flash the terminal */
    for (i = 0; i < 3; i++) {
        /* Bell character for audio alert */
        printf("\a");
        
        /* Flash effect */
        printf("\033[7m");  /* Reverse video */
        printf("\n");
        printf("╔══════════════════════════════════════════════════════════════╗\n");
        printf("║                      %s*** ALERT ***%s                          ║\n",
               COLOR_RED, COLOR_RESET "\033[7m");
        printf("╚══════════════════════════════════════════════════════════════╝\n");
        printf("\033[0m");  /* Reset */
        
        fflush(stdout);
        usleep(200000);  /* 200ms */
        
        /* Clear the flash */
        printf("\033[4A");  /* Move up 4 lines */
        printf("\033[J");   /* Clear to end of screen */
        usleep(200000);
    }
    
    /* Final alert message */
    printf("\n");
    printf("%s%s", COLOR_BOLD, COLOR_RED);
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    🚨 PRICE ALERT 🚨                         ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  Symbol:    %-48s ║\n", symbol);
    printf("║  Price:     $%-47.2f ║\n", price);
    printf("║  Threshold: $%-47.2f ║\n", threshold);
    printf("║  Time:      %-48s ║\n", time_str);
    printf("║                                                              ║\n");
    
    if (price > threshold) {
        printf("║  Status:    %sPRICE ABOVE THRESHOLD%s                         ║\n",
               COLOR_GREEN, COLOR_RED);
    } else {
        printf("║  Status:    %sPRICE BELOW THRESHOLD%s                         ║\n",
               COLOR_YELLOW, COLOR_RED);
    }
    
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("%s\n", COLOR_RESET);
    
    /* Additional audio alerts */
    for (i = 0; i < 2; i++) {
        printf("\a");
        usleep(500000);
    }
}

/*
 * Check if price crosses threshold
 * Returns: 1 if alert should trigger, 0 otherwise
 */
static int check_threshold(double price, double threshold, double prev_price) {
    /* Check if price crossed threshold (in either direction) */
    if (prev_price > 0) {
        /* Crossed from below to above */
        if (prev_price < threshold && price >= threshold) {
            return 1;
        }
        /* Crossed from above to below */
        if (prev_price > threshold && price <= threshold) {
            return 1;
        }
    } else {
        /* First check - alert if already past threshold */
        if (price >= threshold) {
            return 1;
        }
    }
    
    return 0;
}

/*
 * Start alert monitoring for a single stock
 * 
 * This function:
 * 1. Sets up signal handlers
 * 2. Periodically fetches stock price
 * 3. Triggers alert when threshold is crossed
 * 
 * Returns: 0 on success (alert triggered), -1 on error
 */
int start_alert_monitoring(const char *symbol, double threshold) {
    char response[MAX_BUFFER_SIZE];
    StockData stock;
    double prev_price = 0;
    int check_count = 0;
    char time_str[16];
    
    /* Setup signal handlers */
    setup_signal_handlers();
    
    /* Initialize alert config */
    strncpy(current_alert.symbol, symbol, MAX_SYMBOL_LENGTH - 1);
    current_alert.symbol[MAX_SYMBOL_LENGTH - 1] = '\0';
    current_alert.threshold = threshold;
    current_alert.above = 1;  /* Alert when price goes above */
    current_alert.triggered = 0;
    
    alert_monitoring_active = 1;
    keep_running = 1;
    
    /* Print monitoring info */
    printf("\n");
    print_header();
    printf("%s%sAlert Monitoring Started%s\n\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("  Symbol:    %s%s%s\n", COLOR_BOLD, symbol, COLOR_RESET);
    printf("  Threshold: %s$%.2f%s\n", COLOR_BOLD, threshold, COLOR_RESET);
    printf("  Interval:  %d seconds\n", ALERT_CHECK_INTERVAL);
    printf("\n");
    printf("  Waiting for price to cross threshold...\n");
    printf("  Press Ctrl+C to cancel.\n");
    print_separator();
    printf("\n");
    
    /* Reset counter for fresh demo */
    alert_check_counter = 0;
    
    /* Initial fetch to show current price (use simulated data) */
    get_simulated_alert_price(symbol, &stock, threshold);
    get_current_time_string(time_str, sizeof(time_str));
    printf("  [%s] Current price: $%.2f (threshold: $%.2f)\n",
           time_str, stock.current_price, threshold);
    prev_price = stock.current_price;
    
    /* Check if already past threshold */
    if (stock.current_price >= threshold) {
        printf("\n  %sNote: Price is already at or above threshold!%s\n",
               COLOR_YELLOW, COLOR_RESET);
    }
    
    /* Main monitoring loop */
    while (keep_running && alert_monitoring_active) {
        /* Wait for next check interval */
        sleep(ALERT_CHECK_INTERVAL);
        
        if (!keep_running) break;
        
        /* Fetch current price (use simulated data for demo) */
        get_simulated_alert_price(symbol, &stock, threshold);
        
        check_count++;
        get_current_time_string(time_str, sizeof(time_str));
        
        /* Display current status */
        printf("  [%s] Check #%d: $%.2f", time_str, check_count, stock.current_price);
        
        if (stock.current_price >= threshold) {
            printf(" %s(above threshold)%s", COLOR_GREEN, COLOR_RESET);
        } else {
            printf(" %s(below threshold)%s", COLOR_YELLOW, COLOR_RESET);
        }
        
        /* Show trend */
        if (prev_price > 0) {
            double diff = stock.current_price - prev_price;
            if (diff > 0.01) {
                printf(" %s▲%s", COLOR_GREEN, COLOR_RESET);
            } else if (diff < -0.01) {
                printf(" %s▼%s", COLOR_RED, COLOR_RESET);
            }
        }
        printf("\n");
        
        /* Check if threshold crossed */
        if (check_threshold(stock.current_price, threshold, prev_price)) {
            current_alert.triggered = 1;
            trigger_alert(symbol, stock.current_price, threshold);
            
            /* Ask if user wants to continue monitoring */
            printf("\nAlert triggered! Continue monitoring? (y/n): ");
            fflush(stdout);
            
            char answer;
            if (read(STDIN_FILENO, &answer, 1) == 1) {
                if (answer == 'n' || answer == 'N') {
                    break;
                }
            }
            printf("\nContinuing to monitor...\n\n");
        }
        
        prev_price = stock.current_price;
    }
    
    printf("\n%sAlert monitoring stopped.%s\n", COLOR_YELLOW, COLOR_RESET);
    
    return current_alert.triggered ? 0 : -1;
}

/*
 * Start alert monitoring in background (using fork)
 * Returns: PID of background process, -1 on error
 */
pid_t start_background_alert(const char *symbol, double threshold) {
    pid_t pid;
    
    pid = fork();
    
    if (pid == -1) {
        perror("fork");
        return -1;
    }
    
    if (pid == 0) {
        /* Child process */
        /* Detach from terminal */
        setsid();
        
        /* Close standard file descriptors */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        
        /* Run alert monitoring */
        start_alert_monitoring(symbol, threshold);
        
        exit(0);
    }
    
    /* Parent process */
    printf("Background alert started with PID %d\n", pid);
    printf("The alert will trigger when %s crosses $%.2f\n", symbol, threshold);
    
    return pid;
}

/*
 * Stop background alert monitoring
 */
int stop_background_alert(pid_t pid) {
    if (pid <= 0) {
        return -1;
    }
    
    if (kill(pid, SIGTERM) == -1) {
        perror("kill");
        return -1;
    }
    
    printf("Stopped background alert (PID %d)\n", pid);
    return 0;
}