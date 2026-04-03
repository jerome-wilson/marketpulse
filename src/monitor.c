/*
 * MarketPulse - Real-Time Stock Monitoring and Insight Engine
 * Monitor Engine - Real-time stock monitoring with process control
 * 
 * System Programming Concepts:
 * - fork() - Create child processes for parallel fetching
 * - pipe() - Inter-process communication
 * - select() - Multiplexing I/O
 * - sleep() - Timing control
 */

#include "marketpulse.h"
#include <sys/time.h>

/* Global flag for graceful shutdown */
volatile sig_atomic_t keep_running = 1;

/* External function declarations */
extern int fetch_stock_quote(const char *symbol, char *response, size_t response_size);
extern int fetch_indian_stock_quote(const char *symbol, char *response, size_t response_size);
extern int is_indian_stock(const char *symbol);
extern int parse_stock_quote(const char *json, StockData *stock);
extern void print_ai_insight(AIInsight *insight, const char *symbol);
extern void analyze_stock(PriceHistory *history, AIInsight *insight);
extern void add_price(PriceHistory *history, double price);
extern void init_price_history(PriceHistory *history);
extern void set_currency_mode(int use_inr);
extern int is_indian_market_open(void);
extern void get_us_time_string(char *buffer, size_t size);
extern void get_ist_time_string(char *buffer, size_t size);

/* Simulated Indian stock data (for demo when API is unavailable) */
typedef struct {
    const char *symbol;
    const char *name;
    double base_price;
} IndianStockInfo;

static const IndianStockInfo INDIAN_STOCK_DATA[] = {
    {"RELIANCE.BSE", "Reliance Industries", 2963.10},
    {"TCS.BSE", "Tata Consultancy", 4051.40},
    {"HDFCBANK.BSE", "HDFC Bank", 1672.20},
    {"INFY.BSE", "Infosys", 1845.75},
    {"ICICIBANK.BSE", "ICICI Bank", 1256.30},
    {"HINDUNILVR.BSE", "Hindustan Unilever", 2534.80},
    {"SBIN.BSE", "State Bank of India", 825.45},
    {"BHARTIARTL.BSE", "Bharti Airtel", 1678.90},
    {"KOTAKBANK.BSE", "Kotak Mahindra Bank", 1823.60},
    {"ITC.BSE", "ITC Limited", 465.25}
};
#define INDIAN_STOCK_COUNT 10

/*
 * Get simulated Indian stock data
 * Uses realistic base prices with small random variations
 */
static int get_simulated_indian_stock(const char *symbol, StockData *stock) {
    int i;
    double variation;
    
    for (i = 0; i < INDIAN_STOCK_COUNT; i++) {
        if (strcasecmp(symbol, INDIAN_STOCK_DATA[i].symbol) == 0) {
            /* Add small random variation (-2% to +2%) */
            srand((unsigned int)time(NULL) + (unsigned int)i);
            variation = ((rand() % 400) - 200) / 10000.0;  /* -0.02 to +0.02 */
            
            strncpy(stock->symbol, symbol, MAX_SYMBOL_LENGTH - 1);
            strncpy(stock->name, INDIAN_STOCK_DATA[i].name, MAX_COMPANY_NAME - 1);
            
            stock->current_price = INDIAN_STOCK_DATA[i].base_price * (1.0 + variation);
            stock->previous_close = INDIAN_STOCK_DATA[i].base_price;
            stock->change = stock->current_price - stock->previous_close;
            stock->change_percent = (stock->change / stock->previous_close) * 100.0;
            stock->open = INDIAN_STOCK_DATA[i].base_price * (1.0 + variation * 0.5);
            stock->high = stock->current_price * 1.01;
            stock->low = stock->current_price * 0.99;
            stock->timestamp = time(NULL);
            stock->valid = 1;
            
            return 0;
        }
    }
    
    return -1;  /* Symbol not found */
}

/* Price history for AI analysis (one per stock) */
static PriceHistory price_histories[MAX_STOCKS];

/*
 * Fetch and display a single stock quote
 * Returns: 0 on success, -1 on error
 */
