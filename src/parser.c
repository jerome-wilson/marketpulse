/*
 * MarketPulse - Real-Time Stock Monitoring and Insight Engine
 * Custom JSON Parser - No external libraries
 * 
 * This module demonstrates string parsing and memory management
 * without relying on external JSON libraries like cJSON.
 * 
 * Supports parsing:
 * - Numbers (integers and floats)
 * - Strings
 * - Simple objects (single level)
 */

#include "marketpulse.h"
#include <ctype.h>
#include <math.h>

/*
 * Skip whitespace characters in JSON string
 */
static const char *skip_whitespace(const char *json) {
    while (*json && isspace((unsigned char)*json)) {
        json++;
    }
    return json;
}

/*
 * Find a key in JSON object and return pointer to its value
 * 
 * Example: For {"name": "Apple", "price": 178.50}
 * json_find_key(json, "price") returns pointer to "178.50}"
 */
static const char *json_find_key(const char *json, const char *key) {
    char search_key[128];
    const char *pos;
    
    /* Build search pattern: "key": */
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);
    
    pos = strstr(json, search_key);
    if (pos == NULL) {
        return NULL;
    }
    
    /* Move past the key and colon */
    pos += strlen(search_key);
    pos = skip_whitespace(pos);
    
    if (*pos != ':') {
        return NULL;
    }
    pos++;  /* Skip colon */
    
    return skip_whitespace(pos);
}

/*
 * Extract a string value from JSON
 * Returns dynamically allocated string (caller must free)
 * Returns NULL if key not found or not a string
 * 
 * Example: json_get_string({"name": "Apple"}, "name") returns "Apple"
 */
char *json_get_string(const char *json, const char *key) {
    const char *value_start;
    const char *value_end;
    char *result;
    size_t len;
    
    value_start = json_find_key(json, key);
    if (value_start == NULL) {
        return NULL;
    }
    
    /* Check if it's a string (starts with quote) */
    if (*value_start != '"') {
        return NULL;
    }
    
    value_start++;  /* Skip opening quote */
    
    /* Find closing quote (handle escaped quotes) */
    value_end = value_start;
    while (*value_end && *value_end != '"') {
        if (*value_end == '\\' && *(value_end + 1)) {
            value_end += 2;  /* Skip escaped character */
        } else {
            value_end++;
        }
    }
    
    if (*value_end != '"') {
        return NULL;  /* No closing quote found */
    }
    
    /* Allocate and copy string */
    len = value_end - value_start;
    result = malloc(len + 1);
    if (result == NULL) {
        return NULL;
    }
    
    strncpy(result, value_start, len);
    result[len] = '\0';
    
    return result;
}

/*
 * Extract a number value from JSON
 * Returns the number, or NAN if not found
 * 
 * Example: json_get_number({"price": 178.50}, "price") returns 178.50
 */
double json_get_number(const char *json, const char *key) {
    const char *value_start;
    char *end_ptr;
    double result;
    
    value_start = json_find_key(json, key);
    if (value_start == NULL) {
        return NAN;
    }
    
    /* Check for null value */
    if (strncmp(value_start, "null", 4) == 0) {
        return NAN;
    }
    
    /* Parse the number */
    result = strtod(value_start, &end_ptr);
    
    /* Check if parsing was successful */
    if (end_ptr == value_start) {
        return NAN;  /* No number found */
    }
    
    return result;
}

/*
 * Extract an integer value from JSON
 * Returns the integer, or -1 if not found
 */
long json_get_integer(const char *json, const char *key) {
    const char *value_start;
    char *end_ptr;
    long result;
    
    value_start = json_find_key(json, key);
    if (value_start == NULL) {
        return -1;
    }
    
    /* Check for null value */
    if (strncmp(value_start, "null", 4) == 0) {
        return -1;
    }
    
    /* Parse the integer */
    result = strtol(value_start, &end_ptr, 10);
    
    /* Check if parsing was successful */
    if (end_ptr == value_start) {
        return -1;
    }
    
    return result;
}

/*
 * Check if a key exists in JSON
 */
int json_has_key(const char *json, const char *key) {
    return json_find_key(json, key) != NULL;
}

/*
 * Parse Finnhub quote response into StockData structure
 * 
 * Finnhub quote format:
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
int parse_stock_quote(const char *json, StockData *stock) {
    double current_price;
    double change;
    double change_percent;
    double high;
    double low;
    double open_price;
    double prev_close;
    long timestamp;
    
    if (json == NULL || stock == NULL) {
        return -1;
    }
    
    /* Initialize stock data */
    memset(stock, 0, sizeof(StockData));
    stock->valid = 0;
    
    /* Parse current price (required) */
    current_price = json_get_number(json, "c");
    if (isnan(current_price) || current_price == 0) {
        /* Price of 0 usually means invalid symbol */
        return -1;
    }
    
    /* Parse other fields */
    change = json_get_number(json, "d");
    change_percent = json_get_number(json, "dp");
    high = json_get_number(json, "h");
    low = json_get_number(json, "l");
    open_price = json_get_number(json, "o");
    prev_close = json_get_number(json, "pc");
    timestamp = json_get_integer(json, "t");
    
    /* Populate stock structure */
    stock->current_price = current_price;
    stock->change = isnan(change) ? 0.0 : change;
    stock->change_percent = isnan(change_percent) ? 0.0 : change_percent;
    stock->high = isnan(high) ? current_price : high;
    stock->low = isnan(low) ? current_price : low;
    stock->open = isnan(open_price) ? current_price : open_price;
    stock->previous_close = isnan(prev_close) ? current_price : prev_close;
    stock->timestamp = (timestamp > 0) ? (time_t)timestamp : time(NULL);
    stock->valid = 1;
    
    return 0;
}

