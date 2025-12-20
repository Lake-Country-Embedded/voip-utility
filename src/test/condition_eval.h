/*
 * voip-utility - SIP VoIP Testing Utility
 * Condition evaluator
 */

#ifndef VU_CONDITION_EVAL_H
#define VU_CONDITION_EVAL_H

#include "util/error.h"
#include <stdbool.h>

typedef struct vu_condition_eval vu_condition_eval_t;

vu_condition_eval_t *vu_condition_eval_create(void);
void vu_condition_eval_destroy(vu_condition_eval_t *eval);

#endif /* VU_CONDITION_EVAL_H */
