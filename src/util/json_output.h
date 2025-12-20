/*
 * voip-utility - SIP VoIP Testing Utility
 * JSON output formatting for automation
 */

#ifndef VU_JSON_OUTPUT_H
#define VU_JSON_OUTPUT_H

#include <stdio.h>
#include <stdbool.h>
#include <cJSON.h>

/*
 * Initialize JSON output
 * If fp is NULL, uses stdout
 */
void vu_json_output_init(FILE *fp);

/*
 * Set output file for JSON
 */
void vu_json_output_set_file(FILE *fp);

/*
 * Output a JSON object (will be freed after output)
 * Appends newline for JSONL format
 */
void vu_json_output(cJSON *json);

/*
 * Output a JSON object without freeing it
 */
void vu_json_output_keep(const cJSON *json);

/*
 * Flush output
 */
void vu_json_output_flush(void);

/*
 * Helper: Create event JSON object with common fields
 */
cJSON *vu_json_event_create(const char *type);

/*
 * Helper: Add timestamp to JSON object (current time)
 */
void vu_json_add_timestamp(cJSON *json);

/*
 * Helper: Add error information to JSON object
 */
void vu_json_add_error(cJSON *json, int code, const char *message);

/*
 * Event types for common scenarios
 */

/* Registration events */
cJSON *vu_json_event_registering(const char *account_id, const char *server);
cJSON *vu_json_event_registered(const char *account_id, int expires);
cJSON *vu_json_event_registration_failed(const char *account_id, int code, const char *reason);
cJSON *vu_json_event_unregistered(const char *account_id);

/* Call events */
cJSON *vu_json_event_calling(const char *uri, const char *from_account);
cJSON *vu_json_event_call_ringing(int call_id);
cJSON *vu_json_event_call_connected(int call_id, double connect_time_sec);
cJSON *vu_json_event_call_disconnected(int call_id, int code, const char *reason, double duration_sec);
cJSON *vu_json_event_incoming_call(int call_id, const char *from_uri, const char *to_uri);

/* DTMF events */
cJSON *vu_json_event_dtmf_sent(int call_id, const char *digits);
cJSON *vu_json_event_dtmf_received(int call_id, char digit, int duration_ms);

/* Audio events */
cJSON *vu_json_event_beep_detected(int beep_index, double start_time, double duration,
                                    double frequency_hz, double level_db);
cJSON *vu_json_event_audio_started(int call_id, const char *recording_path);
cJSON *vu_json_event_audio_stopped(int call_id, double duration_sec);

/* Test events */
cJSON *vu_json_event_test_started(const char *test_name);
cJSON *vu_json_event_step_started(const char *step_name);
cJSON *vu_json_event_step_completed(const char *step_name, bool passed, const char *reason);
cJSON *vu_json_event_test_completed(const char *test_name, bool passed,
                                     double duration_sec, const char *reason);

#endif /* VU_JSON_OUTPUT_H */
