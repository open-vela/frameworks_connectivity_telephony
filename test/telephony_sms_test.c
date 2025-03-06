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
        syslog(LOG_DEBUG, "%s msg id: %d result err, return.\n", __func__, result->msg_id);
        return;
    }

    judge_data.result = 0;

    if (result->msg_id == EVENT_SEND_MESSAGE_DONE) {
        syslog(LOG_DEBUG, "send message successed, uuid : %s\n", (char*)result->data);
        judge_data.flag = EVENT_SEND_MESSAGE_DONE;
    } else if (result->msg_id == EVENT_SEND_DATA_MESSAGE_DONE) {
        syslog(LOG_DEBUG, "send data message successed");
        judge_data.flag = EVENT_SEND_DATA_MESSAGE_DONE;
    }
}

static void tele_sms_event_response_continuous(tapi_async_result* result)
{
    syslog(LOG_DEBUG, "%s : \n", __func__);
    tele_sms_result_print(result);

    if (result->msg_id == EVENT_SEND_MESSAGE_DONE || result->msg_id == EVENT_SEND_DATA_MESSAGE_DONE) {
        syslog(LOG_DEBUG, "tele_sms_event_response_continuous, uuid : %s\n", (char*)result->data);
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

int tapi_sms_send_message_test(int slot_id, char* number, char* text)
{
    if (number == NULL || text == NULL) {
        syslog(LOG_ERR, "%s, number: %s, text: %s", __func__, number, text);
        return -EINVAL;
    }

    int res = 0;
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

    if (judge_data.result) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int tapi_sms_send_data_message_test(int slot_id, char* to, int port, char* text)
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

int sms_set_and_get_service_center_number_test(int slot_id)
{
    int ret = 0;
    char* smsc_addr = "10086";
    char* smsc_addr_rtn = NULL;
    if (tapi_sms_set_service_center_address(get_tapi_ctx(), slot_id, smsc_addr)) {
        syslog(LOG_ERR, "set service center address execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(5);
    if (tapi_sms_get_service_center_address(get_tapi_ctx(), slot_id, &smsc_addr_rtn)) {
        syslog(LOG_ERR, "get service center address execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (smsc_addr_rtn == NULL) {
        syslog(LOG_ERR, "smsc_addr_rtn is NULL execute fail in %s", __func__);
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

    ret = call_dial_test(slot_id, to, 0);
    if (ret) {
        syslog(LOG_ERR, "call_dial_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    ret = tapi_sms_send_message_test(slot_id, to, text);
    if (ret) {
        syslog(LOG_ERR, "tapi_sms_send_message_test execute fail in %s, ret: %d",
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
    int ret = 0;

    if (tapi_ims_set_service_status_test(slot_id, ims_cap)) {
        syslog(LOG_ERR, "ims set service status execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(3);
    if (tapi_sms_send_message_test(slot_id, to, text)) {
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

    ret = tapi_sms_send_data_message_test(slot_id, to, port, text);
    if (ret) {
        syslog(LOG_ERR, "tapi_sms_send_data_message_test execute fail in %s, ret: %d",
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

    if (tapi_ims_set_service_status_test(slot_id, ims_cap)) {
        syslog(LOG_ERR, "ims set service status execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(3);
    if (tapi_sms_send_data_message_test(slot_id, to, port, text)) {
        syslog(LOG_ERR, "send message execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

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