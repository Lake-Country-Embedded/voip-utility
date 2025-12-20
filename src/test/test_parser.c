/*
 * voip-utility - SIP VoIP Testing Utility
 * Test definition parser implementation
 */

#include "test/test_parser.h"
#include "util/log.h"
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *vu_action_type_name(vu_action_type_t type)
{
    switch (type) {
    case VU_ACTION_WAIT:         return "wait";
    case VU_ACTION_SEND_DTMF:    return "send_dtmf";
    case VU_ACTION_EXPECT_DTMF:  return "expect_dtmf";
    case VU_ACTION_PLAY_AUDIO:   return "play_audio";
    case VU_ACTION_RECORD_AUDIO: return "record_audio";
    case VU_ACTION_EXPECT_BEEPS: return "expect_beeps";
    case VU_ACTION_HANGUP:       return "hangup";
    default:                     return "unknown";
    }
}

static vu_action_type_t parse_action_type(const char *str)
{
    if (!str) return VU_ACTION_NONE;

    if (strcmp(str, "wait") == 0) return VU_ACTION_WAIT;
    if (strcmp(str, "send_dtmf") == 0) return VU_ACTION_SEND_DTMF;
    if (strcmp(str, "expect_dtmf") == 0) return VU_ACTION_EXPECT_DTMF;
    if (strcmp(str, "play_audio") == 0 || strcmp(str, "play") == 0) return VU_ACTION_PLAY_AUDIO;
    if (strcmp(str, "record_audio") == 0 || strcmp(str, "record") == 0) return VU_ACTION_RECORD_AUDIO;
    if (strcmp(str, "expect_beeps") == 0) return VU_ACTION_EXPECT_BEEPS;
    if (strcmp(str, "hangup") == 0) return VU_ACTION_HANGUP;

    return VU_ACTION_NONE;
}

static void safe_strcpy(char *dst, size_t dst_size, const char *src)
{
    if (!src) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

static const char *json_get_string(const cJSON *obj, const char *key, const char *def)
{
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (cJSON_IsString(item) && item->valuestring) {
        return item->valuestring;
    }
    return def;
}

static double json_get_number(const cJSON *obj, const char *key, double def)
{
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (cJSON_IsNumber(item)) {
        return item->valuedouble;
    }
    return def;
}

static bool json_get_bool(const cJSON *obj, const char *key, bool def)
{
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (cJSON_IsBool(item)) {
        return cJSON_IsTrue(item);
    }
    return def;
}

static int parse_action(const cJSON *json, vu_action_t *action)
{
    if (!cJSON_IsObject(json)) return -1;

    memset(action, 0, sizeof(*action));

    const char *type_str = json_get_string(json, "action", NULL);
    if (!type_str) {
        VU_LOG_WARN("Action missing 'action' field");
        return -1;
    }

    action->type = parse_action_type(type_str);
    if (action->type == VU_ACTION_NONE) {
        VU_LOG_WARN("Unknown action type: %s", type_str);
        return -1;
    }

    /* Parse action-specific fields */
    switch (action->type) {
    case VU_ACTION_WAIT:
        action->float_value = json_get_number(json, "seconds", 1.0);
        break;

    case VU_ACTION_SEND_DTMF:
        safe_strcpy(action->value, sizeof(action->value),
                   json_get_string(json, "digits", ""));
        action->int_value = (int)json_get_number(json, "timeout", 5);
        break;

    case VU_ACTION_EXPECT_DTMF:
        safe_strcpy(action->value, sizeof(action->value),
                   json_get_string(json, "pattern", ""));
        action->int_value = (int)json_get_number(json, "timeout", 10);
        break;

    case VU_ACTION_PLAY_AUDIO:
        safe_strcpy(action->value, sizeof(action->value),
                   json_get_string(json, "file", ""));
        action->int_value = json_get_bool(json, "loop", false) ? 1 : 0;
        break;

    case VU_ACTION_RECORD_AUDIO:
        safe_strcpy(action->value, sizeof(action->value),
                   json_get_string(json, "file", ""));
        break;

    case VU_ACTION_EXPECT_BEEPS:
        action->int_value = (int)json_get_number(json, "count", 1);
        action->float_value = json_get_number(json, "frequency", 0);
        break;

    case VU_ACTION_HANGUP:
        action->int_value = (int)json_get_number(json, "code", 200);
        break;

    default:
        break;
    }

    return 0;
}

static int parse_role(const cJSON *json, vu_role_config_t *role)
{
    if (!cJSON_IsObject(json)) return -1;

    memset(role, 0, sizeof(*role));

    safe_strcpy(role->account_id, sizeof(role->account_id),
               json_get_string(json, "account", ""));
    safe_strcpy(role->uri, sizeof(role->uri),
               json_get_string(json, "uri", ""));
    role->auto_answer = json_get_bool(json, "auto_answer", false);
    role->timeout_sec = (int)json_get_number(json, "timeout", 30);

    /* Parse actions array */
    cJSON *actions = cJSON_GetObjectItem(json, "actions");
    if (cJSON_IsArray(actions)) {
        cJSON *action_json;
        cJSON_ArrayForEach(action_json, actions) {
            if (role->action_count >= VU_MAX_TEST_ACTIONS) {
                VU_LOG_WARN("Maximum actions (%d) reached", VU_MAX_TEST_ACTIONS);
                break;
            }

            if (parse_action(action_json, &role->actions[role->action_count]) == 0) {
                role->action_count++;
            }
        }
    }

    return 0;
}

static char *read_file(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size <= 0 || size > 1024 * 1024) {  /* Max 1MB */
        fclose(fp);
        return NULL;
    }

    char *content = malloc(size + 1);
    if (!content) {
        fclose(fp);
        return NULL;
    }

    size_t read_size = fread(content, 1, size, fp);
    fclose(fp);

    content[read_size] = '\0';
    return content;
}

