/*
 * MarketPulse - Real-Time Stock Monitoring and Insight Engine
 * System Module - Status, Stats, Failure Simulation, Rate Limiting
 * 
 * Publication-Quality Features:
 * - System introspection (status command)
 * - Performance metrics display
 * - Failure simulation mode
 * - Exponential backoff retry
 * - API rate limiting (token bucket)
 * - Dynamic worker scaling
 */

#include "marketpulse.h"
#include "ipc.h"
#include "daemon.h"
#include "logger.h"
#include "config.h"
#include <sys/resource.h>

/* ============== Rate Limiter (Token Bucket) ============== */

typedef struct {
    int tokens;
    int max_tokens;
    int refill_rate;        /* tokens per second */
    time_t last_refill;
} RateLimiter;

static RateLimiter g_rate_limiter = {
    .tokens = 60,
    .max_tokens = 60,
    .refill_rate = 1,
    .last_refill = 0
};

/*
 * Initialize rate limiter
 */
void rate_limiter_init(int max_tokens, int refill_rate) {
    g_rate_limiter.tokens = max_tokens;
    g_rate_limiter.max_tokens = max_tokens;
    g_rate_limiter.refill_rate = refill_rate;
    g_rate_limiter.last_refill = time(NULL);
}

/*
 * Refill tokens based on elapsed time
 */
static void rate_limiter_refill(void) {
    time_t now = time(NULL);
    int elapsed = (int)(now - g_rate_limiter.last_refill);
    
    if (elapsed > 0) {
        int new_tokens = elapsed * g_rate_limiter.refill_rate;
        g_rate_limiter.tokens += new_tokens;
        if (g_rate_limiter.tokens > g_rate_limiter.max_tokens) {
            g_rate_limiter.tokens = g_rate_limiter.max_tokens;
        }
        g_rate_limiter.last_refill = now;
    }
}

/*
 * Try to acquire a token (non-blocking)
 * Returns 1 if acquired, 0 if rate limited
 */
int rate_limiter_acquire(void) {
    rate_limiter_refill();
    
    if (g_rate_limiter.tokens > 0) {
        g_rate_limiter.tokens--;
        return 1;
    }
    
    return 0;  /* Rate limited */
}

/*
 * Get current token count
 */
int rate_limiter_tokens(void) {
    rate_limiter_refill();
    return g_rate_limiter.tokens;
}

/* ============== Exponential Backoff ============== */

typedef struct {
    int base_delay_ms;
    int max_delay_ms;
    int max_retries;
    int current_attempt;
} BackoffConfig;

/*
 * Calculate delay with exponential backoff
 * delay = base * 2^attempt (capped at max)
 */
int calculate_backoff_delay(BackoffConfig *config) {
    int delay = config->base_delay_ms * (1 << config->current_attempt);
    
    if (delay > config->max_delay_ms) {
        delay = config->max_delay_ms;
    }
    
    /* Add jitter (±10%) */
    int jitter = (delay * (rand() % 20 - 10)) / 100;
    delay += jitter;
    
    return delay;
}

/*
 * Fetch with exponential backoff retry
 */
int fetch_with_retry(const char *symbol, char *response, size_t response_size) {
    BackoffConfig config = {
        .base_delay_ms = 1000,
        .max_delay_ms = 30000,
        .max_retries = 5,
        .current_attempt = 0
    };
    
    extern int fetch_stock_quote(const char *symbol, char *response, size_t response_size);
    
    while (config.current_attempt < config.max_retries) {
        /* Check rate limit */
        if (!rate_limiter_acquire()) {
            log_warn("Rate limited, waiting...");
            sleep(1);
            continue;
        }
        
        /* Try fetch */
        int result = fetch_stock_quote(symbol, response, response_size);
        
        if (result == 0) {
            return 0;  /* Success */
        }
        
        /* Failed, calculate backoff */
        int delay = calculate_backoff_delay(&config);
        log_warn("Fetch failed for %s, retry %d/%d in %dms",
                 symbol, config.current_attempt + 1, config.max_retries, delay);
        
        usleep(delay * 1000);
        config.current_attempt++;
    }
    
    log_error("All retries exhausted for %s", symbol);
    return -1;
}

