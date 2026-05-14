/*
 * MarketPulse - Real-Time Stock Monitoring and Insight Engine
 * CLI Parser - Command line argument parsing
 * 
 * Supported commands:
 *   marketpulse SYMBOL              - Fetch single stock
 *   marketpulse watch SYMBOL...     - Watch multiple stocks
 *   marketpulse watch sp500         - Watch S&P 500 top 10
 *   marketpulse watch tech          - Watch tech stocks
 *   marketpulse alert SYMBOL PRICE  - Set price alert
 *   marketpulse --help              - Show help
 *   marketpulse --version           - Show version
 */

#include "marketpulse.h"

#define VERSION "1.0.0"
#define BUILD_DATE __DATE__

/* Forward declarations for utility functions */
void str_to_upper(char *str);

/*
 * Print usage information
 */
void print_usage(const char *program_name) {
    printf("\n");
    printf("%s%sMarketPulse%s - Real-Time Stock Monitoring Engine\n", 
           COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("Version %s (Built: %s)\n\n", VERSION, BUILD_DATE);
    
    printf("%sUSAGE:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s <SYMBOL>                    Fetch single stock quote\n", program_name);
    printf("  %s watch <SYMBOL> [SYMBOL...]  Monitor multiple stocks\n", program_name);
    printf("  %s watch sp500                 Monitor S&P 500 top 10\n", program_name);
    printf("  %s alert <SYMBOL> <PRICE>      Alert when price crosses threshold\n", program_name);
    printf("  %s status                      Show system status\n", program_name);
    printf("  %s stats                       Show performance metrics\n", program_name);
    printf("  %s simulate-failure            Run failure simulation\n", program_name);
    printf("  %s top                         Show top market movers\n", program_name);
    printf("  %s stream [SYMBOL...]          Stream live JSON to named pipe\n", program_name);
    printf("  %s daemon start|stop|status    Daemon control\n", program_name);
    printf("  %s --help                      Show this help message\n", program_name);
    
    printf("\n%sEXAMPLES:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s AAPL                        Get Apple stock price\n", program_name);
    printf("  %s watch AAPL GOOGL MSFT       Watch multiple stocks\n", program_name);
    printf("  %s alert TSLA 250              Alert when Tesla crosses $250\n", program_name);
    printf("  %s status                      View system introspection\n", program_name);
    printf("  %s stats                       View performance metrics\n", program_name);
    printf("  %s simulate-failure            Test fault tolerance\n", program_name);
    
    printf("\n%sPRESET GROUPS:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  sp500    Top 10 S&P 500 stocks (AAPL, MSFT, GOOGL, AMZN, etc.)\n");
    printf("  tech     Top 10 tech stocks (AAPL, MSFT, GOOGL, META, NVDA, etc.)\n");
    printf("  %snifty50%s  Top 10 NIFTY50 Indian stocks (RELIANCE, TCS, INFY, etc.) %s[NEW]%s\n",
           COLOR_YELLOW, COLOR_RESET, COLOR_GREEN, COLOR_RESET);
    
    printf("\n%sSYSTEM COMMANDS:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  status           System introspection (workers, memory, flags)\n");
    printf("  stats            Performance metrics (requests, latency, health)\n");
    printf("  simulate-failure Fault tolerance demonstration\n");
    printf("  daemon start     Start background daemon\n");
    printf("  daemon stop      Stop background daemon\n");
    printf("  daemon status    Check daemon status\n");
    
    printf("\n%sSIGNAL CONTROL (daemon mode):%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  kill -USR1 <PID>  Reload configuration\n");
    printf("  kill -USR2 <PID>  Toggle debug mode\n");
    
    printf("\n%sNOTES:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  - Stock symbols are case-insensitive (aapl = AAPL)\n");
    printf("  - Data is fetched from Finnhub API (US stocks only)\n");
    printf("  - Watch mode refreshes every %d seconds\n", DEFAULT_REFRESH_INTERVAL);
    printf("  - Market hours: 9:30 AM - 4:00 PM ET (Mon-Fri)\n");
    
    printf("\n");
}

/*
 * Print version information
 */
void print_version(void) {
    printf("MarketPulse version %s\n", VERSION);
    printf("Built: %s\n", BUILD_DATE);
    printf("API: Finnhub (finnhub.io)\n");
    printf("\n");
    printf("System Programming Project\n");
    printf("Features: socket(), fork(), signal(), select()\n");
}

/*
 * Check if string matches a preset group name
 * Returns 1 if match, 0 otherwise
 */
static int is_preset_group(const char *name) {
    char upper[32];
    strncpy(upper, name, sizeof(upper) - 1);
    upper[sizeof(upper) - 1] = '\0';
    str_to_upper(upper);
    
    if (strcmp(upper, "SP500") == 0 || 
        strcmp(upper, "S&P500") == 0 ||
        strcmp(upper, "SNP500") == 0) {
        return 1;
    }
    
    if (strcmp(upper, "TECH") == 0 ||
        strcmp(upper, "TECHNOLOGY") == 0) {
        return 2;
    }
    
    /* NIFTY50 - Indian stock market */
    if (strcmp(upper, "NIFTY50") == 0 ||
        strcmp(upper, "NIFTY") == 0 ||
        strcmp(upper, "NSE") == 0 ||
        strcmp(upper, "BSE") == 0 ||
        strcmp(upper, "INDIA") == 0) {
        return 3;
    }
    
    return 0;
}

/*
 * Load preset stock group into command
 */
static void load_preset_group(ParsedCommand *cmd, int group_type) {
    int i;
    
    if (group_type == 1) {
        /* S&P 500 top 10 */
        for (i = 0; i < SP500_TOP10_COUNT && i < MAX_STOCKS; i++) {
            strncpy(cmd->symbols[i], SP500_TOP10[i], MAX_SYMBOL_LENGTH - 1);
            cmd->symbols[i][MAX_SYMBOL_LENGTH - 1] = '\0';
        }
        cmd->symbol_count = SP500_TOP10_COUNT;
    } else if (group_type == 2) {
        /* Tech stocks */
        for (i = 0; i < TECH_STOCKS_COUNT && i < MAX_STOCKS; i++) {
            strncpy(cmd->symbols[i], TECH_STOCKS[i], MAX_SYMBOL_LENGTH - 1);
            cmd->symbols[i][MAX_SYMBOL_LENGTH - 1] = '\0';
        }
        cmd->symbol_count = TECH_STOCKS_COUNT;
    } else if (group_type == 3) {
        /* NIFTY50 top 10 - Indian stocks */
        printf("%s[Indian Market]%s Loading NIFTY50 stocks (BSE)...\n", 
               COLOR_YELLOW, COLOR_RESET);
        for (i = 0; i < NIFTY50_TOP10_COUNT && i < MAX_STOCKS; i++) {
            strncpy(cmd->symbols[i], NIFTY50_TOP10[i], MAX_SYMBOL_LENGTH - 1);
            cmd->symbols[i][MAX_SYMBOL_LENGTH - 1] = '\0';
        }
        cmd->symbol_count = NIFTY50_TOP10_COUNT;
    }
}

/*
 * Validate stock symbol format
 * Returns 1 if valid, 0 if invalid
 */
static int is_valid_symbol(const char *symbol) {
    int len = strlen(symbol);
    int i;
    
    /* Check length (1-5 characters typical for US stocks) */
    if (len < 1 || len > 10) {
        return 0;
    }
    
    /* Check characters (letters, dots, and hyphens allowed) */
    for (i = 0; i < len; i++) {
        char c = symbol[i];
        if (!((c >= 'A' && c <= 'Z') || 
              (c >= 'a' && c <= 'z') ||
              c == '.' || c == '-')) {
            return 0;
        }
    }
    
    return 1;
}

/*
 * Parse command line arguments
 * 
 * Returns: 0 on success, -1 on error
 */
int parse_command(int argc, char *argv[], ParsedCommand *cmd) {
    int i;
    int preset;
    
    /* Initialize command structure */
    memset(cmd, 0, sizeof(ParsedCommand));
    cmd->type = CMD_UNKNOWN;
    cmd->symbol_count = 0;
    cmd->alert_threshold = 0.0;
    cmd->refresh_interval = DEFAULT_REFRESH_INTERVAL;
    
    /* No arguments - show help */
    if (argc < 2) {
        cmd->type = CMD_HELP;
        return 0;
    }
    
    /* Check for help flag */
    if (strcmp(argv[1], "--help") == 0 || 
        strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "help") == 0) {
        cmd->type = CMD_HELP;
        return 0;
    }
    
    /* Check for version flag */
    if (strcmp(argv[1], "--version") == 0 || 
        strcmp(argv[1], "-v") == 0 ||
        strcmp(argv[1], "version") == 0) {
        cmd->type = CMD_VERSION;
        return 0;
    }
    
    /* Check for watch command */
    if (strcmp(argv[1], "watch") == 0 || strcmp(argv[1], "WATCH") == 0) {
        cmd->type = CMD_WATCH;
        
        if (argc < 3) {
            fprintf(stderr, "Error: 'watch' command requires at least one stock symbol\n");
            fprintf(stderr, "Usage: %s watch SYMBOL [SYMBOL...]\n", argv[0]);
            return -1;
        }
        
        /* Check for preset groups */
        preset = is_preset_group(argv[2]);
        if (preset > 0) {
            load_preset_group(cmd, preset);
            return 0;
        }
        
        /* Parse individual symbols */
        for (i = 2; i < argc && cmd->symbol_count < MAX_STOCKS; i++) {
            /* Skip any flags */
            if (argv[i][0] == '-') {
                continue;
            }
            
            if (!is_valid_symbol(argv[i])) {
                fprintf(stderr, "Warning: Invalid symbol '%s' - skipping\n", argv[i]);
                continue;
            }
            
            /* Copy and convert to uppercase */
            strncpy(cmd->symbols[cmd->symbol_count], argv[i], MAX_SYMBOL_LENGTH - 1);
            cmd->symbols[cmd->symbol_count][MAX_SYMBOL_LENGTH - 1] = '\0';
            str_to_upper(cmd->symbols[cmd->symbol_count]);
            cmd->symbol_count++;
        }
        
        if (cmd->symbol_count == 0) {
            fprintf(stderr, "Error: No valid stock symbols provided\n");
            return -1;
        }
        
        return 0;
    }
    
    /* Check for alert command */
    if (strcmp(argv[1], "alert") == 0 || strcmp(argv[1], "ALERT") == 0) {
        cmd->type = CMD_ALERT;
        
        if (argc < 4) {
            fprintf(stderr, "Error: 'alert' command requires symbol and price\n");
            fprintf(stderr, "Usage: %s alert SYMBOL PRICE\n", argv[0]);
            return -1;
        }
        
        /* Validate symbol */
        if (!is_valid_symbol(argv[2])) {
            fprintf(stderr, "Error: Invalid stock symbol '%s'\n", argv[2]);
            return -1;
        }
        
        /* Copy symbol */
        strncpy(cmd->symbols[0], argv[2], MAX_SYMBOL_LENGTH - 1);
        cmd->symbols[0][MAX_SYMBOL_LENGTH - 1] = '\0';
        str_to_upper(cmd->symbols[0]);
        cmd->symbol_count = 1;
        
        /* Parse threshold price */
        char *endptr;
        cmd->alert_threshold = strtod(argv[3], &endptr);
        
        if (endptr == argv[3] || cmd->alert_threshold <= 0) {
            fprintf(stderr, "Error: Invalid price threshold '%s'\n", argv[3]);
            return -1;
        }
        
        return 0;
    }
    
    /* Check for status command */
    if (strcmp(argv[1], "status") == 0) {
        cmd->type = CMD_STATUS;
        return 0;
    }
    
    /* Check for stats command */
    if (strcmp(argv[1], "stats") == 0) {
        cmd->type = CMD_STATS;
        return 0;
    }
    
    /* Check for simulate-failure command */
    if (strcmp(argv[1], "simulate-failure") == 0 ||
        strcmp(argv[1], "simulate") == 0 ||
        strcmp(argv[1], "test-failure") == 0) {
        cmd->type = CMD_SIMULATE_FAILURE;
        return 0;
    }

    /* Check for top movers command */
    if (strcmp(argv[1], "top") == 0 || strcmp(argv[1], "TOP") == 0 ||
        strcmp(argv[1], "movers") == 0) {
        cmd->type = CMD_TOP;
        return 0;
    }

    /* Check for stream command (mkfifo live feed) */
    if (strcmp(argv[1], "stream") == 0 || strcmp(argv[1], "STREAM") == 0) {
        cmd->type = CMD_STREAM;
        if (argc < 3) {
            /* Default to tech stocks when no symbols given */
            for (i = 0; i < TECH_STOCKS_COUNT && i < MAX_STOCKS; i++) {
                strncpy(cmd->symbols[i], TECH_STOCKS[i], MAX_SYMBOL_LENGTH - 1);
                cmd->symbols[i][MAX_SYMBOL_LENGTH - 1] = '\0';
            }
            cmd->symbol_count = TECH_STOCKS_COUNT;
            return 0;
        }
        for (i = 2; i < argc && cmd->symbol_count < MAX_STOCKS; i++) {
            if (argv[i][0] == '-') continue;
            if (!is_valid_symbol(argv[i])) {
                fprintf(stderr, "Warning: Invalid symbol '%s' - skipping\n", argv[i]);
                continue;
            }
            strncpy(cmd->symbols[cmd->symbol_count], argv[i], MAX_SYMBOL_LENGTH - 1);
            cmd->symbols[cmd->symbol_count][MAX_SYMBOL_LENGTH - 1] = '\0';
            str_to_upper(cmd->symbols[cmd->symbol_count]);
            cmd->symbol_count++;
        }
        if (cmd->symbol_count == 0) {
            fprintf(stderr, "Error: No valid symbols provided for stream\n");
            return -1;
        }
        return 0;
    }

    /* Check for daemon command */
    if (strcmp(argv[1], "daemon") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: 'daemon' requires subcommand (start|stop|status)\n");
            return -1;
        }
        
        if (strcmp(argv[2], "start") == 0) {
            cmd->type = CMD_DAEMON_START;
            /* Parse optional stock symbols */
            if (argc > 3 && strcmp(argv[3], "watch") == 0) {
                for (i = 4; i < argc && cmd->symbol_count < MAX_STOCKS; i++) {
                    if (is_valid_symbol(argv[i])) {
                        strncpy(cmd->symbols[cmd->symbol_count], argv[i], MAX_SYMBOL_LENGTH - 1);
                        str_to_upper(cmd->symbols[cmd->symbol_count]);
                        cmd->symbol_count++;
                    }
                }
            }
        } else if (strcmp(argv[2], "stop") == 0) {
            cmd->type = CMD_DAEMON_STOP;
        } else if (strcmp(argv[2], "status") == 0) {
            cmd->type = CMD_DAEMON_STATUS;
        } else {
            fprintf(stderr, "Error: Unknown daemon subcommand '%s'\n", argv[2]);
            return -1;
        }
        return 0;
    }
    
    /* Single stock fetch (default command) */
    if (is_valid_symbol(argv[1])) {
        cmd->type = CMD_FETCH_SINGLE;
        strncpy(cmd->symbols[0], argv[1], MAX_SYMBOL_LENGTH - 1);
        cmd->symbols[0][MAX_SYMBOL_LENGTH - 1] = '\0';
        str_to_upper(cmd->symbols[0]);
        cmd->symbol_count = 1;
        return 0;
    }
    
    /* Unknown command */
    fprintf(stderr, "Error: Unknown command or invalid symbol '%s'\n", argv[1]);
    fprintf(stderr, "Run '%s --help' for usage information\n", argv[0]);
    return -1;
}

/*
 * Debug function: Print parsed command
 */
void debug_print_command(const ParsedCommand *cmd) {
#ifdef DEBUG
    int i;
    
    printf("=== Parsed Command ===\n");
    printf("Type: ");
    switch (cmd->type) {
        case CMD_UNKNOWN: printf("UNKNOWN\n"); break;
        case CMD_FETCH_SINGLE: printf("FETCH_SINGLE\n"); break;
        case CMD_WATCH: printf("WATCH\n"); break;
        case CMD_ALERT: printf("ALERT\n"); break;
        case CMD_HELP: printf("HELP\n"); break;
        case CMD_VERSION: printf("VERSION\n"); break;
    }
    printf("Symbol count: %d\n", cmd->symbol_count);
    for (i = 0; i < cmd->symbol_count; i++) {
        printf("  Symbol[%d]: %s\n", i, cmd->symbols[i]);
    }
    if (cmd->type == CMD_ALERT) {
        printf("Alert threshold: $%.2f\n", cmd->alert_threshold);
    }
    printf("Refresh interval: %d seconds\n", cmd->refresh_interval);
    printf("======================\n");
#else
    (void)cmd;
#endif
}