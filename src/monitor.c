/*
 * MarketPulse - Monitor Engine
 *
 * System Programming Concepts:
 *   fork()  / pipe()    - parallel stock fetching
 *   waitpid()           - process synchronisation
 *   select()            - I/O multiplexing for keyboard input
 *   tcgetattr/tcsetattr - terminal raw mode (keyboard controls)
 *   mmap / msync        - persistent price history (via history.c)
 *   mkfifo              - live JSON stream (via stream.c)
 */

#include "marketpulse.h"
#include "history.h"
#include <sys/time.h>
#include <termios.h>

/* ── Signal flag (set by alert.c signal handler) ─────────────────────── */
volatile sig_atomic_t keep_running = 1;

/* External declarations ------------------------------------------------ */
extern int  fetch_stock_quote(const char *symbol, char *response, size_t size);
extern int  fetch_indian_stock_quote(const char *symbol, char *response, size_t size);
extern int  is_indian_stock(const char *symbol);
extern int  parse_stock_quote(const char *json, StockData *stock);
extern void print_ai_insight(AIInsight *insight, const char *symbol);
extern void analyze_stock(PriceHistory *history, AIInsight *insight);
extern void add_price(PriceHistory *history, double price);
extern void init_price_history(PriceHistory *history);
extern void set_currency_mode(int use_inr);
extern int  is_indian_market_open(void);
extern void get_us_time_string(char *buffer, size_t size);
extern void get_ist_time_string(char *buffer, size_t size);

/* ════════════════════════════════════════════════════════════════════════
 * SPARKLINE RENDERER
 * ════════════════════════════════════════════════════════════════════════ */

static void render_sparkline(PriceHistory *history, char *buf, size_t size) {
    /* Bar-chart sparkline using standard Unicode block elements.
     * ▁▂▃▄▅▆▇█ are "Block Elements" (U+2580–U+259F) — guaranteed 1 display
     * column wide in every terminal font, unlike Braille which some fonts
     * render as 2-column-wide. 12 bars per chart; varying heights form a
     * visually clear curve / histogram shape ("many cubes" effect). */
    static const char *BARS[8] = {"▁","▂","▃","▄","▅","▆","▇","█"};

    int n = history->count < 12 ? history->count : 12;
    double min_p, max_p, range;
    int i, pos = 0;

    /* Pad with spaces when not enough data */
    if (n < 2) {
        for (i = 0; i < 12 && pos + 1 < (int)size; i++) buf[pos++] = ' ';
        buf[pos] = '\0';
        return;
    }

    /* Oldest of the n prices in the circular buffer */
    int start = (history->index - n + MAX_PRICE_HISTORY) % MAX_PRICE_HISTORY;

    /* Min / max */
    min_p = max_p = history->prices[start];
    for (i = 1; i < n; i++) {
        int idx = (start + i) % MAX_PRICE_HISTORY;
        if (history->prices[idx] < min_p) min_p = history->prices[idx];
        if (history->prices[idx] > max_p) max_p = history->prices[idx];
    }
    range = max_p - min_p;

    /* Render one block bar per data point */
    for (i = 0; i < n; i++) {
        int idx = (start + i) % MAX_PRICE_HISTORY;
        int level;
        if (range < 0.001) {
            level = 3;  /* flat line — mid-height bar */
        } else {
            level = (int)((history->prices[idx] - min_p) / range * 7.0 + 0.5);
            if (level > 7) level = 7;
            if (level < 0) level = 0;
        }
        const char *bar = BARS[level];
        size_t bar_len = strlen(bar);
        if (pos + (int)bar_len >= (int)size - 1) break;
        memcpy(buf + pos, bar, bar_len);
        pos += (int)bar_len;
    }

    /* Pad right to always occupy exactly 12 display columns */
    int spaces = 12 - n;
    for (i = 0; i < spaces && pos + 1 < (int)size; i++) buf[pos++] = ' ';
    buf[pos] = '\0';
}

/* ════════════════════════════════════════════════════════════════════════
 * TERMINAL RAW MODE  (tcgetattr / tcsetattr)
 * ════════════════════════════════════════════════════════════════════════ */

static struct termios g_orig_termios;
static int g_raw_mode = 0;

static void restore_terminal(void) {
    if (g_raw_mode) {
        printf("\033[?25h");         /* ensure cursor is visible on exit */
        printf("\033[?1049l");       /* exit alternate screen → main screen restored */
        fflush(stdout);
        tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_termios);
        g_raw_mode = 0;
    }
}

static void setup_raw_terminal(void) {
    struct termios raw;
    if (!isatty(STDIN_FILENO)) return;
    tcgetattr(STDIN_FILENO, &g_orig_termios);
    atexit(restore_terminal);
    raw = g_orig_termios;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    printf("\033[?1049h");       /* enter alternate screen — all rendering stays here */
    fflush(stdout);
    g_raw_mode = 1;
}

/* ════════════════════════════════════════════════════════════════════════
 * SORT STOCKS BY CHANGE %
 * ════════════════════════════════════════════════════════════════════════ */

static void sort_stocks_by_change(StockData *stocks, PriceHistory *histories, int count) {
    int i, j;
    for (i = 0; i < count - 1; i++) {
        for (j = 0; j < count - 1 - i; j++) {
            if (stocks[j].change_percent < stocks[j + 1].change_percent) {
                StockData  tmp_s = stocks[j];
                PriceHistory tmp_h = histories[j];
                stocks[j]    = stocks[j + 1];
                histories[j] = histories[j + 1];
                stocks[j + 1]    = tmp_s;
                histories[j + 1] = tmp_h;
            }
        }
    }
}

/* ════════════════════════════════════════════════════════════════════════
 * KEYBOARD HANDLER
 * ════════════════════════════════════════════════════════════════════════ */