int fetch_and_display_single(const char *symbol) {
    char response[MAX_BUFFER_SIZE];
    char profile_response[MAX_BUFFER_SIZE];
    StockData stock;
    char time_str[16];
    char price_str[32];
    char change_str[64];
    
    /* Initialize stock data */
    memset(&stock, 0, sizeof(StockData));
    strncpy(stock.symbol, symbol, MAX_SYMBOL_LENGTH - 1);
    
    printf("\nFetching data for %s%s%s...\n", COLOR_BOLD, symbol, COLOR_RESET);
    
    /* Fetch quote data */
    if (fetch_stock_quote(symbol, response, sizeof(response)) != 0) {
        fprintf(stderr, "%sError: Failed to fetch data for %s%s\n", 
                COLOR_RED, symbol, COLOR_RESET);
        fprintf(stderr, "Please check your internet connection and try again.\n");
        return -1;
    }
    
    /* Parse quote data */
    if (parse_stock_quote(response, &stock) != 0) {
        fprintf(stderr, "%sError: Invalid symbol or no data available for %s%s\n",
                COLOR_RED, symbol, COLOR_RESET);
        return -1;
    }
    
    /* Try to fetch company profile for name */
    if (fetch_company_profile(symbol, profile_response, sizeof(profile_response)) == 0) {
        parse_company_profile(profile_response, &stock);
    }
    
    /* If no name from profile, use symbol */
    if (stock.name[0] == '\0') {
        strncpy(stock.name, symbol, MAX_COMPANY_NAME - 1);
    }
    
    /* Get current time */
    get_current_time_string(time_str, sizeof(time_str));
    
    /* Format price and change */
    format_price(stock.current_price, price_str, sizeof(price_str));
    format_change(stock.change, stock.change_percent, change_str, sizeof(change_str));
    
    /* Display results */
    printf("\n");
    print_separator();
    printf("%s%s Stock: %s (%s)%s\n", 
           COLOR_BOLD, COLOR_CYAN, stock.symbol, stock.name, COLOR_RESET);
    print_separator();
    
    printf("\n");
    printf("  %sPrice:%s      %s%s%s %s\n", 
           COLOR_BOLD, COLOR_RESET,
           get_trend_color(stock.change), price_str, COLOR_RESET,
           get_trend_arrow(stock.change));
    
    printf("  %sChange:%s     %s%s%s\n",
           COLOR_BOLD, COLOR_RESET,
           get_trend_color(stock.change), change_str, COLOR_RESET);
    
    printf("  %sOpen:%s       $%.2f\n", COLOR_BOLD, COLOR_RESET, stock.open);
    printf("  %sHigh:%s       $%.2f\n", COLOR_BOLD, COLOR_RESET, stock.high);
    printf("  %sLow:%s        $%.2f\n", COLOR_BOLD, COLOR_RESET, stock.low);
    printf("  %sPrev Close:%s $%.2f\n", COLOR_BOLD, COLOR_RESET, stock.previous_close);
    
    printf("\n");
    printf("  %sTime:%s       %s\n", COLOR_BOLD, COLOR_RESET, time_str);
    
    /* Market status */
    if (is_market_open()) {
        printf("  %sMarket:%s     %sOPEN%s\n", 
               COLOR_BOLD, COLOR_RESET, COLOR_GREEN, COLOR_RESET);
    } else {
        printf("  %sMarket:%s     %sCLOSED%s\n", 
               COLOR_BOLD, COLOR_RESET, COLOR_RED, COLOR_RESET);
    }
    
    printf("\n");
    print_separator();
    
    return 0;
}

/* Flag to track if we're monitoring Indian stocks */
static int monitoring_indian_stocks = 0;

/*
 * Display stock data in a formatted table
 */
