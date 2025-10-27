#ifdef CONFIG_PHONE_SERVICE
#include "telephony_phone_service_test.h"
#include <stdlib.h>
#ifdef CONFIG_PHONE_SERVICE_WTP
#include "bt_wtp.h"
#endif
#include "remote_operation.h"
#include "tapi_xpc.h"

#define PHONE_SERVICE_ESIM_INCOMING 1000

extern struct judge_type judge_data;

static char incoming_call_id[MAX_CALL_ID_LENGTH + 1] = { 0 };
static exec_cmds_t g_exec_cmds[] = {
#ifdef CONFIG_PHONE_SERVICE_WTP
    { "registerwtpcb", phone_service_register_wtp_cb_exec },
    { "unregisterwtpcb", phone_service_unregister_wtp_cb_exec },
    { "updatelocalinfo", phone_service_update_local_info_exec },
    { "modifydiscovery", phone_service_set_discovery_exec },
    { "modifyvisibility", phone_service_set_visibility_exec },
    { "setaudiotype", phone_service_set_audio_exec },
    { "dialwtp", phone_service_dial_wtp_exec },
    { "hangupwtp", phone_service_hangup_wtp_exec },
    { "answerwtp", phone_service_answer_wtp_exec },
    { "rejectwtp", phone_service_reject_wtp_exec },
#endif
    { "setradiopower", phone_service_set_radiopower_exec },
    { "registeresimcb", phone_service_register_esim_cb_exec },
    { "unregisteresimcb", phone_service_unregister_esim_cb_exec },
    { "dialesim", phone_service_dial_esim_exec },
    { "hangupesim", phone_service_hangup_esim_exec },
    { "sendincomingesim", phone_service_incoming_esim_exec },
    { "answeresim", phone_service_answer_esim_exec },
    { "rejectesim", phone_service_reject_esim_exec },
    { "holdandansweresim", phone_service_hold_and_answer_esim_exec },
    { "releaseandansweresim", phone_service_release_and_answer_esim_exec },
    { "holdandunholdesim", phone_service_hold_and_unhold_esim_exec },
    { "mergeesim", phone_service_merge_esim_exec },
    { "tonesesim", phone_service_send_tones_esim_exec },
    { 0 },
};

static void execute_phone_service_case_cmds(exec_data_t parms)
{
    int i = 0;
    bool find_flag = false;

    syslog(LOG_DEBUG, "%s,cmd=%s", __func__, parms.cmd);
    for (i = 0; g_exec_cmds[i].cmd; i++) {
        if (strcmp(g_exec_cmds[i].cmd, parms.cmd) == 0) {
            find_flag = true;
            g_exec_cmds[i].func_cb(parms);
        }
    }

    if (!find_flag) {
        syslog(LOG_DEBUG, "not find cmd:%s", parms.cmd);
    }
}

void uv_async_callback(uv_async_t* handle)
{
    async_message_t* msg = (async_message_t*)handle->data;
    exec_data_t params;

    params.cmd = msg->cmd;
    params.param = msg->param;
    if (strcmp(msg->cmd, "sendincomingesim") == 0) {
        params.str1 = msg->str1;
    }
    if (strcmp(msg->cmd, "dialesim") == 0) {
        params.str1 = msg->str1;
        params.param1 = 0;
    }

    execute_phone_service_case_cmds(params);
}

