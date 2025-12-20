/*
 * voip-utility - SIP VoIP Testing Utility
 * Condition evaluator (stub implementation)
 */

#include "test/condition_eval.h"
#include <stdlib.h>

struct vu_condition_eval {
    int placeholder;
};

vu_condition_eval_t *vu_condition_eval_create(void)
{
    return calloc(1, sizeof(vu_condition_eval_t));
}

void vu_condition_eval_destroy(vu_condition_eval_t *eval)
{
    free(eval);
}
