#include "telephony_ss_test.h"
#include "remote_operation.h"
#include "telephony_common_test.h"
#include <stdlib.h>
#include <time.h>

extern struct judge_type judge_data;

extern bool response_flag[MAX_MESSAGE_COUNT];
extern int response_ret[MAX_MESSAGE_COUNT];

#ifndef CONFIG_TELEPHONY_DFX
extern struct dfx_judge_data dfx_data;
#endif

static struct {
    int call_barring_property_change_watch_id;
    int ussd_property_change_watch_id;
    int ussd_notification_received_watch_id;
    int ussd_request_received_watch_id;
    char cf_number[64];
    int fdn_enable;
    int cf_type;
    int cw;
} global_data;

static void ss_signal_change(tapi_async_result* result)
{
    tapi_call_barring_info* cb_value;
    int signal = result->msg_id;
    int slot_id = result->arg1;

    switch (signal) {
    case MSG_CALL_BARRING_PROPERTY_CHANGE_IND:
        cb_value = result->data;
        syslog(LOG_DEBUG, "call barring service %s changed to %s in slot[%d] \n",
            cb_value->service_type, cb_value->value, slot_id);
        break;
    case MSG_USSD_PROPERTY_CHANGE_IND:
        syslog(LOG_DEBUG, "ussd state changed to %s in slot[%d] \n", (char*)result->data, slot_id);
        break;
    case MSG_USSD_NOTIFICATION_RECEIVED_IND:
        syslog(LOG_DEBUG, "ussd notification message %s received in slot[%d] \n",
            (char*)result->data, slot_id);
        break;
    case MSG_USSD_REQUEST_RECEIVED_IND:
        syslog(LOG_DEBUG, "ussd request message %s received in slot[%d] \n",
            (char*)result->data, slot_id);
        break;
    default:
        break;
    }
}

