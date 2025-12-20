/*
 * voip-utility - SIP VoIP Testing Utility
 * Action executor (stub implementation)
 */

#include "test/action_executor.h"
#include <stdlib.h>

struct vu_action_executor {
    int placeholder;
};

vu_action_executor_t *vu_action_executor_create(void)
{
    return calloc(1, sizeof(vu_action_executor_t));
}

void vu_action_executor_destroy(vu_action_executor_t *exec)
{
    free(exec);
}
