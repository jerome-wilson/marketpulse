/*
 * MarketPulse - Real-Time Stock Monitoring and Insight Engine
 * ai_insights.c — Groq AI API integration (with Gemini fallback)
 *
 * Sends stock data to the Groq API (Llama 3.3) and returns a
 * natural-language 2-sentence trading commentary.
 *
 * Requires: GROQ_API_KEY environment variable or config/groq_key file
 * API key:  https://console.groq.com (free tier - 30 req/min)
 *
 * Reuses make_https_post() from network.c — same SSL/TLS stack used
 * for Finnhub requests (socket, SSL_connect, SSL_write, SSL_read).
 */

#include "marketpulse.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

/* External function declarations */
extern int is_indian_stock(const char *symbol);

/* ─────────────────────────────────────────────────────────────────────────
 * parse_groq_response — extract the content from Groq's OpenAI-compatible response
 *
 * Groq response structure:
 *   { "choices": [{ "message": { "content": "..." } }] }
 * ─────────────────────────────────────────────────────────────────────── */
static int parse_groq_response(const char *json, char *text_out, size_t size) {
    if (!json || !text_out || size == 0) return -1;

    /* Find "content": in the response */
    const char *pos = strstr(json, "\"content\":");
    if (!pos) return -1;

    pos += 10; /* skip "content": */
    while (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r')
        pos++;
    if (*pos != '"') return -1;
    pos++; /* skip opening quote */

    size_t out_pos = 0;
    while (*pos && out_pos + 1 < size) {
        if (*pos == '"') break;          /* closing quote */
        if (*pos == '\\' && *(pos+1)) {
            pos++;
            switch (*pos) {
                case 'n':  text_out[out_pos++] = ' '; break; /* flatten newlines */
                case '"':  text_out[out_pos++] = '"'; break;
                case '\\': text_out[out_pos++] = '\\'; break;
                default:   text_out[out_pos++] = *pos; break;
            }
        } else {
            text_out[out_pos++] = *pos;
        }
        pos++;
    }
    text_out[out_pos] = '\0';
    return (out_pos > 0) ? 0 : -1;
}

/* ─────────────────────────────────────────────────────────────────────────
 * gemini_analyze_stock — call Groq API for a 2-sentence stock commentary
 *
 * Note: Function name kept as gemini_analyze_stock for compatibility
 * but now uses Groq API (faster, more generous free tier)
 *
 * Returns: 0 on success, -1 if key missing or network/parse error.
 * ─────────────────────────────────────────────────────────────────────── */
int gemini_analyze_stock(const char *symbol,
                          StockData  *stock,
                          AIInsight  *insight,
                          char       *response_buf,
                          size_t      size) {
    if (!symbol || !stock || !insight || !response_buf || size == 0)
        return -1;

    /* API key: env var first, then config/groq_key file */
    const char *api_key = getenv("GROQ_API_KEY");
    static char key_from_file[128] = "";
    if (!api_key || api_key[0] == '\0') {
        if (key_from_file[0] == '\0') {
            FILE *kf = fopen("config/groq_key", "r");
            if (kf) {
                if (fgets(key_from_file, sizeof(key_from_file), kf))
                    key_from_file[strcspn(key_from_file, "\n\r")] = '\0';
                fclose(kf);
            }
        }
        if (key_from_file[0]) api_key = key_from_file;
    }
    if (!api_key || api_key[0] == '\0') return -1;

    /* Determine currency symbol */
    int is_indian = is_indian_stock(symbol);
    const char *currency = is_indian ? "₹" : "$";

    /* ── Build the prompt ── */
    char prompt[512];
    snprintf(prompt, sizeof(prompt),
             "Analyze this stock in exactly 2 sentences for a short-term trader. "
             "Be specific, direct, and concise. "
             "Stock: %s, Price: %s%.2f, Change: %+.2f%%, "
             "Trend: %s, Momentum: %s, "
             "MA5: %s%.2f, MA10: %s%.2f, Volatility: %.2f%%.",
             symbol,
             currency, stock->current_price,
             stock->change_percent,
             insight->trend,
             insight->momentum,
             currency, insight->moving_avg_5,
             currency, insight->moving_avg_10,
             insight->volatility);

    /* ── Build JSON request body (OpenAI-compatible format) ── */
    char json_body[1024];
    /* Escape the prompt for JSON (replace " with \") */
    char escaped_prompt[512];
    size_t ep = 0;
    for (const char *p = prompt; *p && ep + 2 < sizeof(escaped_prompt); p++) {
        if (*p == '"')      { escaped_prompt[ep++] = '\\'; escaped_prompt[ep++] = '"'; }
        else if (*p == '\n'){ escaped_prompt[ep++] = '\\'; escaped_prompt[ep++] = 'n'; }
        else                  escaped_prompt[ep++] = *p;
    }
    escaped_prompt[ep] = '\0';

    snprintf(json_body, sizeof(json_body),
             "{\"model\":\"%s\","
             "\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}],"
             "\"max_tokens\":150,"
             "\"temperature\":0.4}",
             GROQ_MODEL, escaped_prompt);

    /* ── Build the path ── */
    const char *path = "/openai/v1/chat/completions";

    /* ── Make the HTTPS POST request with Bearer auth ── */
    char raw_response[MAX_BUFFER_SIZE];
    
    /* We need to add Authorization header - modify the request */
    /* For now, use a custom approach since make_https_post doesn't support custom headers */
    
    /* Build full request manually */
    char request[2048];
    snprintf(request, sizeof(request),
             "POST %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Authorization: Bearer %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             path, GROQ_HOST, api_key, strlen(json_body), json_body);

    /* Create SSL connection and send request */
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) return -1;
    
    SSL *ssl = NULL;
    int sockfd = -1;
    int result = -1;
    
    /* Resolve hostname */
    struct hostent *server = gethostbyname(GROQ_HOST);
    if (!server) goto cleanup;
    
    /* Create socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) goto cleanup;
    
    /* Connect */
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(GROQ_PORT);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        goto cleanup;
    
    /* SSL handshake */
    ssl = SSL_new(ctx);
    if (!ssl) goto cleanup;
    
    SSL_set_fd(ssl, sockfd);
    SSL_set_tlsext_host_name(ssl, GROQ_HOST);
    
    if (SSL_connect(ssl) <= 0) goto cleanup;
    
    /* Send request */
    if (SSL_write(ssl, request, strlen(request)) <= 0) goto cleanup;
    
    /* Read response */
    int total = 0;
    int bytes;
    while ((bytes = SSL_read(ssl, raw_response + total, sizeof(raw_response) - total - 1)) > 0) {
        total += bytes;
        if (total >= (int)sizeof(raw_response) - 1) break;
    }
    raw_response[total] = '\0';
    
    /* Check for errors in response */
    if (strstr(raw_response, "\"error\"") != NULL) {
        if (strstr(raw_response, "rate_limit") != NULL ||
            strstr(raw_response, "429") != NULL) {
            snprintf(response_buf, size, 
                     "[Rate limited] Groq free tier: wait a moment and try again");
        } else {
            snprintf(response_buf, size, 
                     "[API Error] Check your Groq API key at console.groq.com");
        }
        result = 0;
        goto cleanup;
    }
    
    /* Parse the response */
    result = parse_groq_response(raw_response, response_buf, size);

cleanup:
    if (ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    if (sockfd >= 0) close(sockfd);
    if (ctx) SSL_CTX_free(ctx);
    
    return result;
}

/* ─────────────────────────────────────────────────────────────────────────
 * groq_market_overview — generate AI commentary for multiple stocks
 *
 * Analyzes all watched stocks and provides a market overview.
 * ─────────────────────────────────────────────────────────────────────── */
int groq_market_overview(StockData *stocks, int count, char *response_buf, size_t size) {
    if (!stocks || count <= 0 || !response_buf || size == 0)
        return -1;

    /* API key: env var first, then config/groq_key file */
    const char *api_key = getenv("GROQ_API_KEY");
    static char key_from_file[128] = "";
    if (!api_key || api_key[0] == '\0') {
        if (key_from_file[0] == '\0') {
            FILE *kf = fopen("config/groq_key", "r");
            if (kf) {
                if (fgets(key_from_file, sizeof(key_from_file), kf))
                    key_from_file[strcspn(key_from_file, "\n\r")] = '\0';
                fclose(kf);
            }
        }
        if (key_from_file[0]) api_key = key_from_file;
    }
    if (!api_key || api_key[0] == '\0') return -1;

    /* Build market summary */
    int up_count = 0, down_count = 0;
    double total_change = 0;
    char top_gainer[16] = "", top_loser[16] = "";
    double max_gain = -999, max_loss = 999;
    
    for (int i = 0; i < count; i++) {
        if (!stocks[i].valid) continue;
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
    
    double avg_change = count > 0 ? total_change / count : 0;
    const char *sentiment = avg_change > 0.5 ? "Bullish" : 
                           avg_change < -0.5 ? "Bearish" : "Mixed";

    /* Build the prompt */
    char prompt[1024];
    snprintf(prompt, sizeof(prompt),
             "Provide a brief 2-sentence market overview for a trader. "
             "Market data: %d stocks watched, %d up, %d down. "
             "Average change: %+.2f%%. Sentiment: %s. "
             "Top gainer: %s (%+.2f%%), Top loser: %s (%.2f%%). "
             "Be concise and actionable.",
             count, up_count, down_count, avg_change, sentiment,
             top_gainer, max_gain, top_loser, max_loss);

    /* Build JSON request body */
    char json_body[1536];
    char escaped_prompt[1024];
    size_t ep = 0;
    for (const char *p = prompt; *p && ep + 2 < sizeof(escaped_prompt); p++) {
        if (*p == '"')      { escaped_prompt[ep++] = '\\'; escaped_prompt[ep++] = '"'; }
        else if (*p == '\n'){ escaped_prompt[ep++] = '\\'; escaped_prompt[ep++] = 'n'; }
        else                  escaped_prompt[ep++] = *p;
    }
    escaped_prompt[ep] = '\0';

    snprintf(json_body, sizeof(json_body),
             "{\"model\":\"%s\","
             "\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}],"
             "\"max_tokens\":150,"
             "\"temperature\":0.4}",
             GROQ_MODEL, escaped_prompt);

    const char *path = "/openai/v1/chat/completions";

    char request[2560];
    snprintf(request, sizeof(request),
             "POST %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Authorization: Bearer %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             path, GROQ_HOST, api_key, strlen(json_body), json_body);

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) return -1;
    
    SSL *ssl = NULL;
    int sockfd = -1;
    int result = -1;
    char raw_response[MAX_BUFFER_SIZE];
    
    struct hostent *server = gethostbyname(GROQ_HOST);
    if (!server) goto cleanup2;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) goto cleanup2;
    
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(GROQ_PORT);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        goto cleanup2;
    
    ssl = SSL_new(ctx);
    if (!ssl) goto cleanup2;
    
    SSL_set_fd(ssl, sockfd);
    SSL_set_tlsext_host_name(ssl, GROQ_HOST);
    
    if (SSL_connect(ssl) <= 0) goto cleanup2;
    if (SSL_write(ssl, request, strlen(request)) <= 0) goto cleanup2;
    
    int total = 0;
    int bytes;
    while ((bytes = SSL_read(ssl, raw_response + total, sizeof(raw_response) - total - 1)) > 0) {
        total += bytes;
        if (total >= (int)sizeof(raw_response) - 1) break;
    }
    raw_response[total] = '\0';
    
    if (strstr(raw_response, "\"error\"") != NULL) {
        snprintf(response_buf, size, "[API Error] Could not generate market overview");
        result = 0;
        goto cleanup2;
    }
    
    result = parse_groq_response(raw_response, response_buf, size);

cleanup2:
    if (ssl) { SSL_shutdown(ssl); SSL_free(ssl); }
    if (sockfd >= 0) close(sockfd);
    if (ctx) SSL_CTX_free(ctx);
    
    return result;
}

/* ─────────────────────────────────────────────────────────────────────────
 * fetch_and_display_insight — fetch stock data and display AI insight
 *
 * Used by the `insight SYMBOL` command.
 * ─────────────────────────────────────────────────────────────────────── */
/* Simulated Indian stock data for insight command */
typedef struct {
    const char *symbol;
    const char *name;
    double base_price;
} InsightIndianStock;

static const InsightIndianStock INSIGHT_INDIAN_STOCKS[] = {
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
#define INSIGHT_INDIAN_COUNT 10

static int get_insight_indian_stock(const char *symbol, StockData *stock) {
    for (int i = 0; i < INSIGHT_INDIAN_COUNT; i++) {
        if (strcasecmp(symbol, INSIGHT_INDIAN_STOCKS[i].symbol) == 0) {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            srand((unsigned int)(tv.tv_sec * 1000 + tv.tv_usec / 1000) + (unsigned int)i * 31337);
            double variation = ((rand() % 400) - 200) / 10000.0;

            strncpy(stock->symbol, symbol, MAX_SYMBOL_LENGTH - 1);
            strncpy(stock->name, INSIGHT_INDIAN_STOCKS[i].name, MAX_COMPANY_NAME - 1);
            stock->current_price = INSIGHT_INDIAN_STOCKS[i].base_price * (1.0 + variation);
            stock->previous_close = INSIGHT_INDIAN_STOCKS[i].base_price;
            stock->change = stock->current_price - stock->previous_close;
            stock->change_percent = (stock->change / stock->previous_close) * 100.0;
            stock->open  = INSIGHT_INDIAN_STOCKS[i].base_price * (1.0 + variation * 0.5);
            stock->high  = stock->current_price * 1.01;
            stock->low   = stock->current_price * 0.99;
            stock->timestamp = time(NULL);
            stock->valid = 1;
            return 0;
        }
    }
    return -1;
}

/* Simulated US stock data for insight command */
typedef struct {
    const char *symbol;
    const char *name;
    double base_price;
} InsightUSStock;

static const InsightUSStock INSIGHT_US_STOCKS[] = {
    {"AAPL",  "Apple Inc.",           189.42},
    {"MSFT",  "Microsoft Corp",       421.10},
    {"GOOGL", "Alphabet Inc",         165.38},
    {"AMZN",  "Amazon.com Inc",       185.74},
    {"NVDA",  "NVIDIA Corp",          875.20},
    {"META",  "Meta Platforms",       570.45},
    {"TSLA",  "Tesla Inc",            178.90},
};
#define INSIGHT_US_COUNT 7

static int get_insight_us_stock(const char *symbol, StockData *stock) {
    for (int i = 0; i < INSIGHT_US_COUNT; i++) {
        if (strcasecmp(symbol, INSIGHT_US_STOCKS[i].symbol) == 0) {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            srand((unsigned int)(tv.tv_sec * 1000 + tv.tv_usec / 1000) + (unsigned int)i * 31337);
            double variation = ((rand() % 100) - 50) / 10000.0;

            strncpy(stock->symbol, symbol, MAX_SYMBOL_LENGTH - 1);
            strncpy(stock->name, INSIGHT_US_STOCKS[i].name, MAX_COMPANY_NAME - 1);
            stock->current_price = INSIGHT_US_STOCKS[i].base_price * (1.0 + variation);
            stock->previous_close = INSIGHT_US_STOCKS[i].base_price;
            stock->change = stock->current_price - stock->previous_close;
            stock->change_percent = variation * 100.0;
            stock->open  = INSIGHT_US_STOCKS[i].base_price * (1.0 + variation * 0.4);
            stock->high  = stock->current_price * 1.003;
            stock->low   = stock->current_price * 0.997;
            stock->timestamp = time(NULL);
            stock->valid = 1;
            return 0;
        }
    }
    return -1;
}

int fetch_and_display_insight(const char *symbol) {
    if (!symbol) return -1;
    
    int is_indian = is_indian_stock(symbol);
    const char *currency = is_indian ? "₹" : "$";
    
    printf("\n%s═══ AI Insight for %s ═══%s\n\n", COLOR_CYAN, symbol, COLOR_RESET);
    printf("  Fetching stock data...\n");
    
    StockData stock;
    memset(&stock, 0, sizeof(stock));
    strncpy(stock.symbol, symbol, MAX_SYMBOL_LENGTH - 1);
    
    int got_data = 0;
    
    /* Try simulated data first */
    if (is_indian) {
        if (get_insight_indian_stock(symbol, &stock) == 0) {
            got_data = 1;
        }
    } else {
        if (get_insight_us_stock(symbol, &stock) == 0) {
            got_data = 1;
        }
    }
    
    /* Fallback to API for US stocks */
    if (!got_data && !is_indian) {
        char response[MAX_BUFFER_SIZE];
        if (fetch_stock_quote(symbol, response, sizeof(response)) == 0) {
            if (parse_stock_quote(response, &stock) == 0 && stock.current_price > 0) {
                got_data = 1;
            }
        }
    }
    
    if (!got_data) {
        printf("  %sError: Could not fetch stock data for %s%s\n\n", COLOR_RED, symbol, COLOR_RESET);
        return -1;
    }
    
    /* Display stock info */
    printf("\n  %sStock:%s %s (%s)\n", COLOR_BOLD, COLOR_RESET, symbol, stock.name);
    printf("  %sPrice:%s %s%.2f\n", COLOR_BOLD, COLOR_RESET, currency, stock.current_price);
    printf("  %sChange:%s %s%+.2f (%+.2f%%)%s\n", 
           COLOR_BOLD, COLOR_RESET,
           stock.change_percent >= 0 ? COLOR_GREEN : COLOR_RED,
           stock.change, stock.change_percent, COLOR_RESET);
    
    /* Build price history (simulated for single fetch) */
    PriceHistory history;
    init_price_history(&history);
    add_price(&history, stock.current_price * 0.98);
    add_price(&history, stock.current_price * 0.99);
    add_price(&history, stock.current_price * 0.995);
    add_price(&history, stock.current_price * 1.005);
    add_price(&history, stock.current_price);
    
    /* Analyze stock */
    AIInsight insight;
    analyze_stock(&history, &insight);
    
    /* Display technical analysis */
    printf("\n  %s── Technical Analysis ──%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("  📈 Trend:      %s%s%s\n", 
           strcmp(insight.trend, "Bullish") == 0 ? COLOR_GREEN :
           strcmp(insight.trend, "Bearish") == 0 ? COLOR_RED : COLOR_YELLOW,
           insight.trend, COLOR_RESET);
    printf("  ⚡ Momentum:   %s\n", insight.momentum);
    printf("  📊 MA(5):      %s%.2f\n", currency, insight.moving_avg_5);
    printf("  📊 MA(10):     %s%.2f\n", currency, insight.moving_avg_10);
    printf("  📈 Volatility: %.2f%%\n", insight.volatility);
    printf("  📊 RSI(14):    %.1f\n", insight.rsi);
    
    if (insight.ma_crossover == 1) {
        printf("  %s✨ Signal:     Golden Cross (MA5 above MA10)%s\n", COLOR_GREEN, COLOR_RESET);
    } else if (insight.ma_crossover == -1) {
        printf("  %s⚠  Signal:     Death Cross (MA5 below MA10)%s\n", COLOR_RED, COLOR_RESET);
    }
    
    /* Get AI commentary */
    printf("\n  %s── AI Commentary ──%s\n", COLOR_MAGENTA, COLOR_RESET);
    printf("  Fetching AI analysis...\n");
    
    char ai_response[512];
    if (gemini_analyze_stock(symbol, &stock, &insight, ai_response, sizeof(ai_response)) == 0) {
        printf("\n  %s🤖 %s%s\n", COLOR_CYAN, ai_response, COLOR_RESET);
    } else {
        printf("  %s(AI commentary unavailable)%s\n", COLOR_YELLOW, COLOR_RESET);
    }
    
    printf("\n");
    return 0;
}

/* ─────────────────────────────────────────────────────────────────────────
 * groq_analyze_single_stock — AI analysis for single stock display
 *
 * Called from fetch_and_display_single() to provide AI insights
 * alongside the stock price data.
 * ─────────────────────────────────────────────────────────────────────── */
int groq_analyze_single_stock(const char *symbol, StockData *stock, 
                               char *response_buf, size_t size) {
    if (!symbol || !stock || !response_buf || size == 0)
        return -1;

    /* API key: env var first, then config/groq_key file */
    const char *api_key = getenv("GROQ_API_KEY");
    static char key_from_file[128] = "";
    if (!api_key || api_key[0] == '\0') {
        if (key_from_file[0] == '\0') {
            FILE *kf = fopen("config/groq_key", "r");
            if (kf) {
                if (fgets(key_from_file, sizeof(key_from_file), kf))
                    key_from_file[strcspn(key_from_file, "\n\r")] = '\0';
                fclose(kf);
            }
        }
        if (key_from_file[0]) api_key = key_from_file;
    }
    if (!api_key || api_key[0] == '\0') return -1;

    /* Determine currency symbol */
    int is_indian = (strstr(symbol, ".BSE") != NULL || strstr(symbol, ".NSE") != NULL);
    const char *currency = is_indian ? "₹" : "$";
    const char *market = is_indian ? "Indian (NSE/BSE)" : "US";

    /* Build the prompt */
    char prompt[768];
    snprintf(prompt, sizeof(prompt),
             "Provide a brief 2-3 sentence analysis for %s stock. "
             "Current price: %s%.2f, Change: %+.2f%%, "
             "Open: %s%.2f, High: %s%.2f, Low: %s%.2f. "
             "Market: %s. "
             "Include: 1) Current trend assessment, 2) Key support/resistance levels, "
             "3) Brief trading suggestion. Be concise and actionable.",
             symbol, currency, stock->current_price, stock->change_percent,
             currency, stock->open, currency, stock->high, currency, stock->low,
             market);

    /* Build JSON request body */
    char json_body[1536];
    char escaped_prompt[1024];
    size_t ep = 0;
    for (const char *p = prompt; *p && ep + 2 < sizeof(escaped_prompt); p++) {
        if (*p == '"')      { escaped_prompt[ep++] = '\\'; escaped_prompt[ep++] = '"'; }
        else if (*p == '\n'){ escaped_prompt[ep++] = '\\'; escaped_prompt[ep++] = 'n'; }
        else                  escaped_prompt[ep++] = *p;
    }
    escaped_prompt[ep] = '\0';

    snprintf(json_body, sizeof(json_body),
             "{\"model\":\"%s\","
             "\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}],"
             "\"max_tokens\":200,"
             "\"temperature\":0.4}",
             GROQ_MODEL, escaped_prompt);

    /* Build HTTP request */
    char http_request[2048];
    const char *path = "/openai/v1/chat/completions";
    snprintf(http_request, sizeof(http_request),
             "POST %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Authorization: Bearer %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             path, GROQ_HOST, api_key, strlen(json_body), json_body);

    /* Connect to Groq API */
    SSL_CTX *ctx = NULL;
    SSL *ssl = NULL;
    int sockfd = -1;
    int result = -1;

    ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) goto cleanup;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) goto cleanup;

    struct hostent *host = gethostbyname(GROQ_HOST);
    if (!host) goto cleanup;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(GROQ_PORT);
    memcpy(&addr.sin_addr, host->h_addr_list[0], (size_t)host->h_length);

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        goto cleanup;

    ssl = SSL_new(ctx);
    if (!ssl) goto cleanup;
    SSL_set_fd(ssl, sockfd);
    SSL_set_tlsext_host_name(ssl, GROQ_HOST);

    if (SSL_connect(ssl) <= 0) goto cleanup;

    /* Send request */
    if (SSL_write(ssl, http_request, (int)strlen(http_request)) <= 0)
        goto cleanup;

    /* Read response */
    char raw_response[4096];
    int total_read = 0;
    int bytes;
    while ((bytes = SSL_read(ssl, raw_response + total_read, 
                             (int)(sizeof(raw_response) - 1 - (size_t)total_read))) > 0) {
        total_read += bytes;
        if (total_read >= (int)sizeof(raw_response) - 1) break;
    }
    raw_response[total_read] = '\0';

    /* Check for errors */
    if (strstr(raw_response, "\"error\"")) {
        if (strstr(raw_response, "rate_limit")) {
            snprintf(response_buf, size, 
                     "[Rate limited] Groq free tier: wait a moment and try again");
        } else {
            snprintf(response_buf, size, 
                     "[API Error] Check your Groq API key at console.groq.com");
        }
        result = 0;
        goto cleanup;
    }
    
    /* Parse the response */
    result = parse_groq_response(raw_response, response_buf, size);

cleanup:
    if (ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    if (sockfd >= 0) close(sockfd);
    if (ctx) SSL_CTX_free(ctx);
    
    return result;
}