/*
 * Parse Finnhub company profile response
 * 
 * Finnhub profile format:
 * {
 *   "name": "Apple Inc",
 *   "ticker": "AAPL",
 *   "exchange": "NASDAQ NMS - GLOBAL MARKET",
 *   "finnhubIndustry": "Technology",
 *   "logo": "https://...",
 *   "weburl": "https://www.apple.com"
 * }
 */
int parse_company_profile(const char *json, StockData *stock) {
    char *name;
    char *ticker;
    
    if (json == NULL || stock == NULL) {
        return -1;
    }
    
    /* Get company name */
    name = json_get_string(json, "name");
    if (name != NULL) {
        strncpy(stock->name, name, MAX_COMPANY_NAME - 1);
        stock->name[MAX_COMPANY_NAME - 1] = '\0';
        free(name);
    }
    
    /* Get ticker symbol */
    ticker = json_get_string(json, "ticker");
    if (ticker != NULL) {
        strncpy(stock->symbol, ticker, MAX_SYMBOL_LENGTH - 1);
        stock->symbol[MAX_SYMBOL_LENGTH - 1] = '\0';
        free(ticker);
    }
    
    return 0;
}

/*
 * Parse a simple JSON array of strings
 * Returns count of items parsed
 * 
 * Example: ["AAPL", "GOOGL", "MSFT"]
 */
int parse_string_array(const char *json, char items[][MAX_SYMBOL_LENGTH], int max_items) {
    const char *pos;
    const char *end;
    int count = 0;
    
    if (json == NULL || items == NULL) {
        return 0;
    }
    
    /* Find array start */
    pos = strchr(json, '[');
    if (pos == NULL) {
        return 0;
    }
    pos++;
    
    while (*pos && *pos != ']' && count < max_items) {
        pos = skip_whitespace(pos);
        
        if (*pos == '"') {
            pos++;  /* Skip opening quote */
            end = strchr(pos, '"');
            if (end == NULL) {
                break;
            }
            
            /* Copy string */
            size_t len = end - pos;
            if (len >= MAX_SYMBOL_LENGTH) {
                len = MAX_SYMBOL_LENGTH - 1;
            }
            strncpy(items[count], pos, len);
            items[count][len] = '\0';
            count++;
            
            pos = end + 1;  /* Move past closing quote */
        }
        
        /* Skip comma */
        pos = skip_whitespace(pos);
        if (*pos == ',') {
            pos++;
        }
    }
    
    return count;
}

/*
 * Validate JSON structure (basic check)
 * Returns 1 if valid, 0 if invalid
 */
int json_is_valid(const char *json) {
    int brace_count = 0;
    int bracket_count = 0;
    int in_string = 0;
    
    if (json == NULL || *json == '\0') {
        return 0;
    }
    
    while (*json) {
        if (*json == '"' && (json == json || *(json - 1) != '\\')) {
            in_string = !in_string;
        } else if (!in_string) {
            if (*json == '{') brace_count++;
            else if (*json == '}') brace_count--;
            else if (*json == '[') bracket_count++;
            else if (*json == ']') bracket_count--;
        }
        json++;
    }
    
    return (brace_count == 0 && bracket_count == 0 && !in_string);
}

/*
 * Extract error message from API response
 * Returns dynamically allocated string (caller must free)
 */
char *json_get_error(const char *json) {
    char *error;
    
    /* Try common error field names */
    error = json_get_string(json, "error");
    if (error != NULL) {
        return error;
    }
    
    error = json_get_string(json, "message");
    if (error != NULL) {
        return error;
    }
    
    error = json_get_string(json, "msg");
    if (error != NULL) {
        return error;
    }
    
    return NULL;
}

/*
 * Debug function: Print parsed stock data
 */
void debug_print_stock(const StockData *stock) {
#ifdef DEBUG
    printf("=== Stock Data ===\n");
    printf("Symbol: %s\n", stock->symbol);
    printf("Name: %s\n", stock->name);
    printf("Price: $%.2f\n", stock->current_price);
    printf("Change: %.2f (%.2f%%)\n", stock->change, stock->change_percent);
    printf("High: $%.2f\n", stock->high);
    printf("Low: $%.2f\n", stock->low);
    printf("Open: $%.2f\n", stock->open);
    printf("Prev Close: $%.2f\n", stock->previous_close);
    printf("Valid: %s\n", stock->valid ? "Yes" : "No");
    printf("==================\n");
#else
    (void)stock;  /* Suppress unused warning */
#endif
}