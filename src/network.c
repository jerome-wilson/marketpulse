/*
 * MarketPulse - Real-Time Stock Monitoring and Insight Engine
 * Network module - HTTPS client using OpenSSL
 * 
 * System Programming Concepts:
 * - socket() - Create network socket
 * - connect() - Establish TCP connection
 * - read()/write() - Low-level I/O (via SSL_read/SSL_write)
 * - DNS resolution with getaddrinfo()
 */

#include "marketpulse.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

/* SSL context - initialized once */
static SSL_CTX *ssl_ctx = NULL;
static int ssl_initialized = 0;

/*
 * Initialize OpenSSL library
 * Must be called before any SSL operations
 */
static int init_ssl(void) {
    if (ssl_initialized) {
        return 0;
    }
    
    /* Initialize OpenSSL */
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    
    /* Create SSL context */
    ssl_ctx = SSL_CTX_new(TLS_client_method());
    if (ssl_ctx == NULL) {
        ERR_print_errors_fp(stderr);
        return -1;
    }
    
    /* Set verification mode */
    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, NULL);
    
    ssl_initialized = 1;
    return 0;
}

/*
 * Cleanup SSL resources
 */
void cleanup_ssl(void) {
    if (ssl_ctx) {
        SSL_CTX_free(ssl_ctx);
        ssl_ctx = NULL;
    }
    ssl_initialized = 0;
}

/*
 * Create a TCP socket and connect to host:port
 * Uses getaddrinfo() for DNS resolution
 * 
 * Returns: socket file descriptor on success, -1 on failure
 */
int create_connection(const char *host, int port) {
    struct addrinfo hints, *result, *rp;
    int sockfd = -1;
    char port_str[16];
    int ret;
    
    /* Convert port to string */
    snprintf(port_str, sizeof(port_str), "%d", port);
    
    /* Setup hints for getaddrinfo */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;  /* TCP socket */
    hints.ai_protocol = IPPROTO_TCP;
    
    /* Resolve hostname */
    ret = getaddrinfo(host, port_str, &hints, &result);
    if (ret != 0) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(ret));
        return -1;
    }
    
    /* Try each address until we successfully connect */
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        /* Create socket */
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) {
            continue;  /* Try next address */
        }
        
        /* Attempt to connect */
        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;  /* Success */
        }
        
        /* Connection failed, close socket and try next */
        close(sockfd);
        sockfd = -1;
    }
    
    freeaddrinfo(result);
    
    if (sockfd == -1) {
        fprintf(stderr, "Could not connect to %s:%d\n", host, port);
    }
    
    return sockfd;
}

/*
 * Close socket connection
 */
void close_connection(int sockfd) {
    if (sockfd >= 0) {
        close(sockfd);
    }
}

/*
 * Structure to hold SSL connection info
 */
typedef struct {
    int sockfd;
    SSL *ssl;
} SSLConnection;

/*
 * Create SSL connection over existing socket
 */
static SSLConnection *create_ssl_connection(int sockfd, const char *host) {
    SSLConnection *conn;
    
    /* Initialize SSL if needed */
    if (init_ssl() != 0) {
        return NULL;
    }
    
    conn = malloc(sizeof(SSLConnection));
    if (conn == NULL) {
        return NULL;
    }
    
    conn->sockfd = sockfd;
    conn->ssl = SSL_new(ssl_ctx);
    
    if (conn->ssl == NULL) {
        free(conn);
        return NULL;
    }
    
    /* Set hostname for SNI */
    SSL_set_tlsext_host_name(conn->ssl, host);
    
    /* Attach SSL to socket */
    SSL_set_fd(conn->ssl, sockfd);
    
    /* Perform SSL handshake */
    if (SSL_connect(conn->ssl) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_free(conn->ssl);
        free(conn);
        return NULL;
    }
    
    return conn;
}

/*
 * Close SSL connection
 */
static void close_ssl_connection(SSLConnection *conn) {
    if (conn) {
        if (conn->ssl) {
            SSL_shutdown(conn->ssl);
            SSL_free(conn->ssl);
        }
        if (conn->sockfd >= 0) {
            close(conn->sockfd);
        }
        free(conn);
    }
}

/*
 * Send data over SSL connection
 */
static int ssl_send(SSLConnection *conn, const char *data, size_t len) {
    return SSL_write(conn->ssl, data, (int)len);
}

/*
 * Receive data over SSL connection
 */
static int ssl_recv(SSLConnection *conn, char *buffer, size_t len) {
    return SSL_read(conn->ssl, buffer, (int)len);
}

/*
 * Build and send HTTP GET request
 * Returns: number of bytes sent, -1 on error
 */
