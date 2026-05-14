/*
 * MarketPulse - Live Data Stream via Named Pipe (FIFO)
 *
 * System calls demonstrated:
 *   mkfifo()  - create a named pipe in the filesystem
 *   open()    - open the FIFO for writing (blocks until reader connects)
 *   write()   - stream JSON lines to the FIFO
 *   close()   - release the FIFO file descriptor
 *   unlink()  - remove the FIFO from the filesystem on exit
 *
 * Usage:
 *   Terminal 1:  ./marketpulse stream AAPL MSFT GOOGL
 *   Terminal 2:  cat /tmp/marketpulse_stream.fifo
 */

#include "marketpulse.h"
#include <sys/stat.h>
#include <errno.h>

#define STREAM_FIFO_PATH "/tmp/marketpulse_stream.fifo"

/* External symbols used for fetching / parsing */
extern int fetch_stock_quote(const char *symbol, char *response, size_t size);
extern int fetch_indian_stock_quote(const char *symbol, char *response, size_t size);
extern int is_indian_stock(const char *symbol);
extern int parse_stock_quote(const char *json, StockData *stock);
extern void get_current_time_string(char *buf, size_t size);
extern const char *get_trend_color(double change);
extern const char *get_trend_arrow(double change);

static volatile sig_atomic_t stream_running = 1;

static void stream_sig_handler(int signum) {
    (void)signum;
    stream_running = 0;
}

int run_stream_mode(char symbols[][MAX_SYMBOL_LENGTH], int count) {
    int fifo_fd = -1;
    char response[MAX_BUFFER_SIZE];
    StockData stock;
    char json_line[512];
    char time_str[16];
    int i;

    signal(SIGINT,  stream_sig_handler);
    signal(SIGTERM, stream_sig_handler);
    signal(SIGPIPE, SIG_IGN);   /* don't crash if reader closes the pipe */

    /* ── Create the named pipe ─────────────────────────────────────── */
    if (mkfifo(STREAM_FIFO_PATH, 0644) == -1 && errno != EEXIST) {
        perror("mkfifo");
        return -1;
    }

    /* ── Print header ────────────────────────────────────────────────── */
    printf("\n");
    printf("%s%s📡  MarketPulse Live Stream%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s══════════════════════════════════════════%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  %sFIFO path:%s  %s\n",    COLOR_BOLD, COLOR_RESET, STREAM_FIFO_PATH);
    printf("  %sSymbols:%s   ", COLOR_BOLD, COLOR_RESET);
    for (i = 0; i < count; i++)
        printf("%s%s%s ", COLOR_CYAN, symbols[i], COLOR_RESET);
    printf("\n");
    printf("  %sFormat:%s    JSON — one line per price update\n\n",
           COLOR_BOLD, COLOR_RESET);

    printf("  %s⏳  Waiting for reader...%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("  Open a second terminal and run:\n");
    printf("  %s  cat %s%s\n\n", COLOR_GREEN, STREAM_FIFO_PATH, COLOR_RESET);
    fflush(stdout);

    /* ── open() BLOCKS here until a reader opens the other end ───────── */
    fifo_fd = open(STREAM_FIFO_PATH, O_WRONLY);
    if (fifo_fd == -1) {
        if (errno != EINTR)
            perror("open fifo");
        unlink(STREAM_FIFO_PATH);
        return -1;
    }

    printf("  %s●  Reader connected — streaming%s  (Ctrl+C to stop)\n\n",
           COLOR_GREEN, COLOR_RESET);
    fflush(stdout);

    /* ── Main streaming loop ─────────────────────────────────────────── */
    while (stream_running) {
        for (i = 0; i < count && stream_running; i++) {
            memset(&stock, 0, sizeof(stock));
            strncpy(stock.symbol, symbols[i], MAX_SYMBOL_LENGTH - 1);

            int ok = 0;
            if (is_indian_stock(symbols[i])) {
                if (fetch_indian_stock_quote(symbols[i], response, sizeof(response)) == 0)
                    ok = (parse_stock_quote(response, &stock) == 0);
            } else {
                if (fetch_stock_quote(symbols[i], response, sizeof(response)) == 0)
                    ok = (parse_stock_quote(response, &stock) == 0);
            }

            if (!ok) {
                /* Restore symbol if parse cleared it */
                strncpy(stock.symbol, symbols[i], MAX_SYMBOL_LENGTH - 1);
                continue;
            }

            /* ── Write JSON line to the FIFO ──────────────────────── */
            int len = snprintf(json_line, sizeof(json_line),
                     "{\"symbol\":\"%s\",\"price\":%.2f,"
                     "\"change\":%.2f,\"pct\":%.2f,\"ts\":%ld}\n",
                     stock.symbol, stock.current_price,
                     stock.change, stock.change_percent,
                     (long)time(NULL));

            if (write(fifo_fd, json_line, (size_t)len) == -1) {
                if (errno == EPIPE) {
                    printf("\n%s  Reader disconnected.%s\n", COLOR_YELLOW, COLOR_RESET);
                    stream_running = 0;
                    break;
                }
            }

            /* ── Echo summary line to terminal ────────────────────── */
            get_current_time_string(time_str, sizeof(time_str));
            printf("  [%s]  %s%-14s%s  %s%9.2f%s  %s%+.2f%%%s  %s\n",
                   time_str,
                   COLOR_CYAN, stock.symbol, COLOR_RESET,
                   get_trend_color(stock.change), stock.current_price, COLOR_RESET,
                   get_trend_color(stock.change), stock.change_percent, COLOR_RESET,
                   get_trend_arrow(stock.change));
            fflush(stdout);

            if (i < count - 1) sleep(1);
        }

        /* Wait before next round */
        for (i = 0; i < DEFAULT_REFRESH_INTERVAL && stream_running; i++)
            sleep(1);
    }

    /* ── Cleanup ─────────────────────────────────────────────────────── */
    close(fifo_fd);
    unlink(STREAM_FIFO_PATH);
    printf("\n%s  Stream stopped. FIFO removed.%s\n\n", COLOR_YELLOW, COLOR_RESET);
    return 0;
}
