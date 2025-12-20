/*
 * voip-utility - SIP VoIP Testing Utility
 * CLI argument parsing implementation
 */

#include "cli/cli.h"
#include "util/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#define VERSION "0.1.0"

const char *vu_command_name(vu_command_t cmd)
{
    switch (cmd) {
    case VU_CMD_REGISTER:    return "register";
    case VU_CMD_CALL:        return "call";
    case VU_CMD_RECEIVE:     return "receive";
    case VU_CMD_TEST:        return "test";
    case VU_CMD_INTERACTIVE: return "interactive";
    case VU_CMD_ANALYZE:     return "analyze";
    case VU_CMD_HELP:        return "help";
    case VU_CMD_VERSION:     return "version";
    default:                 return "unknown";
    }
}

void vu_cli_print_version(void)
{
    printf("voip-utility %s\n", VERSION);
    printf("SIP VoIP Testing Utility\n");
}

void vu_cli_print_help(const char *program_name)
{
    printf("Usage: %s [OPTIONS] <COMMAND> [COMMAND_OPTIONS]\n\n", program_name);
    printf("SIP VoIP Testing Utility - automated VoIP testing with expect-style scripting\n\n");

    printf("Commands:\n");
    printf("  register     Register SIP account(s) and show status\n");
    printf("  call         Make an outbound call\n");
    printf("  receive      Wait for incoming calls\n");
    printf("  test         Run automated test from JSON file\n");
    printf("  interactive  Interactive REPL for manual testing\n");
    printf("  analyze      Analyze recorded audio files\n");
    printf("  help         Show this help message\n");
    printf("  version      Show version information\n\n");

    printf("Global Options:\n");
    printf("  -c, --config <file>    Configuration file path\n");
    printf("  -l, --log-level <lvl>  Log level (error, warn, info, debug, trace)\n");
    printf("  -j, --json             Output in JSON format\n");
    printf("  -v, --verbose          Enable verbose output (debug level)\n");
    printf("  -q, --quiet            Quiet mode (errors only)\n");
    printf("  -h, --help             Show help for command\n");
    printf("  -V, --version          Show version\n\n");

    printf("Examples:\n");
    printf("  %s register -a ext6004\n", program_name);
    printf("  %s call -a ext6004 -u sip:6005@192.168.10.10\n", program_name);
    printf("  %s receive -a ext6003 --auto-answer --timeout 60\n", program_name);
    printf("  %s test -f paging_test.json\n", program_name);
    printf("\nUse '%s <command> --help' for more information about a command.\n", program_name);
}

void vu_cli_print_command_help(vu_command_t cmd)
{
    switch (cmd) {
    case VU_CMD_REGISTER:
        printf("Usage: voip-utility register [OPTIONS]\n\n");
        printf("Register SIP account(s) and show status.\n\n");
        printf("Options:\n");
        printf("  -a, --account <id>   Account ID to register (default: all)\n");
        printf("  -t, --timeout <sec>  Wait timeout for registration (default: 30)\n");
        printf("  -u, --unregister     Unregister instead of register\n");
        break;

    case VU_CMD_CALL:
        printf("Usage: voip-utility call [OPTIONS] -u <URI>\n\n");
        printf("Make an outbound call.\n\n");
        printf("Options:\n");
        printf("  -a, --account <id>       Account ID to use\n");
        printf("  -u, --uri <uri>          SIP URI to call (required)\n");
        printf("  -r, --record <path>      Record audio to file\n");
        printf("  -p, --play <file>        Play audio file during call\n");
        printf("  -d, --dtmf <digits>      Send DTMF digits\n");
        printf("  -t, --timeout <sec>      Call timeout (default: 60)\n");
        printf("  -H, --hangup-after <sec> Hangup after N seconds\n");
        break;

    case VU_CMD_RECEIVE:
        printf("Usage: voip-utility receive [OPTIONS]\n\n");
        printf("Wait for incoming calls.\n\n");
        printf("Options:\n");
        printf("  -a, --account <id>       Account ID to use\n");
        printf("  -t, --timeout <sec>      Wait timeout (0 = forever)\n");
        printf("  -A, --auto-answer        Automatically answer incoming calls\n");
        printf("  -D, --answer-delay <ms>  Delay before answering (default: 0)\n");
        printf("  -r, --record <path>      Record audio to file\n");
        printf("  -p, --play <file>        Play audio file after answering\n");
        printf("  -d, --dtmf <digits>      Send DTMF after answering\n");
        printf("  -H, --hangup-after <sec> Hangup after N seconds\n");
        break;

    case VU_CMD_TEST:
        printf("Usage: voip-utility test [OPTIONS] -f <file>\n\n");
        printf("Run automated test from JSON file.\n\n");
        printf("Options:\n");
        printf("  -f, --file <file>    Test definition JSON file (required)\n");
        printf("  -o, --output <dir>   Output directory for results\n");
        printf("  -s, --stop-on-fail   Stop on first failure\n");
        break;

    case VU_CMD_INTERACTIVE:
        printf("Usage: voip-utility interactive [OPTIONS]\n\n");
        printf("Interactive REPL for manual testing.\n\n");
        printf("Options:\n");
        printf("  -a, --account <id>   Default account to use\n");
        break;

    case VU_CMD_ANALYZE:
        printf("Usage: voip-utility analyze [OPTIONS] <file>\n\n");
        printf("Analyze recorded audio files.\n\n");
        printf("Options:\n");
        printf("  -b, --beeps          Show detected beeps\n");
        printf("  -D, --dtmf           Show detected DTMF tones\n");
        printf("  -s, --stats          Show audio statistics\n");
        break;

    default:
        printf("Unknown command. Use 'voip-utility --help' for usage.\n");
    }
}