static void tele_phone_service_async_fun(tapi_async_result* result)
{
    int event = result->msg_id;
    int status = result->status;

    syslog(LOG_DEBUG, "%s:tele_call_async_fun,event=%d,status=%d", __func__, event, result->status);
    if (result->status != OK) {
        syslog(LOG_ERR, "%s:register wtp callback fail,status=%d", __func__, result->status);
    }
    switch (event) {
    case PHONE_SERVICE_WTP_REGISTER_CALLBACK:
        if (judge_data.expect == PHONE_SERVICE_WTP_REGISTER_CALLBACK) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_WTP_REGISTER_CALLBACK;
        }
        break;
    case PHONE_SERVICE_WTP_UNREGISTER_CALLBACK:
        if (judge_data.expect == PHONE_SERVICE_WTP_UNREGISTER_CALLBACK) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_WTP_UNREGISTER_CALLBACK;
        }
        break;
    case PHONE_SERVICE_WTP_SET_LOCAL_INFO:
        if (judge_data.expect == PHONE_SERVICE_WTP_SET_LOCAL_INFO) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_WTP_SET_LOCAL_INFO;
        }
        break;
    case PHONE_SERVICE_WTP_MODIFY_DISCOVERY:
        if (judge_data.expect == PHONE_SERVICE_WTP_MODIFY_DISCOVERY) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_WTP_MODIFY_DISCOVERY;
        }
        break;
    case PHONE_SERVICE_WTP_MODIFY_VISIBILITY:
        if (judge_data.expect == PHONE_SERVICE_WTP_MODIFY_VISIBILITY) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_WTP_MODIFY_VISIBILITY;
        }
        break;
    case PHONE_SERVICE_WTP_SET_AUDIO_TYPE:
        if (judge_data.expect == PHONE_SERVICE_WTP_SET_AUDIO_TYPE) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_WTP_SET_AUDIO_TYPE;
        }
        break;
    case PHONE_SERVICE_WTP_DIAL:
        if (judge_data.expect == PHONE_SERVICE_WTP_DIAL) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_WTP_DIAL;
        }
        break;
    case PHONE_SERVICE_ESIM_DIAL:
        if (judge_data.expect == PHONE_SERVICE_ESIM_DIAL) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_ESIM_DIAL;
        }
        break;
    case PHONE_SERVICE_WTP_HANGUP:
        if (judge_data.expect == PHONE_SERVICE_WTP_HANGUP) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_WTP_HANGUP;
        }
        break;
    case PHONE_SERVICE_ESIM_HANGUP:
        if (judge_data.expect == PHONE_SERVICE_ESIM_HANGUP) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_ESIM_HANGUP;
        }
        break;
    case PHONE_SERVICE_WTP_ANSWER:
        if (judge_data.expect == PHONE_SERVICE_WTP_ANSWER) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_WTP_ANSWER;
        }
        break;
    case PHONE_SERVICE_WTP_REJECT:
        if (judge_data.expect == PHONE_SERVICE_WTP_REJECT) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_WTP_REJECT;
        }
        break;
    case PHONE_SERVICE_ESIM_MODIFY_RADIO_POWER:
        if (judge_data.expect == PHONE_SERVICE_ESIM_MODIFY_RADIO_POWER) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_ESIM_MODIFY_RADIO_POWER;
        }
        break;
    case PHONE_SERVICE_ESIM_UNREGISTER_CALLBACK:
        if (judge_data.expect == PHONE_SERVICE_ESIM_UNREGISTER_CALLBACK) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_ESIM_UNREGISTER_CALLBACK;
        }
        break;
    case PHONE_SERVICE_ESIM_REGISTER_CALLBACK:
        if (judge_data.expect == PHONE_SERVICE_ESIM_REGISTER_CALLBACK) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_ESIM_REGISTER_CALLBACK;
        }
        break;
    case PHONE_SERVICE_ESIM_ANSWER:
        if (judge_data.expect == PHONE_SERVICE_ESIM_ANSWER) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_ESIM_ANSWER;
        }
        break;
    case PHONE_SERVICE_ESIM_REJECT:
        if (judge_data.expect == PHONE_SERVICE_ESIM_REJECT) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_ESIM_REJECT;
        }
        break;
    case PHONE_SERVICE_ESIM_HOLD_AND_ANSWER:
        if (judge_data.expect == PHONE_SERVICE_ESIM_HOLD_AND_ANSWER) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_ESIM_HOLD_AND_ANSWER;
        }
        break;
    case PHONE_SERVICE_ESIM_RELEASE_AND_ANSWER:
        if (judge_data.expect == PHONE_SERVICE_ESIM_RELEASE_AND_ANSWER) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_ESIM_RELEASE_AND_ANSWER;
        }
        break;
    case PHONE_SERVICE_ESIM_HOLD_CALL:
        if (judge_data.expect == PHONE_SERVICE_ESIM_HOLD_CALL) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_ESIM_HOLD_CALL;
        }
        break;
    case PHONE_SERVICE_ESIM_MERGE_CALL:
        if (judge_data.expect == PHONE_SERVICE_ESIM_MERGE_CALL) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_ESIM_MERGE_CALL;
        }
        break;
    case PHONE_SERVICE_ESIM_SEND_TONES:
        if (judge_data.expect == PHONE_SERVICE_ESIM_SEND_TONES) {
            judge_data.result = status;
            judge_data.flag = PHONE_SERVICE_ESIM_SEND_TONES;
        }
        break;
    default:
        break;
    }
}
#ifdef CONFIG_PHONE_SERVICE_WTP
int phone_service_register_wtp_cb_test(async_message_t* msg)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_WTP_REGISTER_CALLBACK;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "registerwtpcb");
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_register_wtp_cb_test is not executed in %s", __func__);
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