static void handle_keypress(char c, StockData *stocks, PriceHistory *histories,
                             int count, int *interval, int *paused,
                             int *show_insights, int *insight_idx) {
    switch (c) {
        case 'q': case 'Q':
            keep_running = 0;
            break;
        case 's': case 'S':
            sort_stocks_by_change(stocks, histories, count);
            break;
        case '+': case '=':
            *interval -= 1;
            if (*interval < 2) *interval = 2;
            break;
        case '-':
            *interval += 2;
            if (*interval > 30) *interval = 30;
            break;
        case 'p': case 'P':
            *paused = !(*paused);
            break;
        case 'i': case 'I':
            if (!(*show_insights)) {
                *show_insights = 1;
                *insight_idx   = 0;
            } else {
                *insight_idx = (*insight_idx + 1) % count;
            }
            break;
        case '\033':
            *show_insights = 0;
            break;
        default:
            break;
    }
}

/* ════════════════════════════════════════════════════════════════════════
 * SIMULATED INDIAN STOCK DATA  (used when API quota is exhausted)
 * ════════════════════════════════════════════════════════════════════════ */

typedef struct {
    const char *symbol;
    const char *name;
    double base_price;
} IndianStockInfo;

static const IndianStockInfo INDIAN_STOCK_DATA[] = {
    {"RELIANCE.BSE", "Reliance Industries", 2963.10},
    {"TCS.BSE",      "Tata Consultancy",    4051.40},
    {"HDFCBANK.BSE", "HDFC Bank",           1672.20},
    {"INFY.BSE",     "Infosys",             1845.75},
    {"ICICIBANK.BSE","ICICI Bank",          1256.30},
    {"HINDUNILVR.BSE","Hindustan Unilever", 2534.80},
    {"SBIN.BSE",     "State Bank of India",  825.45},
    {"BHARTIARTL.BSE","Bharti Airtel",      1678.90},
    {"KOTAKBANK.BSE","Kotak Mahindra Bank", 1823.60},
    {"ITC.BSE",      "ITC Limited",          465.25}
};
#define INDIAN_STOCK_COUNT 10

static int get_simulated_indian_stock(const char *symbol, StockData *stock) {
    int i;
    double variation;
    struct timeval tv;

    for (i = 0; i < INDIAN_STOCK_COUNT; i++) {
        if (strcasecmp(symbol, INDIAN_STOCK_DATA[i].symbol) == 0) {
            if (is_indian_market_open()) {
                /* Market open: vary price each refresh */
                gettimeofday(&tv, NULL);
                srand((unsigned int)(tv.tv_sec * 1000 + tv.tv_usec / 1000)
                      + (unsigned int)i * 31337);
            } else {
                /* Market closed: fix price for the entire calendar day */
                time_t t = time(NULL);
                struct tm *lt = localtime(&t);
                srand((unsigned int)(lt->tm_year * 10000 + lt->tm_mon * 100 + lt->tm_mday)
                      + (unsigned int)i * 31337);
            }
            variation = ((rand() % 400) - 200) / 10000.0;

            strncpy(stock->symbol, symbol, MAX_SYMBOL_LENGTH - 1);
            strncpy(stock->name, INDIAN_STOCK_DATA[i].name, MAX_COMPANY_NAME - 1);
            stock->current_price = INDIAN_STOCK_DATA[i].base_price * (1.0 + variation);
            stock->previous_close = INDIAN_STOCK_DATA[i].base_price;
            stock->change = stock->current_price - stock->previous_close;
            stock->change_percent = (stock->change / stock->previous_close) * 100.0;
            stock->open  = INDIAN_STOCK_DATA[i].base_price * (1.0 + variation * 0.5);
            stock->high  = stock->current_price * 1.01;
            stock->low   = stock->current_price * 0.99;
            stock->timestamp = time(NULL);
            stock->valid = 1;
            return 0;
        }
    }
    return -1;
}

/* ════════════════════════════════════════════════════════════════════════
 * SIMULATED US STOCK DATA  (used for reliable demo data)
 * ════════════════════════════════════════════════════════════════════════ */

typedef struct {
    const char *symbol;
    const char *name;
    double base_price;
} USStockInfo;

static const USStockInfo US_STOCK_DATA[] = {
    {"AAPL",  "Apple Inc.",           189.42},
    {"MSFT",  "Microsoft Corp",       421.10},
    {"GOOGL", "Alphabet Inc",         165.38},
    {"AMZN",  "Amazon.com Inc",       185.74},
    {"NVDA",  "NVIDIA Corp",          875.20},
    {"META",  "Meta Platforms",       570.45},
    {"TSLA",  "Tesla Inc",            178.90},
    {"BRK.B", "Berkshire Hathaway",   465.80},
    {"JPM",   "JPMorgan Chase",       234.60},
    {"V",     "Visa Inc",             280.35},
    {"AMD",   "Advanced Micro Dev",   154.70},
    {"INTC",  "Intel Corp",            25.40},
    {"CRM",   "Salesforce Inc",       290.15},
    {"ORCL",  "Oracle Corp",          145.55},
    {"ADBE",  "Adobe Inc",            395.80},
};
#define US_STOCK_COUNT 15

static int get_simulated_us_stock(const char *symbol, StockData *stock) {
    struct timeval tv;
    double variation;
    int i;

    for (i = 0; i < US_STOCK_COUNT; i++) {
        if (strcasecmp(symbol, US_STOCK_DATA[i].symbol) == 0) {
            if (is_market_open()) {
                /* Market open: vary price each refresh */
                gettimeofday(&tv, NULL);
                srand((unsigned int)(tv.tv_sec * 1000 + tv.tv_usec / 1000)
                      + (unsigned int)i * 31337);
            } else {
                /* Market closed: fix price for the entire calendar day */
                time_t t = time(NULL);
                struct tm *lt = localtime(&t);
                srand((unsigned int)(lt->tm_year * 10000 + lt->tm_mon * 100 + lt->tm_mday)
                      + (unsigned int)i * 31337);
            }
            variation = ((rand() % 100) - 50) / 10000.0; /* ±0.5% */

            strncpy(stock->symbol, symbol, MAX_SYMBOL_LENGTH - 1);
            strncpy(stock->name, US_STOCK_DATA[i].name, MAX_COMPANY_NAME - 1);
            stock->current_price  = US_STOCK_DATA[i].base_price * (1.0 + variation);
            stock->previous_close = US_STOCK_DATA[i].base_price;
            stock->change         = stock->current_price - stock->previous_close;
            stock->change_percent = variation * 100.0;
            stock->high      = stock->current_price * 1.003;
            stock->low       = stock->current_price * 0.997;
            stock->open      = US_STOCK_DATA[i].base_price * (1.0 + variation * 0.4);
            stock->timestamp = tv.tv_sec;
            stock->valid     = 1;
            return 0;
        }
    }
    return -1;
}

