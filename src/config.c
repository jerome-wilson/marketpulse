/*
 * MarketPulse - Real-Time Stock Monitoring and Insight Engine
 * Configuration Implementation - JSON config file support
 */

#include "config.h"
#include "marketpulse.h"
#include "daemon.h"
#include <string.h>
#include <sys/stat.h>

/* Global configuration */
static AppConfig g_config;
static int g_config_loaded = 0;

/* External JSON parser functions */
extern double json_get_number(const char *json, const char *key);
extern char *json_get_string(const char *json, const char *key);

/*
 * Set default configuration values
 */
void config_set_defaults(AppConfig *config) {
    memset(config, 0, sizeof(AppConfig));
    
    /* API settings */
    strncpy(config->api_key, FINNHUB_API_KEY, sizeof(config->api_key) - 1);
    strncpy(config->api_host, FINNHUB_HOST, sizeof(config->api_host) - 1);
    config->api_port = FINNHUB_PORT;
    
    /* Timing */
    config->refresh_interval = DEFAULT_REFRESH_INTERVAL;
    config->alert_check_interval = ALERT_CHECK_INTERVAL;
    config->worker_timeout = 30;
    config->rate_limit_requests = 60;
    
    /* Logging */
    strncpy(config->log_file, "/tmp/marketpulse.log", sizeof(config->log_file) - 1);
    config->log_level = 1;  /* INFO */
    config->log_rotation_size = 10;  /* 10 MB */
    
    /* Database */
    strncpy(config->db_file, "data/marketpulse.db", sizeof(config->db_file) - 1);
    config->db_history_days = 30;
    
    /* UI */
    config->use_colors = 1;
    config->use_unicode = 1;
    
    /* Daemon */
    config->daemon_mode = 0;
    strncpy(config->pid_file, PID_FILE, sizeof(config->pid_file) - 1);
    
    /* AI */
    config->ai_enabled = 1;
    config->ai_adaptive_polling = 1;
    config->ai_volatility_threshold = 2.0;
    
    config->loaded_at = time(NULL);
}

/*
 * Check if config file exists
 */
int config_file_exists(const char *filename) {
    struct stat st;
    return stat(filename, &st) == 0;
}

/*
 * Create default config file
 */
int config_create_default(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        return -1;
    }
    
    fprintf(f, "{\n");
    fprintf(f, "  \"api\": {\n");
    fprintf(f, "    \"key\": \"%s\",\n", FINNHUB_API_KEY);
    fprintf(f, "    \"host\": \"%s\",\n", FINNHUB_HOST);
    fprintf(f, "    \"port\": %d\n", FINNHUB_PORT);
    fprintf(f, "  },\n");
    fprintf(f, "  \"timing\": {\n");
    fprintf(f, "    \"refresh_interval\": %d,\n", DEFAULT_REFRESH_INTERVAL);
    fprintf(f, "    \"alert_check_interval\": %d,\n", ALERT_CHECK_INTERVAL);
    fprintf(f, "    \"worker_timeout\": 30,\n");
    fprintf(f, "    \"rate_limit_requests\": 60\n");
    fprintf(f, "  },\n");
    fprintf(f, "  \"stocks\": [\"AAPL\", \"MSFT\", \"GOOGL\"],\n");
    fprintf(f, "  \"alerts\": [],\n");
    fprintf(f, "  \"logging\": {\n");
    fprintf(f, "    \"file\": \"/tmp/marketpulse.log\",\n");
    fprintf(f, "    \"level\": 1,\n");
    fprintf(f, "    \"rotation_size_mb\": 10\n");
    fprintf(f, "  },\n");
    fprintf(f, "  \"database\": {\n");
    fprintf(f, "    \"file\": \"data/marketpulse.db\",\n");
    fprintf(f, "    \"history_days\": 30\n");
    fprintf(f, "  },\n");
    fprintf(f, "  \"ui\": {\n");
    fprintf(f, "    \"colors\": true,\n");
    fprintf(f, "    \"unicode\": true\n");
    fprintf(f, "  },\n");
    fprintf(f, "  \"ai\": {\n");
    fprintf(f, "    \"enabled\": true,\n");
    fprintf(f, "    \"adaptive_polling\": true,\n");
    fprintf(f, "    \"volatility_threshold\": 2.0\n");
    fprintf(f, "  }\n");
    fprintf(f, "}\n");
    
    fclose(f);
    return 0;
}

/*
 * Load configuration from JSON file
 */