int send_http_request(int sockfd, const char *host, const char *path) {
    /* This function is kept for compatibility but we use SSL version below */
    (void)sockfd;
    (void)host;
    (void)path;
    return -1;  /* Use send_https_request instead */
}

/*
 * Receive HTTP response
 * Returns: number of bytes received, -1 on error
 */
int receive_response(int sockfd, char *buffer, size_t buffer_size) {
    /* This function is kept for compatibility but we use SSL version below */
    (void)sockfd;
    (void)buffer;
    (void)buffer_size;
    return -1;  /* Use SSL version instead */
}

/*
 * Make HTTPS request to Finnhub API
 * 
 * Parameters:
 *   endpoint - API endpoint path (e.g., "/api/v1/quote")
 *   params - Query parameters (e.g., "symbol=AAPL")
 *   response - Buffer to store response
 *   response_size - Size of response buffer
 * 
 * Returns: 0 on success, -1 on error
 */
static int make_api_request(const char *endpoint, const char *params, 
                            char *response, size_t response_size) {
    SSLConnection *conn;
    char request[1024];
    char buffer[MAX_BUFFER_SIZE];
    int sockfd;
    int bytes_received;
    int total_received = 0;
    char *body_start;
    
    /* Create TCP connection */
    sockfd = create_connection(FINNHUB_HOST, FINNHUB_PORT);
    if (sockfd < 0) {
        return -1;
    }
    
    /* Establish SSL connection */
    conn = create_ssl_connection(sockfd, FINNHUB_HOST);
    if (conn == NULL) {
        close(sockfd);
        return -1;
    }
    
    /* Build HTTP request */
    snprintf(request, sizeof(request),
             "GET %s?%s&token=%s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: MarketPulse/1.0\r\n"
             "Accept: application/json\r\n"
             "Connection: close\r\n"
             "\r\n",
             endpoint, params, FINNHUB_API_KEY, FINNHUB_HOST);
    
#ifdef DEBUG
    printf("DEBUG: Sending request:\n%s\n", request);
#endif
    
    /* Send request */
    if (ssl_send(conn, request, strlen(request)) < 0) {
        close_ssl_connection(conn);
        return -1;
    }
    
    /* Receive response */
    memset(buffer, 0, sizeof(buffer));
    while ((bytes_received = ssl_recv(conn, buffer + total_received, 
                                       sizeof(buffer) - total_received - 1)) > 0) {
        total_received += bytes_received;
        if (total_received >= (int)sizeof(buffer) - 1) {
            break;
        }
    }
    
    buffer[total_received] = '\0';
    
#ifdef DEBUG
    printf("DEBUG: Received %d bytes:\n%s\n", total_received, buffer);
#endif
    
    /* Close connection */
    close_ssl_connection(conn);
    
    if (total_received <= 0) {
        return -1;
    }
    
    /* Find the body (after \r\n\r\n) */
    body_start = strstr(buffer, "\r\n\r\n");
    if (body_start == NULL) {
        return -1;
    }
    body_start += 4;  /* Skip \r\n\r\n */
    
    /* Handle chunked transfer encoding */
    /* Finnhub typically uses chunked encoding, so we need to parse it */
    if (strstr(buffer, "Transfer-Encoding: chunked") != NULL) {
        /* Skip the chunk size line */
        char *chunk_data = strchr(body_start, '\n');
        if (chunk_data) {
            chunk_data++;  /* Skip newline */
            /* Find end of chunk (next \r\n with 0) */
            char *chunk_end = strstr(chunk_data, "\r\n0\r\n");
            if (chunk_end) {
                *chunk_end = '\0';
            }
            strncpy(response, chunk_data, response_size - 1);
        } else {
            strncpy(response, body_start, response_size - 1);
        }
    } else {
        strncpy(response, body_start, response_size - 1);
    }
    
    response[response_size - 1] = '\0';
    
    /* Clean up any trailing chunk markers */
    char *end = strstr(response, "\r\n");
    if (end && strlen(end) < 10) {
        *end = '\0';
    }
    
    return 0;
}

/*
 * Fetch stock quote from Finnhub API
 * 
 * API Endpoint: /api/v1/quote
 * Response format:
 * {
 *   "c": 178.50,    // Current price
 *   "d": 1.25,      // Change
 *   "dp": 0.70,     // Percent change
 *   "h": 179.00,    // High price of the day
 *   "l": 177.00,    // Low price of the day
 *   "o": 177.50,    // Open price of the day
 *   "pc": 177.25,   // Previous close price
 *   "t": 1234567890 // Timestamp
 * }
 */