void display_stock_table(StockData *stocks, int count) {
    int i;
    char time_str[16];
    char us_time_str[24];
    char ist_time_str[24];
    char price_str[32];
    int market_open;
    
    get_current_time_string(time_str, sizeof(time_str));
    get_us_time_string(us_time_str, sizeof(us_time_str));
    get_ist_time_string(ist_time_str, sizeof(ist_time_str));
    
    /* Clear screen for clean display */
    clear_screen();
    
    /* Print header */
    print_header();
    
    /* Check appropriate market hours */
    if (monitoring_indian_stocks) {
        market_open = is_indian_market_open();
        printf("  %sTime: %s%s", COLOR_YELLOW, time_str, COLOR_RESET);
        printf("  |  %sIST: %s%s", COLOR_CYAN, ist_time_str, COLOR_RESET);
        printf("  |  %s🇮🇳 NSE/BSE%s", COLOR_CYAN, COLOR_RESET);
        if (market_open) {
            printf("  |  Market: %sOPEN%s", COLOR_GREEN, COLOR_RESET);
        } else {
            printf("  |  Market: %sCLOSED%s", COLOR_RED, COLOR_RESET);
        }
    } else {
        market_open = is_market_open();
        printf("  %sTime: %s%s", COLOR_YELLOW, time_str, COLOR_RESET);
        printf("  |  %s🇺🇸 US: %s%s", COLOR_CYAN, us_time_str, COLOR_RESET);
        printf("  |  %sNYSE/NASDAQ%s", COLOR_CYAN, COLOR_RESET);
        if (market_open) {
            printf("  |  Market: %sOPEN%s", COLOR_GREEN, COLOR_RESET);
        } else {
            printf("  |  Market: %sCLOSED%s", COLOR_RED, COLOR_RESET);
        }
    }
    printf("\n  Press Ctrl+C to exit\n\n");
    
    /* Table header - wider columns for Indian stock symbols */
    printf("  %s%-16s %12s %10s %9s %s%s\n",
           COLOR_BOLD,
           "SYMBOL", "PRICE", "CHANGE", "CHANGE%", "TREND",
           COLOR_RESET);
    print_separator();
    
    /* Display each stock */
    for (i = 0; i < count; i++) {
        if (!stocks[i].valid) {
            printf("  %s%-16s%s %s%12s%s\n", 
                   COLOR_CYAN, stocks[i].symbol, COLOR_RESET,
                   COLOR_RED, "N/A", COLOR_RESET);
            continue;
        }
        
        format_price(stocks[i].current_price, price_str, sizeof(price_str));
        
        printf("  %s%-16s%s %s%12s %+10.2f %+8.2f%%%s  %s\n",
               COLOR_CYAN, stocks[i].symbol, COLOR_RESET,
               get_trend_color(stocks[i].change),
               price_str,
               stocks[i].change,
               stocks[i].change_percent,
               COLOR_RESET,
               get_trend_arrow(stocks[i].change));
    }
    
    print_separator();
    printf("\n");
}

/*
 * Fetch data for a single stock (used by child process)
 * Writes result to pipe
 * Uses simulated data for Indian stocks (free APIs require registration)
 */
static void fetch_stock_child(const char *symbol, int write_fd) {
    char response[MAX_BUFFER_SIZE];
    StockData stock;
    char saved_symbol[MAX_SYMBOL_LENGTH];
    int fetch_result;
    
    /* Save symbol first */
    memset(saved_symbol, 0, sizeof(saved_symbol));
    strncpy(saved_symbol, symbol, MAX_SYMBOL_LENGTH - 1);
    
    memset(&stock, 0, sizeof(StockData));
    strncpy(stock.symbol, symbol, MAX_SYMBOL_LENGTH - 1);
    
    /* Use appropriate data source based on stock type */
    if (is_indian_stock(symbol)) {
        /* Use simulated data for Indian stocks (demo mode) */
        if (get_simulated_indian_stock(symbol, &stock) == 0) {
            stock.valid = 1;
        } else {
            /* Try API as fallback */
            fetch_result = fetch_indian_stock_quote(symbol, response, sizeof(response));
            if (fetch_result == 0) {
                if (parse_stock_quote(response, &stock) == 0) {
                    stock.valid = 1;
                }
            }
        }
    } else {
        fetch_result = fetch_stock_quote(symbol, response, sizeof(response));
        if (fetch_result == 0) {
            if (parse_stock_quote(response, &stock) == 0) {
                stock.valid = 1;
            }
        }
    }
    
    /* Restore symbol (in case parse_stock_quote cleared it) */
    strncpy(stock.symbol, saved_symbol, MAX_SYMBOL_LENGTH - 1);
    
    /* Write stock data to pipe */
    write(write_fd, &stock, sizeof(StockData));
    close(write_fd);
}