void phone_service_register_wtp_cb_exec(exec_data_t parms)
{
    (void)parms;
    int ret = 0;

    static const wtp_callbacks_t cb_info = {
        .size = sizeof(wtp_callbacks_t),
        .connection_state_changed_cb = NULL,
        .discovery_state_changed_cb = NULL,
        .visibility_changed_cb = NULL,
        .transport_requested_cb = NULL,
        .device_found_cb = NULL,
        .remote_info_changed_cb = NULL,
    };

    syslog(LOG_DEBUG, "%s:%d\n", __func__, __LINE__);
    ret = tapi_client_wtp_register_cb(&cb_info, tele_phone_service_async_fun, NULL);
    if (ret < 0) { // if fail here,case will timeout
        syslog(LOG_ERR, "%s:register wtp callback fail,ret=%d", __func__, ret);
    }
}

int phone_service_unregister_wtp_cb_test(async_message_t* msg)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_WTP_UNREGISTER_CALLBACK;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "unregisterwtpcb");
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_unregister_wtp_cb_test is not executed in %s", __func__);
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

void phone_service_unregister_wtp_cb_exec(exec_data_t parms)
{
    (void)parms;
    int ret = 0;

    syslog(LOG_DEBUG, "%s:%d\n", __func__, __LINE__);
    ret = tapi_client_wtp_unregister_cb(tele_phone_service_async_fun, NULL);
    if (ret < 0) { // if fail here,case will timeout
        syslog(LOG_ERR, "%s:unregister wtp callback fail,ret=%d", __func__, ret);
    }
}

int phone_service_update_local_info_test(async_message_t* msg)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_WTP_SET_LOCAL_INFO;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "updatelocalinfo");
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_update_local_info_test is not executed in %s", __func__);
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

void phone_service_update_local_info_exec(exec_data_t parms)
{
    (void)parms;
    int ret = 0;
    wtp_local_t* local_info = (wtp_local_t*)calloc(1, sizeof(wtp_local_t));

    local_info->name = (char*)calloc(5, sizeof(char));
    strcpy(local_info->name, "aa");
    local_info->name = (char*)calloc(5, sizeof(char));
    strcpy(local_info->name, "11");
    local_info->name = (char*)calloc(5, sizeof(char));
    strcpy(local_info->name, "22");
    local_info->position = (wtp_data_t*)malloc(sizeof(wtp_data_t) + sizeof(uint8_t) * 2);

    local_info->position->length = 2;
    local_info->position->type = 5;
    local_info->position->rfu = 6;
    local_info->position->value[0] = 1234 & 0xFF;
    local_info->position->value[1] = (1234 >> 8) & 0xFF;

    syslog(LOG_DEBUG, "%s:%d\n", __func__, __LINE__);
    ret = tapi_wtp_set_local_info(local_info, tele_phone_service_async_fun, NULL);
    if (ret < 0) { // if fail here,case will timeout
        syslog(LOG_ERR, "%s:update local info fail,ret=%d", __func__, ret);
    }

    free(local_info->position);
    free(local_info->number2);
    free(local_info->number1);
    free(local_info->name);
    free(local_info);
}

int phone_service_set_discovery_test(async_message_t* msg, int value)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_WTP_MODIFY_DISCOVERY;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "modifydiscovery");
    msg->param = value;
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_set_discovery_test is not executed in %s", __func__);
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

void phone_service_set_discovery_exec(exec_data_t parms)
{
    int ret = 0;
    int value = parms.param;

    syslog(LOG_DEBUG, "%s:%d\n", __func__, __LINE__);
    ret = tapi_wtp_modify_discovery(value, tele_phone_service_async_fun, NULL);
    if (ret < 0) { // if fail here,case will timeout
        syslog(LOG_ERR, "%s:modify discovery fail,ret=%d", __func__, ret);
    }
}

int phone_service_set_visibility_test(async_message_t* msg, int value)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_WTP_MODIFY_VISIBILITY;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "modifyvisibility");
    msg->param = value;
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_set_visibility_test is not executed in %s", __func__);
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