vu_test_definition_t *vu_test_parse_file(const char *path)
{
    if (!path) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "Path is NULL");
        return NULL;
    }

    VU_LOG_INFO("Parsing test file: %s", path);

    char *content = read_file(path);
    if (!content) {
        VU_SET_ERROR(VU_ERR_IO, "Failed to read test file: %s", path);
        return NULL;
    }

    cJSON *root = cJSON_Parse(content);
    free(content);

    if (!root) {
        const char *error_ptr = cJSON_GetErrorPtr();
        VU_SET_ERROR(VU_ERR_CONFIG_PARSE, "JSON parse error near: %.30s",
                    error_ptr ? error_ptr : "unknown");
        return NULL;
    }

    vu_test_definition_t *def = calloc(1, sizeof(vu_test_definition_t));
    if (!def) {
        cJSON_Delete(root);
        VU_SET_ERROR(VU_ERR_NO_MEMORY, "Failed to allocate test definition");
        return NULL;
    }

    /* Parse metadata */
    safe_strcpy(def->name, sizeof(def->name),
               json_get_string(root, "name", "Unnamed Test"));
    safe_strcpy(def->description, sizeof(def->description),
               json_get_string(root, "description", ""));
    def->timeout_sec = (int)json_get_number(root, "timeout", 60);

    /* Parse caller */
    cJSON *caller = cJSON_GetObjectItem(root, "caller");
    if (caller) {
        if (parse_role(caller, &def->caller) != 0) {
            VU_LOG_WARN("Failed to parse caller configuration");
        }
    }

    /* Parse receiver */
    cJSON *receiver = cJSON_GetObjectItem(root, "receiver");
    if (receiver) {
        if (parse_role(receiver, &def->receiver) != 0) {
            VU_LOG_WARN("Failed to parse receiver configuration");
        }
    }

    /* Parse expected results */
    cJSON *expect = cJSON_GetObjectItem(root, "expect");
    if (cJSON_IsObject(expect)) {
        def->expect_connected = json_get_bool(expect, "connected", true);
        def->expect_beep_count = (int)json_get_number(expect, "beep_count", 0);
        def->expect_beep_freq_hz = json_get_number(expect, "beep_frequency", 0);
    } else {
        def->expect_connected = true;
    }

    cJSON_Delete(root);

    VU_LOG_INFO("Loaded test: %s", def->name);
    VU_LOG_DEBUG("  Caller: account=%s, uri=%s, actions=%d",
                def->caller.account_id, def->caller.uri, def->caller.action_count);
    VU_LOG_DEBUG("  Receiver: account=%s, auto_answer=%d, actions=%d",
                def->receiver.account_id, def->receiver.auto_answer, def->receiver.action_count);

    return def;
}

void vu_test_definition_free(vu_test_definition_t *def)
{
    free(def);
}