int fetch_stock_quote(const char *symbol, char *response, size_t response_size) {
    char params[128];
    
    snprintf(params, sizeof(params), "symbol=%s", symbol);
    
    return make_api_request("/api/v1/quote", params, response, response_size);
}

/*
 * Fetch company profile from Finnhub API
 * 
 * API Endpoint: /api/v1/stock/profile2
 * Response format:
 * {
 *   "name": "Apple Inc",
 *   "ticker": "AAPL",
 *   "exchange": "NASDAQ",
 *   "industry": "Technology",
 *   ...
 * }
 */
int fetch_company_profile(const char *symbol, char *response, size_t response_size) {
    char params[128];
    
    snprintf(params, sizeof(params), "symbol=%s", symbol);
    
    return make_api_request("/api/v1/stock/profile2", params, response, response_size);
}

/*
 * Fetch multiple stock quotes efficiently
 * Uses a single connection for multiple requests when possible
 */
int fetch_multiple_quotes(const char *symbols[], int count, 
                          char responses[][MAX_BUFFER_SIZE]) {
    int i;
    int success_count = 0;
    
    for (i = 0; i < count; i++) {
        if (fetch_stock_quote(symbols[i], responses[i], MAX_BUFFER_SIZE) == 0) {
            success_count++;
        } else {
            responses[i][0] = '\0';
        }
        
        /* Small delay to avoid rate limiting */
        if (i < count - 1) {
            usleep(100000);  /* 100ms delay between requests */
        }
    }
    
    return success_count;
}

/*
 * Check if symbol is an Indian stock (BSE/NSE)
 * Returns: 1 if Indian stock, 0 otherwise
 */
int is_indian_stock(const char *symbol) {
    if (strstr(symbol, ".BSE") != NULL || 
        strstr(symbol, ".bse") != NULL ||
        strstr(symbol, ".NS") != NULL ||
        strstr(symbol, ".ns") != NULL ||
        strstr(symbol, ".NSE") != NULL ||
        strstr(symbol, ".nse") != NULL ||
        strstr(symbol, ".BO") != NULL ||
        strstr(symbol, ".bo") != NULL) {
        return 1;
    }
    return 0;
}

/*
 * Make HTTPS request to Alpha Vantage API (for Indian stocks)
 * 
 * Parameters:
 *   symbol - Stock symbol (e.g., "RELIANCE.BSE")
 *   response - Buffer to store response
 *   response_size - Size of response buffer
 * 
 * Returns: 0 on success, -1 on error
 */
static int make_alphavantage_request(const char *symbol, char *response, size_t response_size) {
    SSLConnection *conn;
    char request[1024];
    char buffer[MAX_BUFFER_SIZE];
    int sockfd;
    int bytes_received;
    int total_received = 0;
    char *body_start;
    
    /* Create TCP connection to Alpha Vantage */
    sockfd = create_connection(ALPHAVANTAGE_HOST, ALPHAVANTAGE_PORT);
    if (sockfd < 0) {
        return -1;
    }
    
    /* Establish SSL connection */
    conn = create_ssl_connection(sockfd, ALPHAVANTAGE_HOST);
    if (conn == NULL) {
        close(sockfd);
        return -1;
    }
    
    /* Build HTTP request for Alpha Vantage GLOBAL_QUOTE */
    snprintf(request, sizeof(request),
             "GET /query?function=GLOBAL_QUOTE&symbol=%s&apikey=%s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: MarketPulse/1.0\r\n"
             "Accept: application/json\r\n"
             "Connection: close\r\n"
             "\r\n",
             symbol, ALPHAVANTAGE_API_KEY, ALPHAVANTAGE_HOST);
    
#ifdef DEBUG
    printf("DEBUG: Alpha Vantage request:\n%s\n", request);
#endif
    
    /* Send request */
    if (ssl_send(conn, request, strlen(request)) < 0) {
        close_ssl_connection(conn);
        return -1;
    }
    
    /* Receive response */
    memset(buffer, 0, sizeof(buffer));
    while ((bytes_received = ssl_recv(conn, buffer + total_received, 
                                       sizeof(buffer) - total_received - 1)) > 0) {
        total_received += bytes_received;
        if (total_received >= (int)sizeof(buffer) - 1) {
            break;
        }
    }
    
    buffer[total_received] = '\0';
    
#ifdef DEBUG
    printf("DEBUG: Alpha Vantage response (%d bytes):\n%s\n", total_received, buffer);
#endif
    
    /* Close connection */
    close_ssl_connection(conn);
    
    if (total_received <= 0) {
        return -1;
    }
    
    /* Find the body (after \r\n\r\n) */
    body_start = strstr(buffer, "\r\n\r\n");
    if (body_start == NULL) {
        return -1;
    }
    body_start += 4;
    
    /* Copy response body */
    strncpy(response, body_start, response_size - 1);
    response[response_size - 1] = '\0';
    
    return 0;
}

