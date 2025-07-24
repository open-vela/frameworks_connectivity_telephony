#include "telephony_sms_test.h"
#include "remote_operation.h"
#include "telephony_call_test.h"
#include "telephony_common_test.h"
#include "telephony_ims_test.h"
#include <stdlib.h>
#include <time.h>
#include <uv.h>

extern struct judge_type judge_data;

extern bool response_flag[MAX_MESSAGE_COUNT];
extern int response_ret[MAX_MESSAGE_COUNT];

#ifndef CONFIG_TELEPHONY_DFX
extern struct dfx_judge_data dfx_data;
#endif

static struct
{
    int sms_incoming_watch_id;
    int sms_immediate_watch_id;
    int sms_report_watch_id;
    int sms_report_switch_watch_id;
} global_data;

static void tele_sms_result_print(tapi_async_result* result)
{
    syslog(LOG_DEBUG, "result->msg_id : %d\n", result->msg_id);
    syslog(LOG_DEBUG, "result->status : %d\n", result->status);
    syslog(LOG_DEBUG, "result->arg1 : %d\n", result->arg1);
    syslog(LOG_DEBUG, "result->arg2 : %d\n", result->arg2);
}

static void tele_sms_event_response(tapi_async_result* result)
{
    syslog(LOG_DEBUG, "%s : \n", __func__);
    tele_sms_result_print(result);

    if (result->status != OK) {
        syslog(LOG_DEBUG, "%s msg id: %d result err.\n", __func__, result->msg_id);
    } else {
        syslog(LOG_DEBUG, "send message successed");
    }

    if (result->msg_id == EVENT_SEND_MESSAGE_DONE) {
        judge_data.flag = EVENT_SEND_MESSAGE_DONE;
    } else if (result->msg_id == EVENT_SEND_DATA_MESSAGE_DONE) {
        judge_data.flag = EVENT_SEND_DATA_MESSAGE_DONE;
    } else if (result->msg_id == MSG_INCOMING_MESSAGE_IND) {
        judge_data.flag = MSG_INCOMING_MESSAGE_IND;
    } else if (result->msg_id == MSG_STATUS_REPORT_MESSAGE_IND) {
        judge_data.flag = MSG_STATUS_REPORT_MESSAGE_IND;
    } else if (result->msg_id == MSG_SMS_REPORT_SWITCH_CHANGED_IND) {
        judge_data.flag = MSG_SMS_REPORT_SWITCH_CHANGED_IND;
    }

    judge_data.result = result->status;
}