#ifndef CONFIG_TELEPHONY_DFX
static void ss_data_logging_cb(tapi_async_result* result)
{
    char* data = (char*)result->data;

    if (result->status != OK) {
        syslog(LOG_ERR, "ss_data_logging_cb fail,status =%d", result->status);
        return;
    }
    syslog(LOG_DEBUG, "data_logging:%s", data);

    for (int i = 0; i < dfx_data.expected_dfx_count; i++) {
        if (dfx_data.received_dfx_flag[i]) {
            continue;
        }
        syslog(LOG_DEBUG, "expected_dfx_value:%d", dfx_data.expected_dfx_value[i]);
        switch (dfx_data.expected_dfx_value[i]) {
        case EVENT_SET_CALL_WAITING_DFX_DONE:
            if (strstr(data, "SS_INFO,ss:set call waiting,NA")) {
                dfx_data.received_dfx_flag[i] = true;
            }
            break;
        case EVENT_GET_CALL_WAITING_DFX_DONE:
            if (strstr(data, "SS_INFO,ss:get call waiting,NA")) {
                dfx_data.received_dfx_flag[i] = true;
            }
            break;
        case EVENT_QUERY_ALL_CALL_BARRING_DFX_DONE:
            if (strstr(data, "SS_INFO,ss:request callbarring,fail_reason:NA")) {
                dfx_data.received_dfx_flag[i] = true;
            }
            break;
        case EVENT_SET_CALL_BARRING_DFX_DONE:
            if (strstr(data, "SS_INFO,ss:set callbarring,NA")) {
                dfx_data.received_dfx_flag[i] = true;
            }
            break;
        case EVENT_SET_CALL_FORWARDING_DFX_DONE:
            if (strstr(data, "SS_INFO,ss:set call forwarding,NA")) {
                dfx_data.received_dfx_flag[i] = true;
            }
            break;
        case EVENT_QUERY_CALL_FORWARDING_DFX_DONE:
            if (strstr(data, "SS_INFO,ss:query call forwarding,NA")) {
                dfx_data.received_dfx_flag[i] = true;
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
#endif

int setup_ss(void** state)
{
    return ss_listen_ss_test(0);
}

int setup_ssAndRadio(void** state)
{
    (void)state;
    if (setup_radio(state)) {
        return -1;
    }

    if (setup_ss(state)) {
        return -1;
    }

    return 0;
}

int teardown_ss(void** state)
{
    (void)state;
    return ss_unlisten_ss_test();
}

int teardown_ssAndRadio(void** state)
{
    (void)state;
    if (teardown_radio(state)) {
        return -1;
    }

    if (teardown_ss(state)) {
        return -1;
    }

    return 0;
}

int ss_listen_ss_test(int slot_id)
{
    global_data.call_barring_property_change_watch_id = -1;
    global_data.call_barring_property_change_watch_id = tapi_ss_register(get_tapi_ctx(), slot_id, MSG_CALL_BARRING_PROPERTY_CHANGE_IND,
        NULL, ss_signal_change);
    if (global_data.call_barring_property_change_watch_id < 0) {
        syslog(LOG_ERR, "call barring property change register fail, ret: %d",
            global_data.call_barring_property_change_watch_id);
        return -1;
    }

    global_data.ussd_property_change_watch_id = -1;
    global_data.ussd_property_change_watch_id = tapi_ss_register(get_tapi_ctx(), slot_id, MSG_USSD_PROPERTY_CHANGE_IND,
        NULL, ss_signal_change);
    if (global_data.ussd_property_change_watch_id < 0) {
        syslog(LOG_ERR, "ussd property change register fail, ret: %d",
            global_data.ussd_property_change_watch_id);
        return -1;
    }

    global_data.ussd_notification_received_watch_id = -1;
    global_data.ussd_notification_received_watch_id = tapi_ss_register(get_tapi_ctx(), slot_id, MSG_USSD_NOTIFICATION_RECEIVED_IND,
        NULL, ss_signal_change);
    if (global_data.ussd_notification_received_watch_id < 0) {
        syslog(LOG_DEBUG, "ussd notification received register fail, ret: %d",
            global_data.ussd_notification_received_watch_id);
        return -1;
    }

    global_data.ussd_request_received_watch_id = -1;
    global_data.ussd_request_received_watch_id = tapi_ss_register(get_tapi_ctx(), slot_id, MSG_USSD_REQUEST_RECEIVED_IND,
        NULL, ss_signal_change);
    if (global_data.ussd_request_received_watch_id < 0) {
        syslog(LOG_ERR, "ussd request received register fail, ret: %d",
            global_data.ussd_request_received_watch_id);
        return -1;
    }

    return 0;
}

int ss_unlisten_ss_test(void)
{
    int ret;
    if (global_data.call_barring_property_change_watch_id < 0
        || global_data.ussd_property_change_watch_id < 0
        || global_data.ussd_notification_received_watch_id < 0
        || global_data.ussd_request_received_watch_id < 0) {
        return -1;
    }

    ret = -1;
    ret = tapi_ss_unregister(get_tapi_ctx(), global_data.call_barring_property_change_watch_id);
    if (ret < 0) {
        syslog(LOG_DEBUG, "call barring property change unregister fail, ret: %d", ret);
        return -1;
    }

    ret = -1;
    ret = tapi_ss_unregister(get_tapi_ctx(), global_data.ussd_property_change_watch_id);
    if (ret < 0) {
        syslog(LOG_ERR, "ussd property change unregister fail, ret: %d", ret);
        return -1;
    }

    ret = -1;
    ret = tapi_ss_unregister(get_tapi_ctx(), global_data.ussd_notification_received_watch_id);
    if (ret < 0) {
        syslog(LOG_DEBUG, "ussd notification received unregister fail, ret: %d", ret);
        return -1;
    }

    ret = -1;
    ret = tapi_ss_unregister(get_tapi_ctx(), global_data.ussd_request_received_watch_id);
    if (ret < 0) {
        syslog(LOG_ERR, "ussd request received unregister fail, ret: %d", ret);
        return -1;
    }

    return 0;
}

static void tele_ss_result_print(tapi_async_result* result)
{
    syslog(LOG_DEBUG, "result->msg_id : %d\n", result->msg_id);
    syslog(LOG_DEBUG, "result->status : %d\n", result->status);
    syslog(LOG_DEBUG, "result->arg1 : %d\n", result->arg1);
    syslog(LOG_DEBUG, "result->arg2 : %d\n", result->arg2);
}

static void tele_ss_async_fun(tapi_async_result* result)
{
    syslog(LOG_DEBUG, "%s : \n", __func__);
    tele_ss_result_print(result);

    if (result->status != OK) {
        syslog(LOG_DEBUG, "%s msg id : %d result err, return.\n", __func__, result->msg_id);
        return;
    }

    if (result->msg_id == EVENT_QUERY_ALL_CALL_BARRING_DONE) {
        if (judge_data.expect == EVENT_QUERY_ALL_CALL_BARRING_DONE) {
            judge_data.result = 0;
            judge_data.flag = EVENT_QUERY_ALL_CALL_BARRING_DONE;
        }
    } else if (result->msg_id == EVENT_REQUEST_CALL_BARRING_DONE) {
        if (judge_data.expect == EVENT_REQUEST_CALL_BARRING_DONE) {
            judge_data.result = 0;
            judge_data.flag = EVENT_REQUEST_CALL_BARRING_DONE;
        }
    } else if (result->msg_id == EVENT_CALL_BARRING_PASSWD_CHANGE_DONE) {
        if (judge_data.expect == EVENT_CALL_BARRING_PASSWD_CHANGE_DONE) {
            judge_data.result = 0;
            judge_data.flag = EVENT_CALL_BARRING_PASSWD_CHANGE_DONE;
        }
    } else if (result->msg_id == EVENT_DISABLE_ALL_INCOMING_DONE) {
        if (judge_data.expect == EVENT_DISABLE_ALL_INCOMING_DONE) {
            judge_data.result = 0;
            judge_data.flag = EVENT_DISABLE_ALL_INCOMING_DONE;
        }
    } else if (result->msg_id == EVENT_DISABLE_ALL_OUTGOING_DONE) {
        if (judge_data.expect == EVENT_DISABLE_ALL_OUTGOING_DONE) {
            judge_data.result = 0;
            judge_data.flag = EVENT_DISABLE_ALL_OUTGOING_DONE;
        }
    } else if (result->msg_id == EVENT_DISABLE_ALL_CALL_BARRINGS_DONE) {
        if (judge_data.expect == EVENT_DISABLE_ALL_CALL_BARRINGS_DONE) {
            judge_data.result = 0;
            judge_data.flag = EVENT_DISABLE_ALL_CALL_BARRINGS_DONE;
        }
    } else if (result->msg_id == EVENT_REQUEST_CALL_FORWARDING_DONE) {
        if (judge_data.expect == EVENT_REQUEST_CALL_FORWARDING_DONE) {
            if (result->arg1 == global_data.cf_type && result->arg2 == 1) {
                judge_data.result = 0;
                judge_data.flag = EVENT_REQUEST_CALL_FORWARDING_DONE;
            }
        }
    } else if (result->msg_id == EVENT_QUERY_CALL_FORWARDING_DONE) {
        tapi_call_forward_info* cf_info = result->data;
        if (judge_data.expect == EVENT_QUERY_CALL_FORWARDING_DONE) {
            syslog(LOG_DEBUG, "global_data.cf_type: %d\n", global_data.cf_type);
            syslog(LOG_DEBUG, "global_data.cf_number: %s\n", global_data.cf_number);
            syslog(LOG_DEBUG, "cf_info->phone_number.number: %s\n", cf_info->phone_number.number);
            if (result->arg1 == global_data.cf_type && cf_info != NULL
                && !strcmp(global_data.cf_number, cf_info->phone_number.number)) {
                judge_data.result = 0;
                judge_data.flag = EVENT_QUERY_CALL_FORWARDING_DONE;
            }
        }
    } else if (result->msg_id == EVENT_REQUEST_CALL_WAITING_DONE) {
        if (judge_data.expect == EVENT_REQUEST_CALL_WAITING_DONE) {
            if (result->arg2 == global_data.cw) {
                judge_data.result = 0;
                judge_data.flag = EVENT_REQUEST_CALL_WAITING_DONE;
            }
        }
    } else if (result->msg_id == EVENT_QUERY_CALL_WAITING_DONE) {
        if (judge_data.expect == EVENT_QUERY_CALL_WAITING_DONE) {
            if (result->arg2 == global_data.cw) {
                judge_data.result = 0;
                judge_data.flag = EVENT_QUERY_CALL_WAITING_DONE;
            }
        }
    } else if (result->msg_id == EVENT_ENABLE_FDN_DONE) {
        if (judge_data.expect == EVENT_ENABLE_FDN_DONE) {
            judge_data.result = 0;
            judge_data.flag = EVENT_ENABLE_FDN_DONE;
        }
    } else if (result->msg_id == EVENT_QUERY_FDN_DONE) {
        if (judge_data.expect == EVENT_QUERY_FDN_DONE) {
            if (result->arg2 == global_data.fdn_enable) {
                judge_data.result = 0;
                judge_data.flag = EVENT_QUERY_FDN_DONE;
            }
        }
    }
}

static void tele_ss_event_response_continuous(tapi_async_result* result)
{
    syslog(LOG_DEBUG, "%s : \n", __func__);
    tele_ss_result_print(result);

    if (result->msg_id == EVENT_REQUEST_CALL_FORWARDING_DONE
        || result->msg_id == EVENT_QUERY_CALL_FORWARDING_DONE
        || result->msg_id == EVENT_QUERY_CALL_WAITING_DONE
        || result->msg_id == EVENT_REQUEST_CALL_WAITING_DONE) {
        for (int i = 0; i < MAX_MESSAGE_COUNT; i++) {
            if (!response_flag[i] && response_ret[i] == result->msg_id) {
                response_flag[i] = TRUE;
                if (result->status != OK) {
                    syslog(LOG_DEBUG, "%s msg id: %d result err, return.\n", __func__, result->msg_id);
                    response_ret[i] = -1;
                } else {
                    response_ret[i] = 0;
                }
                break;
            }
        }
    }
}

int ss_request_call_barring_test(int slot_id)
{
    int res = 0;
#ifndef CONFIG_TELEPHONY_DFX
    int watch_id = 0;

    watch_id = tapi_register(get_tapi_ctx(), 0, MSG_DATA_LOGING_IND,
        NULL, ss_data_logging_cb);
    dfx_data_init();
    dfx_data.expected_dfx_count = 1;
    dfx_data.expected_dfx_value[0] = EVENT_QUERY_ALL_CALL_BARRING_DFX_DONE;
#endif
    judge_data_init();
    judge_data.expect = EVENT_QUERY_ALL_CALL_BARRING_DONE;
    int ret = tapi_ss_request_call_barring(get_tapi_ctx(), slot_id,
        EVENT_QUERY_ALL_CALL_BARRING_DONE, tele_ss_async_fun);
    if (ret) {
        syslog(LOG_ERR, "tapi_ss_request_call_barring execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "tele_ss_async_fun is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }
#ifndef CONFIG_TELEPHONY_DFX
    if (!check_dfx_value()) {
        syslog(LOG_ERR, "check_dfx_value fail for query call barring in %s", __func__);
        ret = -1;
        goto on_exit;
    }
#endif
on_exit:
#ifndef CONFIG_TELEPHONY_DFX
    tapi_unregister(get_tapi_ctx(), watch_id);
#endif
    return res;
}

int ss_set_call_barring_option_test(int slot_id, char* facility, char* pin2)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_REQUEST_CALL_BARRING_DONE;
    int ret = tapi_ss_set_call_barring_option(get_tapi_ctx(), slot_id,
        EVENT_REQUEST_CALL_BARRING_DONE, facility, pin2, tele_ss_async_fun);
    if (ret) {
        syslog(LOG_ERR, "tapi_ss_set_call_barring_option execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "tele_ss_async_fun is not executed in %s", __func__);
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

int ss_set_and_get_call_barring_option_test(int slot_id, char* facility, char* pin2)
{
    int ret = 0;
    char* result = NULL;
#ifndef CONFIG_TELEPHONY_DFX
    int watch_id = 0;

    watch_id = tapi_register(get_tapi_ctx(), 0, MSG_DATA_LOGING_IND,
        NULL, ss_data_logging_cb);
    dfx_data_init();
    dfx_data.expected_dfx_count = 1;
    dfx_data.expected_dfx_value[0] = EVENT_SET_CALL_BARRING_DFX_DONE;
#endif
    if (ss_set_call_barring_option_test(slot_id, facility, pin2)) {
        syslog(LOG_ERR, "set call barring option_test fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(3);
    if (tapi_ss_get_call_barring_option(get_tapi_ctx(), slot_id, "VoiceIncoming", &result)) {
        syslog(LOG_ERR, "get call barring option fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (result == NULL) {
        syslog(LOG_ERR, "result is NULL fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (strcmp(result, "always")) {
        syslog(LOG_ERR, "result is %s fail in %s", result, __func__);
        ret = -1;
        goto on_exit;
    }
#ifndef CONFIG_TELEPHONY_DFX
    if (!check_dfx_value()) {
        syslog(LOG_ERR, "check_dfx_value fail for get call waiting in %s", __func__);
        ret = -1;
        goto on_exit;
    }
#endif
on_exit:
#ifndef CONFIG_TELEPHONY_DFX
    tapi_unregister(get_tapi_ctx(), watch_id);
#endif
    free(result);
    return ret;
}

int ss_change_call_barring_password_test(int slot_id, char* old_passwd, char* new_passwd)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_CALL_BARRING_PASSWD_CHANGE_DONE;
    int ret = tapi_ss_change_call_barring_password(get_tapi_ctx(), slot_id,
        EVENT_CALL_BARRING_PASSWD_CHANGE_DONE, old_passwd, new_passwd,
        tele_ss_async_fun);
    if (ret) {
        syslog(LOG_ERR, "tapi_ss_change_call_barring_password execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "tele_ss_async_fun is not executed in %s", __func__);
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

int ss_change_and_reset_call_barring_password_test(int slot_id, char* old_passwd, char* new_passwd)
{
    int ret = 0;
    if (ss_change_call_barring_password_test(slot_id, old_passwd, new_passwd)) {
        syslog(LOG_ERR, "change call barring password fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(3);
    if (ss_change_call_barring_password_test(slot_id, new_passwd, old_passwd)) {
        syslog(LOG_ERR, "change call barring password fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    return ret;
}

int ss_disable_all_incoming_test(int slot_id, char* passwd)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_DISABLE_ALL_INCOMING_DONE;
    int ret = tapi_ss_disable_all_incoming(get_tapi_ctx(), slot_id,
        EVENT_DISABLE_ALL_INCOMING_DONE, passwd, tele_ss_async_fun);
    if (ret) {
        syslog(LOG_ERR, "tapi_ss_disable_all_incoming execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "tele_ss_async_fun is not executed in %s", __func__);
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

int ss_disable_all_outgoing_test(int slot_id, char* passwd)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_DISABLE_ALL_OUTGOING_DONE;
    int ret = tapi_ss_disable_all_outgoing(get_tapi_ctx(), slot_id,
        EVENT_DISABLE_ALL_OUTGOING_DONE, passwd, tele_ss_async_fun);
    if (ret) {
        syslog(LOG_ERR, "tapi_ss_disable_all_outgoing execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "tele_ss_async_fun is not executed in %s", __func__);
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

int ss_disable_all_call_barrings_test(int slot_id, char* passwd)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_DISABLE_ALL_CALL_BARRINGS_DONE;
    int ret = tapi_ss_disable_all_call_barrings(get_tapi_ctx(), slot_id,
        EVENT_DISABLE_ALL_CALL_BARRINGS_DONE, passwd, tele_ss_async_fun);
    if (ret) {
        syslog(LOG_ERR, "tapi_ss_disable_all_call_barrings execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "tele_ss_async_fun is not executed in %s", __func__);
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

int ss_set_call_forwarding_option_test(int slot_id, int cf_type, char* number)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_REQUEST_CALL_FORWARDING_DONE;
    memset(global_data.cf_number, 0, sizeof(global_data.cf_number));
    strcpy(global_data.cf_number, number);
    syslog(LOG_DEBUG, "global_data.cf_number: %s", global_data.cf_number);
    global_data.cf_type = cf_type;

    int ret = tapi_ss_set_call_forwarding_option(get_tapi_ctx(), slot_id,
        EVENT_REQUEST_CALL_FORWARDING_DONE, cf_type, BEARER_CLASS_VOICE, number,
        tele_ss_async_fun);
    if (ret) {
        syslog(LOG_ERR, "tapi_ss_set_call_forwarding_option execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "tele_ss_async_fun is not executed in %s", __func__);
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

int ss_get_call_forwarding_option_test(int slot_id, int cf_type)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_QUERY_CALL_FORWARDING_DONE;
    global_data.cf_type = cf_type;

    int ret = tapi_ss_query_call_forwarding_option(get_tapi_ctx(), slot_id,
        EVENT_QUERY_CALL_FORWARDING_DONE, cf_type, BEARER_CLASS_VOICE, tele_ss_async_fun);
    if (ret) {
        syslog(LOG_ERR, "tapi_ss_query_call_forwarding_option execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "tele_ss_async_fun is not executed in %s", __func__);
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

int ss_clear_call_forwarding_option_test(int slot_id, int cf_type)
{
    return ss_set_call_forwarding_option_test(slot_id, cf_type, "\0");
}

int ss_set_and_get_call_forwarding_option_test(int slot_id, int cf_type, char* number)
{
    int ret = 0;
#ifndef CONFIG_TELEPHONY_DFX
    int watch_id = 0;

    watch_id = tapi_register(get_tapi_ctx(), 0, MSG_DATA_LOGING_IND,
        NULL, ss_data_logging_cb);
    dfx_data_init();
    dfx_data.expected_dfx_count = 1;
    dfx_data.expected_dfx_value[0] = EVENT_SET_CALL_FORWARDING_DFX_DONE;
#endif
    if (ss_set_call_forwarding_option_test(slot_id, cf_type, number)) {
        syslog(LOG_ERR, "ss_set_call_forwarding_option_test fail");
        ret = -1;
        goto on_exit;
    }

#ifndef CONFIG_TELEPHONY_DFX
    if (!check_dfx_value()) {
        syslog(LOG_ERR, "check_dfx_value fail for set call forwarding in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    dfx_data_init();
    dfx_data.expected_dfx_count = 1;
    dfx_data.expected_dfx_value[0] = EVENT_QUERY_CALL_FORWARDING_DFX_DONE;
#endif
    sleep(3);
    if (ss_get_call_forwarding_option_test(slot_id, cf_type)) {
        syslog(LOG_ERR, "ss_get_call_forwarding_option_test fail");
        ret = -1;
        goto on_exit;
    }
#ifndef CONFIG_TELEPHONY_DFX
    if (!check_dfx_value()) {
        syslog(LOG_ERR, "check_dfx_value fail for query call forwarding in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    dfx_data_init();
    dfx_data.expected_dfx_count = 1;
    dfx_data.expected_dfx_value[0] = EVENT_SET_CALL_FORWARDING_DFX_DONE;
#endif
    sleep(3);
    if (ss_clear_call_forwarding_option_test(slot_id, cf_type)) {
        syslog(LOG_ERR, "ss_clear_call_forwarding_option_test fail");
        ret = -1;
        goto on_exit;
    }
#ifndef CONFIG_TELEPHONY_DFX
    if (!check_dfx_value()) {
        syslog(LOG_ERR, "check_dfx_value fail for clear call forwarding in %s", __func__);
        ret = -1;
        goto on_exit;
    }
#endif
on_exit:
#ifndef CONFIG_TELEPHONY_DFX
    tapi_unregister(get_tapi_ctx(), watch_id);
#endif
    return ret;
}

int ss_call_forwarding_continuous_test(int slot_id, char* phone_num)
{
    int res = 0;
    bool set_call_forwarding_flag = FALSE;

    srand(time(NULL));
    remote_ss_operation_delay(0, 1);
    tapi_context context = get_tapi_ctx();
    init_response_flag(MAX_MESSAGE_COUNT);

    if (context == NULL) {
        syslog(LOG_ERR, "%s, number: %s", __func__, phone_num);
        return -EINVAL;
    }

    for (int i = 0; i < MAX_MESSAGE_COUNT; i++) {
        int random_num;
        random_num = rand() % 2;
        if (random_num) {
            response_ret[i] = EVENT_REQUEST_CALL_FORWARDING_DONE;
            if (set_call_forwarding_flag) {
                int ret = tapi_ss_set_call_forwarding_option(get_tapi_ctx(), slot_id,
                    EVENT_REQUEST_CALL_FORWARDING_DONE, 0, BEARER_CLASS_VOICE, "\0",
                    tele_ss_event_response_continuous);
                if (ret) {
                    syslog(LOG_ERR, "%s execute fail, ret: %d", __func__, ret);
                    res = -1;
                    goto on_exit;
                }
                set_call_forwarding_flag = FALSE;
            } else {
                int ret = tapi_ss_set_call_forwarding_option(get_tapi_ctx(), slot_id,
                    EVENT_REQUEST_CALL_FORWARDING_DONE, 0, BEARER_CLASS_VOICE, phone_num,
                    tele_ss_event_response_continuous);
                if (ret) {
                    syslog(LOG_ERR, "%s execute fail, ret: %d", __func__, ret);
                    res = -1;
                    goto on_exit;
                }
                set_call_forwarding_flag = TRUE;
            }
        } else {
            response_ret[i] = EVENT_QUERY_CALL_FORWARDING_DONE;
            int ret = tapi_ss_query_call_forwarding_option(get_tapi_ctx(), slot_id,
                EVENT_QUERY_CALL_FORWARDING_DONE, 0, BEARER_CLASS_VOICE, tele_ss_event_response_continuous);
            if (ret) {
                syslog(LOG_ERR, "%s execute fail, ret: %d", __func__, ret);
                res = -1;
                goto on_exit;
            }
        }
        if (i == 2 || i == 5 || i == 7) {
            sleep(3);
        }
    }

    if (wait_response(MAX_MESSAGE_COUNT) != 0) {
        syslog(LOG_ERR, "tele_ss_event_response_continuous is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    remote_ss_operation_delay(0, 0);
    return res;
}

int ss_call_waiting_continuous_test(int slot_id)
{
    int res = 0;
    srand(time(NULL));
    bool set_call_waiting_flag = FALSE;

    remote_ss_operation_delay(0, 1);
    tapi_context context = get_tapi_ctx();

    init_response_flag(MAX_MESSAGE_COUNT);

    if (context == NULL) {
        syslog(LOG_ERR, "%s context NULL", __func__);
        return -EINVAL;
    }

    for (int i = 0; i < MAX_MESSAGE_COUNT; i++) {
        int random_num;
        random_num = rand() % 2;
        if (random_num) {
            response_ret[i] = EVENT_REQUEST_CALL_WAITING_DONE;
            if (set_call_waiting_flag) {
                int ret = tapi_ss_set_call_waiting(get_tapi_ctx(), slot_id,
                    EVENT_REQUEST_CALL_WAITING_DONE, FALSE, tele_ss_event_response_continuous);
                if (ret) {
                    syslog(LOG_ERR, "%s execute fail, ret: %d", __func__, ret);
                    res = -1;
                    goto on_exit;
                }
                set_call_waiting_flag = FALSE;
            } else {
                int ret = tapi_ss_set_call_waiting(get_tapi_ctx(), slot_id,
                    EVENT_REQUEST_CALL_WAITING_DONE, TRUE, tele_ss_event_response_continuous);
                if (ret) {
                    syslog(LOG_ERR, "%s execute fail, ret: %d", __func__, ret);
                    res = -1;
                    goto on_exit;
                }
                set_call_waiting_flag = TRUE;
            }
        } else {
            response_ret[i] = EVENT_QUERY_CALL_WAITING_DONE;
            int ret = tapi_ss_get_call_waiting(get_tapi_ctx(), slot_id,
                EVENT_QUERY_CALL_WAITING_DONE, tele_ss_event_response_continuous);
            if (ret) {
                syslog(LOG_ERR, "%s execute fail, ret: %d", __func__, ret);
                res = -1;
                goto on_exit;
            }
        }
        if (i == 2 || i == 5 || i == 7) {
            sleep(3);
        }
    }

    if (wait_response(MAX_MESSAGE_COUNT) != 0) {
        syslog(LOG_ERR, "tele_ss_event_response_continuous is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    remote_ss_operation_delay(0, 0);
    return res;
}

int ss_set_call_waiting_test(int slot_id, bool enable)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_REQUEST_CALL_WAITING_DONE;
    global_data.cw = (int)enable;

    int ret = tapi_ss_set_call_waiting(get_tapi_ctx(), slot_id,
        EVENT_REQUEST_CALL_WAITING_DONE, enable, tele_ss_async_fun);
    if (ret) {
        syslog(LOG_ERR, "tapi_ss_set_call_waiting execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "tele_ss_async_fun is not executed in %s", __func__);
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

int ss_get_call_waiting_test(int slot_id, bool expect)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_QUERY_CALL_WAITING_DONE;
    global_data.cw = (int)expect;

    int ret = tapi_ss_get_call_waiting(get_tapi_ctx(), slot_id,
        EVENT_QUERY_CALL_WAITING_DONE, tele_ss_async_fun);
    if (ret) {
        syslog(LOG_ERR, "tapi_ss_get_call_waiting execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "tele_ss_async_fun is not executed in %s", __func__);
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

int ss_set_and_get_call_waiting_test(int slot_id, bool enable)
{
    int ret = 0;
#ifndef CONFIG_TELEPHONY_DFX
    int watch_id = 0;

    watch_id = tapi_register(get_tapi_ctx(), 0, MSG_DATA_LOGING_IND,
        NULL, ss_data_logging_cb);

    dfx_data_init();
    dfx_data.expected_dfx_count = 1;
    dfx_data.expected_dfx_value[0] = EVENT_SET_CALL_WAITING_DFX_DONE;
#endif
    if (ss_set_call_waiting_test(slot_id, enable)) {
        syslog(LOG_ERR, "set call waiting test fail");
        ret = -1;
        goto on_exit;
    }
#ifndef CONFIG_TELEPHONY_DFX
    if (!check_dfx_value()) {
        syslog(LOG_ERR, "check_dfx_value fail for set call waiting in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    dfx_data_init();
    dfx_data.expected_dfx_count = 1;
    dfx_data.expected_dfx_value[0] = EVENT_GET_CALL_WAITING_DFX_DONE;
#endif
    sleep(3);
    if (ss_get_call_waiting_test(slot_id, enable)) {
        syslog(LOG_ERR, "get call waiting test fail");
        ret = -1;
        goto on_exit;
    }
#ifndef CONFIG_TELEPHONY_DFX
    if (!check_dfx_value()) {
        syslog(LOG_ERR, "check_dfx_value fail for get call waiting in %s", __func__);
        ret = -1;
        goto on_exit;
    }
#endif
on_exit:
#ifndef CONFIG_TELEPHONY_DFX
    tapi_unregister(get_tapi_ctx(), watch_id);
#endif
    return ret;
}

int ss_enable_fdn_test(int slot_id, bool enable, char* passwd)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_ENABLE_FDN_DONE;
    int ret = tapi_ss_enable_fdn(get_tapi_ctx(), slot_id, EVENT_ENABLE_FDN_DONE, enable,
        passwd, tele_ss_async_fun);
    if (ret) {
        syslog(LOG_ERR, "tapi_ss_enable_fdn execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "tele_ss_async_fun is not executed in %s", __func__);
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

int ss_query_fdn_test(int slot_id, bool expect)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_QUERY_FDN_DONE;
    global_data.fdn_enable = (int)expect;
    int ret = tapi_ss_query_fdn(get_tapi_ctx(), slot_id, EVENT_QUERY_FDN_DONE, tele_ss_async_fun);
    if (ret) {
        syslog(LOG_ERR, "tapi_ss_query_fdn execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "tele_ss_async_fun is not executed in %s", __func__);
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

int ss_set_and_get_fdn_test(int slot_id, bool enable, char* passwd)
{
    int res = 0;
    if (ss_enable_fdn_test(slot_id, enable, passwd)) {
        syslog(LOG_ERR, "ss_enable_fdn_test fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (ss_query_fdn_test(slot_id, enable)) {
        syslog(LOG_ERR, "ss_query_fdn_test fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}
