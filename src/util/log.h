/*
 * voip-utility - SIP VoIP Testing Utility
 * Logging infrastructure
 */

#ifndef VU_LOG_H
#define VU_LOG_H

#include <stdio.h>
#include <stdbool.h>

/* Log levels */
typedef enum {
    VU_LOG_ERROR = 0,
    VU_LOG_WARN,
    VU_LOG_INFO,
    VU_LOG_DEBUG,
    VU_LOG_TRACE
} vu_log_level_t;

/* Log configuration */
typedef struct vu_log_config {
    vu_log_level_t level;
    bool json_output;           /* Output logs as JSON for automation */
    bool include_timestamp;
    bool include_source;        /* Include file:line */
    bool color_output;          /* Use ANSI colors */
    FILE *output;               /* Output stream (default stderr) */
} vu_log_config_t;

/*
 * Initialize logging with configuration.
 * If config is NULL, uses defaults (INFO level, stderr, timestamps).
 */
void vu_log_init(const vu_log_config_t *config);

/*
 * Get default log configuration
 */
vu_log_config_t vu_log_default_config(void);

/*
 * Set log level at runtime
 */
void vu_log_set_level(vu_log_level_t level);

/*
 * Get current log level
 */
vu_log_level_t vu_log_get_level(void);

/*
 * Enable/disable JSON output
 */
void vu_log_set_json(bool enabled);

/*
 * Core logging function (internal use via macros)
 */
void vu_log_internal(vu_log_level_t level, const char *file, int line,
                     const char *fmt, ...);

/* Convenience macros */
#define VU_LOG_ERROR(fmt, ...) \
    vu_log_internal(VU_LOG_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define VU_LOG_WARN(fmt, ...) \
    vu_log_internal(VU_LOG_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define VU_LOG_INFO(fmt, ...) \
    vu_log_internal(VU_LOG_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define VU_LOG_DEBUG(fmt, ...) \
    vu_log_internal(VU_LOG_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define VU_LOG_TRACE(fmt, ...) \
    vu_log_internal(VU_LOG_TRACE, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

/* Parse log level from string (for CLI --verbose/--quiet) */
vu_log_level_t vu_log_level_from_string(const char *str);

/* Get log level name */
const char *vu_log_level_name(vu_log_level_t level);

#endif /* VU_LOG_H */