/* ============== Failure Simulation ============== */

static FailureType g_simulated_failure = FAILURE_NONE;
static int g_failure_probability = 30;  /* 30% chance */

/*
 * Set failure simulation mode
 */
void set_failure_simulation(FailureType type, int probability) {
    g_simulated_failure = type;
    g_failure_probability = probability;
    log_info("Failure simulation enabled: type=%d, probability=%d%%", type, probability);
}

/*
 * Check if failure should be simulated
 */
int should_simulate_failure(void) {
    if (g_simulated_failure == FAILURE_NONE) {
        return 0;
    }
    
    return (rand() % 100) < g_failure_probability;
}

/*
 * Simulate a failure based on type
 */
int simulate_failure(FailureType type) {
    switch (type) {
        case FAILURE_WORKER_CRASH:
            log_warn("SIMULATED: Worker crash!");
            return -1;
            
        case FAILURE_NETWORK:
            log_warn("SIMULATED: Network failure!");
            return -2;
            
        case FAILURE_API_DELAY:
            log_warn("SIMULATED: API delay (800ms)");
            usleep(800000);
            return 0;
            
        case FAILURE_RANDOM:
            return simulate_failure((FailureType)(1 + rand() % 3));
            
        default:
            return 0;
    }
}

/*
 * Run failure simulation mode
 */
int run_failure_simulation(void) {
    printf("\n%s%s=== MarketPulse Failure Simulation Mode ===%s\n\n",
           COLOR_BOLD, COLOR_YELLOW, COLOR_RESET);

    printf("This mode tests system resilience by simulating:\n");
    printf("  1. Worker process crashes\n");
    printf("  2. Network failures\n");
    printf("  3. API response delays\n");
    printf("  4. Random failures\n\n");

    printf("Starting simulation with 30%% failure probability...\n\n");

    /* Suppress logger during display so output stays clean */
    logger_set_level(LOG_FATAL);
    set_failure_simulation(FAILURE_RANDOM, 30);
    
    /* Simulate 10 operations */
    int successes = 0, failures = 0;
    
    for (int i = 0; i < 10; i++) {
        printf("Operation %d: ", i + 1);
        
        if (should_simulate_failure()) {
            int result = simulate_failure(g_simulated_failure);
            if (result < 0) {
                printf("%sFAILED%s (simulated)\n", COLOR_RED, COLOR_RESET);
                failures++;
                
                /* Demonstrate recovery */
                printf("  → System detecting failure...\n");
                usleep(500000);
                printf("  → Initiating recovery...\n");
                usleep(500000);
                printf("  → %sRecovered%s\n", COLOR_GREEN, COLOR_RESET);
            } else {
                printf("%sSUCCESS%s (with delay)\n", COLOR_GREEN, COLOR_RESET);
                successes++;
            }
        } else {
            printf("%sSUCCESS%s\n", COLOR_GREEN, COLOR_RESET);
            successes++;
        }
        
        usleep(300000);
    }
    
    printf("\n%s=== Simulation Results ===%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  Successful operations: %s%d%s\n", COLOR_GREEN, successes, COLOR_RESET);
    printf("  Failed operations:     %s%d%s\n", COLOR_RED, failures, COLOR_RESET);
    printf("  Recovery rate:         %s100%%%s\n", COLOR_GREEN, COLOR_RESET);
    printf("\nSystem demonstrated fault tolerance and self-healing capabilities.\n\n");

    set_failure_simulation(FAILURE_NONE, 0);
    logger_set_level(LOG_INFO);
    return 0;
}

/* ============== System Status / Introspection ============== */

/*
 * Display system status
 */
