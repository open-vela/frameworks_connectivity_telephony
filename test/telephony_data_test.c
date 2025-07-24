#include "telephony_data_test.h"
#include "remote_operation.h"
#include "telephony_common_test.h"
#include "telephony_sim_test.h"

extern struct judge_type judge_data;
#ifndef CONFIG_TELEPHONY_DFX
extern struct dfx_judge_data dfx_data;
#endif
static struct
{
    int data_enabled_watch_id;
    int data_connection_state_change_watch_id;
    int data_type_watch_id;
    int data_registration_watch_id;
    int data_registration_state;
    int network_type;
    int data_on;
    int connection_state;
    int data_conn_count;
    int global_dc_count;
} global_data;

static void data_event_response(tapi_async_result* result);
static void data_signal_change(tapi_async_result* result);

int setup_data(void** state)
{
    (void)state;
    return data_listen_data_test(0);
}

int setup_data_enable(void** state)
{
    (void)state;
    int ret = 0;
    bool enable = false;

    if (data_listen_data_test(0)) {
        syslog(LOG_ERR, "listen data fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(3);
    if (data_get_enabled_test(&enable)) {
        syslog(LOG_ERR, "get data enabled fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(3);
    if (!enable) {
        if (data_enable_data_test(1)) {
            syslog(LOG_ERR, "enable data fail in %s", __func__);
            ret = -1;
            goto on_exit;
        }
    }
    sleep(3);

on_exit:
    return ret;
}

int teardown_data(void** state)
{
    (void)state;
    int ret = 0;
    if (data_reset_apn_contexts_test(0)) {
        syslog(LOG_ERR, "reset apn contexts fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(3);
    if (data_unlisten_data_test()) {
        syslog(LOG_ERR, "unlisten data fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    return ret;
}

int teardown_data_enable(void** state)
{
    (void)state;
    int ret = 0;
    bool enable = true;

    if (data_get_enabled_test(&enable)) {
        syslog(LOG_ERR, "get data enabled fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(3);
    if (enable) {
        if (data_enable_data_test(0)) {
            syslog(LOG_ERR, "enable data fail in %s", __func__);
            ret = -1;
            goto on_exit;
        }
    }

    if (data_unlisten_data_test()) {
        syslog(LOG_ERR, "unlisten data fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    return ret;
}

int data_listen_data_test(int slot_id)
{
    global_data.data_enabled_watch_id = -1;
    global_data.data_enabled_watch_id = tapi_data_register(
        get_tapi_ctx(), slot_id, MSG_DATA_ENABLED_CHANGE_IND, NULL, data_signal_change);

    if (global_data.data_enabled_watch_id < 0) {
        syslog(LOG_ERR, "%s, slot_id: %d, target_state: %d, watch_id: %d",
            __func__, slot_id, MSG_DATA_ENABLED_CHANGE_IND,
            global_data.data_enabled_watch_id);
        return -1;
    }

    global_data.data_registration_watch_id = -1;
    global_data.data_registration_watch_id = tapi_data_register(
        get_tapi_ctx(), 0, MSG_DATA_REGISTRATION_STATE_CHANGE_IND, NULL, data_signal_change);
    if (global_data.data_registration_watch_id < 0) {
        syslog(LOG_ERR, "%s, slot_id: %d, target_state: %d, watch_id: %d",
            __func__, 0, MSG_DATA_NETWORK_TYPE_CHANGE_IND,
            global_data.data_registration_watch_id);
        return -1;
    }

    global_data.data_type_watch_id = -1;
    global_data.data_type_watch_id = tapi_data_register(
        get_tapi_ctx(), slot_id, MSG_DATA_NETWORK_TYPE_CHANGE_IND, NULL, data_signal_change);
    if (global_data.data_type_watch_id < 0) {
        syslog(LOG_ERR, "%s, slot_id: %d, target_state: %d, watch_id: %d",
            __func__, slot_id, MSG_DATA_NETWORK_TYPE_CHANGE_IND,
            global_data.data_type_watch_id);
        return -1;
    }

    global_data.data_connection_state_change_watch_id = -1;
    global_data.data_connection_state_change_watch_id = tapi_data_register(
        get_tapi_ctx(), slot_id, MSG_DATA_CONNECTION_STATE_CHANGE_IND, NULL, data_signal_change);
    if (global_data.data_connection_state_change_watch_id < 0) {
        syslog(LOG_DEBUG, "%s, slot_id: %d, target_state: %d, watch_id: %d",
            __func__, slot_id, MSG_DATA_CONNECTION_STATE_CHANGE_IND,
            global_data.data_connection_state_change_watch_id);
        return -1;
    }

    return 0;
}

int data_unlisten_data_test(void)
{
    int ret = -1, res = 0;
    syslog(LOG_INFO, "data enabled watch id: %d, data connection state change watch id: %d",
        global_data.data_enabled_watch_id,
        global_data.data_connection_state_change_watch_id);
    ret = tapi_data_unregister(get_tapi_ctx(), global_data.data_enabled_watch_id);
    if (ret) {
        syslog(LOG_ERR, "unregister data enable change fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    ret = tapi_data_unregister(get_tapi_ctx(), global_data.data_connection_state_change_watch_id);
    if (ret) {
        syslog(LOG_ERR, "unregister data connection state change fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    ret = tapi_data_unregister(get_tapi_ctx(), global_data.data_type_watch_id);
    if (ret) {
        syslog(LOG_ERR, "unregister data type change fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    ret = tapi_data_unregister(get_tapi_ctx(), global_data.data_registration_watch_id);
    if (ret) {
        syslog(LOG_ERR, "unregister data registration state change fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int create_apn_context(tapi_data_context** apn_context, char* id, char* type, char* name, char* apn, char* proto, char* auth)
{
    *apn_context = malloc(sizeof(tapi_data_context));
    if (*apn_context == NULL) {
        return -EINVAL;
    }

    if (id != NULL) {
        (*apn_context)->id = id;
    }

    (*apn_context)->type = atoi(type);
    (*apn_context)->protocol = atoi(proto);
    (*apn_context)->auth_method = atoi(auth);

    if (strlen(name) <= MAX_APN_DOMAIN_LENGTH)
        strcpy((*apn_context)->name, name);

    if (strlen(apn) <= MAX_APN_DOMAIN_LENGTH)
        strcpy((*apn_context)->accesspointname, apn);

    strcpy((*apn_context)->username, "");
    strcpy((*apn_context)->password, "");

    return 0;
}

int data_load_apn_contexts_test(int slot_id)
{
    int ret = -1;
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_APN_LOADED_DONE;

    ret = tapi_data_load_apn_contexts(get_tapi_ctx(), slot_id, EVENT_APN_LOADED_DONE, data_event_response);

    if (ret) {
        syslog(LOG_ERR, "tapi_data_load_apn_contexts_test execute fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "data_event_response called by %s is not execute", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int data_network_type_changed_while_change_rat_test(int slot_id, int expect_rat, int expect_type)
{
    int ret = -1;
    int res = 0;

    judge_data_init();
    global_data.network_type = -1;
    judge_data.expect = MSG_DATA_NETWORK_TYPE_CHANGE_IND;
    ret = tapi_set_pref_net_mode(get_tapi_ctx(), slot_id, EVENT_RAT_MODE_SET_DONE, (tapi_pref_net_mode)expect_rat, NULL);
    if (ret) {
        syslog(LOG_ERR, "tapi_set_pref_net_mode execute fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    sleep(5);
    if (judge()) {
        syslog(LOG_DEBUG, "data_event_response is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (global_data.network_type != expect_type) {
        syslog(LOG_ERR, "network type is not %d in %s", expect_type, __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int data_registration_changed_while_change_radio_power(int slot_id, int expect_power, int expect_registration_state)
{
    int ret = -1;
    int res = 0;

    judge_data_init();
    judge_data.expect = MSG_DATA_REGISTRATION_STATE_CHANGE_IND;
    global_data.data_registration_state = -1;
    ret = tapi_set_radio_power(get_tapi_ctx(), slot_id,
        EVENT_RADIO_STATE_SET_DONE, (bool)expect_power, NULL);
    if (ret) {
        syslog(LOG_DEBUG, "tapi_set_radio_power execute fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "data_event_response is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (global_data.data_registration_state != expect_registration_state) {
        syslog(LOG_ERR, "registration state (%d) is error in %s",
            global_data.data_registration_state, __func__);
        res = -1;
    }

on_exit:
    return res;
}

int data_save_apn_context_test(char* slot_id, char* type, char* name, char* apn, char* proto, char* auth)
{
    int ret = -1;
    int res = 0;
    tapi_data_context* apn_context;
    judge_data_init();
    judge_data.expect = EVENT_APN_ADD_DONE;

    if (create_apn_context(&apn_context, NULL, type, name, apn, proto, auth)) {
        syslog(LOG_ERR, "create_apn_context fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    ret = tapi_data_add_apn_context(get_tapi_ctx(), atoi(slot_id), EVENT_APN_ADD_DONE, apn_context, data_event_response);
    free(apn_context);
    if (ret) {
        syslog(LOG_ERR, "tapi_data_add_apn_context execute fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "data_event_response is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int data_edit_apn_context_test(char* slot_id, char* id, char* type, char* name, char* apn, char* proto, char* auth)
{
    int ret = -1, res = 0;
    tapi_data_context* apn_context;
    judge_data_init();
    judge_data.expect = EVENT_APN_EDIT_DONE;

    if (create_apn_context(&apn_context, id, type, name, apn, proto, auth)) {
        syslog(LOG_ERR, "create_apn_context fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    ret = tapi_data_edit_apn_context(get_tapi_ctx(), atoi(slot_id), EVENT_APN_EDIT_DONE, apn_context, data_event_response);
    free(apn_context);
    if (ret) {
        syslog(LOG_ERR, "tapi_data_add_apn_context execute fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "data_event_response is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int data_remove_apn_context_test(char* slot_id, char* id)
{
    int ret = -1;
    int res = 0;
    tapi_data_context* apn;
    judge_data_init();
    judge_data.expect = EVENT_APN_REMOVAL_DONE;

    apn = malloc(sizeof(tapi_data_context));
    if (apn == NULL) {
        syslog(LOG_ERR, "apn is null in %s", __func__);
        return ret;
    }

    apn->id = id;
    ret = tapi_data_remove_apn_context(get_tapi_ctx(), atoi(slot_id), EVENT_APN_REMOVAL_DONE, apn, data_event_response);
    free(apn);
    if (ret) {
        syslog(LOG_ERR, "tapi_data_remove_apn_context execute fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "data_event_response is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_DEBUG, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int data_reset_apn_contexts_test(char* slot_id)
{
    int ret = -1;
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_APN_RESTORE_DONE;

    ret = tapi_data_reset_apn_contexts(get_tapi_ctx(), atoi(slot_id), EVENT_APN_RESTORE_DONE, data_event_response);
    if (ret) {
        syslog(LOG_ERR, "tapi_data_reset_apn_contexts execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "data_event_response is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int data_request_network_test(int slot_id, char* target_state)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = MSG_DATA_CONNECTION_STATE_CHANGE_IND;
    global_data.connection_state = -100;

    int ret = tapi_data_request_network(get_tapi_ctx(), slot_id, target_state);
    if (ret) {
        syslog(LOG_ERR, "tapi_data_request_network execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "the callback function of request_network is not executed in %s",
            __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (global_data.connection_state != 1) {
        syslog(LOG_ERR, "connection state is error in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int data_release_network_test(int slot_id, char* target_state)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = MSG_DATA_CONNECTION_STATE_CHANGE_IND;
    global_data.connection_state = -100;

    int ret = tapi_data_release_network(get_tapi_ctx(), slot_id, target_state);
    if (ret) {
        syslog(LOG_ERR, "tapi_data_release_network execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "the callback function of release_network is not executed in %s",
            __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (global_data.connection_state != 0) {
        syslog(LOG_ERR, "connection state is error in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int data_enable_data_test(int state)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = MSG_DATA_ENABLED_CHANGE_IND;
    global_data.data_on = -1;

    int ret = tapi_data_enable_data(get_tapi_ctx(), state);
    if (ret) {
        syslog(LOG_ERR, "tapi_data_enable_data execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "the callback function of data_enable is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result in %s is invalid", __func__);
        res = -1;
        goto on_exit;
    }

    if (global_data.data_on != state) {
        syslog(LOG_ERR, "data status is error");
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int data_enable_auto_when_airplane_mode_close_test(void)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = MSG_DATA_CONNECTION_STATE_CHANGE_IND;
    int ret = tapi_set_radio_power(get_tapi_ctx(), 0, EVENT_RADIO_STATE_SET_DONE, false, NULL);
    if (ret) {
        syslog(LOG_ERR, "tapi_set_radio_power(false) execute fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    sleep(10);
    ret = tapi_set_radio_power(get_tapi_ctx(), 0, EVENT_RADIO_STATE_SET_DONE, true, NULL);
    if (ret) {
        syslog(LOG_ERR, "tapi_set_radio_power(true) execute fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "the callback function of data_enable is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result in %s is invalid", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(7);

on_exit:
    return res;
}

// is-data-on
int data_get_enabled_test(bool* result)
{
    int ret = tapi_data_get_enabled(get_tapi_ctx(), result);
    bool flag = *result;
    syslog(LOG_DEBUG, "%s: ret: %d, result: %d", __func__, ret, (int)flag);

    return ret;
}

int data_enabled_test(bool enable)
{
    int ret = 0;
    if (data_get_enabled_test(&enable)) {
        syslog(LOG_ERR, "get data enabled fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(3);
    if (!enable) {
        if (data_enable_data_test(1)) {
            syslog(LOG_ERR, "enable data fail in %s", __func__);
            ret = -1;
            goto on_exit;
        }
    }

on_exit:
    return ret;
}

int data_disabled_test(bool enable)
{
    int ret = 0;
    if (data_get_enabled_test(&enable)) {
        syslog(LOG_ERR, "get data enabled fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(3);
    if (enable) {
        if (data_enable_data_test(0)) {
            syslog(LOG_ERR, "enable data fail in %s", __func__);
            ret = -1;
            goto on_exit;
        }
    }

on_exit:
    return ret;
}

int data_is_ps_attached_test(int slot_id)
{
    bool result = false;
    int ret = tapi_data_is_registered(get_tapi_ctx(), slot_id, &result);
    return ret || !result;
}

int data_get_network_type_test(int slot_id)
{
    tapi_network_type result = NETWORK_TYPE_UNKNOWN;
    int ret = tapi_data_get_network_type(get_tapi_ctx(), slot_id, &result);
    syslog(LOG_DEBUG, "%s: ret: %d, result: %d", __func__, ret, (int)result);
    return ret || result != NETWORK_TYPE_LTE;
}

int data_enable_roaming_test(int state)
{
    int ret = tapi_data_enable_roaming(get_tapi_ctx(), state);
    return ret;
}

bool data_get_roaming_enabled_test(bool* result)
{
    int ret = tapi_data_get_roaming_enabled(get_tapi_ctx(), result);
    return ret;
}

int data_set_preferred_apn_test(int slot_id, char* apn_id)
{
    tapi_data_context* apn = malloc(sizeof(tapi_data_context));
    if (apn == NULL) {
        syslog(LOG_ERR, "apn is null in %s", __func__);
        return -1;
    }

    apn->id = apn_id;
    int ret = tapi_data_set_preferred_apn(get_tapi_ctx(), slot_id, apn);
    free(apn);
    return ret;
}

int data_get_preferred_apn_test(int slot_id)
{
    sleep(2);
    char* apn = NULL;
    int ret = tapi_data_get_preferred_apn(get_tapi_ctx(), slot_id, &apn);
    syslog(LOG_DEBUG, "%s: %s", __func__, apn);

    ret = ret || (apn != NULL && strcmp(apn, "/ril_0/context1")) != 0;

    free(apn);
    return ret;
}

int data_set_and_get_preferred_apn_test(int slot_id, char* apn_id)
{
    int ret = 0;
    if (data_set_preferred_apn_test(slot_id, apn_id)) {
        syslog(LOG_ERR, "set preferred apn fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(3);
    if (data_get_preferred_apn_test(slot_id)) {
        syslog(LOG_ERR, "get preferred apn fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    return ret;
}

int data_set_and_get_default_data_slot_test(int slot_id)
{
    int ret = 0, result = -1;
    if (tapi_data_set_default_slot(get_tapi_ctx(), slot_id)) {
        syslog(LOG_ERR, "set default data slot fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(3);
    if (tapi_data_get_default_slot(get_tapi_ctx(), &result)) {
        syslog(LOG_ERR, "get default data slot fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (result != slot_id) {
        syslog(LOG_ERR, "result != slot_id fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    return ret;
}

int data_load_carrier_apn_test(int slot_id, char* imsi)
{
    int res = 0, ret = -1;

    sleep(3);
    if (sim_set_operator_test(slot_id, imsi)) {
        syslog(LOG_DEBUG, "Remote set sim operator (1) execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    global_data.global_dc_count = 0;
    ret = data_load_apn_contexts_test(slot_id);
    if (ret) {
        syslog(LOG_ERR, "data_load_apn_contexts_test execute fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (global_data.global_dc_count <= 0) {
        syslog(LOG_ERR, "apn count is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (sim_set_operator_test(slot_id, "000")) {
        syslog(LOG_DEBUG, "Remote set sim operator (2) execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int data_apn_after_flight_mode_test(int slot_id)
{
    int res = 0, ret = -1;
    global_data.global_dc_count = 0;
    ret = data_load_apn_contexts_test(slot_id);
    if (ret) {
        syslog(LOG_ERR, "data_load_apn_contexts_test (first) execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    int first_apn_count = global_data.global_dc_count;
    ret = set_radio_power_test(0, false);
    if (ret) {
        syslog(LOG_ERR, "set_radio_power_test (false) execute fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    sleep(5);
    ret = set_radio_power_test(0, true);
    if (ret) {
        syslog(LOG_ERR, "set_radio_power_test (true) execute fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    sleep(5);
    global_data.global_dc_count = 0;
    ret = data_load_apn_contexts_test(slot_id);
    if (ret) {
        syslog(LOG_ERR, "data_load_apn_contexts_test (second) execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (first_apn_count != global_data.global_dc_count) {
        syslog(LOG_ERR, "apn count is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int data_set_data_allow_test(int slot_id)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_DATA_ALLOWED_DONE;
    int ret = tapi_data_set_data_allow(get_tapi_ctx(), slot_id, EVENT_DATA_ALLOWED_DONE, 1, data_event_response);
    if (ret) {
        syslog(LOG_ERR, "tapi_data_set_data_allow execute fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "data_event_response is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int data_send_screen_stat_test(int slot_id)
{
    int ret = tapi_set_fast_dormancy(get_tapi_ctx(), slot_id,
        EVENT_REQUEST_SCREEN_STATE_DONE, 1, data_event_response);
    sleep(4);

    return ret;
}

int data_get_data_call_list_test(int slot_id, int expect)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_DATA_CALL_LIST_QUERY_DONE;
    global_data.data_conn_count = 0;

    int ret = tapi_data_get_data_connection_list(get_tapi_ctx(), slot_id,
        EVENT_DATA_CALL_LIST_QUERY_DONE, data_event_response);
    if (ret) {
        syslog(LOG_DEBUG, "tapi_data_get_data_connection_list execute fail in %s",
            __func__);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "data_event_response in %s is not executed", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_DEBUG, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (global_data.data_conn_count != expect) {
        syslog(LOG_DEBUG, "data_conn_count(%d) is invalid in %s", global_data.data_conn_count, __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

static void data_event_response(tapi_async_result* result)
{
    syslog(LOG_DEBUG, "%s : \n", __func__);
    syslog(LOG_DEBUG, "result->msg_id : %d\n", result->msg_id);
    syslog(LOG_DEBUG, "result->status : %d\n", result->status);
    syslog(LOG_DEBUG, "result->arg1 : %d\n", result->arg1);
    syslog(LOG_DEBUG, "result->arg2 : %d\n", result->arg2);

    if (result->status != OK) {
        syslog(LOG_DEBUG, "%s msg id : %d result err, return.\n", __func__, result->msg_id);
        return;
    }

    int event = result->msg_id;
    int status = result->status;

    switch (event) {
    case EVENT_APN_LOADED_DONE:
        if (judge_data.expect == EVENT_APN_LOADED_DONE) {
            global_data.global_dc_count = result->arg2;
            judge_data.result = status;
            judge_data.flag = EVENT_APN_LOADED_DONE;
        }
        break;
    case EVENT_APN_ADD_DONE:
        if (judge_data.expect == EVENT_APN_ADD_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_APN_ADD_DONE;
        }
        break;
    case EVENT_APN_EDIT_DONE:
        if (judge_data.expect == EVENT_APN_EDIT_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_APN_EDIT_DONE;
        }
        break;
    case EVENT_APN_REMOVAL_DONE:
        if (judge_data.expect == EVENT_APN_REMOVAL_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_APN_REMOVAL_DONE;
        }
        break;
    case EVENT_APN_RESTORE_DONE:
        if (judge_data.expect == EVENT_APN_RESTORE_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_APN_RESTORE_DONE;
        }
        break;
    case EVENT_DATA_ALLOWED_DONE:
        if (judge_data.expect == EVENT_DATA_ALLOWED_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_DATA_ALLOWED_DONE;
        }
    case EVENT_REQUEST_SCREEN_STATE_DONE:
        if (judge_data.expect == EVENT_REQUEST_SCREEN_STATE_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_REQUEST_SCREEN_STATE_DONE;
        }
        break;
    case EVENT_DATA_CALL_LIST_QUERY_DONE:
        if (judge_data.expect == EVENT_DATA_CALL_LIST_QUERY_DONE) {
            global_data.data_conn_count = result->arg2;
            judge_data.result = status;
            judge_data.flag = EVENT_DATA_CALL_LIST_QUERY_DONE;
        }
    default:
        break;
    }
}

static void data_signal_change(tapi_async_result* result)
{
    tapi_data_context* dc;
    tapi_ipv4_settings* ipv4;
    tapi_ipv6_settings* ipv6;
    int signal = result->msg_id;
    int slot_id = result->arg1;
    int param = result->arg2;

    syslog(LOG_DEBUG, "%s : \n", __func__);
    syslog(LOG_DEBUG, "result->msg_id : %d\n", result->msg_id);
    syslog(LOG_DEBUG, "result->status : %d\n", result->status);
    syslog(LOG_DEBUG, "result->arg1 : %d\n", result->arg1);
    syslog(LOG_DEBUG, "result->arg2 : %d\n", result->arg2);

    if (result->status != OK && judge_data.expect != EVENT_DATA_ENABLED_CHANGE_FAIL_IND) {
        syslog(LOG_DEBUG, "%s msg id : %d result err, return.\n", __func__, result->msg_id);
        return;
    }

    switch (signal) {
    case MSG_DATA_REGISTRATION_STATE_CHANGE_IND:
        syslog(LOG_DEBUG, "data registration state changed to %d in slot[%d] \n", param, slot_id);
        if (judge_data.expect == MSG_DATA_REGISTRATION_STATE_CHANGE_IND) {
            global_data.data_registration_state = param;
            judge_data.result = OK;
            judge_data.flag = MSG_DATA_REGISTRATION_STATE_CHANGE_IND;
        }
        break;
    case MSG_DATA_NETWORK_TYPE_CHANGE_IND:
        syslog(LOG_DEBUG, "data network type changed to %d in slot[%d] \n", param, slot_id);
        if (judge_data.expect == MSG_DATA_NETWORK_TYPE_CHANGE_IND) {
            global_data.network_type = param;
            judge_data.result = OK;
            judge_data.flag = MSG_DATA_NETWORK_TYPE_CHANGE_IND;
        }
        break;
    case MSG_DEFAULT_DATA_SLOT_CHANGE_IND:
        syslog(LOG_DEBUG, "data slot changed to slot[%d] \n", param);
        if (judge_data.expect == MSG_DEFAULT_DATA_SLOT_CHANGE_IND) {
            judge_data.result = OK;
            judge_data.flag = MSG_DEFAULT_DATA_SLOT_CHANGE_IND;
        }
        break;
    case MSG_DATA_ENABLED_CHANGE_IND:
        if (judge_data.expect == MSG_DATA_ENABLED_CHANGE_IND) {
            syslog(LOG_DEBUG, "data switch changed to %d in slot[%d] \n", param, slot_id);
            judge_data.result = 0;
            global_data.data_on = param;
            judge_data.flag = MSG_DATA_ENABLED_CHANGE_IND;
        }
        break;
    case MSG_DATA_CONNECTION_STATE_CHANGE_IND:
        if (judge_data.expect == MSG_DATA_CONNECTION_STATE_CHANGE_IND) {
            dc = result->data;
            if (dc != NULL) {
                syslog(LOG_DEBUG, "id (apn_path) = %s \n", dc->id);
                syslog(LOG_DEBUG, "type = %s \n", tapi_utils_apn_type_to_string(dc->type));
                syslog(LOG_DEBUG, "active = %d \n", dc->active);
                global_data.connection_state = dc->active;
                if (dc->ip_settings != NULL) {
                    ipv4 = dc->ip_settings->ipv4;
                    if (ipv4 != NULL) {
                        syslog(LOG_DEBUG, "ipv4-interface = %s; ip = %s; gateway = %s; dns[0] = %s; \n",
                            ipv4->interface, ipv4->ip, ipv4->gateway, ipv4->dns[0]);
                    }
                    ipv6 = dc->ip_settings->ipv6;
                    if (ipv6 != NULL) {
                        syslog(LOG_DEBUG, "ipv6-interface = %s; ip = %s; gateway = %s; dns[0] = %s; \n",
                            ipv6->interface, ipv6->ip, ipv6->gateway, ipv6->dns[0]);
                    }
                }
            }
            judge_data.result = 0;
            judge_data.flag = MSG_DATA_CONNECTION_STATE_CHANGE_IND;
        }
        break;
    default:
        break;
    }
}

int data_release_internet_network_test(int slot_id)
{
    int ret = data_release_network_test(slot_id, "internet");
    syslog(LOG_DEBUG, "%s, slot_id: %d, ret: %d", __func__, slot_id, ret);

    return ret;
}

int data_request_internet_network_test(int slot_id)
{
    int ret = data_request_network_test(slot_id, "internet");
    syslog(LOG_DEBUG, "%s, slot_id: %d, ret: %d", __func__, slot_id, ret);

    return ret;
}

int data_request_ims_network_test(int slot_id)
{
    int ret = data_request_network_test(slot_id, "ims");
    syslog(LOG_DEBUG, "%s, slot_id: %d, ret: %d", __func__, slot_id, ret);

    return ret;
}

int data_release_ims_network_test(int slot_id)
{
    int ret = data_release_network_test(slot_id, "ims");
    syslog(LOG_DEBUG, "%s, slot_id: %d, ret: %d", __func__, slot_id, ret);

    return ret;
}

int data_get_call_list(int slot_id, int expect)
{
    return data_get_data_call_list_test(slot_id, expect);
}

int data_enable_and_get_roaming_test(void)
{
    bool result = false;
    int ret = -1;
    int res = 0;
    ret = data_enable_roaming_test(true);
    if (ret) {
        syslog(LOG_ERR, "enable data roaming failed");
        res = -1;
        goto on_exit;
    }

    sleep(5);
    ret = data_get_roaming_enabled_test(&result);
    if (ret) {
        syslog(LOG_ERR, "get data roaming failed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (!result) {
        syslog(LOG_ERR, "data roaming state is error in %s, state: %d", __func__, (int)result);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int data_disable_and_get_roaming_test(void)
{
    bool result = true;
    int ret = -1;
    int res = 0;
    ret = data_enable_roaming_test(false);
    if (ret) {
        syslog(LOG_ERR, "disable data roaming failed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(5);
    ret = data_get_roaming_enabled_test(&result);
    if (ret) {
        syslog(LOG_ERR, "get data roaming failed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (result) {
        syslog(LOG_ERR, "data roaming state is error in %s, state: %d", __func__, (int)result);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

#ifndef CONFIG_TELEPHONY_DFX
static void data_data_logging_cb(tapi_async_result* result)
{
    char* data = (char*)result->data;

    if (result->status != OK) {
        syslog(LOG_ERR, "data_data_logging_cb fail,status =%d", result->status);
        return;
    }
    syslog(LOG_DEBUG, "data_logging:%s", data);

    for (int i = 0; i < dfx_data.expected_dfx_count; i++) {
        if (dfx_data.received_dfx_flag[i]) {
            continue;
        }
        syslog(LOG_DEBUG, "expected_dfx_value:%d", dfx_data.expected_dfx_value[i]);
        switch (dfx_data.expected_dfx_value[i]) {
        case EVENT_DATA_FAIL_DFX_DONE:
            if (!strcmp("DATA_ACTIVE_FAIL,915000002,modem fail", data)) {
                dfx_data.received_dfx_flag[i] = true;
                judge_data.result = 0;
                judge_data.flag = EVENT_DATA_ENABLED_CHANGE_FAIL_IND;
            }
            break;
        default:
            syslog(LOG_ERR, "unexpected data logging info:%s", data);
            break;
        }
        return;
    }
    syslog(LOG_ERR, "data logging more than expected:%s", data);
}

int data_enabled_fail_test(void)
{
    int ret = 0;
    int watch_id = 0;
    bool target = false;

    if (data_get_enabled_test(&target)) {
        syslog(LOG_ERR, "get data enabled fail in %s", __func__);
        ret = -1;
        return ret;
    }
    if (target) {
        if (data_enable_data_test(0)) {
            syslog(LOG_ERR, "enable data fail in %s", __func__);
            ret = -1;
            goto on_exit;
        }
    }
    watch_id = tapi_register(get_tapi_ctx(), 0, MSG_DATA_LOGING_IND,
        NULL, data_data_logging_cb);

    dfx_data_init();
    dfx_data.expected_dfx_count = 1;
    dfx_data.expected_dfx_value[0] = EVENT_DATA_FAIL_DFX_DONE;
    judge_data_init();
    judge_data.expect = EVENT_DATA_ENABLED_CHANGE_FAIL_IND;
    global_data.data_on = -1;

    if (remote_data_block_operation(true)) {
        syslog(LOG_ERR, "Remote data operation fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    ret = tapi_data_enable_data(get_tapi_ctx(), 1);
    if (ret) {
        syslog(LOG_ERR, "tapi_data_enable_data execute fail in %s, ret: %d",
            __func__, ret);
        ret = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "the callback function of data_enable is not executed in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result in %s is invalid", __func__);
        ret = -1;
        goto on_exit;
    }

    if (!check_dfx_value()) {
        syslog(LOG_ERR, "check_dfx_value fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (remote_data_block_operation(false)) {
        syslog(LOG_ERR, "Remote data operation fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }
on_exit:
    tapi_unregister(get_tapi_ctx(), watch_id);
    return ret;
}
#endif