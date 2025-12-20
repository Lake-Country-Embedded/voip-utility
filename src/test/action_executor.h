/*
 * voip-utility - SIP VoIP Testing Utility
 * Action executor
 */

#ifndef VU_ACTION_EXECUTOR_H
#define VU_ACTION_EXECUTOR_H

#include "util/error.h"

typedef struct vu_action_executor vu_action_executor_t;

vu_action_executor_t *vu_action_executor_create(void);
void vu_action_executor_destroy(vu_action_executor_t *exec);

#endif /* VU_ACTION_EXECUTOR_H */
