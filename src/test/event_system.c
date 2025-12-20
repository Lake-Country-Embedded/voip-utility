/*
 * voip-utility - SIP VoIP Testing Utility
 * Event system (stub implementation)
 */

#include "test/event_system.h"
#include <stdlib.h>

struct vu_event_system {
    int placeholder;
};

vu_event_system_t *vu_event_system_create(void)
{
    return calloc(1, sizeof(vu_event_system_t));
}

void vu_event_system_destroy(vu_event_system_t *es)
{
    free(es);
}
