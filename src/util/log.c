/*
 * voip-utility - SIP VoIP Testing Utility
 * Logging implementation
 */

#include "util/log.h"
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

/* ANSI color codes */
#define ANSI_RED     "\x1b[31m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_GRAY    "\x1b[90m"
#define ANSI_RESET   "\x1b[0m"

/* Global log configuration */
static vu_log_config_t g_log_config = {
    .level = VU_LOG_INFO,
    .json_output = false,
    .include_timestamp = true,
    .include_source = false,
    .color_output = true,
    .output = NULL
};

static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;

vu_log_config_t vu_log_default_config(void)
{
    vu_log_config_t config = {
        .level = VU_LOG_INFO,
        .json_output = false,
        .include_timestamp = true,
        .include_source = false,
        .color_output = true,
        .output = NULL
    };
    return config;
}

void vu_log_init(const vu_log_config_t *config)
{
    pthread_mutex_lock(&g_log_mutex);

    if (config) {
        g_log_config = *config;
    } else {
        g_log_config = vu_log_default_config();
    }

    if (!g_log_config.output) {
        g_log_config.output = stderr;
    }

    /* Disable colors if output is not a TTY */
    if (g_log_config.color_output && !isatty(fileno(g_log_config.output))) {
        g_log_config.color_output = false;
    }

    pthread_mutex_unlock(&g_log_mutex);
}

void vu_log_set_level(vu_log_level_t level)
{
    pthread_mutex_lock(&g_log_mutex);
    g_log_config.level = level;
    pthread_mutex_unlock(&g_log_mutex);
}

vu_log_level_t vu_log_get_level(void)
{
    pthread_mutex_lock(&g_log_mutex);
    vu_log_level_t level = g_log_config.level;
    pthread_mutex_unlock(&g_log_mutex);
    return level;
}

void vu_log_set_json(bool enabled)
{
    pthread_mutex_lock(&g_log_mutex);
    g_log_config.json_output = enabled;
    pthread_mutex_unlock(&g_log_mutex);
}

const char *vu_log_level_name(vu_log_level_t level)
{
    switch (level) {
    case VU_LOG_ERROR: return "ERROR";
    case VU_LOG_WARN:  return "WARN";
    case VU_LOG_INFO:  return "INFO";
    case VU_LOG_DEBUG: return "DEBUG";
    case VU_LOG_TRACE: return "TRACE";
    default:           return "UNKNOWN";
    }
}

vu_log_level_t vu_log_level_from_string(const char *str)
{
    if (!str) return VU_LOG_INFO;

    if (strcasecmp(str, "error") == 0) return VU_LOG_ERROR;
    if (strcasecmp(str, "warn") == 0 || strcasecmp(str, "warning") == 0)
        return VU_LOG_WARN;
    if (strcasecmp(str, "info") == 0) return VU_LOG_INFO;
    if (strcasecmp(str, "debug") == 0) return VU_LOG_DEBUG;
    if (strcasecmp(str, "trace") == 0) return VU_LOG_TRACE;

    return VU_LOG_INFO;
}

static const char *level_color(vu_log_level_t level)
{
    switch (level) {
    case VU_LOG_ERROR: return ANSI_RED;
    case VU_LOG_WARN:  return ANSI_YELLOW;
    case VU_LOG_INFO:  return ANSI_GREEN;
    case VU_LOG_DEBUG: return ANSI_BLUE;
    case VU_LOG_TRACE: return ANSI_GRAY;
    default:           return ANSI_RESET;
    }
}

static void escape_json_string(char *out, size_t out_size, const char *in)
{
    size_t j = 0;
    for (size_t i = 0; in[i] && j < out_size - 1; i++) {
        char c = in[i];
        if (c == '"' || c == '\\') {
            if (j + 2 >= out_size) break;
            out[j++] = '\\';
            out[j++] = c;
        } else if (c == '\n') {
            if (j + 2 >= out_size) break;
            out[j++] = '\\';
            out[j++] = 'n';
        } else if (c == '\r') {
            if (j + 2 >= out_size) break;
            out[j++] = '\\';
            out[j++] = 'r';
        } else if (c == '\t') {
            if (j + 2 >= out_size) break;
            out[j++] = '\\';
            out[j++] = 't';
        } else {
            out[j++] = c;
        }
    }
    out[j] = '\0';
}

void vu_log_internal(vu_log_level_t level, const char *file, int line,
                     const char *fmt, ...)
{
    pthread_mutex_lock(&g_log_mutex);

    /* Check level */
    if (level > g_log_config.level) {
        pthread_mutex_unlock(&g_log_mutex);
        return;
    }

    FILE *out = g_log_config.output ? g_log_config.output : stderr;

    /* Format message */
    char message[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    /* Get timestamp */
    char timestamp[32] = "";
    if (g_log_config.include_timestamp || g_log_config.json_output) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    }

    if (g_log_config.json_output) {
        /* JSON output */
        char escaped_msg[4096];
        escape_json_string(escaped_msg, sizeof(escaped_msg), message);

        fprintf(out, "{\"timestamp\":\"%s\",\"level\":\"%s\"",
                timestamp, vu_log_level_name(level));

        if (g_log_config.include_source) {
            /* Extract just filename */
            const char *filename = strrchr(file, '/');
            filename = filename ? filename + 1 : file;
            fprintf(out, ",\"file\":\"%s\",\"line\":%d", filename, line);
        }

        fprintf(out, ",\"message\":\"%s\"}\n", escaped_msg);
    } else {
        /* Plain text output */
        if (g_log_config.include_timestamp) {
            fprintf(out, "%s ", timestamp);
        }

        if (g_log_config.color_output) {
            fprintf(out, "%s%-5s%s ",
                    level_color(level),
                    vu_log_level_name(level),
                    ANSI_RESET);
        } else {
            fprintf(out, "%-5s ", vu_log_level_name(level));
        }

        if (g_log_config.include_source) {
            const char *filename = strrchr(file, '/');
            filename = filename ? filename + 1 : file;
            if (g_log_config.color_output) {
                fprintf(out, "%s%s:%d%s ", ANSI_GRAY, filename, line, ANSI_RESET);
            } else {
                fprintf(out, "%s:%d ", filename, line);
            }
        }

        fprintf(out, "%s\n", message);
    }

    fflush(out);
    pthread_mutex_unlock(&g_log_mutex);
}