int config_load(const char *filename, AppConfig *config) {
    FILE *f;
    char *buffer;
    long file_size;
    char *value;
    double num;
    
    /* Set defaults first */
    config_set_defaults(config);
    
    /* Check if file exists */
    if (!config_file_exists(filename)) {
        /* Create default config */
        config_create_default(filename);
        return 0;
    }
    
    /* Open and read file */
    f = fopen(filename, "r");
    if (f == NULL) {
        return -1;
    }
    
    fseek(f, 0, SEEK_END);
    file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    buffer = malloc(file_size + 1);
    if (buffer == NULL) {
        fclose(f);
        return -1;
    }
    
    fread(buffer, 1, file_size, f);
    buffer[file_size] = '\0';
    fclose(f);
    
    /* Parse JSON values */
    value = json_get_string(buffer, "key");
    if (value) {
        strncpy(config->api_key, value, sizeof(config->api_key) - 1);
        free(value);
    }
    
    num = json_get_number(buffer, "refresh_interval");
    if (num > 0) config->refresh_interval = (int)num;
    
    num = json_get_number(buffer, "alert_check_interval");
    if (num > 0) config->alert_check_interval = (int)num;
    
    num = json_get_number(buffer, "worker_timeout");
    if (num > 0) config->worker_timeout = (int)num;
    
    num = json_get_number(buffer, "rate_limit_requests");
    if (num > 0) config->rate_limit_requests = (int)num;
    
    num = json_get_number(buffer, "level");
    if (num >= 0) config->log_level = (int)num;
    
    num = json_get_number(buffer, "history_days");
    if (num > 0) config->db_history_days = (int)num;
    
    num = json_get_number(buffer, "volatility_threshold");
    if (num > 0) config->ai_volatility_threshold = num;
    
    free(buffer);
    
    config->loaded_at = time(NULL);
    g_config_loaded = 1;
    
    return 0;
}

/*
 * Save configuration to file
 */
int config_save(const char *filename, const AppConfig *config) {
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        return -1;
    }
    
    fprintf(f, "{\n");
    fprintf(f, "  \"api\": {\n");
    fprintf(f, "    \"key\": \"%s\",\n", config->api_key);
    fprintf(f, "    \"host\": \"%s\",\n", config->api_host);
    fprintf(f, "    \"port\": %d\n", config->api_port);
    fprintf(f, "  },\n");
    fprintf(f, "  \"timing\": {\n");
    fprintf(f, "    \"refresh_interval\": %d,\n", config->refresh_interval);
    fprintf(f, "    \"alert_check_interval\": %d,\n", config->alert_check_interval);
    fprintf(f, "    \"worker_timeout\": %d,\n", config->worker_timeout);
    fprintf(f, "    \"rate_limit_requests\": %d\n", config->rate_limit_requests);
    fprintf(f, "  },\n");
    fprintf(f, "  \"logging\": {\n");
    fprintf(f, "    \"file\": \"%s\",\n", config->log_file);
    fprintf(f, "    \"level\": %d,\n", config->log_level);
    fprintf(f, "    \"rotation_size_mb\": %d\n", config->log_rotation_size);
    fprintf(f, "  },\n");
    fprintf(f, "  \"ai\": {\n");
    fprintf(f, "    \"enabled\": %s,\n", config->ai_enabled ? "true" : "false");
    fprintf(f, "    \"adaptive_polling\": %s,\n", config->ai_adaptive_polling ? "true" : "false");
    fprintf(f, "    \"volatility_threshold\": %.2f\n", config->ai_volatility_threshold);
    fprintf(f, "  }\n");
    fprintf(f, "}\n");
    
    fclose(f);
    return 0;
}

/*
 * Validate configuration
 */
int config_validate(const AppConfig *config) {
    if (config->refresh_interval < 1 || config->refresh_interval > 3600) {
        return -1;
    }
    if (config->api_key[0] == '\0') {
        return -1;
    }
    return 0;
}

/*
 * Print configuration
 */
void config_print(const AppConfig *config) {
    printf("\n=== Configuration ===\n");
    printf("API Host: %s:%d\n", config->api_host, config->api_port);
    printf("Refresh Interval: %d seconds\n", config->refresh_interval);
    printf("Worker Timeout: %d seconds\n", config->worker_timeout);
    printf("Rate Limit: %d requests/min\n", config->rate_limit_requests);
    printf("Log File: %s (level %d)\n", config->log_file, config->log_level);
    printf("Database: %s (%d days history)\n", config->db_file, config->db_history_days);
    printf("AI: %s (adaptive: %s)\n", 
           config->ai_enabled ? "enabled" : "disabled",
           config->ai_adaptive_polling ? "yes" : "no");
    printf("=====================\n\n");
}

/*
 * Add stock to config
 */
int config_add_stock(AppConfig *config, const char *symbol) {
    if (config->stock_count >= MAX_CONFIG_STOCKS) {
        return -1;
    }
    strncpy(config->stocks[config->stock_count], symbol, 15);
    config->stock_count++;
    return 0;
}

/*
 * Remove stock from config
 */
int config_remove_stock(AppConfig *config, const char *symbol) {
    int i, j;
    for (i = 0; i < config->stock_count; i++) {
        if (strcmp(config->stocks[i], symbol) == 0) {
            for (j = i; j < config->stock_count - 1; j++) {
                strcpy(config->stocks[j], config->stocks[j + 1]);
            }
            config->stock_count--;
            return 0;
        }
    }
    return -1;
}

/*
 * Add alert rule
 */
int config_add_alert(AppConfig *config, const char *symbol, double threshold, int above) {
    if (config->alert_count >= MAX_ALERTS) {
        return -1;
    }
    strncpy(config->alerts[config->alert_count].symbol, symbol, 15);
    config->alerts[config->alert_count].threshold = threshold;
    config->alerts[config->alert_count].above = above;
    config->alerts[config->alert_count].enabled = 1;
    config->alert_count++;
    return 0;
}

/*
 * Get global config
 */
AppConfig *config_get_global(void) {
    if (!g_config_loaded) {
        config_set_defaults(&g_config);
    }
    return &g_config;
}

/*
 * Set global config
 */
void config_set_global(AppConfig *config) {
    memcpy(&g_config, config, sizeof(AppConfig));
    g_config_loaded = 1;
}