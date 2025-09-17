#include <string.h>

#include "remote_operation.h"
#include "telephony_call_test.h"
#include "telephony_common_test.h"
#include "telephony_data_test.h"
#include "telephony_sim_test.h"
#include "telephony_ss_test.h"

extern char* phone_num;

extern struct judge_type judge_data;

#ifndef CONFIG_TELEPHONY_DFX
extern struct dfx_judge_data dfx_data;
#endif

static struct
{
    int call_state_watch_id;
    int call_emergencylist_change_watch_id;
    int call_ring_back_tone_change_watch_id;
    int call_slot_change_watch_id;
    int ss_call_barring_watch_id;
    int ss_ussd_property_change_watch_id;
    int ss_ussd_notification_received_watch_id;
    int ss_ussd_request_received_watch_id;
    int default_voicecall_slot;
} global_data;

static struct
{
    char call_id[101];
    char network_name[101];
    unsigned int call_count;
    int two_call_state_sum;
    char hold_call_id[101];
    int current_call_state;
    char number[101];
} test_case_data;

static void test_case_data_init(void)
{
    memset(&test_case_data, 0, sizeof(test_case_data));
    test_case_data.current_call_state = -1;
}

static void call_state_change_cb(tapi_async_result* result)
{
    tapi_call_info* call_info;

    syslog(LOG_DEBUG, "%s : %d\n", __func__, result->status);
    call_info = (tapi_call_info*)result->data;

    syslog(LOG_DEBUG, "call changed call_id : %s\n", call_info->call_id);
    syslog(LOG_DEBUG, "call state: %d \n", call_info->state);
    syslog(LOG_DEBUG, "call LineIdentification: %s \n", call_info->lineIdentification);
    syslog(LOG_DEBUG, "call IncomingLine: %s \n", call_info->incoming_line);
    syslog(LOG_DEBUG, "call Name: %s \n", call_info->name);
    syslog(LOG_DEBUG, "call StartTime: %s \n", call_info->start_time);
    syslog(LOG_DEBUG, "call Multiparty: %d \n", call_info->multiparty);
    syslog(LOG_DEBUG, "call RemoteHeld: %d \n", call_info->remote_held);
    syslog(LOG_DEBUG, "call RemoteMultiparty: %d \n", call_info->remote_multiparty);
    syslog(LOG_DEBUG, "call Information: %s \n", call_info->info);
    syslog(LOG_DEBUG, "call Icon: %d \n", call_info->icon);
    syslog(LOG_DEBUG, "call Emergency: %d \n", call_info->is_emergency_number);
    syslog(LOG_DEBUG, "call disconnect_reason: %d \n\n", call_info->disconnect_reason);

    if (judge_data.expect == CALL_LOCAL_HANGUP) {
        if (call_info->disconnect_reason == CALL_DISCONNECT_REASON_LOCAL_HANGUP) {
            judge_data.result = 0;
        }

        judge_data.flag = CALL_LOCAL_HANGUP;
    } else if (judge_data.expect == CALL_STATE_CHANGE_TO_ACTIVE) {
        if (call_info->state == 0) {
            judge_data.result = 0;
        }

        judge_data.flag = CALL_STATE_CHANGE_TO_ACTIVE;
    } else if (judge_data.expect == CALL_REMOTE_HANGUP) {
        if (call_info->disconnect_reason == CALL_DISCONNECT_REASON_REMOTE_HANGUP) {
            judge_data.result = 0;
        }

        judge_data.flag = CALL_REMOTE_HANGUP;
    } else if (judge_data.expect == CALL_NETWORK_HANGUP) {
        if (call_info->disconnect_reason == CALL_DISCONNECT_REASON_NETWORK_HANGUP) {
            judge_data.flag = CALL_NETWORK_HANGUP;
        }
        judge_data.result = result->status;

    } else if (judge_data.expect == NEW_CALL_INCOMING
        || judge_data.expect == NEW_CALL_WAITING
        || judge_data.expect == INCOMING_CALL_WITH_NETWORK_NAME) {
        char* call_id = call_info->call_id;
        char* number = call_info->lineIdentification;
        strncpy(test_case_data.call_id, call_id, strlen(call_id));
        strncpy(test_case_data.number, number, strlen(number));
        test_case_data.call_id[strlen(call_id)] = '\0';
        test_case_data.number[strlen(number)] = '\0';

        if (judge_data.expect == INCOMING_CALL_WITH_NETWORK_NAME) {
            char* network_name = call_info->name;
            strncpy(test_case_data.network_name, network_name, strlen(network_name));
            test_case_data.network_name[strlen(network_name)] = '\0';
            judge_data.result = 0;
            judge_data.flag = INCOMING_CALL_WITH_NETWORK_NAME;
        } else {
            if (judge_data.expect == NEW_CALL_INCOMING && call_info->state == 4) {
                judge_data.result = 0;
                judge_data.flag = NEW_CALL_INCOMING;
            } else if (judge_data.expect == NEW_CALL_WAITING && call_info->state == 5) {
                judge_data.result = 0;
                judge_data.flag = NEW_CALL_WAITING;
            }
        }
    } else if (judge_data.expect == HANGUP_DUE_TO_NETWORK_EXCEPTION) {
        if (call_info->disconnect_reason == CALL_DISCONNECT_REASON_NETWORK_HANGUP) {
            judge_data.result = 0;
        }

        judge_data.flag = judge_data.expect;
    } else if (judge_data.expect == CALL_STATE_CHANGE_TO_HOLD) {
        if (call_info->state == 1) {
            judge_data.result = 0;
        }

        judge_data.flag = judge_data.expect;
    } else if (judge_data.expect == NEW_CALL_ALERTING) {
        if (call_info->state == 3) {
            judge_data.result = 0;
        }

        judge_data.flag = judge_data.expect;
    } else if (judge_data.expect == NEW_CONFERENCE_CALL) {
        if (call_info->multiparty == 1) {
            judge_data.result = 0;
        }

        judge_data.flag = judge_data.expect;
    } else if (judge_data.expect == CALL_REMOTE_HOLD) {
        judge_data.result = result->status;
        judge_data.flag = CALL_REMOTE_HOLD;
    }
}

static void tele_call_ecc_list_async_fun(tapi_async_result* result)
{
    int status = result->status;
    int list_length = result->arg2;
    ecc_info* ret = result->data;

    syslog(LOG_DEBUG, "%s : \n", __func__);
    syslog(LOG_DEBUG, "msg_id : %d\n", result->msg_id);
    syslog(LOG_DEBUG, "status : %d\n", status);
    syslog(LOG_DEBUG, "list length: %d\n", list_length);

    if (result->status == 0) {
        for (int i = 0; i < list_length; i++) {
            syslog(LOG_DEBUG, "ecc number : %s,%u,%u \n", ret[i].ecc_num, ret[i].category, ret[i].condition);
        }
    }
}

static void tele_call_manager_call_async_fun(tapi_async_result* result)
{
    tapi_call_info* call_info;

    syslog(LOG_DEBUG, "%s : %d\n", __func__, result->status);
    if (result->status != OK) {
        syslog(LOG_ERR, "async result error in %s", __func__);
        return;
    }

    if (result->msg_id == MSG_CALL_ADD_MESSAGE_IND) {
        call_info = (tapi_call_info*)result->data;

        syslog(LOG_DEBUG, "call added call_id : %s\n", call_info->call_id);
        syslog(LOG_DEBUG, "call state: %d \n", call_info->state);
        syslog(LOG_DEBUG, "call LineIdentification: %s \n", call_info->lineIdentification);
        syslog(LOG_DEBUG, "call IncomingLine: %s \n", call_info->incoming_line);
        syslog(LOG_DEBUG, "call Name: %s \n", call_info->name);
        syslog(LOG_DEBUG, "call StartTime: %s \n", call_info->start_time);
        syslog(LOG_DEBUG, "call Multiparty: %d \n", call_info->multiparty);
        syslog(LOG_DEBUG, "call RemoteHeld: %d \n", call_info->remote_held);
        syslog(LOG_DEBUG, "call RemoteMultiparty: %d \n", call_info->remote_multiparty);
        syslog(LOG_DEBUG, "call Information: %s \n", call_info->info);
        syslog(LOG_DEBUG, "call Icon: %d \n", call_info->icon);
        syslog(LOG_DEBUG, "call Emergency: %d \n\n", call_info->is_emergency_number);

    } else if (result->msg_id == MSG_CALL_REMOVE_MESSAGE_IND) {
        syslog(LOG_DEBUG, "call removed call_id : %s\n", (char*)result->data);
    } else if (result->msg_id == MSG_CALL_RING_BACK_TONE_IND) {
        syslog(LOG_DEBUG, "ring back tone status : %d\n", result->arg2);
    } else if (result->msg_id == MSG_CALL_FORWARDED_MESSAGE_IND) {
        syslog(LOG_DEBUG, "call Forwarded: %s\n", (char*)result->data);
    } else if (result->msg_id == MSG_CALL_BARRING_ACTIVE_MESSAGE_IND) {
        syslog(LOG_DEBUG, "call BarringActive: %s\n", (char*)result->data);
    } else if (result->msg_id == MSG_DEFAULT_VOICECALL_SLOT_CHANGE_IND) {
        if (judge_data.expect == MSG_DEFAULT_VOICECALL_SLOT_CHANGE_IND) {
            syslog(LOG_DEBUG, "default voicecall slot: %d\n", result->arg2);
            judge_data.result = OK;
            global_data.default_voicecall_slot = result->arg2;
            judge_data.flag = MSG_DEFAULT_VOICECALL_SLOT_CHANGE_IND;
        }
    }
}