static int monitoring_indian_stocks = 0;

void display_stock_table(StockData *stocks, PriceHistory *histories, int count) {
    int i;
    char time_str[16];
    char us_time_str[24];
    char ist_time_str[24];
    char price_str[32];
    char chart_buf[64];
    int market_open;

    get_current_time_string(time_str, sizeof(time_str));
    get_us_time_string(us_time_str, sizeof(us_time_str));
    get_ist_time_string(ist_time_str, sizeof(ist_time_str));

    clear_screen();
    print_header();

    if (monitoring_indian_stocks) {
        market_open = is_indian_market_open();
        printf("  %sTime: %s%s", COLOR_YELLOW, time_str, COLOR_RESET);
        printf("  |  %sIST: %s%s", COLOR_CYAN, ist_time_str, COLOR_RESET);
        printf("  |  %s🇮🇳 NSE/BSE%s", COLOR_CYAN, COLOR_RESET);
    } else {
        market_open = is_market_open();
        printf("  %sTime: %s%s", COLOR_YELLOW, time_str, COLOR_RESET);
        printf("  |  %s🇺🇸 US: %s%s", COLOR_CYAN, us_time_str, COLOR_RESET);
        printf("  |  %sNYSE/NASDAQ%s", COLOR_CYAN, COLOR_RESET);
    }
    if (market_open)
        printf("  |  Market: %sOPEN%s\n\n", COLOR_GREEN, COLOR_RESET);
    else {
        printf("  |  Market: %sCLOSED%s\n", COLOR_RED, COLOR_RESET);
        if (monitoring_indian_stocks)
            printf("  %s⚠  NSE/BSE is closed · Showing last prices · Opens Mon–Fri 9:15 AM – 3:30 PM IST%s\n\n",
                   COLOR_YELLOW, COLOR_RESET);
        else
            printf("  %s⚠  NYSE/NASDAQ is closed · Showing last prices · Opens Mon–Fri 9:30 AM – 4:00 PM ET%s\n\n",
                   COLOR_YELLOW, COLOR_RESET);
    }

    /* Table header — column order: SYMBOL | PRICE | CHANGE | CHANGE% | CHART | TREND
     * Total display width: 2+18+1+12+1+10+1+9+3+12+3+5 = 77 cols */
    printf("  %s%-18s %12s %10s %9s   %-12s   %s%s\n",
           COLOR_BOLD,
           "SYMBOL", "PRICE", "CHANGE", "CHANGE%", "CHART", "TREND",
           COLOR_RESET);
    /* Inline separator — 75 dashes after 2-space indent = 77 cols total,
     * covering all columns including TREND */
    printf("  %s%s%s\n", COLOR_CYAN,
           "─────────────────────────────────────────────────────────────────────────────",
           COLOR_RESET);

    for (i = 0; i < count; i++) {
        printf("\n");  /* blank line before each row for breathing room */

        int market_closed = is_indian_stock(stocks[i].symbol)
                            ? !is_indian_market_open()
                            : !is_market_open();

        if (!stocks[i].valid) {
            printf("  %s%-18s%s  %sN/A%s\n",
                   COLOR_CYAN, stocks[i].symbol, COLOR_RESET,
                   COLOR_RED, COLOR_RESET);
            continue;
        }

        /* render_sparkline always outputs exactly 12 display columns */
        if (histories != NULL)
            render_sparkline(&histories[i], chart_buf, sizeof(chart_buf));
        else {
            memset(chart_buf, ' ', 12);
            chart_buf[12] = '\0';
        }

        format_price(stocks[i].current_price, price_str, sizeof(price_str));

        const char *color       = get_trend_color(stocks[i].change);
        const char *arrow       = get_trend_arrow(stocks[i].change);
        const char *chart_color = (stocks[i].change >= 0.0) ? COLOR_GREEN : COLOR_RED;

        /* Each field printed separately so color codes don't confuse printf widths */
        printf("  %s%-18s%s", COLOR_CYAN, stocks[i].symbol, COLOR_RESET);
        printf(" %s%12s%s",    color, price_str, COLOR_RESET);
        printf(" %s%+10.2f%s", color, stocks[i].change, COLOR_RESET);
        printf(" %s%+8.2f%%%s", color, stocks[i].change_percent, COLOR_RESET);
        printf("   %s%s%s",    chart_color, chart_buf, COLOR_RESET); /* 12 display cols */
        if (market_closed)
            printf("  %s[CLOSED]%s", COLOR_YELLOW, COLOR_RESET);
        printf("     %s\n",    arrow);
    }

    printf("\n");
    printf("  %s%s%s\n", COLOR_CYAN,
           "─────────────────────────────────────────────────────────────────────────────",
           COLOR_RESET);
    printf("\n");
}

/* ════════════════════════════════════════════════════════════════════════
 * TOP MOVERS
 * ════════════════════════════════════════════════════════════════════════ */

