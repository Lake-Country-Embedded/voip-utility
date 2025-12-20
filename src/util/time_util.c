/*
 * voip-utility - SIP VoIP Testing Utility
 * Time utilities implementation
 */

#include "util/time_util.h"
#include <time.h>
#include <stdio.h>
#include <string.h>

uint64_t vu_time_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

double vu_time_now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

uint64_t vu_time_monotonic_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

double vu_time_monotonic_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

void vu_sleep_ms(uint64_t ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

const char *vu_time_format(uint64_t timestamp_ms)
{
    static __thread char buffer[32];

    time_t secs = timestamp_ms / 1000;
    unsigned int millis = timestamp_ms % 1000;
    struct tm *tm_info = localtime(&secs);

    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d.%03u",
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday,
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec,
             millis);

    return buffer;
}

const char *vu_duration_format(double seconds)
{
    static __thread char buffer[32];

    if (seconds < 0.001) {
        snprintf(buffer, sizeof(buffer), "%.1fus", seconds * 1000000);
    } else if (seconds < 1.0) {
        snprintf(buffer, sizeof(buffer), "%.1fms", seconds * 1000);
    } else if (seconds < 60.0) {
        snprintf(buffer, sizeof(buffer), "%.3fs", seconds);
    } else if (seconds < 3600.0) {
        int mins = (int)(seconds / 60);
        double secs = seconds - mins * 60;
        snprintf(buffer, sizeof(buffer), "%dm%.1fs", mins, secs);
    } else {
        int hours = (int)(seconds / 3600);
        int mins = (int)((seconds - hours * 3600) / 60);
        snprintf(buffer, sizeof(buffer), "%dh%dm", hours, mins);
    }

    return buffer;
}

void vu_timer_start(vu_timer_t *timer, uint64_t timeout_ms)
{
    timer->start_ms = vu_time_monotonic_ms();
    timer->timeout_ms = timeout_ms;
}

uint64_t vu_timer_elapsed_ms(const vu_timer_t *timer)
{
    return vu_time_monotonic_ms() - timer->start_ms;
}

double vu_timer_elapsed_sec(const vu_timer_t *timer)
{
    return (double)vu_timer_elapsed_ms(timer) / 1000.0;
}

bool vu_timer_expired(const vu_timer_t *timer)
{
    if (timer->timeout_ms == 0) {
        return false;  /* No timeout set */
    }
    return vu_timer_elapsed_ms(timer) >= timer->timeout_ms;
}

uint64_t vu_timer_remaining_ms(const vu_timer_t *timer)
{
    if (timer->timeout_ms == 0) {
        return 0;  /* No timeout set */
    }
    uint64_t elapsed = vu_timer_elapsed_ms(timer);
    if (elapsed >= timer->timeout_ms) {
        return 0;
    }
    return timer->timeout_ms - elapsed;
}
