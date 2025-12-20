/*
 * voip-utility - SIP VoIP Testing Utility
 * Time utilities
 */

#ifndef VU_TIME_UTIL_H
#define VU_TIME_UTIL_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Get current time in milliseconds since epoch
 */
uint64_t vu_time_now_ms(void);

/*
 * Get current time in seconds (double precision)
 */
double vu_time_now_sec(void);

/*
 * Get monotonic time in milliseconds (for measuring durations)
 */
uint64_t vu_time_monotonic_ms(void);

/*
 * Get monotonic time in seconds (double precision)
 */
double vu_time_monotonic_sec(void);

/*
 * Sleep for specified milliseconds
 */
void vu_sleep_ms(uint64_t ms);

/*
 * Format timestamp for display (thread-safe)
 * Returns pointer to static buffer - copy if needed
 */
const char *vu_time_format(uint64_t timestamp_ms);

/*
 * Format duration in human-readable form (e.g., "1.234s", "45.6ms")
 * Returns pointer to static buffer - copy if needed
 */
const char *vu_duration_format(double seconds);

/*
 * Timer structure for measuring elapsed time
 */
typedef struct vu_timer {
    uint64_t start_ms;
    uint64_t timeout_ms;
} vu_timer_t;

/*
 * Start a timer with optional timeout (0 = no timeout)
 */
void vu_timer_start(vu_timer_t *timer, uint64_t timeout_ms);

/*
 * Get elapsed time since timer start in milliseconds
 */
uint64_t vu_timer_elapsed_ms(const vu_timer_t *timer);

/*
 * Get elapsed time since timer start in seconds
 */
double vu_timer_elapsed_sec(const vu_timer_t *timer);

/*
 * Check if timer has expired (if timeout was set)
 */
bool vu_timer_expired(const vu_timer_t *timer);

/*
 * Get remaining time until timeout (0 if expired or no timeout)
 */
uint64_t vu_timer_remaining_ms(const vu_timer_t *timer);

#endif /* VU_TIME_UTIL_H */