static void display_movers_for_market(int market_type) {
    /* market_type: 1 = US, 2 = India */
    static const char *US_MOVERS[] = {
        "AAPL", "MSFT", "GOOGL", "TSLA", "AMZN",
        "NVDA", "META", "AMD",   "INTC", "ORCL"
    };
    static const char *INDIAN_MOVERS[] = {
        "RELIANCE.BSE", "TCS.BSE", "HDFCBANK.BSE", "INFY.BSE", "ICICIBANK.BSE",
        "SBIN.BSE", "BHARTIARTL.BSE", "KOTAKBANK.BSE", "ITC.BSE", "HINDUNILVR.BSE"
    };
    
    const char **movers;
    int count;
    const char *market_name;
    const char *currency;
    const char *flag;
    
    if (market_type == 1) {
        movers = US_MOVERS;
        count = 10;
        market_name = "US (NYSE/NASDAQ)";
        currency = "$";
        flag = "🇺🇸";
    } else {
        movers = INDIAN_MOVERS;
        count = 10;
        market_name = "India (NSE/BSE)";
        currency = "₹";
        flag = "🇮🇳";
    }
    
    const int SHOW_N = 5;
    StockData stocks[MAX_STOCKS];
    int i, j;

    printf("\n");
    printf("%s%s╔══════════════════════════════════════════════════════════════╗%s\n",
           COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s%s║          %s  Top Market Movers — %s          ║%s\n",
           COLOR_BOLD, COLOR_CYAN, flag, market_name, COLOR_RESET);
    printf("%s%s╚══════════════════════════════════════════════════════════════╝%s\n\n",
           COLOR_BOLD, COLOR_CYAN, COLOR_RESET);

    /* Fetch spinner */
    printf("  Fetching data for %d stocks", count);
    fflush(stdout);

    /* Initialize and fetch stocks */
    for (i = 0; i < count; i++) {
        memset(&stocks[i], 0, sizeof(StockData));
        strncpy(stocks[i].symbol, movers[i], MAX_SYMBOL_LENGTH - 1);
        if (market_type == 1) {
            if (get_simulated_us_stock(movers[i], &stocks[i]) == 0)
                stocks[i].valid = 1;
        } else {
            if (get_simulated_indian_stock(movers[i], &stocks[i]) == 0)
                stocks[i].valid = 1;
        }
        printf(".");
        fflush(stdout);
    }
    printf(" %sdone%s\n\n", COLOR_GREEN, COLOR_RESET);

    /* Bubble sort descending by change_percent */
    for (i = 0; i < count - 1; i++) {
        for (j = 0; j < count - 1 - i; j++) {
            if (stocks[j].change_percent < stocks[j + 1].change_percent) {
                StockData tmp = stocks[j];
                stocks[j] = stocks[j + 1];
                stocks[j + 1] = tmp;
            }
        }
    }

    /* ── Two-column panel ─────────────────────────────────────────── */
    printf("  %s🟢 TOP GAINERS%s                            %s🔴 TOP LOSERS%s\n",
           COLOR_GREEN, COLOR_RESET, COLOR_RED, COLOR_RESET);
    printf("  %s────────────────────────────────────%s",   COLOR_GREEN, COLOR_RESET);
    printf("  %s────────────────────────────────────%s\n", COLOR_RED,   COLOR_RESET);

    for (i = 0; i < SHOW_N; i++) {
        int li = count - 1 - i;          /* loser index (bottom of sorted list) */

        /* Left: gainer - fixed width formatting */
        if (stocks[i].valid && stocks[i].change_percent > 0) {
            printf("  %s%-14s%s %s%9.2f %s%+7.2f%%%s %s",
                   COLOR_CYAN, stocks[i].symbol, COLOR_RESET,
                   currency, stocks[i].current_price,
                   COLOR_GREEN, stocks[i].change_percent, COLOR_RESET,
                   get_trend_arrow(stocks[i].change));
        } else if (stocks[i].valid) {
            printf("  %s%-14s%s %s%9.2f %s%+7.2f%%%s  ",
                   COLOR_CYAN, stocks[i].symbol, COLOR_RESET,
                   currency, stocks[i].current_price,
                   COLOR_YELLOW, stocks[i].change_percent, COLOR_RESET);
        } else {
            printf("  %s%-14s%s     %s%10s%s      ",
                   COLOR_CYAN, stocks[i].symbol, COLOR_RESET,
                   COLOR_YELLOW, "no data", COLOR_RESET);
        }

        /* Right: loser - fixed width formatting */
        if (stocks[li].valid && stocks[li].change_percent < 0) {
            printf("  %s%-14s%s %s%9.2f %s%+7.2f%%%s %s",
                   COLOR_CYAN, stocks[li].symbol, COLOR_RESET,
                   currency, stocks[li].current_price,
                   COLOR_RED, stocks[li].change_percent, COLOR_RESET,
                   get_trend_arrow(stocks[li].change));
        } else if (stocks[li].valid) {
            printf("  %s%-14s%s %s%9.2f %s%+7.2f%%%s  ",
                   COLOR_CYAN, stocks[li].symbol, COLOR_RESET,
                   currency, stocks[li].current_price,
                   COLOR_YELLOW, stocks[li].change_percent, COLOR_RESET);
        } else {
            printf("  %s%-14s%s     %s%10s%s      ",
                   COLOR_CYAN, stocks[li].symbol, COLOR_RESET,
                   COLOR_YELLOW, "no data", COLOR_RESET);
        }
        printf("\n");
    }

    printf("  %s────────────────────────────────────%s",   COLOR_GREEN, COLOR_RESET);
    printf("  %s────────────────────────────────────%s\n\n", COLOR_RED, COLOR_RESET);

    /* Market summary line */
    double total_change = 0;
    int valid_count = 0;
    int up_count = 0, down_count = 0;
    char top_gainer[MAX_SYMBOL_LENGTH] = "";
    char top_loser[MAX_SYMBOL_LENGTH] = "";
    double max_gain = -999, max_loss = 999;
    
    for (i = 0; i < count; i++) {
        if (stocks[i].valid) {
            total_change += stocks[i].change_percent;
            valid_count++;
            if (stocks[i].change_percent > 0) up_count++;
            else if (stocks[i].change_percent < 0) down_count++;
            
            if (stocks[i].change_percent > max_gain) {
                max_gain = stocks[i].change_percent;
                strncpy(top_gainer, stocks[i].symbol, MAX_SYMBOL_LENGTH - 1);
            }
            if (stocks[i].change_percent < max_loss) {
                max_loss = stocks[i].change_percent;
                strncpy(top_loser, stocks[i].symbol, MAX_SYMBOL_LENGTH - 1);
            }
        }
    }
    
    if (valid_count > 0) {
        double avg = total_change / valid_count;
        const char *col = avg >= 0 ? COLOR_GREEN : COLOR_RED;
        const char *sentiment = avg > 0.3 ? "Bullish" : avg < -0.3 ? "Bearish" : "Mixed";
        printf("  %sMarket Sentiment:%s %s%s%s (%s%+.2f%%%s avg across %d stocks)\n\n",
               COLOR_BOLD, COLOR_RESET, col, sentiment, COLOR_RESET, col, avg, COLOR_RESET, valid_count);
    }

    /* ── AI Insights Section ─────────────────────────────────────────── */
    printf("  %s%s═══ AI Market Analysis ═══%s\n\n", COLOR_BOLD, COLOR_MAGENTA, COLOR_RESET);
    printf("  %sFetching AI insights for %s market...%s\n", COLOR_CYAN, market_name, COLOR_RESET);
    fflush(stdout);

    char ai_response[2048] = "";
    int ai_result = groq_market_overview(stocks, count, ai_response, sizeof(ai_response));
    
    if (ai_result == 0 && ai_response[0]) {
        printf("\n  %s🤖 AI Commentary:%s\n", COLOR_BOLD, COLOR_RESET);
        printf("  %s", COLOR_WHITE);
        /* Word wrap the AI response */
        const char *wp = ai_response;
        int col = 2;
        printf("  ");
        while (*wp) {
            const char *word_end = wp;
            while (*word_end && *word_end != ' ' && *word_end != '\n') word_end++;
            int wlen = (int)(word_end - wp);
            if (col > 2 && col + wlen > 70) {
                printf("\n  ");
                col = 2;
            }
            while (wp < word_end) { putchar(*wp++); col++; }
            if (*wp == '\n') { printf("\n  "); col = 2; wp++; }
            else if (*wp == ' ') { putchar(' '); col++; wp++; }
        }
        printf("%s\n\n", COLOR_RESET);
    } else {
        printf("  %s(AI insights unavailable - check GROQ_API_KEY)%s\n\n", COLOR_YELLOW, COLOR_RESET);
    }
}