/* Parse command from string */
static vu_command_t parse_command(const char *str)
{
    if (!str) return VU_CMD_NONE;

    if (strcmp(str, "register") == 0 || strcmp(str, "reg") == 0)
        return VU_CMD_REGISTER;
    if (strcmp(str, "call") == 0)
        return VU_CMD_CALL;
    if (strcmp(str, "receive") == 0 || strcmp(str, "recv") == 0)
        return VU_CMD_RECEIVE;
    if (strcmp(str, "test") == 0)
        return VU_CMD_TEST;
    if (strcmp(str, "interactive") == 0 || strcmp(str, "repl") == 0)
        return VU_CMD_INTERACTIVE;
    if (strcmp(str, "analyze") == 0)
        return VU_CMD_ANALYZE;
    if (strcmp(str, "help") == 0)
        return VU_CMD_HELP;
    if (strcmp(str, "version") == 0)
        return VU_CMD_VERSION;

    return VU_CMD_NONE;
}

/* Global options (parsed before command) */
static struct option global_options[] = {
    {"config",    required_argument, 0, 'c'},
    {"log-level", required_argument, 0, 'l'},
    {"json",      no_argument,       0, 'j'},
    {"verbose",   no_argument,       0, 'v'},
    {"quiet",     no_argument,       0, 'q'},
    {"help",      no_argument,       0, 'h'},
    {"version",   no_argument,       0, 'V'},
    {0, 0, 0, 0}
};

