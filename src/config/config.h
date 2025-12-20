/*
 * voip-utility - SIP VoIP Testing Utility
 * Configuration handling
 */

#ifndef VU_CONFIG_H
#define VU_CONFIG_H

#include "util/error.h"
#include <stdbool.h>
#include <stdint.h>

#define VU_MAX_ACCOUNTS 8
#define VU_MAX_URI_LEN 256
#define VU_MAX_USERNAME_LEN 64
#define VU_MAX_PASSWORD_LEN 64
#define VU_MAX_PATH_LEN 512

/* Transport type */
typedef enum {
    VU_TRANSPORT_UDP = 0,
    VU_TRANSPORT_TCP,
    VU_TRANSPORT_TLS
} vu_transport_t;

/* SRTP mode */
typedef enum {
    VU_SRTP_DISABLED = 0,
    VU_SRTP_OPTIONAL,
    VU_SRTP_MANDATORY
} vu_srtp_mode_t;

/* SIP account configuration */
typedef struct vu_account_config {
    char id[VU_MAX_URI_LEN];                 /* Unique account identifier */
    char username[VU_MAX_USERNAME_LEN];
    char password[VU_MAX_PASSWORD_LEN];
    char server[VU_MAX_URI_LEN];             /* SIP server hostname/IP */
    uint16_t port;                           /* SIP port (default 5060) */
    char realm[VU_MAX_URI_LEN];              /* Auth realm (often same as server) */
    char display_name[VU_MAX_USERNAME_LEN];

    vu_transport_t transport;
    vu_srtp_mode_t srtp;

    uint32_t reg_timeout_sec;                /* Registration expiry (default 3600) */
    uint32_t reg_retry_interval_sec;         /* Retry on failure (default 30) */
    bool enabled;                            /* Whether to register on startup */
} vu_account_config_t;

/* Beep detection configuration */
typedef struct vu_beep_config {
    double min_level_db;                     /* Minimum level to consider (default -20) */
    double min_duration_sec;                 /* Minimum beep duration (default 0.2) */
    double max_duration_sec;                 /* Maximum beep duration (default 0.6) */
    double target_freq_hz;                   /* Expected frequency (0 = any) */
    double freq_tolerance_hz;                /* Tolerance around target (default 50) */
    double gap_duration_sec;                 /* Min gap between beeps (default 0.1) */
} vu_beep_config_t;

/* Audio configuration */
typedef struct vu_audio_config {
    uint32_t sample_rate;                    /* Sample rate (default 16000) */
    uint32_t frame_duration_ms;              /* Frame size in ms (default 20) */
    char default_codec[32];                  /* Preferred codec (default "PCMU") */
} vu_audio_config_t;

/* Main configuration structure */
typedef struct vu_config {
    /* Accounts */
    vu_account_config_t accounts[VU_MAX_ACCOUNTS];
    int account_count;

    /* Audio settings */
    vu_audio_config_t audio;

    /* Beep detection defaults */
    vu_beep_config_t beep;

    /* Paths */
    char recordings_dir[VU_MAX_PATH_LEN];    /* Directory for recordings */
    char tests_dir[VU_MAX_PATH_LEN];         /* Directory for test files */

    /* Logging */
    char log_level[16];                      /* Log level string */
    bool json_output;                        /* Default JSON output mode */
} vu_config_t;

/*
 * Get default configuration values
 */
vu_config_t vu_config_defaults(void);

/*
 * Load configuration from file.
 * If path is NULL, looks in default locations:
 *   1. ./voip-utility.json
 *   2. ~/.config/voip-utility/config.json
 *   3. /etc/voip-utility/config.json
 *
 * Returns VU_OK on success, error code on failure.
 * If file not found and path was NULL, returns defaults without error.
 */
vu_error_t vu_config_load(vu_config_t *config, const char *path);

/*
 * Save configuration to file
 */
vu_error_t vu_config_save(const vu_config_t *config, const char *path);

/*
 * Find account by ID
 * Returns pointer to account or NULL if not found
 */
vu_account_config_t *vu_config_find_account(vu_config_t *config, const char *id);
const vu_account_config_t *vu_config_find_account_const(const vu_config_t *config, const char *id);

/*
 * Add account to configuration
 * Returns VU_OK on success, VU_ERR_ALREADY_EXISTS if id exists,
 * VU_ERR_NO_MEMORY if max accounts reached
 */
vu_error_t vu_config_add_account(vu_config_t *config, const vu_account_config_t *account);

/*
 * Remove account from configuration
 */
vu_error_t vu_config_remove_account(vu_config_t *config, const char *id);

/*
 * Get transport name string
 */
const char *vu_transport_name(vu_transport_t transport);

/*
 * Parse transport from string
 */
vu_transport_t vu_transport_from_string(const char *str);

/*
 * Get SRTP mode name string
 */
const char *vu_srtp_mode_name(vu_srtp_mode_t mode);

/*
 * Parse SRTP mode from string
 */
vu_srtp_mode_t vu_srtp_mode_from_string(const char *str);

#endif /* VU_CONFIG_H */