int display_top_movers(void) {
    int choice;
    char input[16];

    printf("\n");
    printf("%s%s╔══════════════════════════════════════════════════════════════╗%s\n",
           COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s%s║              📊  MarketPulse — Top Market Movers             ║%s\n",
           COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s%s╚══════════════════════════════════════════════════════════════╝%s\n\n",
           COLOR_BOLD, COLOR_CYAN, COLOR_RESET);

    printf("  %sSelect Market:%s\n\n", COLOR_BOLD, COLOR_RESET);
    printf("    %s1)%s 🇺🇸 US Stocks (NYSE/NASDAQ)\n", COLOR_GREEN, COLOR_RESET);
    printf("    %s2)%s 🇮🇳 Indian Stocks (NSE/BSE)\n", COLOR_GREEN, COLOR_RESET);
    printf("\n");
    printf("  Enter choice (1-2): ");
    fflush(stdout);

    if (fgets(input, sizeof(input), stdin) == NULL) {
        return -1;
    }
    choice = atoi(input);

    if (choice == 1) {
        display_movers_for_market(1);
    } else if (choice == 2) {
        display_movers_for_market(2);
    } else {
        printf("  %sInvalid choice. Showing US stocks...%s\n", COLOR_YELLOW, COLOR_RESET);
        display_movers_for_market(1);
    }

    return 0;
}

/* ════════════════════════════════════════════════════════════════════════
 * SINGLE STOCK DISPLAY
 * ════════════════════════════════════════════════════════════════════════ */

