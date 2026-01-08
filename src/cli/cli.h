/*
 * voip-utility - SIP VoIP Testing Utility
 * CLI argument parsing and dispatch
 */

#ifndef VU_CLI_H
#define VU_CLI_H

#include "util/error.h"
#include "config/config.h"
#include <stdbool.h>

/* Subcommand identifiers */
typedef enum {
    VU_CMD_NONE = 0,
    VU_CMD_REGISTER,
    VU_CMD_CALL,
    VU_CMD_RECEIVE,
    VU_CMD_TEST,
    VU_CMD_INTERACTIVE,
    VU_CMD_ANALYZE,
    VU_CMD_HELP,
    VU_CMD_VERSION
} vu_command_t;

/* Global options (apply to all commands) */
typedef struct vu_global_opts {
    const char *config_file;
    const char *log_level;
    bool json_output;
    bool verbose;
    bool quiet;
} vu_global_opts_t;

/* Register command options */
typedef struct vu_register_opts {
    const char *account_id;     /* Specific account to register (NULL = all) */
    int timeout_sec;            /* Wait for registration result */
    bool unregister;            /* Unregister instead of register */
} vu_register_opts_t;

/* Call command options */
typedef struct vu_call_opts {
    const char *account_id;     /* Account to use */
    const char *uri;            /* SIP URI to call */
    const char *record_path;    /* Recording output path */
    const char *play_file;      /* Audio file to play */
    const char *dtmf;           /* DTMF digits to send */
    int timeout_sec;            /* Call timeout (0 = no timeout) */
    int hangup_after_sec;       /* Hangup after N seconds (0 = manual) */
    int dtmf_delay_ms;          /* Delay before sending DTMF (default 500ms) */
    int play_delay_ms;          /* Delay before playing audio (0 = play immediately) */
    bool auto_answer;           /* Answer incoming calls */
} vu_call_opts_t;

/* Receive command options */
typedef struct vu_receive_opts {
    const char *account_id;     /* Account to use */
    const char *record_path;    /* Recording output path */
    const char *play_file;      /* Audio file to play after answer */
    const char *dtmf;           /* DTMF digits to send after answer */
    int timeout_sec;            /* Wait timeout (0 = forever) */
    int answer_delay_ms;        /* Delay before answering */
    int hangup_after_sec;       /* Hangup after N seconds (0 = wait for remote) */
    bool auto_answer;           /* Automatically answer incoming calls */
} vu_receive_opts_t;

/* Test command options */
typedef struct vu_test_opts {
    const char *test_file;      /* Test definition JSON file */
    const char *output_dir;     /* Output directory for results */
    bool stop_on_fail;          /* Stop on first failure */
} vu_test_opts_t;

/* Interactive command options */
typedef struct vu_interactive_opts {
    const char *account_id;     /* Default account to use */
} vu_interactive_opts_t;

/* Analyze command options */
typedef struct vu_analyze_opts {
    const char *input_file;     /* Audio file to analyze */
    bool show_beeps;            /* Show detected beeps */
    bool show_dtmf;             /* Show detected DTMF */
    bool show_stats;            /* Show audio statistics */
} vu_analyze_opts_t;

/* Parsed CLI arguments */
typedef struct vu_cli_args {
    vu_command_t command;
    vu_global_opts_t global;

    union {
        vu_register_opts_t reg;
        vu_call_opts_t call;
        vu_receive_opts_t receive;
        vu_test_opts_t test;
        vu_interactive_opts_t interactive;
        vu_analyze_opts_t analyze;
    } cmd;
} vu_cli_args_t;

/*
 * Parse command line arguments
 * Returns VU_OK on success, error code on failure.
 */
vu_error_t vu_cli_parse(int argc, char **argv, vu_cli_args_t *args);

/*
 * Print usage help
 */
void vu_cli_print_help(const char *program_name);

/*
 * Print version
 */
void vu_cli_print_version(void);

/*
 * Print help for specific command
 */
void vu_cli_print_command_help(vu_command_t cmd);

/*
 * Get command name string
 */
const char *vu_command_name(vu_command_t cmd);

/* Subcommand entry points */
int vu_cmd_register(const vu_cli_args_t *args, vu_config_t *config);
int vu_cmd_call(const vu_cli_args_t *args, vu_config_t *config);
int vu_cmd_receive(const vu_cli_args_t *args, vu_config_t *config);
int vu_cmd_test(const vu_cli_args_t *args, vu_config_t *config);
int vu_cmd_interactive(const vu_cli_args_t *args, vu_config_t *config);
int vu_cmd_analyze(const vu_cli_args_t *args, vu_config_t *config);

#endif /* VU_CLI_H */