/* Register command options */
static struct option register_options[] = {
    {"account",    required_argument, 0, 'a'},
    {"timeout",    required_argument, 0, 't'},
    {"unregister", no_argument,       0, 'u'},
    {"help",       no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

/* Call command options */
static struct option call_options[] = {
    {"account",      required_argument, 0, 'a'},
    {"uri",          required_argument, 0, 'u'},
    {"record",       required_argument, 0, 'r'},
    {"play",         required_argument, 0, 'p'},
    {"dtmf",         required_argument, 0, 'd'},
    {"timeout",      required_argument, 0, 't'},
    {"hangup-after", required_argument, 0, 'H'},
    {"help",         no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

/* Receive command options */
static struct option receive_options[] = {
    {"account",      required_argument, 0, 'a'},
    {"timeout",      required_argument, 0, 't'},
    {"auto-answer",  no_argument,       0, 'A'},
    {"answer-delay", required_argument, 0, 'D'},
    {"record",       required_argument, 0, 'r'},
    {"play",         required_argument, 0, 'p'},
    {"dtmf",         required_argument, 0, 'd'},
    {"hangup-after", required_argument, 0, 'H'},
    {"help",         no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

/* Test command options */
static struct option test_options[] = {
    {"file",         required_argument, 0, 'f'},
    {"output",       required_argument, 0, 'o'},
    {"stop-on-fail", no_argument,       0, 's'},
    {"help",         no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

/* Interactive command options */
static struct option interactive_options[] = {
    {"account", required_argument, 0, 'a'},
    {"help",    no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

/* Analyze command options */
static struct option analyze_options[] = {
    {"beeps", no_argument, 0, 'b'},
    {"dtmf",  no_argument, 0, 'D'},
    {"stats", no_argument, 0, 's'},
    {"help",  no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

vu_error_t vu_cli_parse(int argc, char **argv, vu_cli_args_t *args)
{
    if (!args) {
        return VU_ERR_INVALID_ARG;
    }

    /* Initialize with zeros - command-specific defaults set after command is known */
    memset(args, 0, sizeof(*args));

    /* Parse global options first */
    int opt;
    optind = 1;

    while ((opt = getopt_long(argc, argv, "+c:l:jvqhV", global_options, NULL)) != -1) {
        switch (opt) {
        case 'c':
            args->global.config_file = optarg;
            break;
        case 'l':
            args->global.log_level = optarg;
            break;
        case 'j':
            args->global.json_output = true;
            break;
        case 'v':
            args->global.verbose = true;
            break;
        case 'q':
            args->global.quiet = true;
            break;
        case 'h':
            args->command = VU_CMD_HELP;
            return VU_OK;
        case 'V':
            args->command = VU_CMD_VERSION;
            return VU_OK;
        default:
            break;
        }
    }

    /* Get command */
    if (optind >= argc) {
        args->command = VU_CMD_HELP;
        return VU_OK;
    }

    args->command = parse_command(argv[optind]);
    if (args->command == VU_CMD_NONE) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "Unknown command: %s", argv[optind]);
        return VU_ERR_INVALID_ARG;
    }

    /* Save position of command and move past it */
    int cmd_start = optind;
    optind++;

    /* Parse command-specific options */
    /* Create new argc/argv starting from command */
    int cmd_argc = argc - cmd_start;
    char **cmd_argv = argv + cmd_start;
    optind = 1;  /* Reset for subcommand parsing */

    switch (args->command) {
    case VU_CMD_REGISTER:
        args->cmd.reg.timeout_sec = 30;  /* default */
        while ((opt = getopt_long(cmd_argc, cmd_argv, "a:t:uh", register_options, NULL)) != -1) {
            switch (opt) {
            case 'a': args->cmd.reg.account_id = optarg; break;
            case 't': args->cmd.reg.timeout_sec = atoi(optarg); break;
            case 'u': args->cmd.reg.unregister = true; break;
            case 'h': vu_cli_print_command_help(VU_CMD_REGISTER); exit(0);
            }
        }
        break;

    case VU_CMD_CALL:
        args->cmd.call.timeout_sec = 60;  /* default */
        while ((opt = getopt_long(cmd_argc, cmd_argv, "a:u:r:p:d:t:H:h", call_options, NULL)) != -1) {
            switch (opt) {
            case 'a': args->cmd.call.account_id = optarg; break;
            case 'u': args->cmd.call.uri = optarg; break;
            case 'r': args->cmd.call.record_path = optarg; break;
            case 'p': args->cmd.call.play_file = optarg; break;
            case 'd': args->cmd.call.dtmf = optarg; break;
            case 't': args->cmd.call.timeout_sec = atoi(optarg); break;
            case 'H': args->cmd.call.hangup_after_sec = atoi(optarg); break;
            case 'h': vu_cli_print_command_help(VU_CMD_CALL); exit(0);
            }
        }
        break;

    case VU_CMD_RECEIVE:
        while ((opt = getopt_long(cmd_argc, cmd_argv, "a:t:AD:r:p:d:H:h", receive_options, NULL)) != -1) {
            switch (opt) {
            case 'a': args->cmd.receive.account_id = optarg; break;
            case 't': args->cmd.receive.timeout_sec = atoi(optarg); break;
            case 'A': args->cmd.receive.auto_answer = true; break;
            case 'D': args->cmd.receive.answer_delay_ms = atoi(optarg); break;
            case 'r': args->cmd.receive.record_path = optarg; break;
            case 'p': args->cmd.receive.play_file = optarg; break;
            case 'd': args->cmd.receive.dtmf = optarg; break;
            case 'H': args->cmd.receive.hangup_after_sec = atoi(optarg); break;
            case 'h': vu_cli_print_command_help(VU_CMD_RECEIVE); exit(0);
            }
        }
        break;

    case VU_CMD_TEST:
        while ((opt = getopt_long(cmd_argc, cmd_argv, "f:o:sh", test_options, NULL)) != -1) {
            switch (opt) {
            case 'f': args->cmd.test.test_file = optarg; break;
            case 'o': args->cmd.test.output_dir = optarg; break;
            case 's': args->cmd.test.stop_on_fail = true; break;
            case 'h': vu_cli_print_command_help(VU_CMD_TEST); exit(0);
            }
        }
        break;

    case VU_CMD_INTERACTIVE:
        while ((opt = getopt_long(cmd_argc, cmd_argv, "a:h", interactive_options, NULL)) != -1) {
            switch (opt) {
            case 'a': args->cmd.interactive.account_id = optarg; break;
            case 'h': vu_cli_print_command_help(VU_CMD_INTERACTIVE); exit(0);
            }
        }
        break;

    case VU_CMD_ANALYZE:
        while ((opt = getopt_long(cmd_argc, cmd_argv, "bDsh", analyze_options, NULL)) != -1) {
            switch (opt) {
            case 'b': args->cmd.analyze.show_beeps = true; break;
            case 'D': args->cmd.analyze.show_dtmf = true; break;
            case 's': args->cmd.analyze.show_stats = true; break;
            case 'h': vu_cli_print_command_help(VU_CMD_ANALYZE); exit(0);
            }
        }
        /* Get input file as positional argument */
        if (optind < cmd_argc) {
            args->cmd.analyze.input_file = cmd_argv[optind];
        }
        break;

    default:
        break;
    }

    return VU_OK;
}