int fetch_and_display_single(const char *symbol) {
    char response[MAX_BUFFER_SIZE];
    char profile_response[MAX_BUFFER_SIZE];
    StockData stock;
    char time_str[16];
    char price_str[32];
    char change_str[64];
    int is_indian = is_indian_stock(symbol);
    int got_data = 0;

    memset(&stock, 0, sizeof(StockData));
    strncpy(stock.symbol, symbol, MAX_SYMBOL_LENGTH - 1);

    printf("\nFetching data for %s%s%s...\n", COLOR_BOLD, symbol, COLOR_RESET);

    /* Try simulated data first (priority for Indian stocks) */
    if (is_indian) {
        set_currency_mode(1);
        if (get_simulated_indian_stock(symbol, &stock) == 0) {
            got_data = 1;
        }
    } else {
        /* For US stocks, try simulated data first, then API */
        if (get_simulated_us_stock(symbol, &stock) == 0) {
            got_data = 1;
            /* Try to get real price from API to overlay */
            if (fetch_stock_quote(symbol, response, sizeof(response)) == 0) {
                StockData api_stock;
                memset(&api_stock, 0, sizeof(StockData));
                if (parse_stock_quote(response, &api_stock) == 0 && api_stock.current_price > 0) {
                    stock.current_price = api_stock.current_price;
                    stock.change = stock.current_price - stock.previous_close;
                    stock.change_percent = (stock.change / stock.previous_close) * 100.0;
                }
            }
        } else {
            /* Symbol not in simulation - try API directly */
            if (fetch_stock_quote(symbol, response, sizeof(response)) == 0) {
                if (parse_stock_quote(response, &stock) == 0 && stock.current_price > 0) {
                    got_data = 1;
                }
            }
        }
    }

    if (!got_data) {
        fprintf(stderr, "%sError: No data available for %s%s\n",
                COLOR_RED, symbol, COLOR_RESET);
        return -1;
    }

    /* Try to get company profile for name */
    if (stock.name[0] == '\0') {
        if (fetch_company_profile(symbol, profile_response, sizeof(profile_response)) == 0)
            parse_company_profile(profile_response, &stock);
    }

    if (stock.name[0] == '\0')
        strncpy(stock.name, symbol, MAX_COMPANY_NAME - 1);

    get_current_time_string(time_str, sizeof(time_str));
    format_price(stock.current_price, price_str, sizeof(price_str));
    format_change(stock.change, stock.change_percent, change_str, sizeof(change_str));

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
    if (is_indian) {
        printf("  %sOpen:%s       ₹%.2f\n", COLOR_BOLD, COLOR_RESET, stock.open);
        printf("  %sHigh:%s       ₹%.2f\n", COLOR_BOLD, COLOR_RESET, stock.high);
        printf("  %sLow:%s        ₹%.2f\n", COLOR_BOLD, COLOR_RESET, stock.low);
        printf("  %sPrev Close:%s ₹%.2f\n", COLOR_BOLD, COLOR_RESET, stock.previous_close);
    } else {
        printf("  %sOpen:%s       $%.2f\n", COLOR_BOLD, COLOR_RESET, stock.open);
        printf("  %sHigh:%s       $%.2f\n", COLOR_BOLD, COLOR_RESET, stock.high);
        printf("  %sLow:%s        $%.2f\n", COLOR_BOLD, COLOR_RESET, stock.low);
        printf("  %sPrev Close:%s $%.2f\n", COLOR_BOLD, COLOR_RESET, stock.previous_close);
    }
    printf("\n");
    printf("  %sTime:%s       %s\n", COLOR_BOLD, COLOR_RESET, time_str);

    if (is_indian) {
        if (is_indian_market_open())
            printf("  %sMarket:%s     %sOPEN%s (NSE/BSE)\n",
                   COLOR_BOLD, COLOR_RESET, COLOR_GREEN, COLOR_RESET);
        else
            printf("  %sMarket:%s     %sCLOSED%s (NSE/BSE)\n",
                   COLOR_BOLD, COLOR_RESET, COLOR_RED, COLOR_RESET);
    } else {
        if (is_market_open())
            printf("  %sMarket:%s     %sOPEN%s\n",
                   COLOR_BOLD, COLOR_RESET, COLOR_GREEN, COLOR_RESET);
        else
            printf("  %sMarket:%s     %sCLOSED%s\n",
                   COLOR_BOLD, COLOR_RESET, COLOR_RED, COLOR_RESET);
    }

    printf("\n");
    print_separator();

    /* ── AI Insights Section ─────────────────────────────────────────── */
    printf("\n");
    printf("  %s%s═══ AI Insights ═══%s\n\n", COLOR_BOLD, COLOR_MAGENTA, COLOR_RESET);
    printf("  %sFetching AI analysis for %s...%s\n", COLOR_CYAN, symbol, COLOR_RESET);
    fflush(stdout);

    char ai_response[2048] = "";
    int ai_result = groq_analyze_single_stock(symbol, &stock, ai_response, sizeof(ai_response));
    
    if (ai_result == 0 && ai_response[0]) {
        printf("\n  %s🤖 AI Commentary:%s\n", COLOR_BOLD, COLOR_RESET);
        printf("  %s", COLOR_WHITE);
        /* Word wrap the AI response */
        const char *wp = ai_response;
        int col = 2;
        printf("  ");
        while (*wp) {
            const char *word_end = wp;
            while (*word_end && *word_end != ' ' && *word_end != '\n') word_end++;
            int wlen = (int)(word_end - wp);
            if (col > 2 && col + wlen > 70) {
                printf("\n  ");
                col = 2;
            }
            while (wp < word_end) { putchar(*wp++); col++; }
            if (*wp == '\n') { printf("\n  "); col = 2; wp++; }
            else if (*wp == ' ') { putchar(' '); col++; wp++; }
        }
        printf("%s\n", COLOR_RESET);
    } else {
        printf("  %s(AI insights unavailable - check GROQ_API_KEY)%s\n", COLOR_YELLOW, COLOR_RESET);
    }

    printf("\n");
    print_separator();
    return 0;
}

/* ════════════════════════════════════════════════════════════════════════
 * PARALLEL STOCK FETCH  (fork + pipe)
 * ════════════════════════════════════════════════════════════════════════ */

static void fetch_stock_child(const char *symbol, int write_fd) {
    char response[MAX_BUFFER_SIZE];
    StockData stock;
    char saved_symbol[MAX_SYMBOL_LENGTH];
    int fetch_result;

    strncpy(saved_symbol, symbol, MAX_SYMBOL_LENGTH - 1);
    memset(&stock, 0, sizeof(StockData));
    strncpy(stock.symbol, symbol, MAX_SYMBOL_LENGTH - 1);

    if (is_indian_stock(symbol)) {
        if (get_simulated_indian_stock(symbol, &stock) == 0) {
            stock.valid = 1;
        } else {
            fetch_result = fetch_indian_stock_quote(symbol, response, sizeof(response));
            if (fetch_result == 0 && parse_stock_quote(response, &stock) == 0)
                stock.valid = 1;
        }
    } else {
        /* Get real LTP from Finnhub (rate-limited; simulation is the fallback) */
        double real_ltp = 0.0;
        if (fetch_stock_quote(symbol, response, sizeof(response)) == 0) {
            StockData api_stock;
            memset(&api_stock, 0, sizeof(StockData));
            if (parse_stock_quote(response, &api_stock) == 0)
                real_ltp = api_stock.current_price;
        }
        /* Simulation provides change%, high, low, open, name */
        if (get_simulated_us_stock(symbol, &stock) == 0) {
            if (real_ltp > 0) {
                /* Overlay real price; recompute change relative to simulated close */
                stock.current_price  = real_ltp;
                stock.change         = real_ltp - stock.previous_close;
                stock.change_percent = (stock.change / stock.previous_close) * 100.0;
            }
            stock.valid = 1;
        } else {
            /* Symbol not in simulation table — use API result directly */
            if (parse_stock_quote(response, &stock) == 0)
                stock.valid = 1;
        }
    }

    strncpy(stock.symbol, saved_symbol, MAX_SYMBOL_LENGTH - 1);
    write(write_fd, &stock, sizeof(StockData));
    close(write_fd);
}

