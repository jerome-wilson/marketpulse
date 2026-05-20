/*
 * MarketPulse - Real-Time Stock Monitoring and Insight Engine
 * Main Entry Point
 * 
 * A Linux/macOS system programming project demonstrating:
 * - Network programming (socket, connect, SSL)
 * - Process control (fork, pipe, waitpid)
 * - Signal handling (signal, sigaction)
 * - File I/O (read, write)
 * - Inter-process communication (shared memory, semaphores)
 * - Daemon mode (double fork, setsid)
 * - Event-driven design (select)
 * 
 * Author: Systems Programming Project
 * Date: 2024
 */

#include "marketpulse.h"
#include "daemon.h"

/* External function declarations */
extern void setup_signal_handlers(void);
extern void cleanup_ssl(void);

/* System module functions */
extern int display_system_status(void);
extern int display_performance_stats(void);
extern int run_failure_simulation(void);

/*
 * Main entry point
 */
int main(int argc, char *argv[]) {
    ParsedCommand cmd;
    int result = 0;
    
    /* Setup signal handlers for graceful shutdown */
    setup_signal_handlers();
    
    /* Parse command line arguments */
    if (parse_command(argc, argv, &cmd) != 0) {
        return 1;
    }
    
#ifdef DEBUG
    debug_print_command(&cmd);
#endif
    
    /* Execute command based on type */
    switch (cmd.type) {
        case CMD_HELP:
            print_usage(argv[0]);
            break;
            
        case CMD_VERSION:
            print_version();
            break;
            
        case CMD_FETCH_SINGLE:
            print_header();
            result = fetch_and_display_single(cmd.symbols[0]);
            break;
            
        case CMD_WATCH:
            print_header();
            result = fetch_and_display_multiple(cmd.symbols, cmd.symbol_count, 1);
            break;
            
        case CMD_ALERT:
            print_header();
            result = start_alert_monitoring(cmd.symbols[0], cmd.alert_threshold);
            break;
            
        case CMD_STATUS:
            result = display_system_status();
            break;
            
        case CMD_STATS:
            result = display_performance_stats();
            break;
            
        case CMD_SIMULATE_FAILURE:
            result = run_failure_simulation();
            break;

        case CMD_TOP:
            result = display_top_movers();
            break;

        case CMD_STREAM:
            result = run_stream_mode(cmd.symbols, cmd.symbol_count);
            break;

        case CMD_INSIGHT:
            print_header();
            result = fetch_and_display_insight(cmd.symbols[0]);
            break;

        case CMD_DAEMON_START:
            printf("Starting MarketPulse daemon...\n");
            if (is_daemon_running()) {
                printf("Daemon is already running.\n");
                result = 1;
            } else {
                printf("Use: ./marketpulse watch AAPL MSFT & (for background)\n");
                printf("Or implement full daemon mode with daemonize()\n");
                result = 0;
            }
            break;
            
        case CMD_DAEMON_STOP:
            result = daemon_stop();
            break;
            
        case CMD_DAEMON_STATUS:
            result = daemon_status();
            break;
            
        case CMD_UNKNOWN:
        default:
            fprintf(stderr, "Error: Unknown command\n");
            print_usage(argv[0]);
            result = 1;
            break;
    }
    
    /* Cleanup */
    cleanup_ssl();
    
    return result;
}