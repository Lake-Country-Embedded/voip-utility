/*
 * voip-utility - SIP VoIP Testing Utility
 * JSON output formatting implementation
 */

#include "util/json_output.h"
#include "util/time_util.h"
#include <pthread.h>
#include <stdlib.h>

static FILE *g_json_output = NULL;
static pthread_mutex_t g_json_mutex = PTHREAD_MUTEX_INITIALIZER;

void vu_json_output_init(FILE *fp)
{
    pthread_mutex_lock(&g_json_mutex);
    g_json_output = fp ? fp : stdout;
    pthread_mutex_unlock(&g_json_mutex);
}

void vu_json_output_set_file(FILE *fp)
{
    pthread_mutex_lock(&g_json_mutex);
    g_json_output = fp;
    pthread_mutex_unlock(&g_json_mutex);
}

void vu_json_output(cJSON *json)
{
    if (!json) return;

    pthread_mutex_lock(&g_json_mutex);
    FILE *out = g_json_output ? g_json_output : stdout;

    char *str = cJSON_PrintUnformatted(json);
    if (str) {
        fprintf(out, "%s\n", str);
        fflush(out);
        free(str);
    }
    pthread_mutex_unlock(&g_json_mutex);

    cJSON_Delete(json);
}

void vu_json_output_keep(const cJSON *json)
{
    if (!json) return;

    pthread_mutex_lock(&g_json_mutex);
    FILE *out = g_json_output ? g_json_output : stdout;

    char *str = cJSON_PrintUnformatted(json);
    if (str) {
        fprintf(out, "%s\n", str);
        fflush(out);
        free(str);
    }
    pthread_mutex_unlock(&g_json_mutex);
}

void vu_json_output_flush(void)
{
    pthread_mutex_lock(&g_json_mutex);
    if (g_json_output) {
        fflush(g_json_output);
    }
    pthread_mutex_unlock(&g_json_mutex);
}

cJSON *vu_json_event_create(const char *type)
{
    cJSON *json = cJSON_CreateObject();
    if (!json) return NULL;

    cJSON_AddStringToObject(json, "type", type);
    vu_json_add_timestamp(json);

    return json;
}

void vu_json_add_timestamp(cJSON *json)
{
    if (!json) return;

    uint64_t now_ms = vu_time_now_ms();
    double now_sec = (double)now_ms / 1000.0;

    cJSON_AddNumberToObject(json, "timestamp", now_sec);
    cJSON_AddStringToObject(json, "timestamp_str", vu_time_format(now_ms));
}

void vu_json_add_error(cJSON *json, int code, const char *message)
{
    if (!json) return;

    cJSON *error = cJSON_CreateObject();
    cJSON_AddNumberToObject(error, "code", code);
    if (message) {
        cJSON_AddStringToObject(error, "message", message);
    }
    cJSON_AddItemToObject(json, "error", error);
}

/* Registration events */

cJSON *vu_json_event_registering(const char *account_id, const char *server)
{
    cJSON *json = vu_json_event_create("registering");
    cJSON_AddStringToObject(json, "account", account_id);
    cJSON_AddStringToObject(json, "server", server);
    return json;
}

cJSON *vu_json_event_registered(const char *account_id, int expires)
{
    cJSON *json = vu_json_event_create("registered");
    cJSON_AddStringToObject(json, "account", account_id);
    cJSON_AddNumberToObject(json, "expires", expires);
    return json;
}

cJSON *vu_json_event_registration_failed(const char *account_id, int code, const char *reason)
{
    cJSON *json = vu_json_event_create("registration_failed");
    cJSON_AddStringToObject(json, "account", account_id);
    cJSON_AddNumberToObject(json, "code", code);
    if (reason) {
        cJSON_AddStringToObject(json, "reason", reason);
    }
    return json;
}

cJSON *vu_json_event_unregistered(const char *account_id)
{
    cJSON *json = vu_json_event_create("unregistered");
    cJSON_AddStringToObject(json, "account", account_id);
    return json;
}

/* Call events */

cJSON *vu_json_event_calling(const char *uri, const char *from_account)
{
    cJSON *json = vu_json_event_create("calling");
    cJSON_AddStringToObject(json, "uri", uri);
    cJSON_AddStringToObject(json, "from_account", from_account);
    return json;
}

cJSON *vu_json_event_call_ringing(int call_id)
{
    cJSON *json = vu_json_event_create("call_ringing");
    cJSON_AddNumberToObject(json, "call_id", call_id);
    return json;
}