void phone_service_set_visibility_exec(exec_data_t parms)
{
    int ret = 0;
    int value = parms.param;

    syslog(LOG_DEBUG, "%s:%d\n", __func__, __LINE__);
    ret = tapi_wtp_modify_visibility(value, tele_phone_service_async_fun, NULL);
    if (ret < 0) { // if fail here,case will timeout
        syslog(LOG_ERR, "%s:modify visibility fail,ret=%d", __func__, ret);
    }
}

int phone_service_set_audio_test(async_message_t* msg, int value)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_WTP_SET_AUDIO_TYPE;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "setaudiotype");
    msg->param = value;
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_set_audio_test is not executed in %s", __func__);
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

void phone_service_set_audio_exec(exec_data_t parms)
{
    int ret = 0;
    int value = parms.param;

    syslog(LOG_DEBUG, "%s:%d\n", __func__, __LINE__);
    ret = tapi_client_set_audio_type(value, tele_phone_service_async_fun, NULL);
    if (ret < 0) { // if fail here,case will timeout
        syslog(LOG_ERR, "%s:set audio fail,ret=%d", __func__, ret);
    }
}

int phone_service_dial_wtp_test(async_message_t* msg)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_WTP_DIAL;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "dialwtp");
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_dial_wtp_test is not executed in %s", __func__);
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

void phone_service_dial_wtp_exec(exec_data_t parms)
{
    (void)parms;
    int ret = 0;
    tapi_call_data_t call_info;

    call_info.wtp_info = (wtp_call_data_t*)calloc(1, sizeof(wtp_call_data_t));
    call_info.phone_info = NULL;
    call_info.wtp_info->remote_bt_addr = "00:00:00:00:00:00";
    call_info.wtp_info->other_info_len = 0;

    ret = tapi_dial_call(call_info, tele_phone_service_async_fun, NULL);
    if (ret < 0) { // if fail here,case will timeout
        syslog(LOG_ERR, "%s:dial fail,ret=%d", __func__, ret);
    }
    free(call_info.wtp_info);
}

int phone_service_hangup_wtp_test(async_message_t* msg)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_WTP_HANGUP;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "hangupwtp");
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_hangup_test is not executed in %s", __func__);
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

void phone_service_hangup_wtp_exec(exec_data_t parms)
{
    (void)parms;
    int ret = 0;
    tapi_call_data_t call_info;

    call_info.wtp_info = (wtp_call_data_t*)calloc(1, sizeof(wtp_call_data_t));
    call_info.phone_info = NULL;
    call_info.wtp_info->remote_bt_addr = "00:00:00:00:00:00";
    call_info.wtp_info->other_info_len = 0;

    ret = tapi_hangup_call(call_info, tele_phone_service_async_fun, NULL);
    if (ret < 0) { // if fail here,case will timeout
        syslog(LOG_ERR, "%s:hangup fail,ret=%d", __func__, ret);
    }
    free(call_info.wtp_info);
}

int phone_service_answer_wtp_test(async_message_t* msg)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_WTP_ANSWER;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "answerwtp");
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_answer_test is not executed in %s", __func__);
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

void phone_service_answer_wtp_exec(exec_data_t parms)
{
    (void)parms;
    tapi_call_data_t call_info;
    int ret = 0;
    call_info.wtp_info = (wtp_call_data_t*)calloc(1, sizeof(wtp_call_data_t));

    call_info.phone_info = NULL;
    call_info.wtp_info->remote_bt_addr = "00:00:00:00:00:00";
    call_info.wtp_info->other_info_len = 0;

    ret = tapi_answer_call(call_info, tele_phone_service_async_fun, NULL);
    if (ret < 0) { // if fail here,case will timeout
        syslog(LOG_ERR, "%s:answer fail,ret=%d", __func__, ret);
    }
    free(call_info.wtp_info);
}

int phone_service_reject_wtp_test(async_message_t* msg)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_WTP_REJECT;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "rejectwtp");
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_reject_test is not executed in %s", __func__);
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