static int fetch_stocks_parallel(char symbols[][MAX_SYMBOL_LENGTH],
                                  int count, StockData *stocks) {
    int i;
    int pipes[MAX_STOCKS][2];
    pid_t pids[MAX_STOCKS];
    int success_count = 0;

    for (i = 0; i < count; i++) {
        if (pipe(pipes[i]) == -1) { perror("pipe"); continue; }

        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork");
            close(pipes[i][0]);
            close(pipes[i][1]);
            continue;
        }
        if (pids[i] == 0) {
            close(pipes[i][0]);
            fetch_stock_child(symbols[i], pipes[i][1]);
            exit(0);
        }
        close(pipes[i][1]);
        usleep(120000); /* stagger requests ~120ms apart to stay within API rate limit */
    }

    for (i = 0; i < count; i++) {
        if (pids[i] > 0) {
            ssize_t bytes = read(pipes[i][0], &stocks[i], sizeof(StockData));
            close(pipes[i][0]);
            if (bytes == sizeof(StockData) && stocks[i].valid)
                success_count++;
            else {
                /* Preserve last known price on transient failure (e.g. rate limit).
                 * Only clear if we've never had a successful fetch for this stock. */
                if (stocks[i].current_price == 0) {
                    memset(&stocks[i], 0, sizeof(StockData));
                    stocks[i].valid = 0;
                }
                strncpy(stocks[i].symbol, symbols[i], MAX_SYMBOL_LENGTH - 1);
            }
            waitpid(pids[i], NULL, 0);
        }
    }
    return success_count;
}

static int fetch_stocks_sequential(char symbols[][MAX_SYMBOL_LENGTH],
                                    int count, StockData *stocks) {
    int i, success_count = 0;
    char response[MAX_BUFFER_SIZE];
    char saved[MAX_SYMBOL_LENGTH];

    for (i = 0; i < count; i++) {
        strncpy(saved, symbols[i], MAX_SYMBOL_LENGTH - 1);

        int ok = 0;
        StockData fresh;
        memset(&fresh, 0, sizeof(StockData));
        strncpy(fresh.symbol, symbols[i], MAX_SYMBOL_LENGTH - 1);

        if (is_indian_stock(symbols[i])) {
            if (fetch_indian_stock_quote(symbols[i], response, sizeof(response)) == 0)
                ok = (parse_stock_quote(response, &fresh) == 0);
        } else {
            if (fetch_stock_quote(symbols[i], response, sizeof(response)) == 0)
                ok = (parse_stock_quote(response, &fresh) == 0);
        }
        if (ok) {
            stocks[i] = fresh;
            stocks[i].valid = 1;
            success_count++;
        } else if (stocks[i].current_price == 0) {
            /* No prior data — keep symbol but mark invalid */
            memset(&stocks[i], 0, sizeof(StockData));
            stocks[i].valid = 0;
        }
        /* else: keep stale data from last successful fetch */
        strncpy(stocks[i].symbol, saved, MAX_SYMBOL_LENGTH - 1);
        if (i < count - 1) usleep(100000);
    }
    return success_count;
}

/* ════════════════════════════════════════════════════════════════════════
 * MAIN WATCH LOOP  (multi-stock, continuous)
 * ════════════════════════════════════════════════════════════════════════ */

static PriceHistory price_histories[MAX_STOCKS];