/*
 * Fetch multiple stocks using fork() for parallel processing
 * 
 * This demonstrates:
 * - fork() for creating child processes
 * - pipe() for IPC
 * - waitpid() for process synchronization
 */
static int fetch_stocks_parallel(char symbols[][MAX_SYMBOL_LENGTH], int count, 
                                  StockData *stocks) {
    int i;
    int pipes[MAX_STOCKS][2];
    pid_t pids[MAX_STOCKS];
    int success_count = 0;
    
    /* Create pipes and fork child processes */
    for (i = 0; i < count; i++) {
        /* Create pipe */
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            continue;
        }
        
        /* Fork child process */
        pids[i] = fork();
        
        if (pids[i] == -1) {
            /* Fork failed */
            perror("fork");
            close(pipes[i][0]);
            close(pipes[i][1]);
            continue;
        }
        
        if (pids[i] == 0) {
            /* Child process */
            close(pipes[i][0]);  /* Close read end */
            fetch_stock_child(symbols[i], pipes[i][1]);
            exit(0);
        }
        
        /* Parent process */
        close(pipes[i][1]);  /* Close write end */
    }
    
    /* Parent: Read results from all children */
    for (i = 0; i < count; i++) {
        if (pids[i] > 0) {
            /* Read stock data from pipe */
            ssize_t bytes_read = read(pipes[i][0], &stocks[i], sizeof(StockData));
            close(pipes[i][0]);
            
            if (bytes_read == sizeof(StockData) && stocks[i].valid) {
                success_count++;
            } else {
                /* Mark as invalid if read failed */
                memset(&stocks[i], 0, sizeof(StockData));
                strncpy(stocks[i].symbol, symbols[i], MAX_SYMBOL_LENGTH - 1);
                stocks[i].valid = 0;
            }
            
            /* Wait for child to finish */
            waitpid(pids[i], NULL, 0);
        }
    }
    
    return success_count;
}

/*
 * Fetch multiple stocks sequentially (fallback method)
 * Automatically uses Alpha Vantage for Indian stocks
 */
static int fetch_stocks_sequential(char symbols[][MAX_SYMBOL_LENGTH], int count,
                                    StockData *stocks) {
    int i;
    int success_count = 0;
    char response[MAX_BUFFER_SIZE];
    char saved_symbol[MAX_SYMBOL_LENGTH];
    int fetch_result;
    
    for (i = 0; i < count; i++) {
        /* Save symbol first */
        memset(saved_symbol, 0, sizeof(saved_symbol));
        strncpy(saved_symbol, symbols[i], MAX_SYMBOL_LENGTH - 1);
        
        memset(&stocks[i], 0, sizeof(StockData));
        strncpy(stocks[i].symbol, symbols[i], MAX_SYMBOL_LENGTH - 1);
        
        /* Use appropriate API based on stock type */
        if (is_indian_stock(symbols[i])) {
            fetch_result = fetch_indian_stock_quote(symbols[i], response, sizeof(response));
        } else {
            fetch_result = fetch_stock_quote(symbols[i], response, sizeof(response));
        }
        
        if (fetch_result == 0) {
            if (parse_stock_quote(response, &stocks[i]) == 0) {
                stocks[i].valid = 1;
                success_count++;
            }
        }
        
        /* Restore symbol (in case parse_stock_quote cleared it) */
        strncpy(stocks[i].symbol, saved_symbol, MAX_SYMBOL_LENGTH - 1);
        
        /* Small delay between requests */
        if (i < count - 1) {
            usleep(100000);  /* 100ms */
        }
    }
    
    return success_count;
}