void phone_service_reject_wtp_exec(exec_data_t parms)
{
    (void)parms;
    tapi_call_data_t call_info;
    int ret = 0;
    call_info.wtp_info = (wtp_call_data_t*)calloc(1, sizeof(wtp_call_data_t));

    call_info.phone_info = NULL;
    call_info.wtp_info->remote_bt_addr = "00:00:00:00:00:00";
    call_info.wtp_info->other_info_len = 0;

    ret = tapi_reject_call(call_info, tele_phone_service_async_fun, NULL);
    if (ret < 0) { // if fail here,case will timeout
        syslog(LOG_ERR, "%s:reject fail,ret=%d", __func__, ret);
    }
    free(call_info.wtp_info);
}
#endif
int set_phone_radio_power_test(async_message_t* msg, int target_state)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_ESIM_MODIFY_RADIO_POWER;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "setradiopower");
    msg->param = target_state;
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "set_phone_radio_power_test is not executed in %s", __func__);
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

void phone_service_set_radiopower_exec(exec_data_t parms)
{
    int ret = 0;
    int value = parms.param;

    syslog(LOG_DEBUG, "%s:%d\n", __func__, __LINE__);
    ret = tapi_client_set_radio_power(value, tele_phone_service_async_fun, NULL);
    if (ret < 0) { // if fail here,case will timeout
        syslog(LOG_ERR, "%s:set radiopower fail,ret=%d", __func__, ret);
    }
}

int phone_service_register_esim_cb_test(async_message_t* msg)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_ESIM_REGISTER_CALLBACK;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "registeresimcb");
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_register_esim_cb_test is not executed in %s", __func__);
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

static void esim_radio_state_change_cb(int radio_state)
{
    printf("%s,%d\n", __func__, radio_state);
}

static void esim_operator_status_changed_cb(int status)
{
    printf("%s,%d\n", __func__, status);
}

static void esim_operator_name_changed_cb(const char* name)
{
    printf("%s,%s\n", __func__, name);
}

static void esim_network_reg_state_changed_cb(int status)
{
    printf("%s,%d\n", __func__, status);
}

static void esim_network_strength_changed_cb(int strength)
{
    printf("%s,%d\n", __func__, strength);
}

static void esim_modem_status_changed_cb(int status)
{
    printf("%s,%d\n", __func__, status);
}

static void esim_radio_power_changed_cb(bool state)
{
    printf("%s,%d\n", __func__, state);
}

static void esim_call_state_changed_cb(tapi_call_info* call_info)
{
    printf("%s\n", __func__);
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

    if ((call_info->state == CALL_STATUS_INCOMING || call_info->state == CALL_STATUS_WAITING) && incoming_call_id[0] == '\0') {
        syslog(LOG_DEBUG, "incoming call,call_id: %s,save call id\n", call_info->call_id);
        memcpy(incoming_call_id, call_info->call_id, strlen(call_info->call_id));
        if (judge_data.expect == PHONE_SERVICE_ESIM_INCOMING) {
            judge_data.result = 0;
            judge_data.flag = PHONE_SERVICE_ESIM_INCOMING;
        }
    }
}

static tele_callbacks_t cb_info = {
    .radio_state_change_cb = esim_radio_state_change_cb,
    .operator_status_changed_cb = esim_operator_status_changed_cb,
    .operator_name_changed_cb = esim_operator_name_changed_cb,
    .network_reg_state_changed_cb = esim_network_reg_state_changed_cb,
    .strength_changed_cb = esim_network_strength_changed_cb,
    .modem_status_changed_cb = esim_modem_status_changed_cb,
    .radio_power_changed_cb = esim_radio_power_changed_cb,
    .call_state_changed_cb = esim_call_state_changed_cb,
};

void phone_service_register_esim_cb_exec(exec_data_t parms)
{
    (void)parms;
    int ret = 0;

    syslog(LOG_DEBUG, "%s:%d\n", __func__, __LINE__);
    ret = tapi_client_register_callbacks(cb_info, tele_phone_service_async_fun, NULL);
    if (ret < 0) { // if fail here,case will timeout
        syslog(LOG_ERR, "%s:register esim callback fail,ret=%d", __func__, ret);
    }
}

int phone_service_unregister_esim_cb_test(async_message_t* msg)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_ESIM_UNREGISTER_CALLBACK;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "unregisteresimcb");
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_unregister_esim_cb_test is not executed in %s", __func__);
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

void phone_service_unregister_esim_cb_exec(exec_data_t parms)
{
    (void)parms;
    int ret = 0;

    syslog(LOG_DEBUG, "%s:%d\n", __func__, __LINE__);
    ret = tapi_client_unregister_callbacks(tele_phone_service_async_fun, NULL);
    if (ret < 0) { // if fail here,case will timeout
        syslog(LOG_ERR, "%s:unregister esim callback fail,ret=%d", __func__, ret);
    }
}

