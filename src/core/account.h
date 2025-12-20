/*
 * voip-utility - SIP VoIP Testing Utility
 * SIP Account management
 */

#ifndef VU_ACCOUNT_H
#define VU_ACCOUNT_H

#include "util/error.h"
#include "config/config.h"
#include <stdbool.h>
#include <pjsua-lib/pjsua.h>

/* Account registration state */
typedef enum {
    VU_ACCOUNT_STATE_UNREGISTERED = 0,
    VU_ACCOUNT_STATE_REGISTERING,
    VU_ACCOUNT_STATE_REGISTERED,
    VU_ACCOUNT_STATE_REGISTRATION_FAILED,
    VU_ACCOUNT_STATE_UNREGISTERING
} vu_account_state_t;

/* Account runtime info */
typedef struct vu_account {
    vu_account_config_t config;
    pjsua_acc_id pjsua_id;          /* PJSUA account handle (-1 = not added) */
    vu_account_state_t state;
    int last_status_code;
    char last_status_text[256];
    uint64_t registration_time_ms;   /* When registration completed */
} vu_account_t;

/* Account manager */
typedef struct vu_account_manager {
    vu_account_t accounts[VU_MAX_ACCOUNTS];
    int account_count;
} vu_account_manager_t;

/*
 * Initialize account manager with accounts from config
 */
vu_error_t vu_account_manager_init(vu_account_manager_t *mgr, const vu_config_t *config);

/*
 * Cleanup account manager
 */
void vu_account_manager_cleanup(vu_account_manager_t *mgr);

/*
 * Add account from config
 */
vu_error_t vu_account_add(vu_account_manager_t *mgr, const vu_account_config_t *config);

/*
 * Remove account by ID
 */
vu_error_t vu_account_remove(vu_account_manager_t *mgr, const char *account_id);

/*
 * Find account by ID
 */
vu_account_t *vu_account_find(vu_account_manager_t *mgr, const char *account_id);

/*
 * Find account by PJSUA account ID
 */
vu_account_t *vu_account_find_by_pjsua_id(vu_account_manager_t *mgr, pjsua_acc_id pjsua_id);

/*
 * Register account with SIP server
 */
vu_error_t vu_account_register(vu_account_t *account);

/*
 * Unregister account from SIP server
 */
vu_error_t vu_account_unregister(vu_account_t *account);

/*
 * Register all enabled accounts
 */
vu_error_t vu_account_register_all(vu_account_manager_t *mgr);

/*
 * Unregister all accounts
 */
void vu_account_unregister_all(vu_account_manager_t *mgr);

/*
 * Wait for registration to complete
 * Returns VU_OK if registered, VU_ERR_TIMEOUT if timeout, or error code.
 */
vu_error_t vu_account_wait_registration(vu_account_t *account, int timeout_sec);

/*
 * Get account state name
 */
const char *vu_account_state_name(vu_account_state_t state);

/*
 * Update account state from PJSUA callback
 */
void vu_account_on_reg_state(vu_account_t *account, int code, const char *reason);

#endif /* VU_ACCOUNT_H */