cJSON *vu_json_event_call_connected(int call_id, double connect_time_sec)
{
    cJSON *json = vu_json_event_create("call_connected");
    cJSON_AddNumberToObject(json, "call_id", call_id);
    cJSON_AddNumberToObject(json, "connect_time_sec", connect_time_sec);
    return json;
}

cJSON *vu_json_event_call_disconnected(int call_id, int code, const char *reason, double duration_sec)
{
    cJSON *json = vu_json_event_create("call_disconnected");
    cJSON_AddNumberToObject(json, "call_id", call_id);
    cJSON_AddNumberToObject(json, "code", code);
    if (reason) {
        cJSON_AddStringToObject(json, "reason", reason);
    }
    cJSON_AddNumberToObject(json, "duration_sec", duration_sec);
    return json;
}

cJSON *vu_json_event_incoming_call(int call_id, const char *from_uri, const char *to_uri)
{
    cJSON *json = vu_json_event_create("incoming_call");
    cJSON_AddNumberToObject(json, "call_id", call_id);
    cJSON_AddStringToObject(json, "from", from_uri);
    cJSON_AddStringToObject(json, "to", to_uri);
    return json;
}

/* DTMF events */

cJSON *vu_json_event_dtmf_sent(int call_id, const char *digits)
{
    cJSON *json = vu_json_event_create("dtmf_sent");
    cJSON_AddNumberToObject(json, "call_id", call_id);
    cJSON_AddStringToObject(json, "digits", digits);
    return json;
}

cJSON *vu_json_event_dtmf_received(int call_id, char digit, int duration_ms)
{
    cJSON *json = vu_json_event_create("dtmf_received");
    cJSON_AddNumberToObject(json, "call_id", call_id);

    char digit_str[2] = {digit, '\0'};
    cJSON_AddStringToObject(json, "digit", digit_str);
    cJSON_AddNumberToObject(json, "duration_ms", duration_ms);

    return json;
}

/* Audio events */

cJSON *vu_json_event_beep_detected(int beep_index, double start_time, double duration,
                                    double frequency_hz, double level_db)
{
    cJSON *json = vu_json_event_create("beep_detected");
    cJSON_AddNumberToObject(json, "beep_index", beep_index);
    cJSON_AddNumberToObject(json, "start_time_sec", start_time);
    cJSON_AddNumberToObject(json, "duration_sec", duration);
    cJSON_AddNumberToObject(json, "frequency_hz", frequency_hz);
    cJSON_AddNumberToObject(json, "level_db", level_db);
    return json;
}

cJSON *vu_json_event_audio_started(int call_id, const char *recording_path)
{
    cJSON *json = vu_json_event_create("audio_started");
    cJSON_AddNumberToObject(json, "call_id", call_id);
    if (recording_path) {
        cJSON_AddStringToObject(json, "recording_path", recording_path);
    }
    return json;
}

cJSON *vu_json_event_audio_stopped(int call_id, double duration_sec)
{
    cJSON *json = vu_json_event_create("audio_stopped");
    cJSON_AddNumberToObject(json, "call_id", call_id);
    cJSON_AddNumberToObject(json, "duration_sec", duration_sec);
    return json;
}

/* Test events */

cJSON *vu_json_event_test_started(const char *test_name)
{
    cJSON *json = vu_json_event_create("test_started");
    cJSON_AddStringToObject(json, "test", test_name);
    return json;
}

cJSON *vu_json_event_step_started(const char *step_name)
{
    cJSON *json = vu_json_event_create("step_started");
    cJSON_AddStringToObject(json, "step", step_name);
    return json;
}

cJSON *vu_json_event_step_completed(const char *step_name, bool passed, const char *reason)
{
    cJSON *json = vu_json_event_create("step_completed");
    cJSON_AddStringToObject(json, "step", step_name);
    cJSON_AddBoolToObject(json, "passed", passed);
    if (reason) {
        cJSON_AddStringToObject(json, "reason", reason);
    }
    return json;
}

cJSON *vu_json_event_test_completed(const char *test_name, bool passed,
                                     double duration_sec, const char *reason)
{
    cJSON *json = vu_json_event_create("test_completed");
    cJSON_AddStringToObject(json, "test", test_name);
    cJSON_AddBoolToObject(json, "passed", passed);
    cJSON_AddNumberToObject(json, "duration_sec", duration_sec);
    if (reason) {
        cJSON_AddStringToObject(json, "reason", reason);
    }
    return json;
}
