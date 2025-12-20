/*
 * voip-utility - SIP VoIP Testing Utility
 * Test command implementation
 */

#include "cli/cli.h"
#include "test/test_engine.h"
#include "util/log.h"

int vu_cmd_test(const vu_cli_args_t *args, vu_config_t *config)
{
    if (!args || !config) return 1;

    const vu_test_opts_t *opts = &args->cmd.test;

    if (!opts->test_file) {
        VU_LOG_ERROR("Test file is required. Use -f <file>");
        return 1;
    }

    VU_LOG_INFO("Running test: %s", opts->test_file);

    vu_test_engine_t *engine = vu_test_engine_create(config);
    if (!engine) {
        VU_LOG_ERROR("Failed to create test engine");
        return 1;
    }

    vu_error_t err = vu_test_engine_load(engine, opts->test_file);
    if (err != VU_OK) {
        VU_LOG_ERROR("Failed to load test: %s", vu_error_str(err));
        vu_test_engine_destroy(engine);
        return 1;
    }

    err = vu_test_engine_run(engine);
    if (err != VU_OK) {
        VU_LOG_ERROR("Test failed: %s", vu_error_str(err));
        vu_test_engine_destroy(engine);
        return 1;
    }

    VU_LOG_INFO("Test completed");
    vu_test_engine_destroy(engine);
    return 0;
}