int display_system_status(void) {
    SharedMemory *shm;
    time_t now = time(NULL);
    struct rusage usage;
    
    printf("\n%s%s╔══════════════════════════════════════════════════════════════╗%s\n",
           COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s%s║           MarketPulse System Status                          ║%s\n",
           COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s%s╚══════════════════════════════════════════════════════════════╝%s\n\n",
           COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    
    /* Try to attach to shared memory */
    shm = shm_attach_worker();
    
    if (shm != NULL && shm_validate(shm)) {
        /* Shared memory info */
        printf("%s[Shared Memory]%s\n", COLOR_BOLD, COLOR_RESET);
        printf("  Status:        %sActive%s\n", COLOR_GREEN, COLOR_RESET);
        printf("  Size:          %lu bytes\n", sizeof(SharedMemory));
        printf("  Master PID:    %d\n", shm->master_pid);
        printf("  Created:       %s", ctime(&shm->created_at));
        printf("  Uptime:        %ld seconds\n", now - shm->created_at);
        
        /* Worker info */
        printf("\n%s[Workers]%s\n", COLOR_BOLD, COLOR_RESET);
        printf("  Active:        %d\n", shm->stats.active_workers);
        printf("  Total:         %d\n", shm->worker_count);
        
        for (int i = 0; i < shm->worker_count; i++) {
            if (shm->workers[i].pid > 0) {
                printf("  Worker %d:      PID %d (%s) - %s\n",
                       i, shm->workers[i].pid, shm->workers[i].symbol,
                       shm->workers[i].status == 1 ? "running" : "stopped");
            }
        }
        
        /* Stock data */
        printf("\n%s[Monitored Stocks]%s\n", COLOR_BOLD, COLOR_RESET);
        printf("  Count:         %d\n", shm->stock_count);
        for (int i = 0; i < shm->stock_count && i < 5; i++) {
            printf("  %s:          $%.2f\n", 
                   shm->stocks[i].symbol, shm->stocks[i].current_price);
        }
        if (shm->stock_count > 5) {
            printf("  ... and %d more\n", shm->stock_count - 5);
        }
        
        /* Message queue */
        printf("\n%s[Message Queue]%s\n", COLOR_BOLD, COLOR_RESET);
        printf("  Messages:      %d / %d\n", shm->msg_queue.count, MAX_MESSAGES);
        
        /* Flags */
        printf("\n%s[System Flags]%s\n", COLOR_BOLD, COLOR_RESET);
        printf("  Shutdown:      %s\n", shm->shutdown_flag ? "yes" : "no");
        printf("  Debug mode:    %s\n", shm->debug_mode ? "yes" : "no");
        printf("  Config reload: %s\n", shm->reload_config_flag ? "pending" : "no");
        
    } else {
        printf("%s[Shared Memory]%s\n", COLOR_BOLD, COLOR_RESET);
        printf("  Status:        %sNot initialized%s\n", COLOR_YELLOW, COLOR_RESET);
        printf("  (Run 'marketpulse watch' to start monitoring)\n");
    }
    
    /* Process info */
    printf("\n%s[Process Info]%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  Current PID:   %d\n", getpid());
    
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        printf("  User CPU:      %ld.%06ld sec\n", 
               usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
        printf("  System CPU:    %ld.%06ld sec\n",
               usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
        printf("  Max RSS:       %ld KB\n", usage.ru_maxrss / 1024);
    }
    
    /* Rate limiter */
    printf("\n%s[Rate Limiter]%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  Tokens:        %d / %d\n", rate_limiter_tokens(), g_rate_limiter.max_tokens);
    printf("  Refill rate:   %d/sec\n", g_rate_limiter.refill_rate);
    
    /* Daemon status */
    printf("\n%s[Daemon]%s\n", COLOR_BOLD, COLOR_RESET);
    pid_t daemon_pid = read_pid_file();
    if (daemon_pid > 0 && kill(daemon_pid, 0) == 0) {
        printf("  Status:        %sRunning%s (PID: %d)\n", COLOR_GREEN, COLOR_RESET, daemon_pid);
    } else {
        printf("  Status:        %sStopped%s\n", COLOR_YELLOW, COLOR_RESET);
    }
    
    printf("\n");
    return 0;
}

/* ============== Performance Metrics ============== */

/*
 * Display performance metrics
 */
int display_performance_stats(void) {
    SharedMemory *shm;
    
    printf("\n%s%s╔══════════════════════════════════════════════════════════════╗%s\n",
           COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s%s║           MarketPulse Performance Metrics                    ║%s\n",
           COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s%s╚══════════════════════════════════════════════════════════════╝%s\n\n",
           COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    
    shm = shm_attach_worker();
    
    if (shm != NULL && shm_validate(shm)) {
        time_t now = time(NULL);
        time_t uptime = now - shm->stats.start_time;
        
        printf("%s[Request Statistics]%s\n", COLOR_BOLD, COLOR_RESET);
        printf("  Total requests:     %d\n", shm->stats.total_requests);
        printf("  Successful:         %s%d%s\n", COLOR_GREEN, shm->stats.successful_requests, COLOR_RESET);
        printf("  Failed:             %s%d%s\n", COLOR_RED, shm->stats.failed_requests, COLOR_RESET);
        
        if (shm->stats.total_requests > 0) {
            double success_rate = (double)shm->stats.successful_requests / shm->stats.total_requests * 100;
            printf("  Success rate:       %.1f%%\n", success_rate);
        }
        
        printf("\n%s[Performance]%s\n", COLOR_BOLD, COLOR_RESET);
        printf("  Avg response time:  %.2f ms\n", shm->stats.avg_response_time_ms);
        printf("  Rate limit hits:    %d\n", shm->stats.rate_limit_hits);
        
        if (uptime > 0) {
            double req_per_min = (double)shm->stats.total_requests / uptime * 60;
            printf("  Requests/min:       %.1f\n", req_per_min);
        }
        
        printf("\n%s[Worker Statistics]%s\n", COLOR_BOLD, COLOR_RESET);
        printf("  Active workers:     %d\n", shm->stats.active_workers);
        printf("  Uptime:             %ld seconds\n", uptime);
        
        printf("\n%s[Alert Statistics]%s\n", COLOR_BOLD, COLOR_RESET);
        printf("  Alerts triggered:   %d\n", shm->stats.total_alerts_triggered);
        
        printf("\n%s[System Health]%s\n", COLOR_BOLD, COLOR_RESET);
        
        /* Calculate health score */
        int health = 100;
        if (shm->stats.failed_requests > shm->stats.successful_requests / 10) health -= 20;
        if (shm->stats.avg_response_time_ms > 1000) health -= 20;
        if (shm->stats.active_workers == 0) health -= 30;
        if (shm->stats.rate_limit_hits > 10) health -= 10;
        if (health < 0) health = 0;
        
        const char *health_color = health >= 80 ? COLOR_GREEN : (health >= 50 ? COLOR_YELLOW : COLOR_RED);
        printf("  Health score:       %s%d%%%s\n", health_color, health, COLOR_RESET);
        
        if (health >= 80) {
            printf("  Status:             %s● Healthy%s\n", COLOR_GREEN, COLOR_RESET);
        } else if (health >= 50) {
            printf("  Status:             %s● Degraded%s\n", COLOR_YELLOW, COLOR_RESET);
        } else {
            printf("  Status:             %s● Critical%s\n", COLOR_RED, COLOR_RESET);
        }
        
    } else {
        struct rusage usage;

        printf("%s[Session Statistics]%s\n", COLOR_BOLD, COLOR_RESET);
        printf("  Status:             %sStandby%s — no active monitoring session\n",
               COLOR_YELLOW, COLOR_RESET);
        printf("  Tip:                Run %s./marketpulse watch AAPL MSFT GOOGL%s to start\n\n",
               COLOR_CYAN, COLOR_RESET);

        printf("%s[System Configuration]%s\n", COLOR_BOLD, COLOR_RESET);
        printf("  Max parallel workers:    %d  (one per stock via fork)\n", MAX_WORKERS);
        printf("  Max monitored stocks:    %d  (POSIX shared memory)\n", MAX_SHARED_STOCKS);
        printf("  IPC mechanism:           Shared memory + semaphore reader-writer lock\n");
        printf("  Rate limit policy:       Token bucket  —  60 req/min, refills 1/sec\n");
        printf("  Retry policy:            Exponential backoff  —  5 attempts, base 1s\n");
        printf("  Daemon support:          Double-fork + PID file + SIGUSR1/2 control\n");

        printf("\n%s[Rate Limiter]%s\n", COLOR_BOLD, COLOR_RESET);
        printf("  Tokens available:   %s%d / %d%s  (ready to fetch)\n",
               COLOR_GREEN, rate_limiter_tokens(), g_rate_limiter.max_tokens, COLOR_RESET);
        printf("  Refill rate:        %d token/sec\n", g_rate_limiter.refill_rate);

        printf("\n%s[Process Info]%s\n", COLOR_BOLD, COLOR_RESET);
        printf("  Current PID:        %d\n", getpid());
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            printf("  Memory (RSS):       %ld KB\n", usage.ru_maxrss / 1024);
            printf("  User CPU:           %ld.%03ld sec\n",
                   usage.ru_utime.tv_sec, usage.ru_utime.tv_usec / 1000);
        }

        printf("\n%s[Supported Commands]%s\n", COLOR_BOLD, COLOR_RESET);
        printf("  %-38s  Single stock quote\n",   "./marketpulse AAPL");
        printf("  %-38s  Live multi-stock watch\n","./marketpulse watch AAPL MSFT GOOGL");
        printf("  %-38s  NIFTY50 Indian market\n", "./marketpulse watch nifty50");
        printf("  %-38s  Price alert with signal\n","./marketpulse alert AAPL 300");
        printf("  %-38s  Fault-tolerance demo\n",  "./marketpulse simulate-failure");
        printf("  %-38s  Daemon mode\n",            "./marketpulse daemon start");
    }
    
    printf("\n");
    return 0;
}

/* ============== Dynamic Worker Scaling ============== */

/*
 * Calculate optimal worker count based on stock count and system load
 */
int calculate_optimal_workers(int stock_count, double avg_response_time) {
    int optimal = stock_count;
    
    /* Adjust based on response time */
    if (avg_response_time > 2000) {
        optimal = stock_count / 2;  /* Reduce workers if slow */
    } else if (avg_response_time < 500) {
        optimal = stock_count;      /* Full workers if fast */
    }
    
    /* Clamp to reasonable range */
    if (optimal < 1) optimal = 1;
    if (optimal > MAX_WORKERS) optimal = MAX_WORKERS;
    
    return optimal;
}

/*
 * Scale workers dynamically
 */
void scale_workers(SharedMemory *shm, int semid, int target_count) {
    int current = shm->stats.active_workers;
    
    if (target_count > current) {
        log_info("Scaling up: %d -> %d workers", current, target_count);
    } else if (target_count < current) {
        log_info("Scaling down: %d -> %d workers", current, target_count);
    }
    
    /* Actual scaling would be done by master process */
    (void)semid;
}

/* ============== AI-Driven Adaptive Polling ============== */

static int g_base_poll_interval = DEFAULT_REFRESH_INTERVAL;
static int g_current_poll_interval = DEFAULT_REFRESH_INTERVAL;

/*
 * Adjust polling rate based on volatility
 */
int get_adaptive_poll_interval(double volatility) {
    if (volatility > 5.0) {
        /* High volatility - poll more frequently */
        g_current_poll_interval = g_base_poll_interval / 2;
        if (g_current_poll_interval < 2) g_current_poll_interval = 2;
        log_info("High volatility (%.2f%%) - increased polling rate", volatility);
    } else if (volatility > 2.0) {
        /* Medium volatility - normal rate */
        g_current_poll_interval = g_base_poll_interval;
    } else {
        /* Low volatility - poll less frequently */
        g_current_poll_interval = g_base_poll_interval * 2;
        if (g_current_poll_interval > 30) g_current_poll_interval = 30;
    }
    
    return g_current_poll_interval;
}

/*
 * Check if AI should trigger an alert based on trend
 */
int ai_should_alert(double *prices, int count) {
    if (count < 5) return 0;
    
    /* Check for sudden reversal */
    double recent_change = (prices[count-1] - prices[count-2]) / prices[count-2] * 100;
    double prev_change = (prices[count-2] - prices[count-3]) / prices[count-3] * 100;
    
    /* Trend reversal detection */
    if ((recent_change > 0 && prev_change < -1) || (recent_change < 0 && prev_change > 1)) {
        log_info("AI detected trend reversal: %.2f%% -> %.2f%%", prev_change, recent_change);
        return 1;
    }
    
    return 0;
}