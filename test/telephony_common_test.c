#include "telephony_common_test.h"
#include "remote_operation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_INIT_VALUE -100
// seconds
#define HANDLE_REPEAT_TIME 1
#define HANDLE_REPEAT_COUNT 10

#define MAX_MODEM_WATCH_COUNT 4
#define MSG_DATA_IND_START MSG_DATA_ENABLED_CHANGE_IND
#define MSG_NETWORK_IND_START MSG_NETWORK_STATE_CHANGE_IND
#define MSG_MODEM_IND_START MSG_RADIO_STATE_CHANGE_IND

static struct
{
    int radio_state_watch_id;
    int phone_state_watch_id;
    int modem_restart_ind_watch_id;
    int oem_hook_raw_watch_id;
    int modem_state;
    bool init_radio_power_state;
    int init_modem_state;
} modem_data;

extern struct judge_type judge_data;

extern bool response_flag[MAX_MESSAGE_COUNT];
extern int response_ret[MAX_MESSAGE_COUNT];

#ifndef CONFIG_TELEPHONY_DFX
extern struct dfx_judge_data dfx_data;
#endif

static void radio_signal_change(tapi_async_result* result);
static int hex_string_to_byte_array(char* hex_str, unsigned char* byte_arr, int arr_len);

static void tele_result_print(tapi_async_result* result)
{
    syslog(LOG_DEBUG, "result->msg_id : %d\n", result->msg_id);
    syslog(LOG_DEBUG, "result->status : %d\n", result->status);
    syslog(LOG_DEBUG, "result->arg1 : %d\n", result->arg1);
    syslog(LOG_DEBUG, "result->arg2 : %d\n", result->arg2);
}

static void tele_modem_async_fun_continuous(tapi_async_result* result)
{
    int event = result->msg_id;

    syslog(LOG_DEBUG, "%s : \n", __func__);
    tele_result_print(result);

    switch (event) {
    case EVENT_MODEM_ENABLE_DONE:
    case EVENT_RADIO_STATE_SET_DONE:
    case EVENT_MODEM_ACTIVITY_INFO_QUERY_DONE:
    case EVENT_MODEM_STATUS_QUERY_DONE:
    case EVENT_OEM_RIL_REQUEST_RAW_DONE:
    case EVENT_OEM_RIL_REQUEST_STRINGS_DONE:
        for (int i = 0; i < MAX_MESSAGE_COUNT; i++) {
            if (!response_flag[i] && response_ret[i] == result->msg_id) {
                syslog(LOG_DEBUG, "%s msg id : %d result err, return.\n", __func__, result->msg_id);
                response_flag[i] = TRUE;
                if (result->status != OK) {
                    response_ret[i] = -1;
                } else {
                    response_ret[i] = 0;
                }
                break;
            }
        }
        break;
    }
}

static void tele_call_async_fun(tapi_async_result* result)
{
    syslog(LOG_DEBUG, "%s : \n", __func__);
    tele_result_print(result);

    if (result->status != OK) {
        syslog(LOG_DEBUG, "%s msg id : %d result err, return.\n", __func__, result->msg_id);
    }

    int event = result->msg_id;
    int status = result->status;

    switch (event) {
    case EVENT_MODEM_ACTIVITY_INFO_QUERY_DONE:
        syslog(LOG_DEBUG, "%s: EVENT_MODEM_ACTIVITY_INFO_QUERY_DONE status: %d\n",
            __func__, result->status);
        if (judge_data.expect == EVENT_MODEM_ACTIVITY_INFO_QUERY_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_MODEM_ACTIVITY_INFO_QUERY_DONE;
        }
        break;
    case EVENT_MODEM_ENABLE_DONE:
        syslog(LOG_DEBUG, "%s: EVENT_MODEM_ENABLE_DONE status: %d\n",
            __func__, result->status);
        if (judge_data.expect == EVENT_MODEM_ENABLE_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_MODEM_ENABLE_DONE;
        }
        break;
    case EVENT_RADIO_STATE_SET_DONE:
        syslog(LOG_DEBUG, "%s: EVENT_RADIO_STATE_SET_DONE status: %d\n",
            __func__, result->status);
        if (judge_data.expect == EVENT_RADIO_STATE_SET_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_RADIO_STATE_SET_DONE;
        }
        break;
    case EVENT_OEM_RIL_REQUEST_RAW_DONE:
        syslog(LOG_DEBUG, "%s: EVENT_OEM_RIL_REQUEST_RAW_DONE status: %d\n",
            __func__, result->status);
        if (judge_data.expect == EVENT_OEM_RIL_REQUEST_RAW_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_OEM_RIL_REQUEST_RAW_DONE;
        }
        break;
    case EVENT_OEM_RIL_REQUEST_STRINGS_DONE:
        syslog(LOG_DEBUG, "%s: EVENT_OEM_RIL_REQUEST_STRINGS_DONE status: %d\n",
            __func__, result->status);
        if (judge_data.expect == EVENT_OEM_RIL_REQUEST_STRINGS_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_OEM_RIL_REQUEST_STRINGS_DONE;
        }
        break;
    case EVENT_MODEM_STATUS_QUERY_DONE:
        syslog(LOG_DEBUG, "%s: EVENT_MODEM_STATUS_QUERY_DONE status: %d\n",
            __func__, result->status);
        if (judge_data.expect == EVENT_MODEM_STATUS_QUERY_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_MODEM_STATUS_QUERY_DONE;
            modem_data.modem_state = result->arg2;
        }
        break;
    case EVENT_RAT_MODE_SET_DONE:
        syslog(LOG_DEBUG, "%s: EVENT_RAT_MODE_SET_DONE status: %d\n",
            __func__, result->status);
        if (judge_data.expect == EVENT_RAT_MODE_SET_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_RAT_MODE_SET_DONE;
        }
        break;
    case EVENT_MODEM_SET_SIGNAL_REPORT_THRESHOLD_DONE:
        syslog(LOG_DEBUG, "%s: EVENT_MODEM_SET_SIGNAL_REPORT_THRESHOLD_DONE status: %d\n",
            __func__, result->status);
        if (judge_data.expect == EVENT_MODEM_SET_SIGNAL_REPORT_THRESHOLD_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_MODEM_SET_SIGNAL_REPORT_THRESHOLD_DONE;
        }
        break;
    case EVENT_MODEM_SUPPRESS_MESSAGE_REPORT_DONE:
        syslog(LOG_DEBUG, "%s: EVENT_MODEM_SUPPRESS_MESSAGE_REPORT_DONE status: %d\n",
            __func__, result->status);
        if (judge_data.expect == EVENT_MODEM_SUPPRESS_MESSAGE_REPORT_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_MODEM_SUPPRESS_MESSAGE_REPORT_DONE;
        }
        break;
    case EVENT_MODEM_ENABLE_MODEM_STATIONARY_DONE:
        syslog(LOG_DEBUG, "%s: EVENT_MODEM_SET_DEVICE_STATIONARY_DONE status: %d\n",
            __func__, result->status);
        if (judge_data.expect == EVENT_MODEM_ENABLE_MODEM_STATIONARY_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_MODEM_ENABLE_MODEM_STATIONARY_DONE;
        }
        break;
    case EVENT_MODEM_SET_MODEM_STATIONARY_THRESHOLD_DONE:
        syslog(LOG_DEBUG, "%s: EVENT_MODEM_SET_DEVICE_STATIONARY_THRESHOLD_DONE status: %d\n",
            __func__, result->status);
        if (judge_data.expect == EVENT_MODEM_SET_MODEM_STATIONARY_THRESHOLD_DONE) {
            judge_data.result = status;
            judge_data.flag = EVENT_MODEM_SET_MODEM_STATIONARY_THRESHOLD_DONE;
        }
        break;
    default:
        break;
    }
}

