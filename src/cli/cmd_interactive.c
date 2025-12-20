/*
 * voip-utility - SIP VoIP Testing Utility
 * Interactive REPL command implementation
 */

#include "cli/cli.h"
#include "util/log.h"

int vu_cmd_interactive(const vu_cli_args_t *args, vu_config_t *config)
{
    (void)args;
    (void)config;

    VU_LOG_INFO("Interactive mode not yet implemented");
    VU_LOG_INFO("Coming soon: REPL for manual VoIP testing");

    /* TODO: Implement readline-based REPL with commands like:
     * register <account>
     * call <uri>
     * hangup
     * dtmf <digits>
     * record <file>
     * play <file>
     * status
     * quit
     */

    return 0;
}
