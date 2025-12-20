/*
 * voip-utility - SIP VoIP Testing Utility
 * Event system
 */

#ifndef VU_EVENT_SYSTEM_H
#define VU_EVENT_SYSTEM_H

#include "util/error.h"

typedef struct vu_event_system vu_event_system_t;

vu_event_system_t *vu_event_system_create(void);
void vu_event_system_destroy(vu_event_system_t *es);

#endif /* VU_EVENT_SYSTEM_H */