int setup_modem(void** state)
{
    (void)state;
    int ret = 0;

    if (modem_register_test(0)) {
        syslog(LOG_ERR, "Modem register execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (get_modem_status_test(0, &modem_data.init_modem_state)) {
        syslog(LOG_DEBUG, "Get modem status execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (get_radio_power_test(0, &modem_data.init_radio_power_state)) {
        syslog(LOG_DEBUG, "Get radio power execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (modem_keep_status_as_expected_test(0, true)) {
        syslog(LOG_ERR, "Modem enable execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (set_radio_power_test(0, true)) {
        syslog(LOG_ERR, "Modem set radio power execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    return ret;
}

int setup_radio(void** state)
{
    (void)state;
    int ret = 0;

    if (modem_register_test(0)) {
        syslog(LOG_ERR, "Modem register execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (get_radio_power_test(0, &modem_data.init_radio_power_state)) {
        syslog(LOG_DEBUG, "Get radio power execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (set_radio_power_test(0, true)) {
        syslog(LOG_ERR, "Modem set radio power execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    return ret;
}

int teardown_modem(void** state)
{
    (void)state;
    int ret = 0;

    if (modem_data.init_modem_state == 0) {
        if (modem_keep_status_as_expected_test(0, false)) {
            syslog(LOG_ERR, "Modem disable execute fail in %s", __func__);
            ret = -1;
            goto on_exit;
        }
    } else {
        if (modem_keep_status_as_expected_test(0, true)) {
            syslog(LOG_ERR, "Modem disable execute fail in %s", __func__);
            ret = -1;
            goto on_exit;
        }
    }

    if (set_radio_power_test(0, modem_data.init_radio_power_state)) {
        syslog(LOG_ERR, "Modem set radio power execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (modem_unregister_test()) {
        syslog(LOG_ERR, "Modem unregister execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    return ret;
}

int teardown_radio(void** state)
{
    (void)state;
    int ret = 0;

    if (set_radio_power_test(0, modem_data.init_radio_power_state)) {
        syslog(LOG_ERR, "Modem set radio power execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (modem_unregister_test()) {
        syslog(LOG_ERR, "Modem unregister execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    return ret;
}

int get_imei_test(int slot_id)
{
    char imei[MAX_IMEI_LENGTH + 1] = { 0 };

    int ret = tapi_get_imei(get_tapi_ctx(), slot_id, imei, sizeof(imei));
    syslog(LOG_DEBUG, "%s, slotId : %d imei : %s \n", __func__, slot_id, imei);

    return ret;
}

int get_modem_revision_test(int slot_id)
{
    char* version = NULL;

    int ret = tapi_get_modem_revision(get_tapi_ctx(), slot_id, &version);
    syslog(LOG_DEBUG, "%s, slotId : %d version : %s \n", __func__, slot_id, version);
    ret = ret || (!version);

    free(version);
    return ret;
}

int get_pref_net_mode_test(int slot_id, tapi_pref_net_mode* value)
{
    int ret = tapi_get_pref_net_mode(get_tapi_ctx(), slot_id, value);
    syslog(LOG_DEBUG, "%s, slotId : %d value :%d \n", __func__, slot_id, *value);
    return ret;
}

int get_phone_state_test(int slot_id, tapi_phone_state target)
{
    tapi_phone_state state;
    int ret = tapi_get_phone_state(get_tapi_ctx(), slot_id, &state);
    syslog(LOG_DEBUG, "%s, slotId : %d state :%d \n", __func__, slot_id, state);
    return ret || (state != target);
}

int radio_power_on_off_pending_test(int slot_id)
{
    int res = 0;
    bool target_state = true;
    int ret = 0;

    // precondition
    remote_radio_on_off_delay(1);
    modem_enable_status_test(0);

    init_response_flag(MID_MESSAGE_COUNT);
    for (int i = 0; i < MID_MESSAGE_COUNT; i++) {
        if (i % 2 == 0) {
            target_state = false;
        } else {
            target_state = true;
        }

        response_ret[i] = EVENT_RADIO_STATE_SET_DONE;
        ret = tapi_set_radio_power(get_tapi_ctx(), slot_id,
            EVENT_RADIO_STATE_SET_DONE, target_state, tele_modem_async_fun_continuous);
        if (ret) {
            syslog(LOG_ERR, "execute fail in %s, ret: %d",
                __func__, ret);
            res = -1;
            goto on_exit;
        }
        sleep(5);
    }
    if (wait_response(MID_MESSAGE_COUNT) != 0) {
        syslog(LOG_ERR, "tele_call_async_fun is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    // restore normal status
    remote_radio_on_off_delay(0);
    set_radio_power_test(slot_id, true);

    return res;
}

int radio_power_on_modem_disable_pending_test(int slot_id)
{
    int res = 0;
    int ret = 0;

    // precondition
    remote_radio_on_off_delay(1);
    if (set_radio_power_test(0, 0)) {
        syslog(LOG_ERR, "precondition set fail");
        res = -1;
        goto on_exit;
    }
    // test
    init_response_flag(MIN_MESSAGE_COUNT);
    for (int i = 0; i < MIN_MESSAGE_COUNT; i++) {
        if (i == 0) {
            response_ret[i] = EVENT_RADIO_STATE_SET_DONE;
            ret = tapi_set_radio_power(get_tapi_ctx(), slot_id,
                EVENT_RADIO_STATE_SET_DONE, 1, tele_modem_async_fun_continuous);
            if (ret) {
                syslog(LOG_ERR, "execute fail in %s, ret: %d",
                    __func__, ret);
                res = -1;
                goto on_exit;
            }
        } else {
            response_ret[i] = EVENT_MODEM_ENABLE_DONE;
            ret = tapi_enable_modem(get_tapi_ctx(), slot_id,
                EVENT_MODEM_ENABLE_DONE, 0, tele_modem_async_fun_continuous);
            if (ret) {
                syslog(LOG_ERR, "execute fail in %s, ret: %d",
                    __func__, ret);
                res = -1;
                goto on_exit;
            }
        }
    }

    if (wait_response(MIN_MESSAGE_COUNT) != 0) {
        syslog(LOG_ERR, "tele_modem_async_fun_continuous is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }
on_exit:
    remote_radio_on_off_delay(0);
    return res;
}

int modem_disable_power_off_pending_test(int slot_id)
{
    int res = 0;
    int ret = 0;
    int random_num = 0;

    remote_radio_on_off_delay(1);
    srand(time(NULL)); // 初始化随机数种子
    // precondition
    if (modem_enable_status_test(0)) {
        syslog(LOG_ERR, "precondition set fail");
        res = -1;
        goto on_exit;
    }
    // testing
    init_response_flag(MIN_MESSAGE_COUNT);

    for (int i = 0; i < MIN_MESSAGE_COUNT; i++) {
        if (i == 0) {
            random_num = rand() % 4; // 0-3
            switch (random_num) {
            case 0:
                response_ret[i] = EVENT_MODEM_ACTIVITY_INFO_QUERY_DONE;
                ret = tapi_get_modem_activity_info(get_tapi_ctx(), slot_id,
                    EVENT_MODEM_ACTIVITY_INFO_QUERY_DONE, tele_modem_async_fun_continuous);
                if (ret) {
                    syslog(LOG_ERR, "execute fail in %s, ret: %d",
                        __func__, ret);
                    res = -1;
                    goto on_exit;
                }
                break;
            case 1:
                response_ret[i] = EVENT_MODEM_STATUS_QUERY_DONE;
                ret = tapi_get_modem_status(get_tapi_ctx(), slot_id,
                    EVENT_MODEM_STATUS_QUERY_DONE, tele_modem_async_fun_continuous);
                if (ret) {
                    syslog(LOG_ERR, "execute fail in %s, ret: %d",
                        __func__, ret);
                    res = -1;
                    goto on_exit;
                }

                break;
            case 2:
                response_ret[i] = EVENT_OEM_RIL_REQUEST_RAW_DONE;
                unsigned char req_data[MAX_INPUT_ARGS_LEN];
                hex_string_to_byte_array("01A0B023", req_data, MAX_INPUT_ARGS_LEN);
                ret = tapi_invoke_oem_ril_request_raw(get_tapi_ctx(), slot_id,
                    EVENT_OEM_RIL_REQUEST_RAW_DONE, req_data, 4, tele_modem_async_fun_continuous);

                if (ret) {
                    syslog(LOG_ERR, "execute fail in %s, ret: %d",
                        __func__, ret);
                    res = -1;
                    goto on_exit;
                }

                break;
            case 3:
            default:

                char* oem_req[MAX_OEM_RIL_RESP_STRINGS_LENTH];
                oem_req[0] = "AT+CPIN?";
                response_ret[i] = EVENT_OEM_RIL_REQUEST_STRINGS_DONE;
                ret = tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), slot_id,
                    EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, 1, tele_modem_async_fun_continuous);
                if (ret) {
                    syslog(LOG_ERR, "execute fail in %s, ret: %d",
                        __func__, ret);
                    res = -1;
                    goto on_exit;
                }

                break;
            }

        } else {
            random_num = rand() % 2; // 0-1
            if (random_num == 0) {
                response_ret[i] = EVENT_RADIO_STATE_SET_DONE;
                ret = tapi_set_radio_power(get_tapi_ctx(), slot_id,
                    EVENT_RADIO_STATE_SET_DONE, 0, tele_modem_async_fun_continuous);
                if (ret) {
                    syslog(LOG_ERR, "execute fail in %s, ret: %d",
                        __func__, ret);
                    res = -1;
                    goto on_exit;
                }
            } else {
                response_ret[i] = EVENT_MODEM_ENABLE_DONE;
                ret = tapi_enable_modem(get_tapi_ctx(), slot_id,
                    EVENT_MODEM_ENABLE_DONE, 0, tele_modem_async_fun_continuous);
                if (ret) {
                    syslog(LOG_ERR, "execute fail in %s, ret: %d",
                        __func__, ret);
                    res = -1;
                    goto on_exit;
                }
            }
        }
    }

    if (wait_response(MIN_MESSAGE_COUNT) != 0) {
        syslog(LOG_ERR, "tele_modem_async_fun_continuous is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    remote_radio_on_off_delay(0);
    if (random_num == 0) {
        tapi_set_radio_power(get_tapi_ctx(), slot_id,
            EVENT_RADIO_STATE_SET_DONE, 1, NULL);
    } else {
        tapi_enable_modem(get_tapi_ctx(), slot_id,
            EVENT_MODEM_ENABLE_DONE, 1, NULL);
    }

    return res;
}

int set_radio_power_test(int slot_id, bool target_state)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_RADIO_STATE_SET_DONE;

    int ret = tapi_set_radio_power(get_tapi_ctx(), slot_id,
        EVENT_RADIO_STATE_SET_DONE, target_state, tele_call_async_fun);

    if (ret) {
        syslog(LOG_ERR, "set_radio_power_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "set_radio_power_test is not executed in %s", __func__);
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

int get_radio_power_test(int slot_id, bool* value)
{
    int ret = tapi_get_radio_power(get_tapi_ctx(), slot_id, value);
    syslog(LOG_DEBUG, "%s, slotId : %d value : %d \n", __func__, slot_id, *value);
    return ret;
}

/* only once set radio power off then on. */
int set_radio_power_off_then_on(int slot_id)
{
    int ret;

    ret = set_radio_power_test(slot_id, 0);
    if (ret) {
        syslog(LOG_ERR, "set radio off test execute fail in %s", __func__);
        return -1;
    }
    sleep(5);

    ret = set_radio_power_test(slot_id, 1);
    if (ret) {
        syslog(LOG_ERR, "set radio on test execute fail in %s", __func__);
        return -1;
    }
    sleep(3);

    return ret;
}

int modem_register_test(int slot_id)
{
    modem_data.radio_state_watch_id = -1;
    modem_data.radio_state_watch_id = tapi_register(get_tapi_ctx(),
        slot_id, MSG_RADIO_STATE_CHANGE_IND, NULL, radio_signal_change);
    if (modem_data.radio_state_watch_id < 0) {
        syslog(LOG_ERR, "%s, radio state change registered fail, ret: %d", __func__,
            modem_data.radio_state_watch_id);
        return -1;
    }

    modem_data.phone_state_watch_id = -1;
    modem_data.phone_state_watch_id = tapi_register(get_tapi_ctx(),
        slot_id, MSG_PHONE_STATE_CHANGE_IND, NULL, radio_signal_change);
    if (modem_data.phone_state_watch_id < 0) {
        syslog(LOG_ERR, "%s, phone state change registered fail, ret: %d", __func__,
            modem_data.phone_state_watch_id);
        return -1;
    }

    modem_data.modem_restart_ind_watch_id = -1;
    modem_data.modem_restart_ind_watch_id = tapi_register(get_tapi_ctx(),
        slot_id, MSG_MODEM_RESTART_IND, NULL, radio_signal_change);
    if (modem_data.modem_restart_ind_watch_id < 0) {
        syslog(LOG_ERR, "%s, modem restart ind registered fail, ret: %d", __func__,
            modem_data.modem_restart_ind_watch_id);
        return -1;
    }

    modem_data.oem_hook_raw_watch_id = -1;
    modem_data.oem_hook_raw_watch_id = tapi_register(get_tapi_ctx(),
        slot_id, MSG_OEM_HOOK_RAW_IND, NULL, radio_signal_change);
    if (modem_data.oem_hook_raw_watch_id < 0) {
        syslog(LOG_ERR, "%s, oem hook raw registered fail, ret: %d", __func__,
            modem_data.oem_hook_raw_watch_id);
        return -1;
    }

    return 0;
}

int modem_unregister_test(void)
{
    int ret = -1, res = 0;
    ret = tapi_unregister(get_tapi_ctx(), modem_data.radio_state_watch_id);
    if (ret) {
        syslog(LOG_ERR, "unregister radio state change fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    ret = tapi_unregister(get_tapi_ctx(), modem_data.phone_state_watch_id);
    if (ret) {
        syslog(LOG_ERR, "unregister phone state change fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    ret = tapi_unregister(get_tapi_ctx(), modem_data.modem_restart_ind_watch_id);
    if (ret) {
        syslog(LOG_ERR, "unregister modem restart change fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    ret = tapi_unregister(get_tapi_ctx(), modem_data.oem_hook_raw_watch_id);
    if (ret) {
        syslog(LOG_ERR, "unregister oem hook raw fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

static int hex_string_to_byte_array(char* hex_str, unsigned char* byte_arr, int arr_len)
{
    char* str;
    int len;
    int i, j;

    if (hex_str == NULL)
        return -EINVAL;

    len = strlen(hex_str);
    if (!len || (len % 2) != 0 || len > arr_len * 2)
        return -EINVAL;

    str = strdup(hex_str);

    for (i = 0, j = 0; i < len; i += 2, j++) {
        // uppercase char 'a'~'f'
        if (str[i] >= 'a' && str[i] <= 'f')
            str[i] = str[i] & ~0x20;

        if (str[i + 1] >= 'a' && str[i + 1] <= 'f')
            str[i + 1] = str[i + 1] & ~0x20;

        // convert the first character to decimal.
        if (str[i] >= 'A' && str[i] <= 'F')
            byte_arr[j] = (str[i] - 'A' + 10) << 4;
        else if (str[i] >= '0' && str[i] <= '9')
            byte_arr[j] = (str[i] & ~0x30) << 4;
        else
            return -EINVAL;

        // convert the second character to decimal
        // and combine with the previous decimal.
        if (str[i + 1] >= 'A' && str[i + 1] <= 'F')
            byte_arr[j] |= (str[i + 1] - 'A' + 10);
        else if (str[i + 1] >= '0' && str[i + 1] <= '9')
            byte_arr[j] |= (str[i + 1] & ~0x30);
        else
            return -EINVAL;
    }

    free(str);
    return 0;
}

int modem_invoke_oem_ril_request_raw_test(int slot_id, char* oem_req, int length)
{
    int res = 0;
    unsigned char req_data[MAX_INPUT_ARGS_LEN];
    hex_string_to_byte_array(oem_req, req_data, MAX_INPUT_ARGS_LEN);
    judge_data_init();
    judge_data.expect = EVENT_OEM_RIL_REQUEST_RAW_DONE;

    int ret = tapi_invoke_oem_ril_request_raw(get_tapi_ctx(), slot_id,
        EVENT_OEM_RIL_REQUEST_RAW_DONE, req_data, length, tele_call_async_fun);

    if (ret) {
        syslog(LOG_ERR, "modem_invoke_oem_ril_request_raw_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "modem_invoke_oem_ril_request_raw_test is not executed in %s", __func__);
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

int modem_invoke_oem_ril_request_strings_test(int slot_id, char* req_data, int length)
{
    char* result;
    char* ptr = NULL;
    // FIXME: oem_req should be inited
    char* oem_req[MAX_OEM_RIL_RESP_STRINGS_LENTH] = { 0 };
    int count = 0;
    int res = 0;
    result = strtok_r(req_data, "|", &ptr);
    while (result != NULL) {
        if (count < length)
            oem_req[count] = result;

        count++;
        result = strtok_r(NULL, "|", &ptr);
    }

    judge_data_init();
    judge_data.expect = EVENT_OEM_RIL_REQUEST_STRINGS_DONE;

    int ret = tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), slot_id,
        EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, length, tele_call_async_fun);

    if (ret) {
        syslog(LOG_ERR, "modem_invoke_oem_ril_request_strings_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "modem_invoke_oem_ril_request_strings_test is not executed in %s", __func__);
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

int get_modem_activity_info_test(int slot_id)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_MODEM_ACTIVITY_INFO_QUERY_DONE;

    int ret = tapi_get_modem_activity_info(get_tapi_ctx(), slot_id,
        EVENT_MODEM_ACTIVITY_INFO_QUERY_DONE, tele_call_async_fun);

    if (ret) {
        syslog(LOG_ERR, "get_modem_activity_info_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "get_modem_activity_info_test is not executed in %s", __func__);
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

int enable_modem_test(int slot_id, bool target_state)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_MODEM_ENABLE_DONE;

    int ret = tapi_enable_modem(get_tapi_ctx(), slot_id,
        EVENT_MODEM_ENABLE_DONE, target_state, tele_call_async_fun);

    if (ret) {
        syslog(LOG_ERR, "enable_modem_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "enable_modem_test is not executed in %s", __func__);
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

int modem_enable_disable_pending_test(int slot_id)
{
    int res = 0;
    int target_state = 0;

    // precondition
    tapi_enable_modem(get_tapi_ctx(), slot_id,
        EVENT_MODEM_ENABLE_DONE, 0, NULL);
    sleep(2);

    init_response_flag(MID_MESSAGE_COUNT);

    for (int i = 0; i < MID_MESSAGE_COUNT; i++) {
        target_state = !target_state;
        response_ret[i] = EVENT_MODEM_ENABLE_DONE;
        int ret = tapi_enable_modem(get_tapi_ctx(), slot_id,
            EVENT_MODEM_ENABLE_DONE, target_state, tele_modem_async_fun_continuous);
        if (ret) {
            syslog(LOG_ERR, "execute fail in %s, ret: %d",
                __func__, ret);
            res = -1;
            goto on_exit;
        }
        if (i == 2)
            sleep(3);
    }

    if (wait_response(MID_MESSAGE_COUNT) != 0) {
        syslog(LOG_ERR, "tele_modem_async_fun_countinuous is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int modem_reset_test(int slot_id)
{
    int res = 0;
    if (enable_modem_test(0, 0)) {
        syslog(LOG_DEBUG, "Modem disable execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
    sleep(10);

    if (enable_modem_test(0, 1)) {
        syslog(LOG_DEBUG, "Modem enbale execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
    sleep(10);

on_exit:
    return res;
}

int get_modem_status_test(int slot_id, int* state)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_MODEM_STATUS_QUERY_DONE;
    modem_data.modem_state = -1;

    int ret = tapi_get_modem_status(get_tapi_ctx(), slot_id,
        EVENT_MODEM_STATUS_QUERY_DONE, tele_call_async_fun);

    if (ret) {
        syslog(LOG_ERR, "get_modem_status_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "get_modem_status_test is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

    *state = modem_data.modem_state;

on_exit:
    return res;
}

int set_pref_net_mode_test(int slot_id, tapi_pref_net_mode target_state)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = EVENT_RAT_MODE_SET_DONE;

    int ret = tapi_set_pref_net_mode(get_tapi_ctx(), slot_id,
        EVENT_RAT_MODE_SET_DONE, target_state, tele_call_async_fun);

    if (ret) {
        syslog(LOG_ERR, "set_pref_net_mode_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "set_pref_net_mode_test is not executed in %s", __func__);
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

static void radio_signal_change(tapi_async_result* result)
{
    int signal = result->msg_id;
    int slot_id = result->arg1;
    int param = result->arg2;

    switch (signal) {
    case MSG_RADIO_STATE_CHANGE_IND:
        syslog(LOG_DEBUG, "radio state changed to %d in slot[%d] \n", param, slot_id);
        if (judge_data.expect == MSG_RADIO_STATE_CHANGE_IND) {
            judge_data.result = OK;
        }
        break;
    case MSG_PHONE_STATE_CHANGE_IND:
        syslog(LOG_DEBUG, "phone state changed to %d in slot[%d] \n", param, slot_id);
        judge_data.phone_state_value = param;
        if (judge_data.expect == MSG_PHONE_STATE_CHANGE_IND) {
            judge_data.result = OK;
        }
        break;
    case MSG_MODEM_RESTART_IND:
        syslog(LOG_DEBUG, "modem restart in slot[%d] \n", slot_id);
        if (judge_data.expect == MSG_MODEM_RESTART_IND) {
            judge_data.result = OK;
        }
        break;
    case MSG_OEM_HOOK_RAW_IND:
        syslog(LOG_DEBUG, "oem hook raw in slot[%d] \n", slot_id);
        if (judge_data.expect == MSG_OEM_HOOK_RAW_IND) {
            judge_data.result = OK;
        }
        break;
    default:
        break;
    }
}

int modem_enable_status_test(int slot_id)
{
    int res = 0;
    int real_state = 0;

    if (get_modem_status_test(slot_id, &real_state)) {
        syslog(LOG_DEBUG, "Get modem status execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (real_state == 1) {
        if (enable_modem_test(0, 0)) {
            syslog(LOG_DEBUG, "Modem disable execute fail in %s", __func__);
            res = -1;
            goto on_exit;
        }
        sleep(10);
    }

    if (enable_modem_test(0, 1)) {
        syslog(LOG_DEBUG, "Modem enable execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
    sleep(10);

on_exit:
    return res;
}

int modem_disable_status_test(int slot_id)
{
    int res = 0;
    int real_state = 0;

    if (get_modem_status_test(slot_id, &real_state)) {
        syslog(LOG_DEBUG, "Get modem status execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (real_state == 0) {
        if (enable_modem_test(0, 1)) {
            syslog(LOG_DEBUG, "Modem enable execute fail in %s", __func__);
            res = -1;
            goto on_exit;
        }
        sleep(10);
    }

    if (enable_modem_test(0, 0)) {
        syslog(LOG_DEBUG, "Modem disable execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
    sleep(10);

on_exit:
    return res;
}

int modem_keep_status_as_expected_test(int slot_id, bool expected_state)
{
    int res = 0;
    int real_state = 0;

    if (get_modem_status_test(slot_id, &real_state)) {
        syslog(LOG_DEBUG, "Get modem status execute fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (real_state != expected_state) {
        if (enable_modem_test(0, expected_state)) {
            syslog(LOG_DEBUG, "Modem %s execute fail in %s", expected_state == 1 ? "enable" : "disable", __func__);
            res = -1;
            goto on_exit;
        }
        sleep(10);
    }

on_exit:
    return res;
}

int set_signal_report_threshold_test(int slot_id, int type)
{
    int res = 0;

    judge_data_init();
    judge_data.expect = EVENT_MODEM_SET_SIGNAL_REPORT_THRESHOLD_DONE;

    int ret = tapi_set_signal_report_threshold(get_tapi_ctx(), slot_id,
        EVENT_MODEM_SET_SIGNAL_REPORT_THRESHOLD_DONE, type, tele_call_async_fun);

    if (ret) {
        syslog(LOG_ERR, "set_signal_report_threshold_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "set_signal_report_threshold_test is not executed in %s", __func__);
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

int suppress_message_report(int slot_id, bool target_state)
{
    int res = 0;

    judge_data_init();
    judge_data.expect = EVENT_MODEM_SUPPRESS_MESSAGE_REPORT_DONE;

    int ret = tapi_suppress_message_report(get_tapi_ctx(), slot_id,
        EVENT_MODEM_SUPPRESS_MESSAGE_REPORT_DONE, target_state, tele_call_async_fun);

    if (ret) {
        syslog(LOG_ERR, "suppress_message_report execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "suppress_message_report is not executed in %s", __func__);
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

int enable_modem_stationary(int slot_id, bool target_state)
{
    int res = 0;

    judge_data_init();
    judge_data.expect = EVENT_MODEM_ENABLE_MODEM_STATIONARY_DONE;

    int ret = tapi_enable_modem_stationary(get_tapi_ctx(), slot_id,
        EVENT_MODEM_ENABLE_MODEM_STATIONARY_DONE, target_state, tele_call_async_fun);

    if (ret) {
        syslog(LOG_ERR, "enable_modem_stationary execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "enable_modem_stationary is not executed in %s", __func__);
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

int set_modem_stationary_threshold(int slot_id, int value)
{
    int res = 0;

    judge_data_init();
    judge_data.expect = EVENT_MODEM_SET_MODEM_STATIONARY_THRESHOLD_DONE;

    int ret = tapi_set_modem_stationary_threshold(get_tapi_ctx(), slot_id,
        EVENT_MODEM_SET_MODEM_STATIONARY_THRESHOLD_DONE, value, tele_call_async_fun);

    if (ret) {
        syslog(LOG_ERR, "set_modem_stationary_threshold execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "set_modem_stationary_threshold is not executed in %s", __func__);
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

#ifndef CONFIG_TELEPHONY_DFX
static void deal_with_abnormal_data(char* data, int id)
{
    char* token;
    int index = 0;
    int type_id = -1;
    char type_id_str[50] = { 0 };

    if (strstr(data, "DATA_INTERRUPTION_INFO") != NULL) {
        return;
    }
    token = strtok(data, ",");
    while (token != NULL) {
        if (index == 1) {
            strncpy(type_id_str, token, 50);
            break;
        }
        index++;
        token = strtok(NULL, ",");
    }
    syslog(LOG_DEBUG, "type_id_str:%s", type_id_str);
    index = 0;
    token = strtok(type_id_str, "_");
    while (token != NULL) {
        if (index == 0) {
            type_id = atoi(token);
            break;
        }
        index++;
        token = strtok(NULL, ",");
    }
    syslog(LOG_DEBUG, "type_id:%d", type_id);

    if (type_id == id) {
        judge_data.result = 0;
        judge_data.flag = EVENT_MODEM_CHECK_ABNORMAL_EVENT_REPORT_DONE;
    } else {
        judge_data.result = -1;
        judge_data.flag = EVENT_MODEM_CHECK_ABNORMAL_EVENT_REPORT_DONE;
    }
}

static void common_data_logging_cb(tapi_async_result* result)
{
    char* data = (char*)result->data;

    if (result->status != OK) {
        syslog(LOG_ERR, "common_data_logging_cb fail,status =%d", result->status);
        return;
    }
    syslog(LOG_DEBUG, "data_logging:%s", data);

    for (int i = 0; i < dfx_data.expected_dfx_count; i++) {
        if (dfx_data.received_dfx_flag[i]) {
            continue;
        }
        syslog(LOG_DEBUG, "expected_dfx_value:%d", dfx_data.expected_dfx_value[i]);
        switch (dfx_data.expected_dfx_value[i]) {
        case EVENT_MODEM_CHECK_ABNORMAL_EVENT_REPORT_DONE:
            deal_with_abnormal_data(data, *((int*)result->user_obj));
            break;
        case EVENT_OOS_DFX_DONE:
            if (strstr(data, "OOS_INFO,915300004")) {
                dfx_data.received_dfx_flag[i] = true;
                if (dfx_data.expected_dfx_count == i + 1) {
                    judge_data.result = 0;
                    judge_data.flag = EVENT_OOS_DFX_DONE;
                }
            }
            break;
        case EVENT_DISABLE_MODEM_DFX_DONE:
            if (strstr(data, "RAT_DURATION") || strstr(data, "DATA_ACTIVE_DURATION")
                || strstr(data, "SIGNAL_LEVEL_DURATION") || strstr(data, "IMS_DURATION")
                || strstr(data, "IMS_STATE_CHANGED_COUNT") || strstr(data, "CELLINFO_CHANGED_COUNT")
                || strstr(data, "NETWORK_SIGNAL_CHANGED_COUNT")) {
                dfx_data.received_dfx_flag[i] = true;
                if (dfx_data.expected_dfx_count == i + 1) {
                    judge_data.result = 0;
                    judge_data.flag = EVENT_DISABLE_MODEM_DFX_DONE;
                }
            }
            break;
        default:
            syslog(LOG_ERR, "unexpected data logging info:%s", data);
            break;
        }
        return;
    }
}

int check_abnormal_event_report(bool unexpected_data_flag, int abnormal_data_type_id)
{
    int res = 0;
    int watch_id = 0;
    int* type_id = (int*)malloc(sizeof(int));

    judge_data_init();
    judge_data.expect = EVENT_MODEM_CHECK_ABNORMAL_EVENT_REPORT_DONE;

    *type_id = abnormal_data_type_id;
    watch_id = tapi_register(get_tapi_ctx(), 0, MSG_DATA_LOGING_IND,
        type_id, common_data_logging_cb);

    dfx_data_init();
    dfx_data.expected_dfx_count = 1;
    dfx_data.expected_dfx_value[0] = EVENT_MODEM_CHECK_ABNORMAL_EVENT_REPORT_DONE;
    if (unexpected_data_flag) {
        remote_unexpected_abnormal_event_report();
    } else {
        remote_abnormal_event_report(*type_id);
    }

    if (judge()) {
        if (!unexpected_data_flag) {
            syslog(LOG_DEBUG, "check_abnormal_event_report is not executed in %s", __func__);
            res = -1;
        }
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    free(type_id);
    tapi_unregister(get_tapi_ctx(), watch_id);
    return res;
}

int check_oos_dfx(void)
{
    int ret = 0;
    int watch_id = 0;

    dfx_data_init();
    dfx_data.expected_dfx_count = 2;
    dfx_data.expected_dfx_value[0] = EVENT_OOS_DFX_DONE;
    dfx_data.expected_dfx_value[1] = EVENT_OOS_DFX_DONE;
    judge_data_init();
    judge_data.expect = EVENT_OOS_DFX_DONE;

    watch_id = tapi_register(get_tapi_ctx(), 0, MSG_DATA_LOGING_IND,
        NULL, common_data_logging_cb);

    if (remote_trigger_oos(0)) {
        syslog(LOG_ERR, "Remote trigger oos fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }
    if (remote_trigger_oos(1)) {
        syslog(LOG_ERR, "Remote trigger oos fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "the callback function not executed in %s", __func__);
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

on_exit:
    tapi_unregister(get_tapi_ctx(), watch_id);
    return ret;
}

int check_disable_modem_duration_dfx(void)
{
    int ret = 0;
    int watch_id = 0;

    dfx_data_init();
    dfx_data.expected_dfx_count = 7;
    for (int i = 0; i < 7; i++) {
        dfx_data.expected_dfx_value[i] = EVENT_DISABLE_MODEM_DFX_DONE;
    }
    judge_data_init();
    judge_data.expect = EVENT_DISABLE_MODEM_DFX_DONE;

    watch_id = tapi_register(get_tapi_ctx(), 0, MSG_DATA_LOGING_IND,
        NULL, common_data_logging_cb);

    sleep(10);

    if (enable_modem_test(0, 0)) {
        syslog(LOG_DEBUG, "Modem disable execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_ERR, "the callback function is not executed in %s", __func__);
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

on_exit:
    tapi_unregister(get_tapi_ctx(), watch_id);
    return ret;
}
#endif

static void modem_upgrade_action_cb(tapi_async_result* result)
{
    int signal = result->msg_id;
    int slot_id = result->arg1;
    int param = result->arg2;
    int status = result->status;

    switch (signal) {
    case MSG_MODEM_UPGRADE_STATE_IND:
        syslog(LOG_DEBUG, "modem_upgrade_state_cb arg2=%d in slot[%d] \n", param, slot_id);
        if (judge_data.expect == EVENT_MODEM_UPGRADE_STATE_TWO_PARM_IND) {
            int param2 = *((int*)result->data);
            syslog(LOG_DEBUG, "modem_upgrade_state_cb ext_info=%d", param2);
            if (param2 == -1) {
                judge_data.result = OK;
                judge_data.flag = EVENT_MODEM_UPGRADE_STATE_TWO_PARM_IND;
            } else {
                judge_data.result = ERROR;
            }
        } else if (judge_data.expect == EVENT_MODEM_UPGRADE_STATE_ONE_PARM_IND) {
            judge_data.result = OK;
            judge_data.flag = EVENT_MODEM_UPGRADE_STATE_ONE_PARM_IND;
        } else {
            syslog(LOG_DEBUG, "unexpected event in %s", __func__);
            judge_data.result = ERROR;
        }
        break;
    case EVENT_CHECK_MODEM_UPGRADE_STATE_DONE:
        syslog(LOG_DEBUG, "modem_upgrade_state_cb arg2= %d in slot[%d],status=%d \n", param, slot_id, status);
        if (judge_data.expect == EVENT_CHECK_MODEM_UPGRADE_STATE_DONE) {
            judge_data.result = OK;
            judge_data.flag = EVENT_CHECK_MODEM_UPGRADE_STATE_DONE;
        }
        break;
    case EVENT_SEND_MODEM_UPGRADE_CMD_DONE:
        syslog(LOG_DEBUG, "modem_upgrade_state_cb arg2=%d in slot[%d],status=%d \n", param, slot_id, status);
        if (judge_data.expect == EVENT_SEND_MODEM_UPGRADE_CMD_DONE) {
            judge_data.result = OK;
            judge_data.flag = EVENT_SEND_MODEM_UPGRADE_CMD_DONE;
        }
        break;
    default:
        syslog(LOG_DEBUG, "unexpected event in %s", __func__);
        break;
    }
}

int trigger_modem_upgrade_state_test(int slot_id, int report_state)
{
    int modem_upgrade_state_watch_id = -1;
    int res = 0;
    int ret = 0;

    judge_data_init();
    if (report_state == 2 || report_state == 3) {
        judge_data.expect = EVENT_MODEM_UPGRADE_STATE_TWO_PARM_IND;
    } else {
        judge_data.expect = EVENT_MODEM_UPGRADE_STATE_ONE_PARM_IND;
    }

    modem_upgrade_state_watch_id = tapi_register(get_tapi_ctx(), slot_id, MSG_MODEM_UPGRADE_STATE_IND,
        NULL, modem_upgrade_action_cb);
    if (ret) {
        syslog(LOG_ERR, "register modem upgrade state fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

    remote_modem_upgrade_state_report(slot_id, report_state);

    if (judge()) {
        syslog(LOG_DEBUG, "trigger_modem_upgrade_state_test is not executed in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (judge_data.result) {
        syslog(LOG_ERR, "async result is invalid in %s", __func__);
        res = -1;
        goto on_exit;
    }

    ret = tapi_unregister(get_tapi_ctx(), modem_upgrade_state_watch_id);
    if (ret) {
        syslog(LOG_ERR, "unregister modem upgrade state fail in %s, ret: %d", __func__, ret);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}

int check_modem_upgrade_state_test(int slot_id)
{
    int ret = 0;
    int res = 0;

    judge_data_init();
    judge_data.expect = EVENT_CHECK_MODEM_UPGRADE_STATE_DONE;

    ret = tapi_check_modem_upgrade_state(get_tapi_ctx(), slot_id,
        EVENT_CHECK_MODEM_UPGRADE_STATE_DONE, modem_upgrade_action_cb);

    if (ret) {
        syslog(LOG_ERR, "check_modem_upgrade_state_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "check_modem_upgrade_state_test is not executed in %s", __func__);
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

int send_modem_upgrade_cmd_test(int slot_id, int cmd_id)
{
    int ret = 0;
    int res = 0;

    judge_data_init();
    judge_data.expect = EVENT_SEND_MODEM_UPGRADE_CMD_DONE;

    ret = tapi_modem_upgrade_cmd(get_tapi_ctx(), slot_id,
        EVENT_SEND_MODEM_UPGRADE_CMD_DONE, cmd_id, modem_upgrade_action_cb);

    if (ret) {
        syslog(LOG_ERR, "send_modem_upgrade_cmd_test execute fail in %s, ret: %d",
            __func__, ret);
        res = -1;
        goto on_exit;
    }

    if (judge()) {
        syslog(LOG_DEBUG, "send_modem_upgrade_cmd_test is not executed in %s", __func__);
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