#ifndef CONFIG_TELEPHONY_DFX
static void sms_data_logging_cb(tapi_async_result* result)
{
    char* data = (char*)result->data;

    if (result->status != OK) {
        syslog(LOG_ERR, "sms_data_logging_cb fail,status =%d", result->status);
        return;
    }
    syslog(LOG_DEBUG, "data_logging:%s", data);

    for (int i = 0; i < dfx_data.expected_dfx_count; i++) {
        if (dfx_data.received_dfx_flag[i]) {
            continue;
        }
        syslog(LOG_DEBUG, "expected_dfx_value:%d", dfx_data.expected_dfx_value[i]);
        switch (dfx_data.expected_dfx_value[i]) {
        case EVENT_SEND_MESSAGE_DFX_DONE:
            if (!strcmp("SMS_INFO,5,4,1,0,dbacga", data)) {
                dfx_data.received_dfx_flag[i] = true;
            }
            break;
        case EVENT_RECEIVE_MESSAGE_DFX_DONE:
            if (!strcmp("SMS_INFO,5,4,2,0,dbacga", data)) {
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

static void tele_sms_event_response_continuous(tapi_async_result* result)
{
    syslog(LOG_DEBUG, "%s : \n", __func__);
    tele_sms_result_print(result);

    if (result->msg_id == EVENT_SEND_MESSAGE_DONE || result->msg_id == EVENT_SEND_DATA_MESSAGE_DONE) {
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

int setup_sms(void** state)
{
    (void)state;
    return sms_listen_sms_test(0);
}

int teardown_sms(void** state)
{
    (void)state;
    return sms_unlisten_sms_test(0);
}

int setup_sms_and_call(void** state)
{
    if (setup_sms(state) < 0) {
        return -1;
    }

    if (setup_call(state) < 0) {
        return -1;
    }

    return 0;
}

int teardown_sms_and_call(void** state)
{
    if (teardown_call(state) < 0) {
        return -1;
    }

    if (teardown_sms(state) < 0) {
        return -1;
    }

    return 0;
}

int sms_listen_sms_test(int slot_id)
{
    global_data.sms_incoming_watch_id = -1;
    global_data.sms_incoming_watch_id = tapi_sms_register(get_tapi_ctx(),
        slot_id, MSG_INCOMING_MESSAGE_IND, NULL, tele_sms_event_response);

    if (global_data.sms_incoming_watch_id < 0) {
        syslog(LOG_ERR, "%s, slot_id: %d, sms_incoming_watch_id < 0\n",
            __func__, slot_id);
        return -1;
    }

    global_data.sms_immediate_watch_id = -1;
    global_data.sms_immediate_watch_id = tapi_sms_register(get_tapi_ctx(),
        slot_id, MSG_IMMEDIATE_MESSAGE_IND, NULL, tele_sms_event_response);

    if (global_data.sms_immediate_watch_id < 0) {
        syslog(LOG_ERR, "%s, slot_id: %d, sms_immediate_watch_id < 0\n",
            __func__, slot_id);
        return -1;
    }

    global_data.sms_report_watch_id = -1;
    global_data.sms_report_watch_id = tapi_sms_register(get_tapi_ctx(),
        slot_id, MSG_STATUS_REPORT_MESSAGE_IND, NULL, tele_sms_event_response);

    if (global_data.sms_report_watch_id < 0) {
        syslog(LOG_ERR, "%s, slot_id: %d, sms_report_watch_id < 0\n",
            __func__, slot_id);
        return -1;
    }

    global_data.sms_report_switch_watch_id = -1;
    global_data.sms_report_switch_watch_id = tapi_sms_register(get_tapi_ctx(),
        slot_id, MSG_SMS_REPORT_SWITCH_CHANGED_IND, NULL, tele_sms_event_response);

    if (global_data.sms_report_switch_watch_id < 0) {
        syslog(LOG_ERR, "%s, slot_id: %d, sms_report_switch_watch_id < 0\n",
            __func__, slot_id);
        return -1;
    }

    return 0;
}

int sms_unlisten_sms_test(int slot_id)
{
    int ret = tapi_sms_unregister(get_tapi_ctx(), global_data.sms_incoming_watch_id);
    if (ret) {
        syslog(LOG_ERR, "%s, slot_id: %d, sms_incoming_watch_id < 0\n",
            __func__, slot_id);
        return -1;
    }

    ret = tapi_sms_unregister(get_tapi_ctx(), global_data.sms_immediate_watch_id);
    if (ret) {
        syslog(LOG_ERR, "%s, slot_id: %d, sms_immediate_watch_id < 0\n",
            __func__, slot_id);
        return -1;
    }

    ret = tapi_sms_unregister(get_tapi_ctx(), global_data.sms_report_watch_id);
    if (ret) {
        syslog(LOG_ERR, "%s, slot_id: %d, sms_report_watch_id < 0\n",
            __func__, slot_id);
        return -1;
    }

    ret = tapi_sms_unregister(get_tapi_ctx(), global_data.sms_report_switch_watch_id);
    if (ret) {
        syslog(LOG_ERR, "%s, slot_id: %d, sms_report_switch_watch_id < 0\n",
            __func__, slot_id);
        return -1;
    }

    return 0;
}

int sms_send_message_test(int slot_id, char* number, char* text, int* result)
{
    if (number == NULL || text == NULL) {
        syslog(LOG_ERR, "%s, number: %s, text: %s", __func__, number, text);
        return -EINVAL;
    }

    int res = 0;
#ifndef CONFIG_TELEPHONY_DFX
    int watch_id = 0;

    watch_id = tapi_register(get_tapi_ctx(), 0, MSG_DATA_LOGING_IND,
        NULL, sms_data_logging_cb);
    dfx_data_init();
    dfx_data.expected_dfx_count = 1;
    dfx_data.expected_dfx_value[0] = EVENT_SEND_MESSAGE_DFX_DONE;
#endif
    judge_data_init();
    judge_data.expect = EVENT_SEND_MESSAGE_DONE;

    int ret = tapi_sms_send_message(get_tapi_ctx(), slot_id, 0, number, text,
        EVENT_SEND_MESSAGE_DONE, tele_sms_event_response);
    if (ret) {
        syslog(LOG_ERR, "tapi_sms_send_message execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "tele_sms_event_response is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    *result = judge_data.result;
#ifndef CONFIG_TELEPHONY_DFX
    if (!check_dfx_value()) {
        syslog(LOG_ERR, "check_dfx_value fail for clear call forwarding in %s", __func__);
        res = -1;
        goto on_exit;
    }
#endif
on_exit:
#ifndef CONFIG_TELEPHONY_DFX
    tapi_unregister(get_tapi_ctx(), watch_id);
#endif
    return res;
}

int sms_receive_message_test(int slot_id)
{
    int res = 0;
#ifndef CONFIG_TELEPHONY_DFX
    int watch_id = 0;

    watch_id = tapi_register(get_tapi_ctx(), 0, MSG_DATA_LOGING_IND,
        NULL, sms_data_logging_cb);
    dfx_data_init();
    dfx_data.expected_dfx_count = 1;
    dfx_data.expected_dfx_value[0] = EVENT_RECEIVE_MESSAGE_DFX_DONE;
#endif
    judge_data_init();
    judge_data.expect = MSG_INCOMING_MESSAGE_IND;

    int ret = remote_sms_send_message(slot_id);
    if (ret) {
        syslog(LOG_ERR, "sms_receive_message_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "sms_receive_message_test is not executed in %s", __func__);
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
        syslog(LOG_ERR, "check_dfx_value fail for clear call forwarding in %s", __func__);
        res = -1;
        goto on_exit;
    }
#endif
on_exit:
#ifndef CONFIG_TELEPHONY_DFX
    tapi_unregister(get_tapi_ctx(), watch_id);
#endif
    return res;
}

int sms_receive_english_long_message_test(int slot_id)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = MSG_INCOMING_MESSAGE_IND;

    int ret = remote_sms_send_english_long_message(slot_id);
    if (ret) {
        syslog(LOG_ERR, "sms_receive_english_long_message_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "sms_receive_english_long_message_test is not executed in %s", __func__);
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

int sms_receive_chinese_long_message_test(int slot_id)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = MSG_INCOMING_MESSAGE_IND;

    int ret = remote_sms_send_chinese_long_message(slot_id);
    if (ret) {
        syslog(LOG_ERR, "sms_receive_chinese_long_message_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "sms_receive_chinese_long_message_test is not executed in %s", __func__);
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

int sms_send_data_message_test(int slot_id, char* to, int port, char* text)
{
    if (to == NULL || text == NULL) {
        syslog(LOG_ERR, "%s, number: %s, text: %s", __func__, to, text);
        return -EINVAL;
    }

    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_SEND_DATA_MESSAGE_DONE;

    int ret = tapi_sms_send_data_message(get_tapi_ctx(), slot_id, 0, to, port, text,
        EVENT_SEND_DATA_MESSAGE_DONE, tele_sms_event_response);
    if (ret) {
        syslog(LOG_ERR, "tapi_sms_send_data_message execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "tele_sms_event_response is not executed in %s", __func__);
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

int sms_receive_report_test(int slot_id, char* to, int port, char* text)
{
    int ret = 0;
    judge_data_init();
    judge_data.expect = MSG_STATUS_REPORT_MESSAGE_IND;

    if (tapi_sms_enable_delivery_report(get_tapi_ctx(), slot_id, 1)) {
        syslog(LOG_ERR, "tapi_sms_enable_delivery_report execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    ret = tapi_sms_send_data_message(get_tapi_ctx(), slot_id, 0, to, port, text,
        EVENT_SEND_DATA_MESSAGE_DONE, tele_sms_event_response);
    if (ret) {
        syslog(LOG_ERR, "tapi_sms_send_data_message execute fail in %s, ret: %d",
            __func__, ret);
        ret = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "tele_sms_event_response is not executed in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (tapi_sms_enable_delivery_report(get_tapi_ctx(), slot_id, 0)) {
        syslog(LOG_ERR, "tapi_sms_enable_delivery_report execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }
on_exit:
    return ret;
}

int sms_set_and_get_service_center_number_test(int slot_id)
{
    int ret = 0;
    char* smsc_addr = "10086";
    char smsc_addr_rtn[MAX_CENTER_ADDRESS_LENGTH + 1] = { 0 };
    if (tapi_sms_set_service_center_address(get_tapi_ctx(), slot_id, smsc_addr)) {
        syslog(LOG_ERR, "set service center address execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(5);
    if (tapi_sms_get_service_center_address(
            get_tapi_ctx(), slot_id, smsc_addr_rtn, sizeof(smsc_addr_rtn))) {
        syslog(LOG_ERR, "get service center address execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (strcmp(smsc_addr_rtn, smsc_addr) != 0) {
        syslog(LOG_ERR, "smsc_addr_rtn is invalid in %s", __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    return ret;
}

int sms_send_message_in_dialing(int slot_id, char* to, char* text)
{
    int ret = -1;
    int res = 0;
    int result = 1;

    ret = call_dial_test(slot_id, to, 0);
    if (ret) {
        syslog(LOG_ERR, "call_dial_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    ret = sms_send_message_test(slot_id, to, text, &result);
    if (ret) {
        syslog(LOG_ERR, "sms_send_message_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    ret = call_hangup_current_call_test(slot_id);
    if (ret) {
        syslog(LOG_ERR, "call_hangup_current_call_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int sms_send_message_in_special_ims_cap(int slot_id, char* to, char* text, int ims_cap)
{
    int ret = 0, result = 1;

    if (ims_set_service_status_test(slot_id, ims_cap)) {
        syslog(LOG_ERR, "ims set service status execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(3);
    if (sms_send_message_test(slot_id, to, text, &result)) {
        syslog(LOG_ERR, "send message execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    return ret;
}

int sms_send_data_message_in_dialing(int slot_id, char* to, char* text, int port)
{
    int ret = -1;
    int res = 0;

    ret = call_dial_test(slot_id, to, 0);
    if (ret) {
        syslog(LOG_ERR, "call_dial_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    ret = sms_send_data_message_test(slot_id, to, port, text);
    if (ret) {
        syslog(LOG_ERR, "sms_send_data_message_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    ret = call_hangup_current_call_test(slot_id);
    if (ret) {
        syslog(LOG_ERR, "call_hangup_current_call_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int sms_send_data_message_in_special_ims_cap(int slot_id, char* to, int port, char* text, int ims_cap)
{
    int ret = 0;

    if (ims_set_service_status_test(slot_id, ims_cap)) {
        syslog(LOG_ERR, "ims set service status execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(3);
    if (sms_send_data_message_test(slot_id, to, port, text)) {
        syslog(LOG_ERR, "send message execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    return ret;
}

int sms_send_message_fail_in_airplane_test(int slot_id, char* to, char* text)
{
    int ret = 0, result = 1;

    if (set_radio_power_test(0, false)) {
        syslog(LOG_ERR, "set radio power execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(10);
    if (sms_send_message_test(slot_id, to, text, &result)) {
        syslog(LOG_ERR, "send message execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (result != -1) {
        syslog(LOG_ERR, "result error execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (set_radio_power_test(0, true)) {
        syslog(LOG_ERR, "set radio power execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(5);

on_exit:
    return ret;
}

int sms_set_and_get_cell_broadcast_power(int slot_id, bool enable)
{
    int ret = 0;
    bool result = false;

    if (tapi_sms_set_cell_broadcast_power_on(get_tapi_ctx(), slot_id, enable)) {
        syslog(LOG_ERR, "tapi_sms_set_cell_broadcast_power_on execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(5);
    if (tapi_sms_get_cell_broadcast_power_on(get_tapi_ctx(), 0, &result)) {
        syslog(LOG_ERR, "tapi_sms_get_cell_broadcast_power_on execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (result != enable) {
        syslog(LOG_ERR, "result error execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    return ret;
}

int sms_set_and_get_cell_broadcast_topics(int slot_id, char* topics)
{
    int ret = 0;
    char* result = NULL;

    if (tapi_sms_set_cell_broadcast_topics(get_tapi_ctx(), slot_id, topics)) {
        syslog(LOG_ERR, "tapi_sms_set_cell_broadcast_topics execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(5);
    if (tapi_sms_get_cell_broadcast_topics(get_tapi_ctx(), 0, &result)) {
        syslog(LOG_ERR, "tapi_sms_get_cell_broadcast_power_on execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (result == NULL) {
        syslog(LOG_ERR, "result is NULL execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (strcmp(result, topics) != 0) {
        syslog(LOG_ERR, "result invaild execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    free(result);
    return ret;
}

int sms_send_short_sms_continuous(int slot_id, char* to)
{
    char* text = "hello";
    int res = 0;

    remote_sms_delay(0, 1);
    tapi_context context = get_tapi_ctx();

    init_response_flag(MAX_MESSAGE_COUNT);

    if (context == NULL || to == NULL || text == NULL) {
        syslog(LOG_ERR, "%s, number: %s, text: %s", __func__, to, text);
        return -EINVAL;
    }

    for (int i = 0; i < MAX_MESSAGE_COUNT; i++) {
        response_ret[i] = EVENT_SEND_MESSAGE_DONE;
        int ret = tapi_sms_send_message(context, slot_id, 0, to, text,
            EVENT_SEND_MESSAGE_DONE, tele_sms_event_response_continuous);
        if (ret) {
            syslog(LOG_ERR, "tapi_sms_send_message execute fail in %s, ret: %d",
                __func__, ret);
            res = -1;
            goto on_exit;
        }
    }

    if (wait_response(MAX_MESSAGE_COUNT) != 0) {
        syslog(LOG_ERR, "tele_sms_event_response_continuous is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    remote_sms_delay(0, 0);
    return res;
}

int sms_send_short_data_sms_continuous(int slot_id, char* to)
{
    char* text = "hello";
    int res = 0;

    remote_sms_delay(0, 1);
    tapi_context context = get_tapi_ctx();

    init_response_flag(MAX_MESSAGE_COUNT);

    if (context == NULL || to == NULL) {
        syslog(LOG_ERR, "%s, number: %s, text: %s", __func__, to, text);
        return -EINVAL;
    }

    for (int i = 0; i < MAX_MESSAGE_COUNT; i++) {
        response_ret[i] = EVENT_SEND_DATA_MESSAGE_DONE;
        int ret = tapi_sms_send_data_message(get_tapi_ctx(), slot_id, 0, to, 0, text,
            EVENT_SEND_DATA_MESSAGE_DONE, tele_sms_event_response_continuous);
        if (ret) {
            syslog(LOG_ERR, "tapi_sms_send_data_message execute fail in %s, ret: %d",
                __func__, ret);
            res = -1;
            goto on_exit;
        }
        if (i == 2) {
            sleep(1);
        }
        if (i == 3) {
            sleep(3);
        }
    }

    if (wait_response(MAX_MESSAGE_COUNT) != 0) {
        syslog(LOG_ERR, "tele_sms_event_response_continuous is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    remote_sms_delay(0, 0);
    return res;
}

int sms_send_short_mix_sms_continuous(int slot_id, char* to)
{
    char* text = "hello";
    int res = 0;
    srand(time(NULL));

    remote_sms_delay(0, 1);
    tapi_context context = get_tapi_ctx();

    init_response_flag(MAX_MESSAGE_COUNT);

    if (context == NULL || to == NULL) {
        syslog(LOG_ERR, "%s, number: %s, text: %s", __func__, to, text);
        return -EINVAL;
    }

    for (int i = 0; i < MAX_MESSAGE_COUNT; i++) {
        int random_num;
        random_num = rand() % 2;
        if (random_num) {
            response_ret[i] = EVENT_SEND_DATA_MESSAGE_DONE;
            int ret = tapi_sms_send_data_message(get_tapi_ctx(), slot_id, 0, to, 0, text,
                EVENT_SEND_DATA_MESSAGE_DONE, tele_sms_event_response_continuous);
            if (ret) {
                syslog(LOG_ERR, "tapi_sms_send_data_message execute fail in %s, ret: %d",
                    __func__, ret);
                res = -1;
                goto on_exit;
            }
        } else {
            response_ret[i] = EVENT_SEND_MESSAGE_DONE;
            int ret = tapi_sms_send_message(context, slot_id, 0, to, text,
                EVENT_SEND_MESSAGE_DONE, tele_sms_event_response_continuous);
            if (ret) {
                syslog(LOG_ERR, "tapi_sms_send_message execute fail in %s, ret: %d",
                    __func__, ret);
                res = -1;
                goto on_exit;
            }
        }
        if (i == 3) {
            sleep(2);
        }
        if (i == 4) {
            sleep(4);
        }
    }
    if (wait_response(MAX_MESSAGE_COUNT) != 0) {
        syslog(LOG_ERR, "tele_sms_event_response_continuous is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    remote_sms_delay(0, 0);
    return res;
}