#ifndef CONFIG_TELEPHONY_DFX
static void call_data_logging_cb(tapi_async_result* result)
{
    char* data = (char*)result->data;

    if (result->status != OK) {
        syslog(LOG_ERR, "call_data_logging_cb fail,status =%d", result->status);
        return;
    }
    syslog(LOG_DEBUG, "data_logging:%s", data);

    for (int i = 0; i < dfx_data.expected_dfx_count; i++) {
        if (dfx_data.received_dfx_flag[i]) {
            continue;
        }
        syslog(LOG_DEBUG, "expected_dfx_value:%d", dfx_data.expected_dfx_value[i]);
        switch (dfx_data.expected_dfx_value[i]) {
        case EVENT_DIAL_CALL_DFX_DONE:
            if (!strcmp("CALL_INFO,1,1,1,0,NA", data)) {
                dfx_data.received_dfx_flag[i] = true;
            }
            break;
        case EVENT_INCOMING_CALL_DFX_DONE:
            if (!strcmp("CALL_INFO,4,2,3,6,NA", data)) {
                dfx_data.received_dfx_flag[i] = true;
            }
            break;
        case EVENT_ANSWER_CALL_DFX_DONE:
            if (!strcmp("CALL_INFO,1,2,1,0,NA", data)) {
                dfx_data.received_dfx_flag[i] = true;
            }
            break;
        case EVENT_DIAL_ECC_CALL_DFX_DONE:
            if (strstr(data, "CALL_INFO,2,1,1,0,NA:status:")) {
                dfx_data.received_dfx_flag[i] = true;
            }
            break;
        case EVENT_HANGUP_CALL_DFX_DONE:
            if (!strcmp("CALL_INFO,4,3,3,0,NA", data)) {
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

int setup_call(void** state)
{
    (void)state;

    return call_listen_call_test(0);
}

int setup_callAndData(void** state)
{
    if (setup_data_enable(state) || setup_call(state)) {
        return -1;
    }

    return 0;
}

int setup_callAndRadio(void** state)
{
    if (setup_radio(state) || setup_call(state)) {
        return -1;
    }

    return 0;
}

int teardown_call(void** state)
{
    (void)state;
    int res = 0;
    if (remote_command_response_fail(0, 0)) {
        syslog(LOG_ERR, "Remote command response fail in %s", __func__);
        res = -1;
    }

    if (call_get_call_count(0)) {
        sleep(3);
        if (call_hangup_all_test(0)) {
            syslog(LOG_ERR, "Hangup all call execute fail in %s", __func__);
            res = -1;
        }
    }

    if (call_unlisten_call_test()) {
        syslog(LOG_ERR, "Unlisten call execute fail in %s", __func__);
        res = -1;
    }
    return res;
}

int teardown_callAndData(void** state)
{
    if (teardown_data_enable(state) || teardown_call(state)) {
        return -1;
    }

    return 0;
}

int teardown_callAndRadio(void** state)
{
    if (teardown_call(state) || teardown_radio(state)) {
        return -1;
    }

    return 0;
}

int call_listen_call_test(int slot_id)
{
    global_data.call_state_watch_id = -1;
    global_data.call_state_watch_id = tapi_call_register_call_state_change(
        get_tapi_ctx(), slot_id, NULL, call_state_change_cb);

    if (global_data.call_state_watch_id < 0) {
        syslog(LOG_ERR, "%s, call state change registered fail, ret: %d",
            __func__, global_data.call_state_watch_id);
        return -1;
    }

    global_data.call_emergencylist_change_watch_id = -1;
    global_data.call_emergencylist_change_watch_id = tapi_call_register_emergency_list_change(
        get_tapi_ctx(), slot_id, NULL, tele_call_ecc_list_async_fun);

    if (global_data.call_emergencylist_change_watch_id < 0) {
        syslog(LOG_ERR, "%s, emergency list change registered fail, ret: %d",
            __func__, global_data.call_emergencylist_change_watch_id);
        return -1;
    }

    global_data.call_ring_back_tone_change_watch_id = -1;
    global_data.call_ring_back_tone_change_watch_id = tapi_call_register_ringback_tone_change(
        get_tapi_ctx(), slot_id, NULL, tele_call_manager_call_async_fun);

    if (global_data.call_ring_back_tone_change_watch_id < 0) {
        syslog(LOG_ERR, "%s, ring back change registered fail, ret: %d",
            __func__, global_data.call_ring_back_tone_change_watch_id);
        return -1;
    }

    global_data.call_slot_change_watch_id = -1;
    global_data.call_slot_change_watch_id = tapi_call_register_default_voicecall_slot_change(
        get_tapi_ctx(), NULL, tele_call_manager_call_async_fun);
    if (global_data.call_slot_change_watch_id < 0) {
        syslog(LOG_ERR, "%s, voicecall slot change registered fail, ret: %d",
            __func__, global_data.call_slot_change_watch_id);
        return -1;
    }

    return 0;
}

static void tele_call_async_fun(tapi_async_result* result)
{
    syslog(LOG_DEBUG, "%s : \n", __func__);
    syslog(LOG_DEBUG, "result->msg_id : %d\n", result->msg_id);
    syslog(LOG_DEBUG, "result->status : %d\n", result->status);
    syslog(LOG_DEBUG, "result->arg1 : %d\n", result->arg1);
    syslog(LOG_DEBUG, "result->arg2 : %d\n", result->arg2);

    int event = result->msg_id;
    int status = result->status;

    switch (event) {
    case EVENT_REQUEST_DIAL_DONE:
        syslog(LOG_DEBUG, "%s: EVENT_REQUEST_DIAL_DONE status: %d\n",
            __func__, result->status);
        if (judge_data.expect == EVENT_REQUEST_DIAL_DONE) {
            judge_data.result = status;
            if (status == OK) {
                char* call_id = (char*)result->data;
                strncpy(test_case_data.call_id, call_id, strlen(call_id));
                test_case_data.call_id[strlen(call_id)] = '\0';
            }
            judge_data.flag = EVENT_REQUEST_DIAL_DONE;
        }
        break;
    case EVENT_REQUEST_START_DTMF_DONE:
        syslog(LOG_DEBUG, "%s: EVENT_REQUEST_START_DTMF_DONE status: %d\n",
            __func__, result->status);
        if (judge_data.expect == EVENT_REQUEST_START_DTMF_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_REQUEST_START_DTMF_DONE;
        }
        break;
    case EVENT_REQUEST_STOP_DTMF_DONE:
        syslog(LOG_DEBUG, "%s: EVENT_REQUEST_STOP_DTMF_DONE status: %d\n",
            __func__, result->status);
        if (judge_data.expect == EVENT_REQUEST_STOP_DTMF_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_REQUEST_STOP_DTMF_DONE;
        }
        break;
    case EVENT_REQUEST_CALL_MERGE_DONE:
        syslog(LOG_DEBUG, "%s: EVENT_REQUEST_CALL_MERGE_DONE status: %d\n",
            __func__, result->status);
        if (judge_data.expect == EVENT_REQUEST_CALL_MERGE_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_REQUEST_CALL_MERGE_DONE;
        }
        break;
    case EVENT_REQUEST_CALL_SEPARATE_DONE:
        syslog(LOG_DEBUG, "%s: EVENT_REQUEST_CALL_SEPARATE_DONE status: %d\n",
            __func__, result->status);
        if (judge_data.expect == EVENT_REQUEST_CALL_SEPARATE_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_REQUEST_CALL_SEPARATE_DONE;
        }
        break;
    default:
        break;
    }
}

int call_dial_test(int slot_id, char* phone_number, int hide_caller_id)
{
    int res = 0;

    test_case_data_init();
    judge_data_init();
    judge_data.expect = EVENT_REQUEST_DIAL_DONE;
    int ret = tapi_call_dial(get_tapi_ctx(), slot_id,
        phone_number, hide_caller_id, EVENT_REQUEST_DIAL_DONE, tele_call_async_fun);

    if (ret) {
        syslog(LOG_ERR, "call_dial_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "call_dial_test is not executed in %s", __func__);
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

int call_hold_call_test(int slot_id)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = CALL_STATE_CHANGE_TO_HOLD;
    int ret = tapi_call_hold_call(get_tapi_ctx(), slot_id);

    if (ret) {
        syslog(LOG_ERR, "call_hold_call_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "call_hold_call_test is not executed in %s", __func__);
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

int call_unhold_call_test(int slot_id)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = CALL_STATE_CHANGE_TO_ACTIVE;
    int ret = tapi_call_unhold_call(get_tapi_ctx(), slot_id);

    if (ret) {
        syslog(LOG_ERR, "call_unhold_call_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "call_unhold_call_test is not executed in %s", __func__);
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

int call_merge_call_test(int slot_id)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_REQUEST_CALL_MERGE_DONE;
    int ret = tapi_call_merge_call(get_tapi_ctx(), slot_id, EVENT_REQUEST_CALL_MERGE_DONE,
        tele_call_async_fun);

    if (ret) {
        syslog(LOG_ERR, "call_merge_call_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "call_merge_call_test is not executed in %s", __func__);
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

int call_separate_call_test(int slot_id)
{
    syslog(LOG_DEBUG, "%s called, current call id: %s\n",
        __func__, test_case_data.call_id);

    if (test_case_data.call_id[0] == 0) {
        syslog(LOG_DEBUG, "no call to separate\n");
        return -1;
    }

    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_REQUEST_CALL_SEPARATE_DONE;
    int ret = tapi_call_separate_call(get_tapi_ctx(), slot_id, EVENT_REQUEST_CALL_SEPARATE_DONE,
        test_case_data.call_id, tele_call_async_fun);

    if (ret) {
        syslog(LOG_ERR, "call_separate_call_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "call_separate_call_test is not executed in %s", __func__);
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

int call_release_and_swap_test(int slot_id)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = CALL_STATE_CHANGE_TO_ACTIVE;
    int ret = tapi_call_release_and_swap(get_tapi_ctx(), slot_id);

    if (ret) {
        syslog(LOG_ERR, "call_release_and_swap_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "call_release_and_swap_test is not executed in %s", __func__);
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

int call_hold_and_answer_test(int slot_id)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = CALL_STATE_CHANGE_TO_ACTIVE;
    int ret = tapi_call_hold_and_answer(get_tapi_ctx(), slot_id);

    if (ret) {
        syslog(LOG_ERR, "call_hold_and_answer_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "call_hold_and_answer_test is not executed in %s", __func__);
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

int call_release_and_answer_test(int slot_id)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = CALL_STATE_CHANGE_TO_ACTIVE;
    int ret = tapi_call_release_and_answer(get_tapi_ctx(), slot_id);

    if (ret) {
        syslog(LOG_ERR, "call_release_and_answer_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "call_release_and_answer_test is not executed in %s", __func__);
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

int call_send_tones_test(int slot_id)
{
    int ret = tapi_call_send_tones(get_tapi_ctx(), slot_id, "11");
    syslog(LOG_DEBUG, "%s, ret: %d", __func__, ret);
    return ret;
}

int call_hangup_current_call_test(int slot_id)
{
    syslog(LOG_DEBUG, "%s called, current call id: %s\n",
        __func__, test_case_data.call_id);

    if (test_case_data.call_id[0] == 0) {
        syslog(LOG_DEBUG, "no call to hanup\n");
        return -1;
    }

    int res = 0;
    judge_data_init();
    judge_data.expect = CALL_LOCAL_HANGUP;
    int ret = tapi_call_hangup_by_id(get_tapi_ctx(), slot_id, test_case_data.call_id);

    if (ret) {
        syslog(LOG_ERR, "call_hangup_current_call_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "call_hangup_current_call_test is not executed in %s", __func__);
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

int call_unlisten_call_test(void)
{
    int ret = -1, res = 0;
    ret = tapi_unregister(get_tapi_ctx(), global_data.call_state_watch_id);
    if (ret) {
        syslog(LOG_ERR, "unregister call state change fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    ret = tapi_unregister(get_tapi_ctx(), global_data.call_emergencylist_change_watch_id);
    if (ret) {
        syslog(LOG_ERR, "unregister emergency list change fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    ret = tapi_unregister(get_tapi_ctx(), global_data.call_ring_back_tone_change_watch_id);
    if (ret) {
        syslog(LOG_ERR, "unregister ring back tone change fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    ret = tapi_unregister(get_tapi_ctx(), global_data.call_slot_change_watch_id);
    if (ret) {
        syslog(LOG_ERR, "unregister slot id change fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

static void call_list_query_complete(tapi_async_result* result)
{
    tapi_call_info* call_info;

    syslog(LOG_DEBUG, "%s : \n", __func__);
    if (result->status != OK)
        return;

    syslog(LOG_DEBUG, "call count: %d \n\n", result->arg2);
    if (judge_data.expect == GET_ALL_CALLS) {
        judge_data.result = 0;
        test_case_data.call_count = result->arg2;
        judge_data.flag = judge_data.expect;
    } else if (judge_data.expect == GET_TWO_CALL_STATES) {
        judge_data.result = 0;
        test_case_data.two_call_state_sum = 0;
        judge_data.flag = judge_data.expect;
    }

    call_info = result->data;

    for (int i = 0; i < result->arg2; i++) {

        test_case_data.two_call_state_sum += call_info[i].state;

        if (judge_data.expect == GET_HOLD_CALL_ID) {
            if (call_info[i].state == 1) {
                char* hold_call_id = call_info[i].call_id;
                strncpy(test_case_data.hold_call_id, hold_call_id, strlen(hold_call_id));
                test_case_data.hold_call_id[strlen(hold_call_id)] = '\0';
                judge_data.result = 0;
                judge_data.flag = judge_data.expect;
            }
        }

        if (judge_data.expect == GET_CURRENT_CALL_STATE) {
            test_case_data.current_call_state = call_info[i].state;
            judge_data.result = 0;
            judge_data.flag = judge_data.expect;
        }

        syslog(LOG_DEBUG, "call id: %s \n", call_info[i].call_id);
        syslog(LOG_DEBUG, "call state: %d \n", call_info[i].state);
        syslog(LOG_DEBUG, "call LineIdentification: %s \n", call_info[i].lineIdentification);
        syslog(LOG_DEBUG, "call IncomingLine: %s \n", call_info[i].incoming_line);
        syslog(LOG_DEBUG, "call Name: %s \n", call_info[i].name);
        syslog(LOG_DEBUG, "call StartTime: %s \n", call_info[i].start_time);
        syslog(LOG_DEBUG, "call Multiparty: %d \n", call_info[i].multiparty);
        syslog(LOG_DEBUG, "call RemoteHeld: %d \n", call_info[i].remote_held);
        syslog(LOG_DEBUG, "call RemoteMultiparty: %d \n", call_info[i].remote_multiparty);
        syslog(LOG_DEBUG, "call Information: %s \n", call_info[i].info);
        syslog(LOG_DEBUG, "call Icon: %d \n", call_info[i].icon);
        syslog(LOG_DEBUG, "call Emergency: %d \n\n", call_info[i].is_emergency_number);
    }
}

int call_get_call_count(int slot_id)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = GET_ALL_CALLS;
    test_case_data.call_count = 0;
    int ret = tapi_call_get_all_calls(get_tapi_ctx(), slot_id, 0, call_list_query_complete);

    if (ret) {
        syslog(LOG_ERR, "call_get_call_count execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "call_get_call_count is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

    return test_case_data.call_count;

on_exit:
    return res;
}

static int call_get_two_call_state(int slot_id)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = GET_TWO_CALL_STATES;
    test_case_data.two_call_state_sum = -22;
    int ret = tapi_call_get_all_calls(get_tapi_ctx(), slot_id, 0, call_list_query_complete);

    if (ret) {
        syslog(LOG_ERR, "call_get_two_call_state execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "call_get_two_call_state is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

    return test_case_data.two_call_state_sum;

on_exit:
    return res;
}

static char* get_hold_call_id(int slot_id)
{
    judge_data_init();
    judge_data.expect = GET_HOLD_CALL_ID;
    int ret = tapi_call_get_all_calls(get_tapi_ctx(), slot_id, 0, call_list_query_complete);

    if (ret) {
        syslog(LOG_ERR, "get_hold_call_id execute fail in %s, ret: %d",
            __func__, ret);
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "get_hold_call_id is not executed in %s", __func__);
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        goto on_exit;
    }

    return test_case_data.hold_call_id;

on_exit:
    return NULL;
}

int get_current_call_state_test(int slot_id)
{
    int res = 0;
    test_case_data.current_call_state = -2;
    judge_data_init();
    judge_data.expect = GET_CURRENT_CALL_STATE;
    int ret = tapi_call_get_all_calls(get_tapi_ctx(), slot_id, 0, call_list_query_complete);

    if (ret) {
        syslog(LOG_ERR, "get_current_call_state_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "get_current_call_state_test is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

    return test_case_data.current_call_state;

on_exit:
    return res;
}

int call_hangup_all_test(int slot_id)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = CALL_LOCAL_HANGUP;
    int ret = tapi_call_hangup_all_calls(get_tapi_ctx(), slot_id);

    if (ret) {
        syslog(LOG_ERR, "call_hangup_all_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "call_hangup_all_test is not executed in %s", __func__);
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

int call_dial_with_area_code_test(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, "02510086", 0)) {
        syslog(LOG_ERR, "dial call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_dial_with_pause_code_test(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, "10086,001", 0)) {
        syslog(LOG_ERR, "dial call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_dial_with_wait_code_test(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, "10086;001", 0)) {
        syslog(LOG_ERR, "dial call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_dial_with_numerous_code_test(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, "10086,001;001", 0)) {
        syslog(LOG_ERR, "dial call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_dial_conference_test(int slot_id)
{
    int res = 0;
    test_case_data_init();
    judge_data_init();
    judge_data.expect = NEW_CONFERENCE_CALL;

    char* number[2] = { "10086", "10010" };
    int ret = tapi_call_dial_conferece(get_tapi_ctx(), slot_id, number, 2);

    if (ret) {
        syslog(LOG_ERR, "call_dial_conference_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "call_dial_conference_test is not executed in %s", __func__);
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

int contain_ecc_list_with_sim_card(void)
{
    int ret = 0;
    char* correct_ecc_list_with_sim_card[] = { "110", "119", "120", "118", "999", "000", "08", "911", "112" };

    for (int i = 0; i < sizeof(correct_ecc_list_with_sim_card) / sizeof(char*); i++) {
        if (tapi_call_is_emergency_number(get_tapi_ctx(), correct_ecc_list_with_sim_card[i]) == -1) {
            syslog(LOG_ERR, "%s is not in ecc list\n", correct_ecc_list_with_sim_card[i]);
            ret |= -1;
        }
    }
    return ret;
}

int contain_ecc_list_without_sim_card(void)
{
    int ret = 0;
    char* correct_ecc_list_without_sim_card[] = { "119", "118", "999", "110", "08", "000" };

    for (int i = 0; i < sizeof(correct_ecc_list_without_sim_card) / sizeof(char*); i++) {
        if (tapi_call_is_emergency_number(get_tapi_ctx(), correct_ecc_list_without_sim_card[i]) == -1) {
            syslog(LOG_ERR, "%s is not in ecc list\n", correct_ecc_list_without_sim_card[i]);
            ret |= -1;
        }
    }
    return ret;
}

int exclude_error_ecc_list(void)
{
    int ret = 0;
    char* error_ecc_list[] = { "0", "07", "234" };

    for (int i = 0; i < sizeof(error_ecc_list) / sizeof(char*); i++) {
        if (tapi_call_is_emergency_number(get_tapi_ctx(), error_ecc_list[i]) != -1) {
            syslog(LOG_ERR, "%s is not emergency number\n", error_ecc_list[i]);
            ret |= -1;
        }
    }
    return ret;
}

int call_compare_ecc_list_without_sim_card(int slot_id)
{
    int res = 0;
    if (contain_ecc_list_without_sim_card()) {
        syslog(LOG_ERR, "contain_ecc_list_without_sim_card fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (exclude_error_ecc_list()) {
        syslog(LOG_ERR, "exclude_error_ecc_list fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_compare_ecc_list_with_sim_card(int slot_id)
{
    int res = 0;
    if (contain_ecc_list_with_sim_card()) {
        syslog(LOG_ERR, "contain_ecc_list_with_sim_card fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (exclude_error_ecc_list()) {
        syslog(LOG_ERR, "exclude_error_ecc_list fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_load_and_compare_ecclist_with_china_sim_card_test(int slot_id)
{
    int res = 0;
    ecc_info out[MAX_ECC_LIST_SIZE];

    if (sim_set_operator_test(0, "46000")) {
        syslog(LOG_ERR, "sim set 46000 operator test fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    int size = tapi_call_get_ecc_list(get_tapi_ctx(), slot_id, out);
    if (size <= 0) {
        syslog(LOG_ERR, "get ecclist size is empty in %s", __func__);
        res = -1;
        goto on_exit;
    }

    syslog(LOG_INFO, "ecc list: \n");
    for (int i = 0; i < size; i++) {
        syslog(LOG_DEBUG, "ecc number : %s,%u,%u \n", out[i].ecc_num, out[i].category, out[i].condition);
    }

    if (call_compare_ecc_list_with_sim_card(slot_id)) {
        syslog(LOG_ERR, "ecclist compare execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (sim_set_operator_test(0, "000")) {
        syslog(LOG_ERR, "sim set 000 operator test fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_load_and_compare_ecclist_without_sim_card_test(int slot_id)
{
    int res = 0;
    if (sim_listen_sim_test(slot_id)) {
        syslog(LOG_DEBUG, "Sim listen execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (remote_sim_absent_operation_test(slot_id)) {
        syslog(LOG_ERR, "sim_absent_test fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    ecc_info out[MAX_ECC_LIST_SIZE];
    int size = tapi_call_get_ecc_list(get_tapi_ctx(), slot_id, out);
    if (size <= 0) {
        syslog(LOG_ERR, "get ecclist size is empty in %s", __func__);
        res = -1;
        goto on_exit;
    }

    syslog(LOG_INFO, "ecc list: \n");
    for (int i = 0; i < size; i++) {
        syslog(LOG_DEBUG, "ecc number : %s,%u,%u \n", out[i].ecc_num, out[i].category, out[i].condition);
    }

    if (call_compare_ecc_list_without_sim_card(slot_id)) {
        syslog(LOG_ERR, "ecclist compare execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (remote_sim_insert_operation_test(slot_id)) {
        syslog(LOG_ERR, "sim_absent_test fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (sim_unlisten_sim_test()) {
        syslog(LOG_DEBUG, "Sim unlisten execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(5);
    if (modem_reset_test(slot_id)) {
        syslog(LOG_ERR, "modem_reset_test fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_set_default_voicecall_slot_test(int slot_id)
{
    int res = 0;
    judge_data_init();
    global_data.default_voicecall_slot = -100;
    judge_data.expect = MSG_DEFAULT_VOICECALL_SLOT_CHANGE_IND;

    int ret = tapi_call_set_default_slot(get_tapi_ctx(), slot_id);
    if (ret) {
        syslog(LOG_ERR, "tapi_call_set_default_slot execute fail, ret: %d", ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "tele_call_manager_call_async_fun was not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (global_data.default_voicecall_slot != slot_id) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_get_default_voicecall_slot_test(void)
{
    int result = -1;
    int ret = tapi_call_get_default_slot(get_tapi_ctx(), &result);
    syslog(LOG_DEBUG, "%s, ret: %d, voicecall_slot: %d", __func__, ret, result);

    return ret;
}

int answer_incoming_call_test(int slot_id)
{
    int res = 0;

    res = call_answer_call_test(slot_id, test_case_data.call_id);
    return res;
}

int call_answer_call_test(int slot_id, char* call_id)
{
    if (test_case_data.call_id[0] == 0) {
        syslog(LOG_ERR, "current call id is NULL\n");
        return -1;
    }

    int res = 0;
    judge_data_init();
    judge_data.expect = CALL_STATE_CHANGE_TO_ACTIVE;
    int ret = tapi_call_answer_by_id(get_tapi_ctx(), slot_id, call_id);

    if (ret) {
        syslog(LOG_ERR, "call_answer_call_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "call_answer_call_test is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "Async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_answer_error(int slot_id)
{
    int res = 0;

    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call incoming fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (remote_command_response_fail(slot_id, 1)) {
        syslog(LOG_ERR, "Remote command response fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Call answer fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (remote_command_response_fail(slot_id, 0)) {
        syslog(LOG_ERR, "Remote command response fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

static void generic_callback_test(tapi_async_result* result)
{
    syslog(LOG_DEBUG, "%s: generic callback status: %d\n", __func__, result->status);

    if (judge_data.expect == EVENT_GENERIC_CALLBACK_STATUS) {
        judge_data.result = result->status;
        judge_data.flag = judge_data.expect;
    }
}

int call_answer_call_aysnc_test(int slot_id, char* call_id)
{
    int res = 0;

    if (test_case_data.call_id[0] == 0) {
        syslog(LOG_ERR, "current call id is NULL\n");
        return -1;
    }

    judge_data_init();
    judge_data.expect = EVENT_GENERIC_CALLBACK_STATUS;

    int ret = tapi_call_answer_by_id_async(get_tapi_ctx(), slot_id, call_id,
        NULL, generic_callback_test);
    if (ret) {
        syslog(LOG_ERR, "call_answer_call_test execute fail in %s, ret:%d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "call_answer_call_test is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    res = judge_data.result;

on_exit:
    return res;
}

int call_check_alerting_status(void)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = NEW_CALL_ALERTING;

    if (judge()) {
        syslog(LOG_DEBUG, "Call check status is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "Async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int remote_operation_call_reject_test(int slot_id, char* phone_number)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = CALL_REMOTE_HANGUP;

    remote_call_hangup_with_disconnect_reason(slot_id, phone_number, DISCONNECT_REASON_REMOTE_HANGUP);

    if (judge()) {
        syslog(LOG_ERR, "No hangup call message received in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "Unsolicited message error in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int remote_operation_call_network_reject_test(int slot_id, char* phone_number)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = CALL_NETWORK_HANGUP;

    remote_call_hangup_with_disconnect_reason(slot_id, phone_number, DISCONNECT_REASON_NETWORK_HANGUP);

    if (judge()) {
        syslog(LOG_ERR, "No hangup call message received in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "Unsolicited message error in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int remote_operation_call_hold_test(int slot_id, char* phone_number)
{
    int res = 0;
    judge_data_init();
    test_case_data_init();
    judge_data.expect = CALL_REMOTE_HOLD;

    remote_call_operation(slot_id, phone_number, HOLD_CALL);

    if (judge()) {
        syslog(LOG_ERR, "No call hold callback received in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "Unsolicited message error in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int remote_operation_call_waiting_test(int slot_id, char* phone_number)
{
    int res = 0;
    judge_data_init();
    test_case_data_init();
    judge_data.expect = NEW_CALL_WAITING;

    remote_call_operation(slot_id, phone_number, INCOMING_CALL);

    if (judge()) {
        syslog(LOG_ERR, "No waiting call message received in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "Unsolicited message error in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int remote_operation_third_call_waiting_test(int slot_id, char* phone_number)
{
    int res = 0;
    judge_data_init();
    test_case_data_init();
    judge_data.expect = NEW_CALL_WAITING;

    remote_call_operation(slot_id, phone_number, INCOMING_CALL);

    if (judge()) {
        syslog(LOG_ERR, "No waiting call message received in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "Unsolicited message error in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int remote_operation_call_incoming_test(int slot_id, char* phone_number)
{
    int res = 0;
    judge_data_init();
    test_case_data_init();
    judge_data.expect = NEW_CALL_INCOMING;

    remote_call_operation(slot_id, phone_number, INCOMING_CALL);

    if (judge()) {
        syslog(LOG_ERR, "No incoming call message received in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "Unsolicited message error in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int remote_operation_call_active_test(int slot_id, char* phone_number)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = CALL_STATE_CHANGE_TO_ACTIVE;

    remote_call_operation(slot_id, phone_number, ACTIVE_CALL);

    if (judge()) {
        syslog(LOG_ERR, "No active call message received in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "Unsolicited message error in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_dtmf_after_dial_test(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, phone_num, 0)) {
        syslog(LOG_ERR, "Dial call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_check_alerting_status()) {
        syslog(LOG_ERR, "Check alerting status execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_start_dtmf_test(slot_id)) {
        syslog(LOG_ERR, "Start dtmf execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_stop_dtmf_test(slot_id)) {
        syslog(LOG_ERR, "Stop dtmf execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_dial_after_caller_reject(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, phone_num, 0)) {
        syslog(LOG_ERR, "Dial call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_reject_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call reject execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_dial_test(slot_id, phone_num, 0)) {
        syslog(LOG_ERR, "Dial call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hangup_current_call_test(slot_id)) {
        syslog(LOG_ERR, "Hangup current call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_dial_and_check_status_in_call_active(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, phone_num, 0)) {
        syslog(LOG_ERR, "Dial call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_active_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call reject execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_dial_test(slot_id, "10001", 0)) {
        syslog(LOG_ERR, "Dial call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_get_two_call_state(slot_id) != 4) {
        syslog(LOG_ERR, "Get call state execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_outgoing_remote_answer_and_hangup(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, phone_num, 0) < 0) {
        syslog(LOG_ERR, "Dail call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_active_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call active fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_reject_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call reject fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_outgoing_remote_answer_and_network_hangup(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, phone_num, 0) < 0) {
        syslog(LOG_ERR, "Dail call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_active_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call active fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_network_reject_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call reject fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_incoming_after_remote_hangup(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, phone_num, 0)) {
        syslog(LOG_ERR, "Dail call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_reject_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote hangup with disconnect reason execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call incoming fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hangup_current_call_test(slot_id)) {
        syslog(LOG_ERR, "Hangup all call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_dial_another_after_reject(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, phone_num, 0)) {
        syslog(LOG_ERR, "Dail call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_reject_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote hangup with disconnect reason execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_dial_test(slot_id, "10001", 0)) {
        syslog(LOG_ERR, "Dail call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hangup_current_call_test(slot_id)) {
        syslog(LOG_ERR, "Hangup all call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_hangup_after_dialing(int slot_id)
{
    int res = 0;
    if (call_listen_call_test(slot_id)) {
        syslog(LOG_ERR, "Listen call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_dial_test(slot_id, phone_num, 0)) {
        syslog(LOG_ERR, "Dial call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_hangup_current_call_test(slot_id)) {
        syslog(LOG_ERR, "Hanup current call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_set_voicecall_slot(int slot_id)
{
    int res = 0, result = 0;
    if (tapi_call_get_default_slot(get_tapi_ctx(), &result)) {
        syslog(LOG_ERR, "Get default voicecall slot execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (result != slot_id) {
        if (call_set_default_voicecall_slot_test(slot_id)) {
            syslog(LOG_ERR, "Set default voicecall slot %d execute fail in %s", slot_id, __func__);
            res = -1;
            goto on_exit;
        }
    }

on_exit:
    return res;
}

int call_clear_voicecall_slot(void)
{
    int res = 0, result = 0;
    if (tapi_call_get_default_slot(get_tapi_ctx(), &result)) {
        syslog(LOG_ERR, "Get default voicecall slot execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (result != -1) {
        if (call_set_default_voicecall_slot_test(-1)) {
            syslog(LOG_ERR, "Set default voicecall slot -1 execute fail in %s", __func__);
            res = -1;
            goto on_exit;
        }
    }

on_exit:
    return res;
}

int call_abnormal_answer_again_test(int slot_id)
{
    int res = 0;

    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call incoming fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Call answer fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    syslog(LOG_INFO, "Call answer again for call id:%s", test_case_data.call_id);
    if (ERROR != call_answer_call_aysnc_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "answer again not reply error in %s", __func__);
        res = -1;
        goto on_exit;
    }
    syslog(LOG_INFO, "successfully recieved expect error reply in %s", __func__);

    sleep(3);
    if (call_hangup_current_call_test(slot_id)) {
        syslog(LOG_ERR, "Hangup current call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

// todo
int call_dial_to_phone_out_of_service(int slot_id)
{
    // todo: caller out of service

    int ret1 = call_listen_call_test(slot_id);
    int ret2 = call_dial_test(slot_id, phone_num, 0);
    sleep(5);
    int ret3 = call_hangup_current_call_test(slot_id);
    int ret4 = call_unlisten_call_test();

    return ret1 || ret2 || ret3 || ret4;
}

int call_dial_without_sim_card(int slot_id)
{
    int ret = 0;
    if (sim_listen_sim_test(slot_id)) {
        syslog(LOG_ERR, "Sim listen sim fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_sim_absent_operation_test(slot_id)) {
        syslog(LOG_ERR, "Remote sim absent operation fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(5);
    if (call_dial_test(slot_id, phone_num, 0) != -1) {
        syslog(LOG_ERR, "Dial call execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(5);
    if (remote_sim_insert_operation_test(slot_id)) {
        syslog(LOG_ERR, "Remote sim insert operation fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(10);
    if (sim_unlisten_sim_test()) {
        syslog(LOG_ERR, "Sim unlisten sim fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    return ret;
}

int call_dial_number_test(int slot_id)
{
    int res = 0;
#ifndef CONFIG_TELEPHONY_DFX
    int watch_id = 0;

    watch_id = tapi_register(get_tapi_ctx(), 0, MSG_DATA_LOGING_IND,
        NULL, call_data_logging_cb);

    dfx_data_init();
    dfx_data.expected_dfx_count = 1;
    dfx_data.expected_dfx_value[0] = EVENT_DIAL_CALL_DFX_DONE;
#endif
    if (call_dial_test(slot_id, phone_num, 0)) {
        syslog(LOG_ERR, "Dial call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
#ifndef CONFIG_TELEPHONY_DFX
    if (!check_dfx_value()) {
        syslog(LOG_ERR, "check_dfx_value fail in %s", __func__);
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

int call_dial_ecc_number_test(int slot_id)
{
    int res = 0;
#ifndef CONFIG_TELEPHONY_DFX
    int watch_id = 0;

    watch_id = tapi_register(get_tapi_ctx(), 0, MSG_DATA_LOGING_IND,
        NULL, call_data_logging_cb);

    dfx_data_init();
    dfx_data.expected_dfx_count = 2;
    dfx_data.expected_dfx_value[0] = EVENT_DIAL_CALL_DFX_DONE;
    dfx_data.expected_dfx_value[1] = EVENT_DIAL_ECC_CALL_DFX_DONE;
#endif
    if (call_dial_test(slot_id, "911", 0)) {
        syslog(LOG_ERR, "Dial call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
#ifndef CONFIG_TELEPHONY_DFX
    if (!check_dfx_value()) {
        syslog(LOG_ERR, "check_dfx_value for answer call fail in %s", __func__);
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

int call_dial_ecc_number_without_sim_card(int slot_id)
{
    int ret = 0;
    if (sim_listen_sim_test(slot_id)) {
        syslog(LOG_ERR, "Sim listen sim fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_sim_absent_operation_test(slot_id)) {
        syslog(LOG_ERR, "Remote sim absent operation fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(5);
    if (call_dial_ecc_number_test(0)) {
        syslog(LOG_ERR, "Dial call execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(5);
    if (remote_sim_insert_operation_test(slot_id)) {
        syslog(LOG_ERR, "Remote sim insert operation fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    sleep(10);
    if (sim_unlisten_sim_test()) {
        syslog(LOG_ERR, "Sim unlisten sim fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    return ret;
}

int call_dial_long_phone_number_test(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, "167101398140", 0)) {
        syslog(LOG_ERR, "Dial call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_dial_short_phone_number_test(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, "11", 0)) {
        syslog(LOG_ERR, "Dial call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_dial_with_enable_hide_callerid_test(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, phone_num, 1)) {
        syslog(LOG_ERR, "Dial call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_dial_with_disabled_hide_callerid_test(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, phone_num, 2)) {
        syslog(LOG_ERR, "Dial call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_dial_with_default_hide_callerid_test(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, phone_num, 0)) {
        syslog(LOG_ERR, "Dial call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_start_dtmf_test(int slot_id)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_REQUEST_START_DTMF_DONE;
    int ret = tapi_call_start_dtmf(get_tapi_ctx(), slot_id, '0', EVENT_REQUEST_START_DTMF_DONE,
        tele_call_async_fun);
    if (ret) {
        syslog(LOG_ERR, "call_start_dtmf_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "call_start_dtmf_test is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "Async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_stop_dtmf_test(int slot_id)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_REQUEST_STOP_DTMF_DONE;
    int ret = tapi_call_stop_dtmf(get_tapi_ctx(), slot_id, EVENT_REQUEST_STOP_DTMF_DONE,
        tele_call_async_fun);
    if (ret) {
        syslog(LOG_ERR, "call_stop_dtmf_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "call_stop_dtmf_test is not executed in %s", __func__);
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

int call_incoming_and_check_number(int slot_id)
{
    int res = 0;
    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (strcmp(phone_num, test_case_data.number)) {
        syslog(LOG_ERR, "Number is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_reject_second_call_in_call_active(int slot_id)
{
    int res = 0;
    if (ss_set_and_get_call_waiting_test(slot_id, true)) {
        syslog(LOG_ERR, "Set call waiting fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_waiting_test(slot_id, "10010")) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_hangup_current_call_test(slot_id)) {
        syslog(LOG_ERR, "Hangup current call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (ss_set_and_get_call_waiting_test(slot_id, false)) {
        syslog(LOG_ERR, "Set call waiting fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_swap_two_call_times_in_second_active(int slot_id)
{
    int res = 0;
    if (ss_set_and_get_call_waiting_test(slot_id, true)) {
        syslog(LOG_ERR, "Set call waiting fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_dial_test(slot_id, phone_num, 0) < 0) {
        syslog(LOG_ERR, "Dail call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_active_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call active fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_waiting_test(slot_id, "10010")) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hold_and_answer_test(slot_id)) {
        syslog(LOG_ERR, "Hold and answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    for (int i = 0; i < 3; i++) {
        sleep(3);
        if (call_hold_call_test(slot_id)) {
            syslog(LOG_ERR, "Hold call fail in %s", __func__);
            res = -1;
            goto on_exit;
        }

        sleep(3);
        if (call_unhold_call_test(slot_id)) {
            syslog(LOG_ERR, "Hold call fail in %s", __func__);
            res = -1;
            goto on_exit;
        }
    }

    if (ss_set_and_get_call_waiting_test(slot_id, false)) {
        syslog(LOG_ERR, "Set call waiting fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_transfer_in_active_and_hold_call(int slot_id)
{
    int res = 0;

    if (ss_set_and_get_call_waiting_test(slot_id, true)) {
        syslog(LOG_ERR, "Set call waiting fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_dial_test(slot_id, phone_num, 0) < 0) {
        syslog(LOG_ERR, "Dail call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_active_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call active fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_waiting_test(slot_id, "10010")) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hold_and_answer_test(slot_id)) {
        syslog(LOG_ERR, "Hold and answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (tapi_call_transfer(get_tapi_ctx(), slot_id)) {
        syslog(LOG_ERR, "Transfer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_get_call_count(slot_id) != 0) {
        syslog(LOG_ERR, "Get call count fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (ss_set_and_get_call_waiting_test(0, false) < 0) {
        syslog(LOG_ERR, "Set call waiting false fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_incoming_and_check_number_in_call(int slot_id)
{
    int res = 0;
    if (ss_set_and_get_call_waiting_test(slot_id, true)) {
        syslog(LOG_ERR, "Set call waiting fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_waiting_test(slot_id, "10010")) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (strcmp("10010", test_case_data.number)) {
        syslog(LOG_ERR, "Number is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (ss_set_and_get_call_waiting_test(slot_id, false)) {
        syslog(LOG_ERR, "Set call waiting fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_incoming_answer_and_hangup(int slot_id)
{
    int res = 0;
#ifndef CONFIG_TELEPHONY_DFX
    int watch_id = 0;

    watch_id = tapi_register(get_tapi_ctx(), 0, MSG_DATA_LOGING_IND,
        NULL, call_data_logging_cb);

    dfx_data_init();
    dfx_data.expected_dfx_count = 1;
    dfx_data.expected_dfx_value[0] = EVENT_INCOMING_CALL_DFX_DONE;
#endif
    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
#ifndef CONFIG_TELEPHONY_DFX
    if (!check_dfx_value()) {
        syslog(LOG_ERR, "check_dfx_value fail for incoming call in %s", __func__);
        res = -1;
        goto on_exit;
    }

    dfx_data_init();
    dfx_data.expected_dfx_count = 1;
    dfx_data.expected_dfx_value[0] = EVENT_ANSWER_CALL_DFX_DONE;
#endif
    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
#ifndef CONFIG_TELEPHONY_DFX
    if (!check_dfx_value()) {
        syslog(LOG_ERR, "check_dfx_value for answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    dfx_data_init();
    dfx_data.expected_dfx_count = 1;
    dfx_data.expected_dfx_value[0] = EVENT_HANGUP_CALL_DFX_DONE;
#endif
    sleep(3);
    if (call_hangup_current_call_test(slot_id)) {
        syslog(LOG_ERR, "Hangup call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
#ifndef CONFIG_TELEPHONY_DFX
    tapi_unregister(get_tapi_ctx(), watch_id);
#endif
    return res;
}

int call_incoming_answer_and_remote_hangup(int slot_id)
{
    int res = 0;
    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_reject_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call reject fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_remote_hold_and_unhold_after_incoming_answer(int slot_id)
{
    int res = 0;
    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_hold_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call reject fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_active_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call reject fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_remote_hold_after_local_unhold_in_actve(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, phone_num, 0)) {
        syslog(LOG_ERR, "Dial call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_active_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call active fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hold_call_test(slot_id)) {
        syslog(LOG_ERR, "Hold call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_unhold_call_test(slot_id)) {
        syslog(LOG_ERR, "Hold call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_hold_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call reject fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_remote_hold_after_local_hold_in_actve(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, phone_num, 0)) {
        syslog(LOG_ERR, "Dial call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_active_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call active fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hold_call_test(slot_id)) {
        syslog(LOG_ERR, "Hold call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_hold_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call reject fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_unhold_call_test(slot_id)) {
        syslog(LOG_ERR, "Hold call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_active_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call reject fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

// todo
int call_display_the_network_of_incoming_call(int slot_id, char* network_name)
{
    int ret1 = call_listen_call_test(slot_id);
    judge_data_init();
    test_case_data_init();
    judge_data.expect = INCOMING_CALL_WITH_NETWORK_NAME;

    // todo: incoming call with network name

    if (judge() != 0)
        return -1;

    int ret2 = strncmp(network_name,
        test_case_data.network_name,
        strlen(test_case_data.network_name) + 1);

    if ((ret1 || ret2 || judge_data.result) != 0)
        return -1;

    if (test_case_data.call_id[0] != '\0') {
        sleep(5);
        int ret3 = call_hangup_current_call_test(slot_id);
        int ret4 = call_unlisten_call_test();

        if ((ret3 || ret4) != 0)
            return -1;
    }

    return call_unlisten_call_test();
}

// todo
int call_display_the_network_of_incoming_call_in_call_process(int slot_id,
    char* network_name)
{
    int ret1 = call_listen_call_test(slot_id);
    judge_data_init();
    test_case_data_init();
    judge_data.expect = NEW_CALL_INCOMING;

    // todo: incoming call

    if ((judge() || ret1 || judge_data.result) != 0)
        return -1;

    sleep(10);
    int ret2 = call_answer_call_test(slot_id, test_case_data.call_id);
    sleep(10);
    judge_data_init();
    test_case_data_init();
    judge_data.expect = INCOMING_CALL_WITH_NETWORK_NAME;

    // todo: incoming call with network name

    if ((judge() || ret2 || judge_data.result) != 0)
        return -1;

    int ret3 = strncmp(network_name,
        test_case_data.network_name,
        strlen(test_case_data.network_name) + 1);

    if (ret3 != 0)
        return -1;

    if (call_get_call_count(slot_id) != 0) {
        sleep(5);
        int ret4 = call_hangup_all_test(slot_id);
        int ret5 = call_unlisten_call_test();

        if ((ret4 || ret5) != 0)
            return -1;
    }

    return call_unlisten_call_test();
}

// todo
int call_dial_active_hangup_due_to_dialer_network_exception(int slot_id)
{
    int ret1 = call_listen_call_test(slot_id);
    int ret2 = call_dial_test(slot_id, phone_num, 0);
    sleep(5);

    judge_data_init();
    judge_data.expect = CALL_STATE_CHANGE_TO_ACTIVE;

    // todo: caller answer

    if ((judge() || ret1 || ret2 || judge_data.result) != 0)
        return -1;

    sleep(5);
    judge_data_init();
    judge_data.expect = HANGUP_DUE_TO_NETWORK_EXCEPTION;

    // todo: hangup due to dialer network exception

    if ((judge() || judge_data.result) != 0) {
        if (call_get_call_count(slot_id) != 0)
            call_hangup_all_test(slot_id);
        call_unlisten_call_test();

        return -1;
    }

    return call_unlisten_call_test();
}

// todo
int call_dial_active_hangup_due_to_caller_network_exception(int slot_id)
{
    int ret1 = call_listen_call_test(slot_id);
    int ret2 = call_dial_test(slot_id, phone_num, 0);
    sleep(5);

    judge_data_init();
    judge_data.expect = CALL_STATE_CHANGE_TO_ACTIVE;

    // todo: caller answer

    if ((judge() || ret1 || ret2 || judge_data.result) != 0)
        return -1;

    sleep(5);
    judge_data_init();
    judge_data.expect = HANGUP_DUE_TO_NETWORK_EXCEPTION;

    // todo: hangup due to caller network exception

    if ((judge() || judge_data.result) != 0) {
        if (call_get_call_count(slot_id) != 0)
            call_hangup_all_test(slot_id);
        call_unlisten_call_test();

        return -1;
    }

    return call_unlisten_call_test();
}

int call_check_status_in_call_active(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, phone_num, 0)) {
        syslog(LOG_ERR, "Dail fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_active_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Call active fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (get_current_call_state_test(slot_id) != CALL_STATUS_ACTIVE) {
        syslog(LOG_ERR, "Get current call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_dial_and_keep_in_call_active(int slot_id, char* phone_number)
{
    int res = 0;
    if (call_dial_test(slot_id, phone_number, 0)) {
        syslog(LOG_ERR, "Dail fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_active_test(slot_id, phone_number)) {
        syslog(LOG_ERR, "Call active fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (get_current_call_state_test(slot_id) != CALL_STATUS_ACTIVE) {
        syslog(LOG_ERR, "Get current call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_check_dialing_status_with_multi_call(int slot_id)
{
    int res = 0;
    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Call incoming fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Call answer fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_dial_test(slot_id, "10010", 0)) {
        syslog(LOG_ERR, "Dial fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_get_call_count(slot_id) != 2) {
        syslog(LOG_ERR, "Get call count fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_get_two_call_state(slot_id) != 4) {
        syslog(LOG_ERR, "Get call state fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_incoming_hangup_first_answer_call(int slot_id)
{
    int res = 0;
    if (ss_set_and_get_call_waiting_test(0, true) < 0) {
        syslog(LOG_ERR, "Set call waiting true fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Call incoming fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Call answer fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(1);
    char old_call_id[80];
    strncpy(old_call_id, test_case_data.call_id, strlen(test_case_data.call_id) + 1);

    if (remote_operation_call_waiting_test(slot_id, "10001")) {
        syslog(LOG_ERR, "Call waiting fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(1);
    char new_call_id[101];
    strncpy(new_call_id, test_case_data.call_id, strlen(test_case_data.call_id) + 1);

    if (tapi_call_hangup_by_id(get_tapi_ctx(), slot_id, old_call_id)) {
        syslog(LOG_ERR, "Hangup old call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, new_call_id)) {
        syslog(LOG_ERR, "Answer new call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup all call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (ss_set_and_get_call_waiting_test(0, false)) {
        syslog(LOG_ERR, "Set call waiting false fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_release_and_answer(int slot_id)
{
    int res = 0;
    if (ss_set_and_get_call_waiting_test(0, true) < 0) {
        syslog(LOG_ERR, "Set call waiting true fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_waiting_test(slot_id, "10010")) {
        syslog(LOG_ERR, "Waiting call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hold_and_answer_test(slot_id)) {
        syslog(LOG_ERR, "Hold and answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_third_call_waiting_test(slot_id, "10001")) {
        syslog(LOG_ERR, "Waiting third call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_release_and_answer_test(slot_id)) {
        syslog(LOG_ERR, "Release and answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_get_two_call_state(slot_id) < 0) {
        syslog(LOG_ERR, "Get two call state fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_get_call_count(slot_id) != 2) {
        syslog(LOG_ERR, "Get call count fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup all call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (ss_set_and_get_call_waiting_test(0, false) < 0) {
        syslog(LOG_ERR, "Set call waiting false fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

// todo: 44
int call_incoming_hold_and_recover_by_dialer(int slot_id)
{
    int ret1 = call_listen_call_test(slot_id);
    judge_data_init();
    test_case_data_init();
    judge_data.expect = NEW_CALL_INCOMING;

    // todo: incoming call

    sleep(5);

    if ((judge() || ret1 || judge_data.result) != 0)
        return -1;

    sleep(5);
    int ret2 = call_answer_call_test(slot_id, test_case_data.call_id);
    judge_data_init();
    judge_data.expect = CALL_STATE_CHANGE_TO_HOLD;

    // todo: dialer hold

    if ((ret2 || judge() || judge_data.result) != 0)
        return -1;

    sleep(5);
    judge_data_init();
    judge_data.expect = CALL_STATE_CHANGE_TO_ACTIVE;

    // todo: dialer active

    if (judge() != 0 || judge_data.result != 0)
        return -1;

    sleep(5);
    int ret3 = call_hangup_all_test(slot_id);
    int ret4 = call_unlisten_call_test();

    return ret3 || ret4;
}

int call_outgoing_hold_and_unhold_by_caller(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, phone_num, 0) < 0) {
        syslog(LOG_ERR, "Dail fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_check_alerting_status()) {
        syslog(LOG_ERR, "Check alerting fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (remote_operation_call_active_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Active call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_hold_call_test(slot_id)) {
        syslog(LOG_ERR, "Hold call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_unhold_call_test(slot_id)) {
        syslog(LOG_ERR, "Unhold call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_merge_by_user(int slot_id)
{
    int res = 0;
    if (ss_set_and_get_call_waiting_test(0, true) < 0) {
        syslog(LOG_ERR, "Set call waiting true fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_dial_test(slot_id, phone_num, 0)) {
        syslog(LOG_ERR, "Dail fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_check_alerting_status()) {
        syslog(LOG_ERR, "Check alerting fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_active_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Active call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hold_call_test(slot_id)) {
        syslog(LOG_ERR, "Hold call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_waiting_test(slot_id, "10010")) {
        syslog(LOG_ERR, "Waiting call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hold_and_answer_test(slot_id)) {
        syslog(LOG_ERR, "Hold and answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_merge_call_test(slot_id)) {
        syslog(LOG_ERR, "Merge call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup all call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (ss_set_and_get_call_waiting_test(0, false) < 0) {
        syslog(LOG_ERR, "Set call waiting false fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_separate_by_user(int slot_id)
{
    int res = 0;
    if (ss_set_and_get_call_waiting_test(0, true) < 0) {
        syslog(LOG_ERR, "Set call waiting true fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_dial_test(slot_id, phone_num, 0)) {
        syslog(LOG_ERR, "Dail fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_check_alerting_status()) {
        syslog(LOG_ERR, "Check alerting fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_active_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Active call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_waiting_test(slot_id, "10010")) {
        syslog(LOG_ERR, "Waiting call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hold_and_answer_test(slot_id)) {
        syslog(LOG_ERR, "Hold and answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_merge_call_test(slot_id)) {
        syslog(LOG_ERR, "Merge call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (get_current_call_state_test(slot_id)) {
        syslog(LOG_ERR, "Call state is not in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_separate_call_test(slot_id)) {
        syslog(LOG_ERR, "Separate call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup all call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (ss_set_and_get_call_waiting_test(0, false) < 0) {
        syslog(LOG_ERR, "Set call waiting false fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_release_and_swap_other_call(int slot_id)
{
    int res = 0;
    if (ss_set_and_get_call_waiting_test(0, true) < 0) {
        syslog(LOG_ERR, "Set call waiting true fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_dial_test(slot_id, phone_num, 0)) {
        syslog(LOG_ERR, "Dail fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_check_alerting_status()) {
        syslog(LOG_ERR, "Check alerting fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_active_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Active call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_waiting_test(slot_id, "10010")) {
        syslog(LOG_ERR, "Waiting call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hold_and_answer_test(slot_id)) {
        syslog(LOG_ERR, "Hold and answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_release_and_swap_test(slot_id)) {
        syslog(LOG_ERR, "Release and swap call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup all call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (ss_set_and_get_call_waiting_test(0, false) < 0) {
        syslog(LOG_ERR, "Set call waiting false fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_outgoing_active_and_send_tones(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, phone_num, 0) < 0) {
        syslog(LOG_ERR, "Dail execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_check_alerting_status()) {
        syslog(LOG_ERR, "Check alerting execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_active_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Active call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_send_tones_test(slot_id)) {
        syslog(LOG_ERR, "Send tones execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_incoming_hold_and_resume_by_caller(int slot_id)
{
    int res = 0;
    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    for (int i = 0; i < 3; i++) {
        sleep(3);
        if (call_hold_call_test(slot_id)) {
            syslog(LOG_ERR, "Hold call fail in %s", __func__);
            res = -1;
            goto on_exit;
        }

        sleep(3);
        if (call_unhold_call_test(slot_id)) {
            syslog(LOG_ERR, "Unhold call fail in %s", __func__);
            res = -1;
            goto on_exit;
        }
    }

    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

// todo: 49
int call_swap_dial_reject_swap(int slot_id)
{
    int ret1 = call_listen_call_test(slot_id);
    judge_data_init();
    test_case_data_init();
    judge_data.expect = NEW_CALL_INCOMING;

    // todo: incoming call

    if (judge() != 0 || judge_data.result != 0 || ret1)
        return -1;

    sleep(5);
    if (call_answer_call_test(slot_id, test_case_data.call_id) != 0)
        return -1;

    sleep(5);
    judge_data_init();
    judge_data.expect = CALL_STATE_CHANGE_TO_HOLD;
    int ret2 = tapi_call_hold_call(get_tapi_ctx(), slot_id);

    if ((judge() || judge_data.result || ret2) != 0)
        return -1;

    sleep(5);
    if (call_dial_test(slot_id, phone_num, 0))
        return -1;

    sleep(5);
    judge_data_init();
    judge_data.expect = CALL_REMOTE_HANGUP;

    // todo: caller reject

    if ((judge() || judge_data.result) != 0)
        return -1;

    sleep(5);
    judge_data_init();
    judge_data.expect = CALL_STATE_CHANGE_TO_ACTIVE;
    int ret3 = tapi_call_unhold_call(get_tapi_ctx(), slot_id);

    if ((judge() || judge_data.result || ret3) != 0)
        return -1;

    int ret4 = call_hangup_all_test(slot_id);
    int ret5 = call_unlisten_call_test();

    return ret4 || ret5;
}

int call_unhold_first_incoming_call_after_hangup_second_call(int slot_id)
{
    int res = 0;
    if (ss_set_and_get_call_waiting_test(slot_id, true)) {
        syslog(LOG_ERR, "Set call waiting fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_waiting_test(slot_id, "10010")) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hangup_current_call_test(slot_id)) {
        syslog(LOG_ERR, "Hangup call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_unhold_call_test(slot_id)) {
        syslog(LOG_ERR, "Unhold call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (ss_set_and_get_call_waiting_test(slot_id, false)) {
        syslog(LOG_ERR, "Set call waiting false fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

// todo: 51
int call_incoming_second_call_swap_answer_hangup_swap(int slot_id)
{
    int ret1 = call_listen_call_test(slot_id);
    judge_data_init();
    test_case_data_init();
    judge_data.expect = NEW_CALL_INCOMING;

    // todo: incoming call

    if (judge() != 0 || judge_data.result != 0 || ret1)
        return -1;

    sleep(10);

    if (call_answer_call_test(slot_id, test_case_data.call_id) != 0)
        return -1;

    judge_data_init();
    test_case_data_init();
    judge_data.expect = NEW_CALL_WAITING;

    // todo: incoming another call

    if (judge() != 0 || judge_data.result != 0)
        return -1;

    sleep(5);
    judge_data_init();
    judge_data.expect = CALL_STATE_CHANGE_TO_HOLD;
    int ret2 = tapi_call_hold_call(get_tapi_ctx(), slot_id);

    if ((judge() || judge_data.result || ret2) != 0)
        return -1;

    sleep(5);
    if (call_answer_call_test(slot_id, test_case_data.call_id) != 0)
        return -1;

    sleep(10);

    if (call_hangup_current_call_test(slot_id) != 0)
        return -1;

    sleep(5);
    judge_data_init();
    judge_data.expect = CALL_STATE_CHANGE_TO_ACTIVE;
    int ret3 = tapi_call_unhold_call(get_tapi_ctx(), slot_id);

    if ((judge() || judge_data.result || ret3) != 0)
        return -1;

    int ret4 = call_hangup_all_test(slot_id);
    int ret5 = call_unlisten_call_test();

    return ret4 || ret5;
}

int call_hold_current_call_and_reject_new_incoming(int slot_id)
{
    int res = 0;
    if (ss_set_and_get_call_waiting_test(0, true) < 0) {
        syslog(LOG_ERR, "Set call waiting true fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hold_call_test(slot_id)) {
        syslog(LOG_ERR, "Hold call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (remote_operation_call_waiting_test(slot_id, "10010")) {
        syslog(LOG_ERR, "Waiting call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hangup_current_call_test(slot_id)) {
        syslog(LOG_ERR, "Hangup current call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_unhold_call_test(slot_id)) {
        syslog(LOG_ERR, "Unhold call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup all call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (ss_set_and_get_call_waiting_test(0, false) < 0) {
        syslog(LOG_ERR, "Set call waiting false fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_incoming_and_hangup_new_call(int slot_id)
{
    int res = 0;
    if (ss_set_and_get_call_waiting_test(0, true) < 0) {
        syslog(LOG_ERR, "Set call waiting true fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_waiting_test(slot_id, "10010")) {
        syslog(LOG_ERR, "Waiting call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hangup_current_call_test(slot_id)) {
        syslog(LOG_ERR, "Hangup current call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_get_call_count(slot_id) != 1) {
        syslog(LOG_ERR, "Get call count fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup all call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (ss_set_and_get_call_waiting_test(0, false) < 0) {
        syslog(LOG_ERR, "Set call waiting false fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_hold_first_call_and_answer_second_call(int slot_id)
{
    int res = 0;
    if (ss_set_and_get_call_waiting_test(0, true) < 0) {
        syslog(LOG_ERR, "Set call waiting true fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_waiting_test(slot_id, "10010")) {
        syslog(LOG_ERR, "Waiting call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hold_and_answer_test(slot_id)) {
        syslog(LOG_ERR, "Hold and answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_get_two_call_state(slot_id) < 0) {
        syslog(LOG_ERR, "Get two call state fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_get_call_count(slot_id) != 2) {
        syslog(LOG_ERR, "Get call count fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup all call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (ss_set_and_get_call_waiting_test(0, false) < 0) {
        syslog(LOG_ERR, "Set call waiting false fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

// todo: 55
int call_dial_second_call_active_and_hangup_by_dialer(int slot_id)
{
    int ret1 = call_listen_call_test(slot_id);
    judge_data_init();
    test_case_data_init();
    judge_data.expect = NEW_CALL_INCOMING;

    // todo: incoming call

    if (judge() != 0 || judge_data.result != 0 || ret1)
        return -1;

    sleep(10);

    if (call_answer_call_test(slot_id, test_case_data.call_id) != 0)
        return -1;

    sleep(10);

    if (call_dial_test(slot_id, phone_num, 0))
        return -1;

    sleep(10);
    judge_data_init();
    judge_data.expect = CALL_STATE_CHANGE_TO_ACTIVE;

    // todo: caller answer

    if ((judge() || judge_data.result) != 0)
        return -1;

    sleep(10);

    if (call_hangup_current_call_test(slot_id) != 0)
        return -1;

    sleep(10);
    if (call_get_call_count(slot_id) != 1
        || get_current_call_state_test(slot_id) != CALL_STATUS_ACTIVE)
        return -1;

    int ret2 = call_hangup_all_test(slot_id);
    int ret3 = call_unlisten_call_test();

    return ret2 || ret3;
}

int call_dial_second_call_and_reject_by_caller(int slot_id)
{
    int res = 0;
    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Caller answer fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_dial_test(slot_id, "10010", 0)) {
        syslog(LOG_ERR, "Dial fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_reject_test(slot_id, "10010")) {
        syslog(LOG_ERR, "Caller hangup fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_get_call_count(slot_id) != 1) {
        syslog(LOG_ERR, "Get call count fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_dial_second_call_active_and_hangup_by_caller(int slot_id)
{
    int res = 0;
    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Caller answer fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_dial_test(slot_id, "10010", 0)) {
        syslog(LOG_ERR, "Dial fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_active_test(slot_id, "10010")) {
        syslog(LOG_ERR, "Caller active fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_reject_test(slot_id, "10010")) {
        syslog(LOG_ERR, "Caller hangup fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_get_call_count(slot_id) != 1) {
        syslog(LOG_ERR, "Get call count fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_hangup_hold_call_in_two_calls(int slot_id)
{
    int res = 0;
    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Caller answer fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_dial_test(slot_id, "10010", 0)) {
        syslog(LOG_ERR, "Dial fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_get_call_count(slot_id) != 2) {
        syslog(LOG_ERR, "Get call count fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (get_hold_call_id(slot_id) == NULL) {
        syslog(LOG_ERR, "Get hold call id fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (tapi_call_hangup_by_id(get_tapi_ctx(), slot_id, test_case_data.hold_call_id)) {
        syslog(LOG_ERR, "Hangup call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_get_call_count(slot_id) != 1) {
        syslog(LOG_ERR, "Get call count fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_swap_in_two_calling(int slot_id)
{
    int res = 0;
    if (ss_set_and_get_call_waiting_test(0, true) < 0) {
        syslog(LOG_ERR, "Set call waiting true fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_waiting_test(slot_id, "10010")) {
        syslog(LOG_ERR, "Waiting call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hold_and_answer_test(slot_id)) {
        syslog(LOG_ERR, "Hold and answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_get_call_count(slot_id) != 2) {
        syslog(LOG_ERR, "Get call count fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_get_two_call_state(slot_id) != 1) {
        syslog(LOG_ERR, "Get two call state fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_hold_call_test(slot_id)) {
        syslog(LOG_ERR, "Hold call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_get_call_count(slot_id) != 2) {
        syslog(LOG_ERR, "Get call count fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_get_two_call_state(slot_id) != 1) {
        syslog(LOG_ERR, "Get two call state fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_unhold_call_test(slot_id)) {
        syslog(LOG_ERR, "Unhold call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_get_call_count(slot_id) != 2) {
        syslog(LOG_ERR, "Get call count fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_get_two_call_state(slot_id) != 1) {
        syslog(LOG_ERR, "Get two call state fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup all call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (ss_set_and_get_call_waiting_test(0, false) < 0) {
        syslog(LOG_ERR, "Set call waiting false fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_dial_in_two_calling(int slot_id)
{
    int res = 0;

    if (ss_set_and_get_call_waiting_test(0, true) < 0) {
        syslog(LOG_ERR, "Set call waiting true fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_waiting_test(slot_id, "10010")) {
        syslog(LOG_ERR, "Waiting call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hold_and_answer_test(slot_id)) {
        syslog(LOG_ERR, "Hold and answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_get_call_count(slot_id) != 2) {
        syslog(LOG_ERR, "Get call count fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_dial_test(slot_id, "10000", 0) != -1) {
        syslog(LOG_ERR, "Dial third call error in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (ss_set_and_get_call_waiting_test(0, false) < 0) {
        syslog(LOG_ERR, "Set call waiting false fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_hangup_current_call_and_resume_call(int slot_id)
{
    int res = 0;
    if (ss_set_and_get_call_waiting_test(slot_id, true)) {
        syslog(LOG_ERR, "Set call waiting fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_answer_call_test(slot_id, test_case_data.call_id)) {
        syslog(LOG_ERR, "Answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_waiting_test(slot_id, "10010")) {
        syslog(LOG_ERR, "Waiting call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hold_and_answer_test(slot_id)) {
        syslog(LOG_ERR, "Hold and answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);

    if (call_get_call_count(slot_id) != 2) {
        syslog(LOG_ERR, "Get call count fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_get_two_call_state(slot_id) != 1) {
        syslog(LOG_ERR, "Get two call state fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (call_hangup_current_call_test(slot_id)) {
        syslog(LOG_ERR, "Hangup current call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_unhold_call_test(slot_id)) {
        syslog(LOG_ERR, "Unhold call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hangup_all_test(slot_id)) {
        syslog(LOG_ERR, "Hangup all call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (ss_set_and_get_call_waiting_test(slot_id, false)) {
        syslog(LOG_ERR, "Set call waiting fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

// todo: 63
int call_dial_third_call(int slot_id)
{
    int ret1 = call_listen_call_test(slot_id);
    judge_data_init();
    test_case_data_init();
    judge_data.expect = NEW_CALL_INCOMING;

    // todo: incoming call

    if ((ret1 || judge() || judge_data.result) != 0)
        return -1;

    sleep(10);

    if (call_answer_call_test(slot_id, test_case_data.call_id) != 0)
        return -1;
    sleep(10);
    judge_data_init();
    test_case_data_init();
    judge_data.expect = NEW_CALL_WAITING;

    // todo: incoming another call
    if ((judge() || judge_data.result) != 0)
        return -1;
    sleep(10);
    if (call_answer_call_test(slot_id, test_case_data.call_id))
        return -1;
    sleep(10);

    if (call_get_call_count(slot_id) != 2 || call_get_two_call_state(slot_id) != 1) {
        call_hangup_all_test(slot_id);
        call_unlisten_call_test();
        return -1;
    }

    int ret2 = call_dial_test(slot_id, phone_num, 0);

    if (!ret2) {
        call_hangup_all_test(slot_id);
        call_unlisten_call_test();
        return -1;
    }

    int ret3 = call_hangup_all_test(slot_id);
    int ret4 = call_unlisten_call_test();

    return ret3 || ret4;
}

int call_dial_and_remote_active(int slot_id, char* phone_number)
{
    int res = 0;
    if (call_dial_test(slot_id, phone_number, 0) < 0) {
        syslog(LOG_ERR, "Dail fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_active_test(slot_id, phone_number)) {
        syslog(LOG_ERR, "Remote call active fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hangup_current_call_test(slot_id)) {
        syslog(LOG_ERR, "Local hangup fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_dial_and_remote_hangup(int slot_id)
{
    int res = 0;
    if (call_dial_test(slot_id, phone_num, 0) < 0) {
        syslog(LOG_ERR, "Dail fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_reject_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call reject fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_incoming_and_local_hangup(int slot_id)
{
    int res = 0;
    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (call_hangup_current_call_test(slot_id)) {
        syslog(LOG_ERR, "Hangup call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_incoming_and_remote_hangup(int slot_id)
{
    int res = 0;
    if (remote_operation_call_incoming_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Incoming call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_reject_test(slot_id, phone_num)) {
        syslog(LOG_ERR, "Remote call reject fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int call_incoming_and_remote_hangup_for_times(int slot_id)
{
    int res = 0;
    char numbers[3][11] = { "10086", "10010", "10001" };
    for (int i = 0; i < 3; i++) {
        if (remote_operation_call_incoming_test(slot_id, numbers[i])) {
            syslog(LOG_ERR, "Incoming call %s fail in %s", phone_num, __func__);
            res = -1;
            goto on_exit;
        }
        sleep(3);

        if (remote_operation_call_reject_test(slot_id, numbers[i])) {
            syslog(LOG_ERR, "Remote call %s reject fail in %s", phone_num, __func__);
            res = -1;
            goto on_exit;
        }
        sleep(3);
    }

on_exit:
    return res;
}

int call_dial_in_active_test(int slot_id)
{
    int res = 0;

    if (call_dial_test(0, phone_num, 0)) {
        syslog(LOG_ERR, "Dial fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (remote_operation_call_active_test(0, phone_num)) {
        syslog(LOG_ERR, "Remote call active fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(1);
    if (get_current_call_state_test(0) != CALL_STATUS_ACTIVE) {
        syslog(LOG_ERR, "Current call state is not active in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int incoming_call_with_unexpected_status(int target_status)
{
    int res = 0;

    remote_incoming_call_state_change(0, phone_num, target_status);
    sleep(3);
    if (call_get_call_count(0) != 0) {
        res = -1;
    }

    return res;
}

#ifndef CONFIG_TELEPHONY_DFX
int check_call_dial_ecc_number_dfx(int slot_id)
{
    int res = 0;
    int watch_id = 0;

    watch_id = tapi_register(get_tapi_ctx(), 0, MSG_DATA_LOGING_IND,
        NULL, call_data_logging_cb);

    dfx_data_init();
    dfx_data.expected_dfx_count = 2;
    dfx_data.expected_dfx_value[0] = EVENT_DIAL_CALL_DFX_DONE;
    dfx_data.expected_dfx_value[1] = EVENT_DIAL_ECC_CALL_DFX_DONE;

    if (call_dial_test(slot_id, "911", 0)) {
        syslog(LOG_ERR, "Dial call execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (!check_dfx_value()) {
        syslog(LOG_ERR, "check_dfx_value for answer call fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    tapi_unregister(get_tapi_ctx(), watch_id);
    return res;
}
#endif