/*
 * Fetch Indian stock quote from Alpha Vantage API
 * 
 * API Response format:
 * {
 *   "Global Quote": {
 *     "01. symbol": "RELIANCE.BSE",
 *     "02. open": "1234.50",
 *     "03. high": "1250.00",
 *     "04. low": "1230.00",
 *     "05. price": "1245.75",
 *     "06. volume": "1234567",
 *     "07. latest trading day": "2024-01-15",
 *     "08. previous close": "1240.00",
 *     "09. change": "5.75",
 *     "10. change percent": "0.46%"
 *   }
 * }
 * 
 * We convert this to Finnhub-compatible format for the parser
 */
int fetch_indian_stock_quote(const char *symbol, char *response, size_t response_size) {
    char av_response[MAX_BUFFER_SIZE];
    char *price_str, *change_str, *pct_str, *open_str, *high_str, *low_str, *pc_str;
    double price = 0, change = 0, pct = 0, open_val = 0, high = 0, low = 0, pc = 0;
    
    /* Fetch from Alpha Vantage */
    if (make_alphavantage_request(symbol, av_response, sizeof(av_response)) != 0) {
        return -1;
    }
    
    /* Parse Alpha Vantage response and convert to Finnhub format */
    /* Look for "05. price" */
    price_str = strstr(av_response, "\"05. price\"");
    if (price_str) {
        price_str = strchr(price_str, ':');
        if (price_str) {
            price_str = strchr(price_str, '"');
            if (price_str) {
                price = atof(price_str + 1);
            }
        }
    }
    
    /* Look for "09. change" */
    change_str = strstr(av_response, "\"09. change\"");
    if (change_str) {
        change_str = strchr(change_str, ':');
        if (change_str) {
            change_str = strchr(change_str, '"');
            if (change_str) {
                change = atof(change_str + 1);
            }
        }
    }
    
    /* Look for "10. change percent" */
    pct_str = strstr(av_response, "\"10. change percent\"");
    if (pct_str) {
        pct_str = strchr(pct_str, ':');
        if (pct_str) {
            pct_str = strchr(pct_str, '"');
            if (pct_str) {
                pct = atof(pct_str + 1);
            }
        }
    }
    
    /* Look for "02. open" */
    open_str = strstr(av_response, "\"02. open\"");
    if (open_str) {
        open_str = strchr(open_str, ':');
        if (open_str) {
            open_str = strchr(open_str, '"');
            if (open_str) {
                open_val = atof(open_str + 1);
            }
        }
    }
    
    /* Look for "03. high" */
    high_str = strstr(av_response, "\"03. high\"");
    if (high_str) {
        high_str = strchr(high_str, ':');
        if (high_str) {
            high_str = strchr(high_str, '"');
            if (high_str) {
                high = atof(high_str + 1);
            }
        }
    }
    
    /* Look for "04. low" */
    low_str = strstr(av_response, "\"04. low\"");
    if (low_str) {
        low_str = strchr(low_str, ':');
        if (low_str) {
            low_str = strchr(low_str, '"');
            if (low_str) {
                low = atof(low_str + 1);
            }
        }
    }
    
    /* Look for "08. previous close" */
    pc_str = strstr(av_response, "\"08. previous close\"");
    if (pc_str) {
        pc_str = strchr(pc_str, ':');
        if (pc_str) {
            pc_str = strchr(pc_str, '"');
            if (pc_str) {
                pc = atof(pc_str + 1);
            }
        }
    }
    
    /* Convert to Finnhub-compatible JSON format */
    snprintf(response, response_size,
             "{\"c\":%.2f,\"d\":%.2f,\"dp\":%.2f,\"h\":%.2f,\"l\":%.2f,\"o\":%.2f,\"pc\":%.2f,\"t\":%ld}",
             price, change, pct, high, low, open_val, pc, (long)time(NULL));
    
    return 0;
}

/*
 * Test API connection
 * Returns: 0 if API is reachable, -1 otherwise
 */
int test_api_connection(void) {
    char response[MAX_BUFFER_SIZE];
    
    /* Try to fetch a known stock */
    if (fetch_stock_quote("AAPL", response, sizeof(response)) == 0) {
        /* Check if response contains expected data */
        if (strstr(response, "\"c\":") != NULL) {
            return 0;  /* Success */
        }
    }
    
    return -1;  /* Failed */
}