int phone_service_dial_test(async_message_t* msg, char* num)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_ESIM_DIAL;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "dialesim");
    memset(msg->str1, '\0', sizeof(msg->str1));
    strcpy(msg->str1, num);
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_dial_test dial is not executed in %s", __func__);
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

int phone_service_hangup_test(async_message_t* msg)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_ESIM_HANGUP;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "hangupesim");
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_hangup_test hangup executed fail in %s", __func__);
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

int phone_service_dial_and_hangup_esim_test(async_message_t* msg, char* number)
{
    int res = 0;

    if (phone_service_dial_test(msg, number)) {
        syslog(LOG_DEBUG, "dial executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
    if (phone_service_hangup_test(msg)) {
        syslog(LOG_DEBUG, "hangup executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
on_exit:
    return res;
}

void phone_service_dial_esim_exec(exec_data_t parms)
{
    int ret = 0;
    tapi_call_data_t call_info;
    char* phone_number = parms.str1;
    int hide_caller_id = parms.param;

    call_info.phone_info = (tapi_cell_call_data_t*)calloc(1, sizeof(tapi_cell_call_data_t));
    call_info.phone_info->slot = 0;
    call_info.phone_info->phone_number = phone_number;
    call_info.phone_info->hide_callerid = hide_caller_id;
    call_info.phone_info->call_id = NULL;
    call_info.wtp_info = NULL;

    ret = tapi_dial_call(call_info, tele_phone_service_async_fun, NULL);
    if (ret < 0) { // if fail here,case will timeout
        syslog(LOG_ERR, "%s:dial esim fail,ret=%d", __func__, ret);
    }
    free(call_info.phone_info);
}

void phone_service_hangup_esim_exec(exec_data_t parms)
{
    (void)parms;
    int ret = 0;
    tapi_call_data_t call_info;

    call_info.phone_info = (tapi_cell_call_data_t*)calloc(1, sizeof(tapi_cell_call_data_t));
    call_info.phone_info->slot = 0;
    call_info.phone_info->call_id = NULL;
    call_info.wtp_info = NULL;
    ret = tapi_hangup_call(call_info, tele_phone_service_async_fun, NULL);
    if (ret < 0) { // if fail here,case will timeout
        syslog(LOG_ERR, "%s:hangup esim fail,ret=%d", __func__, ret);
    }
    free(call_info.phone_info);
}

int phone_service_incoming_esim_test(async_message_t* msg, char* num)
{
    int res = 0;

    memset(incoming_call_id, 0, sizeof(incoming_call_id));
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_ESIM_INCOMING;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "sendincomingesim");
    memset(msg->str1, '\0', sizeof(msg->str1));
    strcpy(msg->str1, num);
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_incoming_esim_test incoming executed fail in %s", __func__);
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

void phone_service_incoming_esim_exec(exec_data_t parms)
{
    char* phone_num = parms.str1;
    remote_call_operation(0, phone_num, INCOMING_CALL);
}

int phone_service_answer_esim_test(async_message_t* msg)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_ESIM_ANSWER;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "answeresim");
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_answer_esim_test answer executed fail in %s", __func__);
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

void phone_service_answer_esim_exec(exec_data_t parms)
{
    (void)parms;
    int ret = 0;
    tapi_call_data_t call_info;

    call_info.phone_info = (tapi_cell_call_data_t*)calloc(1, sizeof(tapi_cell_call_data_t));
    call_info.phone_info->slot = 0;
    call_info.phone_info->call_id = incoming_call_id;
    call_info.wtp_info = NULL;

    ret = tapi_answer_call(call_info, tele_phone_service_async_fun, NULL);
    if (ret < 0) { // if fail here,case will timeout
        syslog(LOG_ERR, "%s:answer esim fail,ret=%d", __func__, ret);
    }
    free(call_info.phone_info);
}

int phone_service_incoming_answer_and_hangup_esim_test(async_message_t* msg)
{
    int res = 0;

    if (phone_service_register_esim_cb_test(msg)) {
        syslog(LOG_DEBUG, "register esim cb executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (phone_service_incoming_esim_test(msg, "10086")) {
        syslog(LOG_DEBUG, "incoming call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (phone_service_answer_esim_test(msg)) {
        syslog(LOG_DEBUG, "answer call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (phone_service_hangup_test(msg)) {
        syslog(LOG_DEBUG, "hangup executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (phone_service_unregister_esim_cb_test(msg)) {
        syslog(LOG_DEBUG, "unregister esim cb executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
on_exit:
    return res;
}

int phone_service_reject_esim_test(async_message_t* msg)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_ESIM_REJECT;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "rejectesim");
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_reject_esim_test answer executed fail in %s", __func__);
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

void phone_service_reject_esim_exec(exec_data_t parms)
{
    (void)parms;
    int ret = 0;
    tapi_call_data_t call_info;

    call_info.phone_info = (tapi_cell_call_data_t*)calloc(1, sizeof(tapi_cell_call_data_t));
    call_info.phone_info->slot = 0;
    call_info.phone_info->call_id = incoming_call_id;
    call_info.wtp_info = NULL;

    ret = tapi_reject_call(call_info, tele_phone_service_async_fun, NULL);
    if (ret < 0) { // if fail here,case will timeout
        syslog(LOG_ERR, "%s:answer esim fail,ret=%d", __func__, ret);
    }
    free(call_info.phone_info);
}

int phone_service_incoming_and_reject_esim_test(async_message_t* msg)
{
    int res = 0;

    if (phone_service_register_esim_cb_test(msg)) {
        syslog(LOG_DEBUG, "register esim cb executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
    if (phone_service_incoming_esim_test(msg, "10086")) {
        syslog(LOG_DEBUG, "incoming call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (phone_service_reject_esim_test(msg)) {
        syslog(LOG_DEBUG, "reject call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (phone_service_unregister_esim_cb_test(msg)) {
        syslog(LOG_DEBUG, "unregister esim cb executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
on_exit:
    return res;
}

int phone_service_hold_and_answer_esim_test(async_message_t* msg)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_ESIM_HOLD_AND_ANSWER;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "holdandansweresim");
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_hold_and_answer_esim_test answer executed fail in %s", __func__);
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

void phone_service_hold_and_answer_esim_exec(exec_data_t parms)
{
    (void)parms;
    int ret = 0;

    ret = tapi_hold_and_answer_call(0, tele_phone_service_async_fun, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "%s:hold and answer call fail,ret=%d", __func__, ret);
    }
}

int phone_service_release_answer_esim_test(async_message_t* msg)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_ESIM_RELEASE_AND_ANSWER;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "releaseandansweresim");
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_release_answer_esim_test answer executed fail in %s", __func__);
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

void phone_service_release_and_answer_esim_exec(exec_data_t parms)
{
    (void)parms;
    int ret = 0;

    ret = tapi_release_and_answer_call(0, tele_phone_service_async_fun, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "%s:release and answer call fail,ret=%d", __func__, ret);
    }
}

int phone_service_release_and_answer_esim_test(async_message_t* msg)
{
    int res = 0;

    if (phone_service_register_esim_cb_test(msg)) {
        syslog(LOG_DEBUG, "register esim cb executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (phone_service_incoming_esim_test(msg, "10086")) {
        syslog(LOG_DEBUG, "incoming call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (phone_service_answer_esim_test(msg)) {
        syslog(LOG_DEBUG, "answer call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (phone_service_incoming_esim_test(msg, "10010")) {
        syslog(LOG_DEBUG, "incoming call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
    sleep(3);
    if (phone_service_hold_and_answer_esim_test(msg)) {
        syslog(LOG_DEBUG, "hold and answer call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
    if (phone_service_incoming_esim_test(msg, "10001")) {
        syslog(LOG_DEBUG, "incoming call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
    sleep(3);
    if (phone_service_release_answer_esim_test(msg)) {
        syslog(LOG_DEBUG, "release and answer call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
    sleep(3);
    if (phone_service_hangup_test(msg)) {
        syslog(LOG_DEBUG, "hangup executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (phone_service_unregister_esim_cb_test(msg)) {
        syslog(LOG_DEBUG, "unregister esim cb executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
on_exit:
    return res;
}

int phone_service_hold_unhold_test(async_message_t* msg, int hold)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_ESIM_HOLD_CALL;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "holdandunholdesim");
    msg->param = hold;
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_hold_unhold_test executed fail in %s", __func__);
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

void phone_service_hold_and_unhold_esim_exec(exec_data_t parms)
{
    int ret = 0;
    bool hold_flag;
    int hold = parms.param;

    if (hold == 0) {
        hold_flag = true;
    } else {
        hold_flag = false;
    }
    ret = tapi_hold_call(0, hold_flag, tele_phone_service_async_fun, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "%s:hold and unhold call fail,ret=%d", __func__, ret);
    }
}

int phone_service_hold_and_unhold_esim_test(async_message_t* msg)
{
    int res = 0;

    if (phone_service_register_esim_cb_test(msg)) {
        syslog(LOG_DEBUG, "register esim cb executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (phone_service_incoming_esim_test(msg, "10086")) {
        syslog(LOG_DEBUG, "incoming call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (phone_service_answer_esim_test(msg)) {
        syslog(LOG_DEBUG, "answer call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
    sleep(3);
    if (phone_service_hold_unhold_test(msg, 0)) {
        syslog(LOG_DEBUG, "hold/unhold call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
    sleep(3);
    if (phone_service_hold_unhold_test(msg, 1)) {
        syslog(LOG_DEBUG, "hold/unhold call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
    sleep(3);
    if (phone_service_hangup_test(msg)) {
        syslog(LOG_DEBUG, "hangup executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (phone_service_unregister_esim_cb_test(msg)) {
        syslog(LOG_DEBUG, "unregister esim cb executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
on_exit:
    return res;
}

int phone_service_merge_esim(async_message_t* msg)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_ESIM_MERGE_CALL;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "mergeesim");
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_merge_esim_test executed fail in %s", __func__);
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

void phone_service_merge_esim_exec(exec_data_t parms)
{
    (void)parms;
    int ret = 0;

    ret = tapi_merge_call(0, tele_phone_service_async_fun, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "%s:merge call fail,ret=%d", __func__, ret);
    }
}

int phone_service_merge_esim_test(async_message_t* msg)
{
    int res = 0;

    if (phone_service_register_esim_cb_test(msg)) {
        syslog(LOG_DEBUG, "register esim cb executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (phone_service_incoming_esim_test(msg, "10086")) {
        syslog(LOG_DEBUG, "incoming call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (phone_service_answer_esim_test(msg)) {
        syslog(LOG_DEBUG, "answer call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
    if (phone_service_incoming_esim_test(msg, "10010")) {
        syslog(LOG_DEBUG, "incoming call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
    sleep(3);
    if (phone_service_hold_and_answer_esim_test(msg)) {
        syslog(LOG_DEBUG, "merge call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
    sleep(3);
    if (phone_service_merge_esim(msg)) {
        syslog(LOG_DEBUG, "merge call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
    sleep(3);
    if (phone_service_hangup_test(msg)) {
        syslog(LOG_DEBUG, "hangup executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (phone_service_unregister_esim_cb_test(msg)) {
        syslog(LOG_DEBUG, "unregister esim cb executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
on_exit:
    return res;
}

int phone_service_send_tones_test(async_message_t* msg)
{
    int res = 0;
    judge_data_init();
    judge_data.expect = PHONE_SERVICE_ESIM_SEND_TONES;

    memset(msg->cmd, '\0', sizeof(msg->cmd));
    strcpy(msg->cmd, "tonesesim");
    msg->async.data = (void*)msg;
    uv_async_send(&msg->async);

    if (judge()) {
        syslog(LOG_DEBUG, "phone_service_send_tones_test executed fail in %s", __func__);
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

void phone_service_send_tones_esim_exec(exec_data_t parms)
{
    (void)parms;
    int ret = 0;

    ret = tapi_send_tones("11", tele_phone_service_async_fun, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "%s:send tones fail,ret=%d", __func__, ret);
    }
}

int phone_service_send_tones_esim_test(async_message_t* msg)
{
    int res = 0;

    if (phone_service_register_esim_cb_test(msg)) {
        syslog(LOG_DEBUG, "register esim cb executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (phone_service_incoming_esim_test(msg, "10086")) {
        syslog(LOG_DEBUG, "incoming call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    sleep(3);
    if (phone_service_answer_esim_test(msg)) {
        syslog(LOG_DEBUG, "answer call executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (phone_service_send_tones_test(msg)) {
        syslog(LOG_DEBUG, "send tones executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }
    sleep(3);
    if (phone_service_hangup_test(msg)) {
        syslog(LOG_DEBUG, "hangup executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

    if (phone_service_unregister_esim_cb_test(msg)) {
        syslog(LOG_DEBUG, "unregister esim cb executed fail in %s", __func__);
        res = -1;
        goto on_exit;
    }

on_exit:
    return res;
}
#endif
