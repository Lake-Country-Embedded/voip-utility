/*
 * voip-utility - SIP VoIP Testing Utility
 * Configuration implementation
 */

#include "config/config.h"
#include "util/log.h"
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>

/* Helper to safely copy string */
static void safe_strcpy(char *dst, size_t dst_size, const char *src)
{
    if (!src) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

/* Helper to get string from JSON with default */
static const char *json_get_string(const cJSON *obj, const char *key, const char *def)
{
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (cJSON_IsString(item) && item->valuestring) {
        return item->valuestring;
    }
    return def;
}

/* Helper to get number from JSON with default */
static double json_get_number(const cJSON *obj, const char *key, double def)
{
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (cJSON_IsNumber(item)) {
        return item->valuedouble;
    }
    return def;
}

/* Helper to get bool from JSON with default */
static bool json_get_bool(const cJSON *obj, const char *key, bool def)
{
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (cJSON_IsBool(item)) {
        return cJSON_IsTrue(item);
    }
    return def;
}

vu_config_t vu_config_defaults(void)
{
    vu_config_t config = {0};

    /* Audio defaults */
    config.audio.sample_rate = 16000;
    config.audio.frame_duration_ms = 20;
    safe_strcpy(config.audio.default_codec, sizeof(config.audio.default_codec), "PCMU");

    /* Beep detection defaults */
    config.beep.min_level_db = -40.0;
    config.beep.min_duration_sec = 0.05;
    config.beep.max_duration_sec = 5.0;
    config.beep.target_freq_hz = 0;  /* Any frequency */
    config.beep.freq_tolerance_hz = 50.0;
    config.beep.gap_duration_sec = 0.1;

    /* Paths - use current directory by default */
    safe_strcpy(config.recordings_dir, sizeof(config.recordings_dir), ".");
    safe_strcpy(config.tests_dir, sizeof(config.tests_dir), ".");

    /* Logging */
    safe_strcpy(config.log_level, sizeof(config.log_level), "info");
    config.json_output = false;

    return config;
}

const char *vu_transport_name(vu_transport_t transport)
{
    switch (transport) {
    case VU_TRANSPORT_UDP: return "udp";
    case VU_TRANSPORT_TCP: return "tcp";
    case VU_TRANSPORT_TLS: return "tls";
    default: return "udp";
    }
}

vu_transport_t vu_transport_from_string(const char *str)
{
    if (!str) return VU_TRANSPORT_UDP;
    if (strcasecmp(str, "tcp") == 0) return VU_TRANSPORT_TCP;
    if (strcasecmp(str, "tls") == 0) return VU_TRANSPORT_TLS;
    return VU_TRANSPORT_UDP;
}

const char *vu_srtp_mode_name(vu_srtp_mode_t mode)
{
    switch (mode) {
    case VU_SRTP_DISABLED:  return "disabled";
    case VU_SRTP_OPTIONAL:  return "optional";
    case VU_SRTP_MANDATORY: return "mandatory";
    default: return "disabled";
    }
}

vu_srtp_mode_t vu_srtp_mode_from_string(const char *str)
{
    if (!str) return VU_SRTP_DISABLED;
    if (strcasecmp(str, "optional") == 0) return VU_SRTP_OPTIONAL;
    if (strcasecmp(str, "mandatory") == 0 || strcasecmp(str, "required") == 0)
        return VU_SRTP_MANDATORY;
    return VU_SRTP_DISABLED;
}

/* Parse a single account from JSON */
static vu_error_t parse_account(const cJSON *json, vu_account_config_t *account)
{
    memset(account, 0, sizeof(*account));

    /* Required fields */
    const char *id = json_get_string(json, "id", NULL);
    if (!id || !id[0]) {
        VU_SET_ERROR(VU_ERR_CONFIG_INVALID, "Account missing 'id' field");
        return VU_ERR_CONFIG_INVALID;
    }
    safe_strcpy(account->id, sizeof(account->id), id);

    const char *username = json_get_string(json, "username", NULL);
    if (!username || !username[0]) {
        VU_SET_ERROR(VU_ERR_CONFIG_INVALID, "Account '%s' missing 'username'", id);
        return VU_ERR_CONFIG_INVALID;
    }
    safe_strcpy(account->username, sizeof(account->username), username);

    const char *server = json_get_string(json, "server", NULL);
    if (!server || !server[0]) {
        VU_SET_ERROR(VU_ERR_CONFIG_INVALID, "Account '%s' missing 'server'", id);
        return VU_ERR_CONFIG_INVALID;
    }
    safe_strcpy(account->server, sizeof(account->server), server);

    /* Optional fields with defaults */
    safe_strcpy(account->password, sizeof(account->password),
                json_get_string(json, "password", ""));
    safe_strcpy(account->realm, sizeof(account->realm),
                json_get_string(json, "realm", ""));
    safe_strcpy(account->display_name, sizeof(account->display_name),
                json_get_string(json, "display_name", ""));

    account->port = (uint16_t)json_get_number(json, "port", 5060);
    account->transport = vu_transport_from_string(json_get_string(json, "transport", "udp"));
    account->srtp = vu_srtp_mode_from_string(json_get_string(json, "srtp", "disabled"));
    account->reg_timeout_sec = (uint32_t)json_get_number(json, "reg_timeout_sec", 3600);
    account->reg_retry_interval_sec = (uint32_t)json_get_number(json, "reg_retry_interval_sec", 30);
    account->enabled = json_get_bool(json, "enabled", true);

    return VU_OK;
}

/* Read file contents into string */
static char *read_file(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size <= 0 || size > 10 * 1024 * 1024) {  /* Max 10MB */
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

/* Check if file exists */
static bool file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

/* Get home directory */
static const char *get_home_dir(void)
{
    const char *home = getenv("HOME");
    if (home) return home;

    struct passwd *pw = getpwuid(getuid());
    if (pw) return pw->pw_dir;

    return "/tmp";
}

vu_error_t vu_config_load(vu_config_t *config, const char *path)
{
    if (!config) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "config is NULL");
        return VU_ERR_INVALID_ARG;
    }

    /* Start with defaults */
    *config = vu_config_defaults();

    /* Determine config path */
    char config_path[VU_MAX_PATH_LEN];
    bool found = false;

    if (path) {
        safe_strcpy(config_path, sizeof(config_path), path);
        if (file_exists(config_path)) {
            found = true;
        }
    } else {
        /* Try default locations */
        const char *locations[] = {
            "./voip-utility.json",
            NULL,  /* Will be filled with ~/.config/voip-utility/config.json */
            "/etc/voip-utility/config.json"
        };

        char home_config[VU_MAX_PATH_LEN];
        snprintf(home_config, sizeof(home_config), "%s/.config/voip-utility/config.json",
                 get_home_dir());
        locations[1] = home_config;

        for (size_t i = 0; i < sizeof(locations) / sizeof(locations[0]); i++) {
            if (file_exists(locations[i])) {
                safe_strcpy(config_path, sizeof(config_path), locations[i]);
                found = true;
                break;
            }
        }
    }

    if (!found) {
        if (path) {
            VU_SET_ERROR(VU_ERR_CONFIG_NOT_FOUND, "Config file not found: %s", path);
            return VU_ERR_CONFIG_NOT_FOUND;
        }
        /* No config file found, use defaults */
        return VU_OK;
    }

    /* Read and parse config file */
    char *content = read_file(config_path);
    if (!content) {
        VU_SET_ERROR(VU_ERR_IO, "Failed to read config file: %s", config_path);
        return VU_ERR_IO;
    }

    cJSON *root = cJSON_Parse(content);
    free(content);

    if (!root) {
        const char *error_ptr = cJSON_GetErrorPtr();
        VU_SET_ERROR(VU_ERR_CONFIG_PARSE, "JSON parse error near: %.20s",
                     error_ptr ? error_ptr : "unknown");
        return VU_ERR_CONFIG_PARSE;
    }

    /* Parse accounts array */
    cJSON *accounts = cJSON_GetObjectItem(root, "accounts");
    if (cJSON_IsArray(accounts)) {
        cJSON *account_json;
        cJSON_ArrayForEach(account_json, accounts) {
            if (config->account_count >= VU_MAX_ACCOUNTS) {
                VU_LOG_WARN("Max accounts (%d) reached, ignoring remaining", VU_MAX_ACCOUNTS);
                break;
            }

            vu_error_t err = parse_account(account_json, &config->accounts[config->account_count]);
            if (err == VU_OK) {
                config->account_count++;
            } else {
                VU_LOG_WARN("Skipping invalid account: %s", vu_get_last_error()->message);
            }
        }
    }

    /* Parse audio settings */
    cJSON *audio = cJSON_GetObjectItem(root, "audio");
    if (cJSON_IsObject(audio)) {
        config->audio.sample_rate = (uint32_t)json_get_number(audio, "sample_rate",
                                                               config->audio.sample_rate);
        config->audio.frame_duration_ms = (uint32_t)json_get_number(audio, "frame_duration_ms",
                                                                     config->audio.frame_duration_ms);
        safe_strcpy(config->audio.default_codec, sizeof(config->audio.default_codec),
                    json_get_string(audio, "default_codec", config->audio.default_codec));
    }

    /* Parse beep detection settings */
    cJSON *beep = cJSON_GetObjectItem(root, "beep_detection");
    if (cJSON_IsObject(beep)) {
        config->beep.min_level_db = json_get_number(beep, "min_level_db", config->beep.min_level_db);
        config->beep.min_duration_sec = json_get_number(beep, "min_duration_sec", config->beep.min_duration_sec);
        config->beep.max_duration_sec = json_get_number(beep, "max_duration_sec", config->beep.max_duration_sec);
        config->beep.target_freq_hz = json_get_number(beep, "target_freq_hz", config->beep.target_freq_hz);
        config->beep.freq_tolerance_hz = json_get_number(beep, "freq_tolerance_hz", config->beep.freq_tolerance_hz);
        config->beep.gap_duration_sec = json_get_number(beep, "gap_duration_sec", config->beep.gap_duration_sec);
    }

    /* Parse paths */
    safe_strcpy(config->recordings_dir, sizeof(config->recordings_dir),
                json_get_string(root, "recordings_dir", config->recordings_dir));
    safe_strcpy(config->tests_dir, sizeof(config->tests_dir),
                json_get_string(root, "tests_dir", config->tests_dir));

    /* Parse logging */
    safe_strcpy(config->log_level, sizeof(config->log_level),
                json_get_string(root, "log_level", config->log_level));
    config->json_output = json_get_bool(root, "json_output", config->json_output);

    cJSON_Delete(root);

    VU_LOG_DEBUG("Loaded config from %s with %d accounts", config_path, config->account_count);
    return VU_OK;
}