int fetch_and_display_multiple(char symbols[][MAX_SYMBOL_LENGTH],
                                int count, int continuous) {
    StockData stocks[MAX_STOCKS];
    int i;
    int use_parallel = (count > 1);
    int has_indian  = 0;
    int current_interval = DEFAULT_REFRESH_INTERVAL;
    int paused = 0;
    int show_insights = 0;
    int insight_idx   = 0;

    /* Detect Indian stocks */
    for (i = 0; i < count; i++) {
        if (is_indian_stock(symbols[i])) { has_indian = 1; break; }
    }
    set_currency_mode(has_indian);
    monitoring_indian_stocks = has_indian;

    /* Initialise price histories */
    for (i = 0; i < count; i++)
        init_price_history(&price_histories[i]);

    /* ── Load persisted price history from mmap'd file ────────────── */
    PriceHistoryFile *hist_file = history_open(HISTORY_FILE);
    if (hist_file != NULL) {
        for (i = 0; i < count; i++)
            history_load(hist_file, symbols[i], &price_histories[i]);
    }

    /* ── Enable terminal raw mode for keyboard controls ──────────── */
    setup_raw_terminal();

    if (has_indian) {
        printf("%s[Indian Market]%s Starting monitor for %d NIFTY stock(s)...\n",
               COLOR_YELLOW, COLOR_RESET, count);
        printf("Currency: %s₹ INR%s  |  %s[Demo Mode]%s Simulated data\n",
               COLOR_GREEN, COLOR_RESET, COLOR_MAGENTA, COLOR_RESET);
    } else {
        printf("Starting monitor for %d stock(s)...\n", count);
    }
    printf("Press %s[Q]%s to quit or use keyboard controls.\n\n",
           COLOR_BOLD, COLOR_RESET);
    sleep(1);

    /* ════════════════════ MAIN LOOP ════════════════════ */
    while (keep_running) {

        /* Skip fetch when paused */
        if (!paused) {
            if (use_parallel)
                fetch_stocks_parallel(symbols, count, stocks);
            else
                fetch_stocks_sequential(symbols, count, stocks);

            /* Update price histories and persist to mmap */
            for (i = 0; i < count; i++) {
                if (stocks[i].valid) {
                    int market_live = is_indian_stock(stocks[i].symbol)
                                      ? is_indian_market_open()
                                      : is_market_open();
                    if (market_live) {
                        add_price(&price_histories[i], stocks[i].current_price);
                        if (hist_file != NULL)
                            history_save(hist_file, symbols[i], &price_histories[i]);
                    }
                }
            }
        }

        /* ── Render table ──────────────────────────────────────── */
        display_stock_table(stocks, price_histories, count);

        if (!continuous) break;

        /* ── [I] Market Overview panel (general market insights) ─ */
        if (show_insights) {
            /* Calculate market statistics */
            int up_count = 0, down_count = 0, valid_count = 0;
            double total_change = 0;
            char top_gainer[16] = "", top_loser[16] = "";
            double max_gain = -999, max_loss = 999;
            
            for (int i = 0; i < count; i++) {
                if (!stocks[i].valid) continue;
                valid_count++;
                total_change += stocks[i].change_percent;
                if (stocks[i].change_percent > 0) up_count++;
                else if (stocks[i].change_percent < 0) down_count++;
                
                if (stocks[i].change_percent > max_gain) {
                    max_gain = stocks[i].change_percent;
                    strncpy(top_gainer, stocks[i].symbol, 15);
                }
                if (stocks[i].change_percent < max_loss) {
                    max_loss = stocks[i].change_percent;
                    strncpy(top_loser, stocks[i].symbol, 15);
                }
            }
            
            double avg_change = valid_count > 0 ? total_change / valid_count : 0;
            const char *sentiment = avg_change > 0.5 ? "Bullish" : 
                                   avg_change < -0.5 ? "Bearish" : "Mixed";
            const char *sent_color = avg_change > 0.5 ? COLOR_GREEN : 
                                    avg_change < -0.5 ? COLOR_RED : COLOR_YELLOW;

            printf("\n  %s%s═══ Market Overview ═══%s\n\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
            printf("  📊 Overall Sentiment: %s%s%s (%d/%d stocks up)\n", 
                   sent_color, sentiment, COLOR_RESET, up_count, valid_count);
            printf("  📈 Top Gainer: %s%s (%+.2f%%)%s\n", 
                   COLOR_GREEN, top_gainer, max_gain, COLOR_RESET);
            printf("  📉 Top Loser:  %s%s (%.2f%%)%s\n", 
                   COLOR_RED, top_loser, max_loss, COLOR_RESET);
            printf("  ⚡ Avg Change: %s%+.2f%%%s\n\n", 
                   avg_change >= 0 ? COLOR_GREEN : COLOR_RED, avg_change, COLOR_RESET);

            /* Get AI market overview */
            printf("  %sFetching AI market analysis...%s\n", COLOR_CYAN, COLOR_RESET);
            fflush(stdout);
            
            char ai_overview[1024] = "";
            int r = groq_market_overview(stocks, count, ai_overview, sizeof(ai_overview));
            if (r == 0 && ai_overview[0]) {
                printf("\n  %s%s── AI Market Commentary ──%s\n", COLOR_BOLD, COLOR_MAGENTA, COLOR_RESET);
                printf("  %s🤖 ", COLOR_WHITE);
                /* Word wrap the AI response */
                const char *wp = ai_overview;
                int col = 5;
                while (*wp) {
                    const char *word_end = wp;
                    while (*word_end && *word_end != ' ') word_end++;
                    int wlen = (int)(word_end - wp);
                    if (col > 0 && col + wlen > 70) {
                        printf("\n     ");
                        col = 5;
                    }
                    while (wp < word_end) { putchar(*wp++); col++; }
                    if (*wp == ' ') { putchar(' '); col++; wp++; }
                }
                printf("%s\n\n", COLOR_RESET);
            } else {
                printf("  %s(AI market overview unavailable)%s\n\n", COLOR_YELLOW, COLOR_RESET);
            }

            printf("  %sTip:%s Use '%s./marketpulse insight SYMBOL%s' for individual stock AI analysis\n",
                   COLOR_CYAN, COLOR_RESET, COLOR_BOLD, COLOR_RESET);
            printf("  %s[Esc]%s Close overview\n\n", COLOR_BOLD, COLOR_RESET);
        }

        /* ── Keyboard controls hint ────────────────────────────── */
        if (paused) {
            printf("  %s⏸  PAUSED%s  — press %s[P]%s to resume\n",
                   COLOR_YELLOW, COLOR_RESET, COLOR_BOLD, COLOR_RESET);
        } else {
            printf("  %s[Q]%s Quit  %s[S]%s Sort  "
                   "%s[+]%s Faster  %s[-]%s Slower  %s[P]%s Pause  "
                   "%s[I]%s Insights"
                   "  — refresh: %s%ds%s\n",
                   COLOR_BOLD, COLOR_RESET,
                   COLOR_BOLD, COLOR_RESET,
                   COLOR_BOLD, COLOR_RESET,
                   COLOR_BOLD, COLOR_RESET,
                   COLOR_BOLD, COLOR_RESET,
                   COLOR_BOLD, COLOR_RESET,
                   COLOR_CYAN, current_interval, COLOR_RESET);
        }
        fflush(stdout);
        printf("\033[?25h"); /* restore cursor after full frame is rendered */
        fflush(stdout);
        for (i = 0; i < current_interval && keep_running; i++) {
            fd_set fds;
            struct timeval tv = {1, 0};
            FD_ZERO(&fds);
            FD_SET(STDIN_FILENO, &fds);
            int ret = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
            if (ret > 0 && FD_ISSET(STDIN_FILENO, &fds)) {
                char ch = 0;
                if (read(STDIN_FILENO, &ch, 1) == 1)
                    handle_keypress(ch, stocks, price_histories, count,
                                    &current_interval, &paused,
                                    &show_insights, &insight_idx);
                /* Break the wait loop immediately on keypress */
                break;
            }
        }

        /* If paused, keep spinning until P or Q is pressed */
        while (paused && keep_running) {
            fd_set fds;
            struct timeval tv = {1, 0};
            FD_ZERO(&fds);
            FD_SET(STDIN_FILENO, &fds);
            if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
                char ch = 0;
                if (read(STDIN_FILENO, &ch, 1) == 1)
                    handle_keypress(ch, stocks, price_histories, count,
                                    &current_interval, &paused,
                                    &show_insights, &insight_idx);
            }
        }
    }

    /* ── Cleanup ─────────────────────────────────────────────────── */
    restore_terminal();
    if (hist_file != NULL)
        history_close(hist_file);

    printf("\n%sMonitoring stopped.%s\n", COLOR_YELLOW, COLOR_RESET);
    return 0;
}

/* ════════════════════════════════════════════════════════════════════════
 * PUBLIC INTERFACE
 * ════════════════════════════════════════════════════════════════════════ */

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

void stop_monitoring(void) {
    keep_running = 0;
}