/*
 * Fetch and display multiple stocks with continuous monitoring
 * 
 * Parameters:
 *   symbols - Array of stock symbols
 *   count - Number of symbols
 *   continuous - If 1, keep refreshing; if 0, fetch once
 * 
 * Returns: 0 on success, -1 on error
 */
int fetch_and_display_multiple(char symbols[][MAX_SYMBOL_LENGTH], int count, 
                                int continuous) {
    StockData stocks[MAX_STOCKS];
    AIInsight insight;
    int i;
    int refresh_count = 0;
    int use_parallel = (count > 1);  /* Use parallel fetching for multiple stocks */
    int has_indian_stocks = 0;
    
    /* Check if any symbols are Indian stocks */
    for (i = 0; i < count; i++) {
        if (is_indian_stock(symbols[i])) {
            has_indian_stocks = 1;
            break;
        }
    }
    
    /* Set currency mode and tracking flag based on stock type */
    set_currency_mode(has_indian_stocks);
    monitoring_indian_stocks = has_indian_stocks;
    
    /* Initialize price histories */
    for (i = 0; i < count; i++) {
        init_price_history(&price_histories[i]);
    }
    
    if (has_indian_stocks) {
        printf("%s[Indian Market]%s Starting monitor for %d NIFTY stock(s)...\n", 
               COLOR_YELLOW, COLOR_RESET, count);
        printf("Currency: %s₹ INR%s (Indian Rupees)\n", COLOR_GREEN, COLOR_RESET);
        printf("%s[Demo Mode]%s Using simulated data (free APIs require registration)\n",
               COLOR_MAGENTA, COLOR_RESET);
    } else {
        printf("Starting monitor for %d stock(s)...\n", count);
    }
    printf("Press Ctrl+C to stop.\n\n");
    sleep(1);
    
    while (keep_running) {
        /* Fetch all stocks */
        if (use_parallel) {
            fetch_stocks_parallel(symbols, count, stocks);
        } else {
            fetch_stocks_sequential(symbols, count, stocks);
        }
        
        /* Update price histories for AI analysis */
        for (i = 0; i < count; i++) {
            if (stocks[i].valid) {
                add_price(&price_histories[i], stocks[i].current_price);
            }
        }
        
        /* Display stock table */
        display_stock_table(stocks, count);
        
        /* Generate and display AI insights (every 3rd refresh) */
        refresh_count++;
        if (refresh_count % 3 == 0 && count > 0) {
            /* Rotate through stocks - analyze a different stock each time */
            int stock_to_analyze = (refresh_count / 3) % count;
            
            /* Find next stock with enough history, starting from rotation index */
            for (i = 0; i < count; i++) {
                int idx = (stock_to_analyze + i) % count;
                if (price_histories[idx].count >= 3) {
                    analyze_stock(&price_histories[idx], &insight);
                    print_ai_insight(&insight, stocks[idx].symbol);
                    break;
                }
            }
        }
        
        /* Exit if not continuous mode */
        if (!continuous) {
            break;
        }
        
        /* Wait before next refresh */
        printf("  Refreshing in %d seconds...\n", DEFAULT_REFRESH_INTERVAL);
        
        /* Use interruptible sleep */
        for (i = 0; i < DEFAULT_REFRESH_INTERVAL && keep_running; i++) {
            sleep(1);
        }
    }
    
    printf("\n%sMonitoring stopped.%s\n", COLOR_YELLOW, COLOR_RESET);
    return 0;
}

/*
 * Start monitoring based on parsed command
 */
int start_monitoring(ParsedCommand *cmd) {
    switch (cmd->type) {
        case CMD_FETCH_SINGLE:
            return fetch_and_display_single(cmd->symbols[0]);
            
        case CMD_WATCH:
            return fetch_and_display_multiple(cmd->symbols, cmd->symbol_count, 1);
            
        default:
            fprintf(stderr, "Error: Invalid command type for monitoring\n");
            return -1;
    }
}

/*
 * Stop monitoring (called from signal handler)
 */
void stop_monitoring(void) {
    keep_running = 0;
}