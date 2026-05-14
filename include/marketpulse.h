/*
 * MarketPulse - Real-Time Stock Monitoring and Insight Engine
 * Main header file with common definitions
 */

#ifndef MARKETPULSE_H
#define MARKETPULSE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>

/* ============== Configuration ============== */

#define FINNHUB_API_KEY "d6tt5npr01qhkb45h26gd6tt5npr01qhkb45h270"
#define FINNHUB_HOST "finnhub.io"
#define FINNHUB_PORT 443

/* Buffer sizes */
#define MAX_BUFFER_SIZE 8192
#define MAX_SYMBOL_LENGTH 16
#define MAX_COMPANY_NAME 64
#define MAX_STOCKS 50
#define MAX_PRICE_HISTORY 20

/* Timing */
#define DEFAULT_REFRESH_INTERVAL 5  /* seconds */
#define ALERT_CHECK_INTERVAL 2      /* seconds */

/* ============== Data Structures ============== */

/* Stock data structure */
typedef struct {
    char symbol[MAX_SYMBOL_LENGTH];
    char name[MAX_COMPANY_NAME];
    double current_price;
    double previous_close;
    double change;
    double change_percent;
    double high;
    double low;
    double open;
    time_t timestamp;
    int valid;  /* 1 if data is valid, 0 otherwise */
} StockData;

/* Price history for AI analysis */
typedef struct {
    double prices[MAX_PRICE_HISTORY];
    int count;
    int index;  /* circular buffer index */
} PriceHistory;

/* Alert configuration */
typedef struct {
    char symbol[MAX_SYMBOL_LENGTH];
    double threshold;
    int above;  /* 1 = alert when above, 0 = alert when below */
    int triggered;
} AlertConfig;

/* AI Insight result */
typedef struct {
    char trend[32];           /* "Bullish", "Bearish", "Neutral" */
    char momentum[32];        /* "Strong", "Weak", "Moderate" */
    double moving_avg_5;
    double moving_avg_10;
    double volatility;
    char recommendation[128];
} AIInsight;

/* Command types */
typedef enum {
    CMD_UNKNOWN,
    CMD_FETCH_SINGLE,
    CMD_WATCH,
    CMD_ALERT,
    CMD_HELP,
    CMD_VERSION,
    CMD_STATUS,             /* System status/introspection */
    CMD_STATS,              /* Performance metrics */
    CMD_SIMULATE_FAILURE,   /* Failure simulation mode */
    CMD_DAEMON_START,       /* Start daemon */
    CMD_DAEMON_STOP,        /* Stop daemon */
    CMD_DAEMON_STATUS,      /* Daemon status */
    CMD_TOP,                /* Top market movers */
    CMD_STREAM              /* Live FIFO stream (mkfifo) */
} CommandType;

/* Failure simulation types */
typedef enum {
    FAILURE_NONE = 0,
    FAILURE_WORKER_CRASH,
    FAILURE_NETWORK,
    FAILURE_API_DELAY,
    FAILURE_RANDOM
} FailureType;

/* Parsed command structure */
typedef struct {
    CommandType type;
    char symbols[MAX_STOCKS][MAX_SYMBOL_LENGTH];
    int symbol_count;
    double alert_threshold;
    int refresh_interval;
} ParsedCommand;

/* ============== Color Codes for Terminal ============== */

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_BOLD    "\033[1m"

/* Arrow symbols */
#define ARROW_UP   "▲"
#define ARROW_DOWN "▼"
#define ARROW_FLAT "►"

/* ============== Predefined Stock Lists ============== */

/* Top 10 S&P 500 stocks for demo */
static const char *SP500_TOP10[] = {
    "AAPL", "MSFT", "GOOGL", "AMZN", "NVDA",
    "META", "TSLA", "BRK.B", "JPM", "V"
};
#define SP500_TOP10_COUNT 10

/* Tech stocks */
static const char *TECH_STOCKS[] = {
    "AAPL", "MSFT", "GOOGL", "META", "NVDA",
    "AMD", "INTC", "CRM", "ORCL", "ADBE"
};
#define TECH_STOCKS_COUNT 10

/* NIFTY50 Top 10 Indian stocks (BSE) */
static const char *NIFTY50_TOP10[] = {
    "RELIANCE.BSE", "TCS.BSE", "HDFCBANK.BSE", "INFY.BSE", "ICICIBANK.BSE",
    "HINDUNILVR.BSE", "SBIN.BSE", "BHARTIARTL.BSE", "KOTAKBANK.BSE", "ITC.BSE"
};
#define NIFTY50_TOP10_COUNT 10

/* Alpha Vantage API for Indian stocks */
#define ALPHAVANTAGE_API_KEY "demo"
#define ALPHAVANTAGE_HOST "www.alphavantage.co"
#define ALPHAVANTAGE_PORT 443

/* ============== Global Variables ============== */

extern volatile sig_atomic_t keep_running;
extern volatile sig_atomic_t alert_triggered;

/* ============== Function Declarations ============== */

/* cli.c */
int parse_command(int argc, char *argv[], ParsedCommand *cmd);
void print_usage(const char *program_name);
void print_version(void);

/* network.c */
int create_connection(const char *host, int port);
int send_http_request(int sockfd, const char *host, const char *path);
int receive_response(int sockfd, char *buffer, size_t buffer_size);
int fetch_stock_quote(const char *symbol, char *response, size_t response_size);
int fetch_company_profile(const char *symbol, char *response, size_t response_size);
void close_connection(int sockfd);

/* parser.c */
int parse_stock_quote(const char *json, StockData *stock);
int parse_company_profile(const char *json, StockData *stock);
char *json_get_string(const char *json, const char *key);
double json_get_number(const char *json, const char *key);

/* monitor.c */
int start_monitoring(ParsedCommand *cmd);
int fetch_and_display_single(const char *symbol);
int fetch_and_display_multiple(char symbols[][MAX_SYMBOL_LENGTH], int count, int continuous);
void display_stock_table(StockData *stocks, PriceHistory *histories, int count);
int display_top_movers(void);
void clear_screen(void);

/* stream.c */
int run_stream_mode(char symbols[][MAX_SYMBOL_LENGTH], int count);

/* alert.c */
int start_alert_monitoring(const char *symbol, double threshold);
void setup_signal_handlers(void);
void signal_handler(int signum);
void trigger_alert(const char *symbol, double price, double threshold);

/* ai.c */
void init_price_history(PriceHistory *history);
void add_price(PriceHistory *history, double price);
void analyze_stock(PriceHistory *history, AIInsight *insight);
double calculate_moving_average(PriceHistory *history, int periods);
double calculate_volatility(PriceHistory *history);
double calculate_momentum(PriceHistory *history);
void print_ai_insight(AIInsight *insight, const char *symbol);

/* utils.c */
void get_current_time_string(char *buffer, size_t size);
void format_price(double price, char *buffer, size_t size);
void format_change(double change, double change_percent, char *buffer, size_t size);
const char *get_trend_arrow(double change);
const char *get_trend_color(double change);
void print_header(void);
void print_separator(void);
int is_market_open(void);
void sleep_ms(int milliseconds);

#endif /* MARKETPULSE_H */