vu_error_t vu_config_save(const vu_config_t *config, const char *path)
{
    if (!config || !path) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "config or path is NULL");
        return VU_ERR_INVALID_ARG;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        VU_SET_ERROR(VU_ERR_NO_MEMORY, "Failed to create JSON object");
        return VU_ERR_NO_MEMORY;
    }

    /* Add accounts array */
    cJSON *accounts = cJSON_AddArrayToObject(root, "accounts");
    for (int i = 0; i < config->account_count; i++) {
        const vu_account_config_t *acc = &config->accounts[i];
        cJSON *account = cJSON_CreateObject();

        cJSON_AddStringToObject(account, "id", acc->id);
        cJSON_AddStringToObject(account, "username", acc->username);
        cJSON_AddStringToObject(account, "password", acc->password);
        cJSON_AddStringToObject(account, "server", acc->server);
        cJSON_AddNumberToObject(account, "port", acc->port);
        cJSON_AddStringToObject(account, "realm", acc->realm);
        cJSON_AddStringToObject(account, "display_name", acc->display_name);
        cJSON_AddStringToObject(account, "transport", vu_transport_name(acc->transport));
        cJSON_AddStringToObject(account, "srtp", vu_srtp_mode_name(acc->srtp));
        cJSON_AddNumberToObject(account, "reg_timeout_sec", acc->reg_timeout_sec);
        cJSON_AddNumberToObject(account, "reg_retry_interval_sec", acc->reg_retry_interval_sec);
        cJSON_AddBoolToObject(account, "enabled", acc->enabled);

        cJSON_AddItemToArray(accounts, account);
    }

    /* Add audio settings */
    cJSON *audio = cJSON_AddObjectToObject(root, "audio");
    cJSON_AddNumberToObject(audio, "sample_rate", config->audio.sample_rate);
    cJSON_AddNumberToObject(audio, "frame_duration_ms", config->audio.frame_duration_ms);
    cJSON_AddStringToObject(audio, "default_codec", config->audio.default_codec);

    /* Add beep detection settings */
    cJSON *beep = cJSON_AddObjectToObject(root, "beep_detection");
    cJSON_AddNumberToObject(beep, "min_level_db", config->beep.min_level_db);
    cJSON_AddNumberToObject(beep, "min_duration_sec", config->beep.min_duration_sec);
    cJSON_AddNumberToObject(beep, "max_duration_sec", config->beep.max_duration_sec);
    cJSON_AddNumberToObject(beep, "target_freq_hz", config->beep.target_freq_hz);
    cJSON_AddNumberToObject(beep, "freq_tolerance_hz", config->beep.freq_tolerance_hz);
    cJSON_AddNumberToObject(beep, "gap_duration_sec", config->beep.gap_duration_sec);

    /* Add paths */
    cJSON_AddStringToObject(root, "recordings_dir", config->recordings_dir);
    cJSON_AddStringToObject(root, "tests_dir", config->tests_dir);

    /* Add logging */
    cJSON_AddStringToObject(root, "log_level", config->log_level);
    cJSON_AddBoolToObject(root, "json_output", config->json_output);

    /* Write to file */
    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);

    if (!json_str) {
        VU_SET_ERROR(VU_ERR_NO_MEMORY, "Failed to serialize config");
        return VU_ERR_NO_MEMORY;
    }

    FILE *fp = fopen(path, "w");
    if (!fp) {
        free(json_str);
        VU_SET_ERROR(VU_ERR_IO, "Failed to open config file for writing: %s", path);
        return VU_ERR_IO;
    }

    fprintf(fp, "%s\n", json_str);
    fclose(fp);
    free(json_str);

    return VU_OK;
}

