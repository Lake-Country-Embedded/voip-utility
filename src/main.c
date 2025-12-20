/*
 * voip-utility - SIP VoIP Testing Utility
 * Main entry point
 */

#include "cli/cli.h"
#include "config/config.h"
#include "util/log.h"
#include "util/error.h"
#include "util/json_output.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

/* Global running flag */
static volatile int g_running = 1;

/* Signal handler */
static void signal_handler(int sig)
{
    (void)sig;
    g_running = 0;
    VU_LOG_INFO("Received signal, shutting down...");
}

int main(int argc, char **argv)
{
    int exit_code = 0;
    vu_cli_args_t args;
    vu_config_t config;

    /* Initialize logging with defaults first */
    vu_log_init(NULL);

    /* Parse command line */
    vu_error_t err = vu_cli_parse(argc, argv, &args);
    if (err != VU_OK) {
        fprintf(stderr, "Error: %s\n", vu_get_last_error()->message);
        fprintf(stderr, "Try '%s --help' for usage.\n", argv[0]);
        return 1;
    }

    /* Handle help and version before anything else */
    if (args.command == VU_CMD_HELP) {
        vu_cli_print_help(argv[0]);
        return 0;
    }

    if (args.command == VU_CMD_VERSION) {
        vu_cli_print_version();
        return 0;
    }

    /* Configure logging based on arguments */
    vu_log_config_t log_config = vu_log_default_config();

    if (args.global.verbose) {
        log_config.level = VU_LOG_DEBUG;
    } else if (args.global.quiet) {
        log_config.level = VU_LOG_ERROR;
    } else if (args.global.log_level) {
        log_config.level = vu_log_level_from_string(args.global.log_level);
    }

    log_config.json_output = args.global.json_output;
    vu_log_init(&log_config);

    /* Initialize JSON output if requested */
    if (args.global.json_output) {
        vu_json_output_init(stdout);
    }

    /* Load configuration */
    err = vu_config_load(&config, args.global.config_file);
    if (err != VU_OK && args.global.config_file) {
        /* Config file was explicitly specified but failed to load */
        VU_LOG_ERROR("Failed to load config: %s", vu_get_last_error()->message);
        return 1;
    }

    /* Override config log level if set via CLI */
    if (!args.global.log_level && !args.global.verbose && !args.global.quiet) {
        vu_log_set_level(vu_log_level_from_string(config.log_level));
    }

    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    VU_LOG_DEBUG("voip-utility starting, command: %s", vu_command_name(args.command));

    /* Dispatch to command handler */
    switch (args.command) {
    case VU_CMD_REGISTER:
        exit_code = vu_cmd_register(&args, &config);
        break;

    case VU_CMD_CALL:
        exit_code = vu_cmd_call(&args, &config);
        break;

    case VU_CMD_RECEIVE:
        exit_code = vu_cmd_receive(&args, &config);
        break;

    case VU_CMD_TEST:
        exit_code = vu_cmd_test(&args, &config);
        break;

    case VU_CMD_INTERACTIVE:
        exit_code = vu_cmd_interactive(&args, &config);
        break;

    case VU_CMD_ANALYZE:
        exit_code = vu_cmd_analyze(&args, &config);
        break;

    default:
        VU_LOG_ERROR("Unknown command");
        exit_code = 1;
    }

    VU_LOG_DEBUG("voip-utility exiting with code %d", exit_code);
    return exit_code;
}

/* Export running flag for other modules */
int vu_is_running(void)
{
    return g_running;
}

void vu_request_shutdown(void)
{
    g_running = 0;
}