vu_account_config_t *vu_config_find_account(vu_config_t *config, const char *id)
{
    if (!config || !id) return NULL;

    for (int i = 0; i < config->account_count; i++) {
        if (strcmp(config->accounts[i].id, id) == 0) {
            return &config->accounts[i];
        }
    }
    return NULL;
}

const vu_account_config_t *vu_config_find_account_const(const vu_config_t *config, const char *id)
{
    return vu_config_find_account((vu_config_t *)config, id);
}

vu_error_t vu_config_add_account(vu_config_t *config, const vu_account_config_t *account)
{
    if (!config || !account) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "config or account is NULL");
        return VU_ERR_INVALID_ARG;
    }

    if (vu_config_find_account(config, account->id)) {
        VU_SET_ERROR(VU_ERR_ALREADY_EXISTS, "Account '%s' already exists", account->id);
        return VU_ERR_ALREADY_EXISTS;
    }

    if (config->account_count >= VU_MAX_ACCOUNTS) {
        VU_SET_ERROR(VU_ERR_NO_MEMORY, "Max accounts (%d) reached", VU_MAX_ACCOUNTS);
        return VU_ERR_NO_MEMORY;
    }

    config->accounts[config->account_count++] = *account;
    return VU_OK;
}

vu_error_t vu_config_remove_account(vu_config_t *config, const char *id)
{
    if (!config || !id) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "config or id is NULL");
        return VU_ERR_INVALID_ARG;
    }

    for (int i = 0; i < config->account_count; i++) {
        if (strcmp(config->accounts[i].id, id) == 0) {
            /* Shift remaining accounts */
            for (int j = i; j < config->account_count - 1; j++) {
                config->accounts[j] = config->accounts[j + 1];
            }
            config->account_count--;
            return VU_OK;
        }
    }

    VU_SET_ERROR(VU_ERR_NOT_FOUND, "Account '%s' not found", id);
    return VU_ERR_NOT_FOUND;
}
