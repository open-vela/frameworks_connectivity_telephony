/****************************************************************************
 * framework/telephony/telephony_phone_tool.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/
#include "tapi_phone.h"
#include "tapi_tool.h"

MLinkedList* g_wtp_device_list = NULL;
#ifdef CONFIG_PHONE_SERVICE_WTP
typedef struct {
    wtp_remote_t* remote; // remote device info
    wtp_param_t* param; // wtp call param,just used by dial senario,NULL for other scenario
    uint16_t other_info_len; // reserved field
    uint8_t value[0]; // reserved field
} wtp_whole_data_t; // wtp call info

wtp_whole_data_t* g_incoming_wtp_call = NULL;
wtp_whole_data_t* g_connected_wtp_call = NULL;

static void register_wtp_callback_done(tapi_async_result* result)
{
    if (result->status != OK) {
        syslog(LOG_ERR, "%s:register wtp callback fail,status=%d", __func__, result->status);
    } else {
        syslog(LOG_DEBUG, "%s:register wtp callback success", __func__);
    }
}

static void unregister_wtp_callback_done(tapi_async_result* result)
{
    if (result->status != OK) {
        syslog(LOG_ERR, "%s:unregister wtp callback fail,status=%d", __func__, result->status);
    } else {
        syslog(LOG_DEBUG, "%s:unregister wtp callback success", __func__);
    }
}

static void set_wtp_local_info_done(tapi_async_result* result)
{
    if (result->status != OK) {
        syslog(LOG_ERR, "%s:set wtp local info fail,status=%d", __func__, result->status);
    } else {
        syslog(LOG_DEBUG, "%s:set wtp local info success", __func__);
    }
}

static void modify_wtp_discovery_done(tapi_async_result* result)
{
    if (result->status != OK) {
        syslog(LOG_ERR, "%s:set modify wtp discovery fail,status=%d", __func__, result->status);
    } else {
        syslog(LOG_DEBUG, "%s:set modify wtp discovery success", __func__);
    }
}

static void modify_wtp_visibility_done(tapi_async_result* result)
{
    if (result->status != OK) {
        syslog(LOG_ERR, "%s:set modify wtp visibility fail,status=%d", __func__, result->status);
    } else {
        syslog(LOG_DEBUG, "%s:set modify wtp visibility success", __func__);
    }
}
#endif

static void dial_call_done(tapi_async_result* result)
{
    if (result->status != OK) {
        syslog(LOG_ERR, "%s:dial call fail,status=%d", __func__, result->status);
    } else {
        syslog(LOG_DEBUG, "%s:dial call success", __func__);
    }
}

static void hangup_call_done(tapi_async_result* result)
{
    if (result->status != OK) {
        syslog(LOG_ERR, "%s:hangup call fail,status=%d", __func__, result->status);
    } else {
        syslog(LOG_DEBUG, "%s:hangup call success", __func__);
    }
}

static void answer_call_done(tapi_async_result* result)
{
    if (result->status != OK) {
        syslog(LOG_ERR, "%s:answer call fail,status=%d", __func__, result->status);
    } else {
        syslog(LOG_DEBUG, "%s:answer call success", __func__);
    }
}

static void reject_call_done(tapi_async_result* result)
{
    if (result->status != OK) {
        syslog(LOG_ERR, "%s:reject call fail,status=%d", __func__, result->status);
    } else {
        syslog(LOG_DEBUG, "%s:reject call success", __func__);
    }
}

static void release_and_answer_call_callback_done(tapi_async_result* result)
{
    if (result->status != OK) {
        syslog(LOG_ERR, "%s:release and answer call fail,status=%d", __func__, result->status);
    } else {
        syslog(LOG_DEBUG, "%s:release and answer callsuccess", __func__);
    }
}

static void hold_and_answer_call_callback_done(tapi_async_result* result)
{
    if (result->status != OK) {
        syslog(LOG_ERR, "%s:hold and answer call fail,status=%d", __func__, result->status);
    } else {
        syslog(LOG_DEBUG, "%s:hold and answer call success", __func__);
    }
}

static void hold_call_callback_done(tapi_async_result* result)
{
    if (result->status != OK) {
        syslog(LOG_ERR, "%s:hold call fail,status=%d", __func__, result->status);
    } else {
        syslog(LOG_DEBUG, "%s:hold call success", __func__);
    }
}

static void merge_call_callback_done(tapi_async_result* result)
{
    if (result->status != OK) {
        syslog(LOG_ERR, "%s:merge call fail,status=%d", __func__, result->status);
    } else {
        syslog(LOG_DEBUG, "%s:merge call success", __func__);
    }
}

static void send_tones_callback_done(tapi_async_result* result)
{
    if (result->status != OK) {
        syslog(LOG_ERR, "%s:send tones fail,status=%d", __func__, result->status);
    } else {
        syslog(LOG_DEBUG, "%s:send tones success", __func__);
    }
}

#ifdef CONFIG_PHONE_SERVICE_WTP
static void set_audio_type_done(tapi_async_result* result)
{
    if (result->status != OK) {
        syslog(LOG_ERR, "%s:set audio type fail,status=%d", __func__, result->status);
    } else {
        syslog(LOG_DEBUG, "%s:set audio type success", __func__);
    }
}

static void free_wtp_call_data(void* data)
{
    wtp_whole_data_t* wtp_data = (wtp_whole_data_t*)data;

    if (wtp_data == NULL) {
        return;
    }

    if (wtp_data->remote != NULL) {
        if (wtp_data->remote->addr != NULL) {
            free(wtp_data->remote->addr);
            wtp_data->remote->addr = NULL;
        }
        if (wtp_data->remote->name != NULL) {
            free(wtp_data->remote->name);
            wtp_data->remote->name = NULL;
        }
        if (wtp_data->remote->number1 != NULL) {
            free(wtp_data->remote->number1);
            wtp_data->remote->number1 = NULL;
        }
        if (wtp_data->remote->number2 != NULL) {
            free(wtp_data->remote->number2);
            wtp_data->remote->number2 = NULL;
        }
        if (wtp_data->remote->position != NULL) {
            free(wtp_data->remote->position);
            wtp_data->remote->position = NULL;
        }
        free(wtp_data->remote);
    }

    if (wtp_data->param != NULL) {
        free(wtp_data->param);
        wtp_data->param = NULL;
    }

    free(wtp_data); // other_info will be free directly
}

static wtp_whole_data_t* save_wtp_call_data(wtp_remote_t* src, wtp_param_t* param)
{
    wtp_whole_data_t* target = (wtp_whole_data_t*)calloc(1, sizeof(wtp_whole_data_t));

    syslog(LOG_INFO, "%s", __func__);
    if (target == NULL) {
        syslog(LOG_ERR, "%s:calloc wtp_whole_data_t fail", __func__);
        return NULL;
    }

    target->remote = (wtp_remote_t*)calloc(1, sizeof(wtp_remote_t));
    if (target->remote == NULL) {
        syslog(LOG_ERR, "%s:calloc wtp_remote_t fail", __func__);
        free(target);
        return NULL;
    }
    target->remote->addr_type = src->addr_type;
    target->remote->signal = src->signal;
    target->remote->rfu = src->rfu;

    if (src->addr != NULL) {
        target->remote->addr = (bt_address_t*)calloc(1, sizeof(bt_address_t));
        if (target->remote->addr == NULL) {
            syslog(LOG_ERR, "%s:calloc bt_address_t fail", __func__);
            free_wtp_call_data(target);
            return NULL;
        }
        memcpy(target->remote->addr->addr, src->addr->addr, BT_ADDR_LENGTH);
    }

    if (src->name != NULL) {
        int len = strlen(src->name) + 1;
        target->remote->name = (char*)calloc(len, sizeof(char));
        if (target->remote->name == NULL) {
            syslog(LOG_ERR, "%s:calloc name fail", __func__);
            free_wtp_call_data(target);
            return NULL;
        }
        strcpy(target->remote->name, src->name);
    }

    if (src->number1 != NULL) {
        int len = strlen(src->number1) + 1;
        target->remote->number1 = (char*)calloc(len, sizeof(char));
        if (target->remote->number1 == NULL) {
            syslog(LOG_ERR, "%s:calloc number1 fail", __func__);
            free_wtp_call_data(target);
            return NULL;
        }
        strcpy(target->remote->number1, src->number1);
    }

    if (src->number2 != NULL) {
        int len = strlen(src->number2) + 1;
        target->remote->number2 = (char*)calloc(len, sizeof(char));
        if (target->remote->number2 == NULL) {
            syslog(LOG_ERR, "%s:calloc number2 fail", __func__);
            free_wtp_call_data(target);
            return NULL;
        }
        strcpy(target->remote->number2, src->number2);
    }

    if (src->position != NULL) {
        target->remote->position = (wtp_data_t*)calloc(1, sizeof(wtp_data_t) + sizeof(uint8_t) * src->position->length);
        if (target->remote->position == NULL) {
            syslog(LOG_ERR, "%s:calloc position fail", __func__);
            free_wtp_call_data(target);
            return NULL;
        }
        target->remote->position->length = src->position->length;
        target->remote->position->type = src->position->type;
        target->remote->position->rfu = src->position->rfu;
        if (src->position->length != 0) {
            memcpy(target->remote->position->value, src->position->value, src->position->length);
        }
    }

    if (param != NULL) {
        target->param = (wtp_param_t*)calloc(1, sizeof(wtp_param_t));
        if (target->param == NULL) {
            printf("%s:unexpected:save param fail\n", __func__);
            free_wtp_call_data(target);
            return NULL;
        }
        memcpy(target->param, param, sizeof(wtp_param_t));
    }

    return target;
}

static void wtp_call_conn_cb(void* cookie, wtp_remote_t* remote,
    profile_connection_state_t state, bt_status_t reason, wtp_param_t* param)
{
    printf("%s,%d,%d\n", __func__, state, reason);
    if (state == PROFILE_STATE_DISCONNECTED) {
        if (g_connected_wtp_call != NULL) {
            free_wtp_call_data(g_connected_wtp_call);
            g_connected_wtp_call = NULL;
        }
    } else if (state == PROFILE_STATE_CONNECTED) {
        if (g_connected_wtp_call != NULL) { // don't support multiple calls currently,or else need judge by remote info
            printf("%s:abnormal state,g_connected_wtp_call != NULL\n", __func__);
            free_wtp_call_data(g_connected_wtp_call);
        }
        g_connected_wtp_call = save_wtp_call_data(remote, param);
        if (g_connected_wtp_call == NULL) {
            printf("%s:unexpected:save remote device fail\n", __func__);
            return;
        }
    }
}

static void wtp_call_discovery_cb(void* cookie, bool started,
    bt_status_t reason)
{
    printf("%s,%d,%d\n", __func__, started, reason);
}

static void wtp_call_visibility_cb(void* cookie, bool visible,
    bt_status_t reason)
{
    printf("%s,%d,%d\n", __func__, visible, reason);
}

static char* bt_address_to_string(const bt_address_t* addr)
{
    char* str;

    if (addr == NULL) {
        syslog(LOG_ERR, "%s:addr is null", __func__);
        return NULL;
    }
    if (sizeof(addr->addr) != BT_ADDR_LENGTH) {
        syslog(LOG_ERR, "%s:addr length is wrong", __func__);
        return NULL;
    }

    str = (char*)calloc(18, sizeof(char));
    if (str == NULL) {
        syslog(LOG_ERR, "%s:addr calloc fail", __func__);
        return NULL;
    }
    snprintf(str, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
        addr->addr[0], addr->addr[1], addr->addr[2],
        addr->addr[3], addr->addr[4], addr->addr[5]);
    syslog(LOG_DEBUG, "%s:%s", __func__, str);
    return str;
}

static void wtp_call_requested_cb(void* cookie, wtp_remote_t* remote,
    wtp_param_t* param)
{
    char* str;

    printf("%s\n", __func__);
    if (g_incoming_wtp_call != NULL) {
        printf("%s,unexpected:don't support multi call\n", __func__);
        return;
    }

    g_incoming_wtp_call = save_wtp_call_data(remote, param);
    if (g_incoming_wtp_call == NULL) {
        printf("%s,unexpected:save call data fail\n", __func__);
        return;
    }
    str = bt_address_to_string(remote->addr);
    printf("---------------------comming wtp call-----------------------------------\n");
    printf("1-%s\n", str);
    printf("---------------------comming wtp call-----------------------------------\n");
    free(str);
}

static void wtp_device_foreach(int id, void* user_data)
{
    wtp_whole_data_t* wtp_data = (wtp_whole_data_t*)user_data;
    char* str;

    str = bt_address_to_string(wtp_data->remote->addr);
    printf("%d-%s\n", id, str);
    free(str);
}

static bool wtp_remote_compare(void* node_data, void* data)
{
    wtp_whole_data_t* data1 = (wtp_whole_data_t*)node_data;
    wtp_whole_data_t* data2 = (wtp_whole_data_t*)data;

    if (data1 == NULL || data2 == NULL) {
        return false;
    }
    if (memcmp(data1->remote->addr, data2->remote->addr, sizeof(data1->remote->addr) != 0)) { // if addr is different,it's not same device
        return false;
    } else {
        return true;
    }
}

bool linked_list_contain_same_data(MLinkedList* list, void* data, bool (*compare_func)(void* node_data, void* data))
{
    MListNode* current;

    if (!list || !compare_func) {
        return false;
    }

    SIMPLEQ_FOREACH(current, &list->head, entries)
    {
        if (compare_func(current->data, data)) {
            return true;
        }
    }
    return false;
}

static void wtp_call_device_found_cb(void* cookie, wtp_remote_t* remote, wtp_param_t* param)
{
    MListNode* new_node;
    MListNode* current;

    printf("%s\n", __func__);

    wtp_whole_data_t* wtp_data = (wtp_whole_data_t*)calloc(1, sizeof(wtp_whole_data_t)); // consider other_info if needed
    if (wtp_data == NULL) {
        printf("%s:calloc fail\n", __func__);
    }

    wtp_data = save_wtp_call_data(remote, param);
    if (wtp_data == NULL) {
        printf("%s:unexpected:save call data fail\n", __func__);
        return;
    }

    if (linked_list_contain_same_data(g_wtp_device_list, wtp_data, wtp_remote_compare)) {
        free_wtp_call_data(wtp_data);
        printf("%s:find same device,no need record\n", __func__);
        return;
    }
    printf("-----------------new device found------------------------------\n");
    if (g_wtp_device_list == NULL) {
        g_wtp_device_list = malloc(sizeof(MLinkedList));
        if (g_wtp_device_list) {
            SIMPLEQ_INIT(&g_wtp_device_list->head);
            g_wtp_device_list->next_id = 1;
        } else {
            printf("%s:create linkedlist fail\n", __func__);
            return;
        }
    }
    new_node = malloc(sizeof(MListNode));
    if (new_node == NULL) {
        printf("%s:malloc fail\n", __func__);
        return;
    }
    if (g_wtp_device_list->next_id > INT_MAX - 1) {
        g_wtp_device_list->next_id = 1;
    }
    new_node->id = g_wtp_device_list->next_id++;
    new_node->data = wtp_data;
    SIMPLEQ_INSERT_TAIL(&g_wtp_device_list->head, new_node, entries);

    SIMPLEQ_FOREACH(current, &g_wtp_device_list->head, entries)
    {
        wtp_device_foreach(current->id, current->data);
    }
    printf("-----------------new device found------------------------------\n");
}

static void wtp_call_remote_info_update_cb(void* cookie, wtp_remote_t* remote)
{

    printf("%s\n", __func__);

    syslog(LOG_INFO, "%d", remote->addr_type);
    syslog(LOG_INFO, "%d", remote->signal);
    syslog(LOG_INFO, "%d", remote->rfu);
    for (int i = 0; i < BT_ADDR_LENGTH; i++) {
        syslog(LOG_INFO, "%u", remote->addr->addr[i]);
    }
    syslog(LOG_INFO, "%s", remote->name);
    syslog(LOG_INFO, "%s", remote->number1);
    syslog(LOG_INFO, "%s", remote->number2);
    syslog(LOG_INFO, "%d", remote->position->length);
    syslog(LOG_INFO, "%d", remote->position->type);
    syslog(LOG_INFO, "%d", remote->position->rfu);
    for (int i = 0; i < remote->position->length; i++) {
        syslog(LOG_INFO, "%d", remote->position->value[i]);
    }
}

static int telephonytool_cmd_set_audio_type(char* pargs)
{
    char dst[1][MAX_INPUT_ARGS_LEN];
    int cnt;
    int ret = 0;
    int type;

    if (strlen(pargs) == 0) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    cnt = split_input(dst, 1, pargs, " ");
    if (!(cnt == 1)) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    type = atoi(dst[0]);
    if (!(type == 1 || type == 0)) {
        syslog(LOG_ERR, "%s:parameter value is not supported currently", __func__);
        return -1;
    }

    ret = tapi_client_set_audio_type(type, set_audio_type_done, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "%s:set audio type fail,ret=%d", __func__, ret);
    }
    return ret;
}
#endif

static int telephonytool_cmd_reject_call(char* pargs)
{
    char dst[2][MAX_INPUT_ARGS_LEN];
    int cnt;
    int ret = -1;
    int type;
    tapi_call_data_t call_info;

    if (strlen(pargs) == 0) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    cnt = split_input(dst, 2, pargs, " ");
    if (!(cnt == 1 || cnt == 2)) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    type = atoi(dst[0]);
    if (type == 0) {
        call_info.phone_info = (tapi_cell_call_data_t*)calloc(1, sizeof(tapi_cell_call_data_t));
        if (call_info.phone_info == NULL) {
            syslog(LOG_ERR, "%s:calloc fail", __func__);
            goto done;
        }
        call_info.phone_info->slot = 0;
        call_info.phone_info->call_id = dst[1];
        call_info.wtp_info = NULL;

        ret = tapi_reject_call(call_info, reject_call_done, NULL);
        free(call_info.phone_info);
    } else if (type == 1) {
#ifdef CONFIG_PHONE_SERVICE_WTP
        if (g_incoming_wtp_call != NULL) { // just can reject incoming call
            call_info.wtp_info = (tapi_wtp_call_data_t*)calloc(1, sizeof(tapi_wtp_call_data_t)); ////need consider calloc other_info if exist
            if (call_info.wtp_info == NULL) {
                syslog(LOG_ERR, "%s:calloc fail", __func__);
                return -1;
            }
            call_info.phone_info = NULL;
            call_info.wtp_info->remote_bt_addr = bt_address_to_string(g_incoming_wtp_call->remote->addr);
            if (call_info.wtp_info->remote_bt_addr == NULL) {
                syslog(LOG_ERR, "%s: addr is not correct", __func__);
                return -1;
            }
            call_info.wtp_info->other_info_len = 0;

            ret = tapi_reject_call(call_info, reject_call_done, NULL);
            if (call_info.wtp_info->remote_bt_addr != NULL) {
                free(call_info.wtp_info->remote_bt_addr);
            }
            if (call_info.wtp_info) {
                free(call_info.wtp_info);
            }
            if (ret < 0) {
                syslog(LOG_ERR, "%s:reject call fail,ret=%d", __func__, ret);
            }
            free_wtp_call_data(g_incoming_wtp_call);
            g_incoming_wtp_call = NULL;
        } else {
            syslog(LOG_ERR, "%s: no incoming wtp call exist", __func__);
            ret = -1;
        }
#else
        syslog(LOG_ERR, "%s: CONFIG_PHONE_SERVICE_WTP not supporrt", __func__);
#endif
    } else {
        syslog(LOG_ERR, "%s:not supporrt", __func__);
    }
done:
    if (ret < 0) {
        syslog(LOG_ERR, "%s:reject call fail,ret=%d", __func__, ret);
    }
    return ret;
}

static int telephonytool_cmd_answer_call(char* pargs)
{
    char dst[2][MAX_INPUT_ARGS_LEN];
    int cnt;
    int ret = -1;
    int type;
    tapi_call_data_t call_info;

    if (strlen(pargs) == 0) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    cnt = split_input(dst, 2, pargs, " ");
    if (!(cnt == 1 || cnt == 2)) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    type = atoi(dst[0]);
    if (type == 0) {
        call_info.phone_info = (tapi_cell_call_data_t*)calloc(1, sizeof(tapi_cell_call_data_t));
        if (call_info.phone_info == NULL) {
            syslog(LOG_ERR, "%s:calloc fail", __func__);
            goto done;
        }
        call_info.phone_info->slot = 0;
        call_info.phone_info->call_id = dst[1];
        call_info.wtp_info = NULL;

        ret = tapi_answer_call(call_info, answer_call_done, NULL);
        free(call_info.phone_info);
    } else if (type == 1) {
#ifdef CONFIG_PHONE_SERVICE_WTP
        if (g_incoming_wtp_call != NULL) {
            call_info.wtp_info = (tapi_wtp_call_data_t*)calloc(1, sizeof(tapi_wtp_call_data_t)); // call_info.wtp_info->other_info_len = 0;
            if (call_info.wtp_info == NULL) {
                syslog(LOG_ERR, "%s:calloc fail", __func__);
                return -1;
            }
            call_info.phone_info = NULL;
            call_info.wtp_info->remote_bt_addr = bt_address_to_string(g_incoming_wtp_call->remote->addr);
            if (call_info.wtp_info->remote_bt_addr == NULL) {
                syslog(LOG_ERR, "%s: addr is not correct", __func__);
                return -1;
            }
            call_info.wtp_info->other_info_len = 0;

            ret = tapi_answer_call(call_info, answer_call_done, NULL);
            if (call_info.wtp_info->remote_bt_addr != NULL) {
                free(call_info.wtp_info->remote_bt_addr);
            }
            if (call_info.wtp_info != NULL) {
                free(call_info.wtp_info);
            }
            if (ret < 0) {
                syslog(LOG_ERR, "%s:answer call fail,ret=%d", __func__, ret);
            }
            free_wtp_call_data(g_incoming_wtp_call);
            g_incoming_wtp_call = NULL;
        } else {
            syslog(LOG_ERR, "%s: no incoming wtp call exist", __func__);
            ret = -1;
        }
#else
        syslog(LOG_ERR, "%s: CONFIG_PHONE_SERVICE_WTP not supporrt", __func__);
#endif
    } else {
        syslog(LOG_ERR, "%s:not supporrt", __func__);
    }
done:
    if (ret < 0) {
        syslog(LOG_ERR, "%s:answer call fail,ret=%d", __func__, ret);
    }
    return ret;
}

static int telephonytool_cmd_hangup_call(char* pargs)
{
    char dst[2][MAX_INPUT_ARGS_LEN];
    int cnt;
    int ret = -1;
    int type;
    tapi_call_data_t call_info;

    if (strlen(pargs) == 0) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    cnt = split_input(dst, 2, pargs, " ");
    if (!(cnt == 1 || cnt == 2)) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    type = atoi(dst[0]);
    if (type == 0) {
        call_info.phone_info = (tapi_cell_call_data_t*)calloc(1, sizeof(tapi_cell_call_data_t));
        if (call_info.phone_info == NULL) {
            syslog(LOG_ERR, "%s:calloc fail", __func__);
            goto done;
        }
        call_info.phone_info->slot = 0;
        call_info.phone_info->call_id = dst[1];
        call_info.wtp_info = NULL;
        ret = tapi_hangup_call(call_info, hangup_call_done, NULL);
        free(call_info.phone_info);
    } else if (type == 1) {
#ifdef CONFIG_PHONE_SERVICE_WTP
        if (g_connected_wtp_call != NULL) { // just hangup connected call
            call_info.wtp_info = (tapi_wtp_call_data_t*)calloc(1, sizeof(tapi_wtp_call_data_t)); // need consider calloc other_info if exist
            if (call_info.wtp_info == NULL) {
                syslog(LOG_ERR, "%s:calloc fail", __func__);
                return -1;
            }
            call_info.phone_info = NULL;
            call_info.wtp_info->remote_bt_addr = bt_address_to_string(g_connected_wtp_call->remote->addr);
            if (call_info.wtp_info->remote_bt_addr == NULL) {
                syslog(LOG_ERR, "%s: addr is not correct", __func__);
                return -1;
            }
            call_info.wtp_info->other_info_len = 0;

            ret = tapi_hangup_call(call_info, hangup_call_done, NULL);
            if (call_info.wtp_info->remote_bt_addr != NULL) {
                free(call_info.wtp_info->remote_bt_addr);
            }
            if (call_info.wtp_info != NULL) {
                free(call_info.wtp_info);
            }
            if (ret < 0) {
                syslog(LOG_ERR, "%s:hangup call fail,ret=%d", __func__, ret);
            }
        } else {
            syslog(LOG_ERR, "%s: no connected wtp call exist", __func__);
            ret = -1;
        }
#else
        syslog(LOG_ERR, "%s: CONFIG_PHONE_SERVICE_WTP not supporrt", __func__);
#endif
    } else {
        syslog(LOG_ERR, "%s:not supporrt", __func__);
    }
done:
    if (ret < 0) {
        syslog(LOG_ERR, "%s:hangup call fail,ret=%d", __func__, ret);
    }
    return ret;
}

static int telephonytool_cmd_hangup_all_call(char* pargs)
{
    char dst[1][MAX_INPUT_ARGS_LEN];
    int cnt;
    int ret = -1;
    int type;
    tapi_call_data_t call_info;

    if (strlen(pargs) == 0) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    cnt = split_input(dst, 1, pargs, " ");
    if (!(cnt == 1)) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    type = atoi(dst[0]);
    if (type == 0) {
        call_info.phone_info = (tapi_cell_call_data_t*)calloc(1, sizeof(tapi_cell_call_data_t));
        if (call_info.phone_info == NULL) {
            syslog(LOG_ERR, "%s:calloc fail", __func__);
            goto done;
        }
        call_info.phone_info->slot = 0;
        call_info.phone_info->call_id = NULL;
        call_info.wtp_info = NULL;

        ret = tapi_hangup_call(call_info, hangup_call_done, NULL);
        free(call_info.phone_info);
    } else {
        syslog(LOG_ERR, "%s:not supporrt", __func__);
    }
done:
    if (ret < 0) {
        syslog(LOG_ERR, "%s:hangup all call fail,ret=%d", __func__, ret);
    }
    return ret;
}

static int telephonytool_cmd_release_and_answer(char* pargs)
{
    int ret = 0;

    printf("%s\n", __func__);

    ret = tapi_release_and_answer_call(0, release_and_answer_call_callback_done, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "%s:release and answer call fail,ret=%d", __func__, ret);
    }
    return ret;
}

static int telephonytool_cmd_hold_and_answer(char* pargs)
{
    int ret = 0;

    printf("%s\n", __func__);

    ret = tapi_hold_and_answer_call(0, hold_and_answer_call_callback_done, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "%s:hold and answer call fail,ret=%d", __func__, ret);
    }
    return ret;
}

static int telephonytool_cmd_hold(char* pargs)
{
    char dst[1][MAX_INPUT_ARGS_LEN];
    int cnt;
    int ret = -1;
    bool hold_flag;

    if (strlen(pargs) == 0) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    cnt = split_input(dst, 1, pargs, " ");
    if (!(cnt == 1)) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    hold_flag = atoi(dst[0]) ? true : false;
    ret = tapi_hold_call(0, hold_flag, hold_call_callback_done, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "%s:hold call fail,ret=%d", __func__, ret);
    }
    return ret;
}

static int telephonytool_cmd_merge(char* pargs)
{
    int ret = 0;

    printf("%s\n", __func__);

    ret = tapi_merge_call(0, merge_call_callback_done, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "%s:merge call fail,ret=%d", __func__, ret);
    }
    return ret;
}

static int telephonytool_cmd_send_tones(char* pargs)
{
    char dst[1][MAX_INPUT_ARGS_LEN];
    int cnt;
    int ret = -1;

    if (strlen(pargs) == 0) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    cnt = split_input(dst, 1, pargs, " ");
    if (!(cnt == 1)) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    ret = tapi_send_tones(dst[0], send_tones_callback_done, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "%s:send tones fail,ret=%d", __func__, ret);
    }
    return ret;
}

void* linked_list_find(MLinkedList* list, int target_id)
{
    if (!list)
        return NULL;

    MListNode* current;
    SIMPLEQ_FOREACH(current, &list->head, entries)
    {
        if (current->id == target_id) {
            return current->data;
        }
    }
    return NULL;
}

static int telephonytool_cmd_dial_call(char* pargs)
{
    char dst[3][MAX_INPUT_ARGS_LEN];
    int cnt;
    int ret = -1;
    int type;
    tapi_call_data_t call_info;

    if (strlen(pargs) == 0) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        ret = -EINVAL;
        goto done;
    }
    cnt = split_input(dst, 3, pargs, " ");
    if (!(cnt == 3 || cnt == 2)) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        ret = -EINVAL;
        goto done;
    }
    type = atoi(dst[0]);
    if (type == 0) {
        call_info.phone_info = (tapi_cell_call_data_t*)calloc(1, sizeof(tapi_cell_call_data_t));
        if (call_info.phone_info == NULL) {
            syslog(LOG_ERR, "%s:calloc fail", __func__);
            goto done;
        }
        call_info.phone_info->slot = 0;
        call_info.phone_info->phone_number = dst[1];
        call_info.phone_info->hide_callerid = atoi(dst[2]);
        call_info.phone_info->call_id = NULL;
        call_info.wtp_info = NULL;

        ret = tapi_dial_call(call_info, dial_call_done, NULL);
        free(call_info.phone_info);

    } else if (type == 1) {
#ifdef CONFIG_PHONE_SERVICE_WTP
        int id = atoi(dst[1]);
        wtp_whole_data_t* wtp_data = linked_list_find(g_wtp_device_list, id);

        if (wtp_data == NULL) {
            syslog(LOG_ERR, "%s:no device found for dial", __func__);
            goto done;
        }

        call_info.wtp_info = (tapi_wtp_call_data_t*)calloc(1, sizeof(tapi_wtp_call_data_t)); // need consider calloc other_info if exist
        if (call_info.wtp_info == NULL) {
            syslog(LOG_ERR, "%s:calloc fail", __func__);
            goto done;
        }
        call_info.phone_info = NULL;
        call_info.wtp_info->remote_bt_addr = bt_address_to_string(wtp_data->remote->addr);
        if (call_info.wtp_info->remote_bt_addr == NULL) {
            syslog(LOG_ERR, "%s: addr is not correct", __func__);
            free(call_info.wtp_info);
            goto done;
        }
        call_info.wtp_info->other_info_len = 0;

        ret = tapi_dial_call(call_info, dial_call_done, NULL);
        if (call_info.wtp_info->remote_bt_addr != NULL) {
            free(call_info.wtp_info->remote_bt_addr);
        }
        if (call_info.wtp_info != NULL) {
            free(call_info.wtp_info);
        }
#else
        syslog(LOG_ERR, "%s: CONFIG_PHONE_SERVICE_WTP not supporrt", __func__);
#endif
    } else {
        syslog(LOG_ERR, "%s:not supporrt", __func__);
    }
done:
    if (ret < 0) {
        syslog(LOG_ERR, "%s:dial call fail,ret=%d", __func__, ret);
    }
    return ret;
}

#ifdef CONFIG_PHONE_SERVICE_WTP
static int telephonytool_cmd_modify_wtp_visibility(char* pargs)
{
    char dst[1][MAX_INPUT_ARGS_LEN];
    int cnt;
    int ret = 0;
    int enable;

    if (strlen(pargs) == 0) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    cnt = split_input(dst, 1, pargs, " ");
    if (cnt != 1) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    syslog(LOG_INFO, "%s,%s", __func__, dst[0]);
    enable = atoi(dst[0]);

    ret = tapi_wtp_modify_visibility(enable, modify_wtp_visibility_done, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "%s:modify wtp visibility fail,ret=%d", __func__, ret);
    }
    return ret;
}

static int telephonytool_cmd_modify_wtp_discovery(char* pargs)
{
    char dst[1][MAX_INPUT_ARGS_LEN];
    int cnt;
    int ret = 0;
    int enable;

    if (strlen(pargs) == 0) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    cnt = split_input(dst, 1, pargs, " ");
    if (cnt != 1) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    syslog(LOG_INFO, "%s,%s", __func__, dst[0]);
    enable = atoi(dst[0]);
    if (g_wtp_device_list != NULL) {
        MListNode* entry = NULL;

        while ((entry = SIMPLEQ_FIRST(&g_wtp_device_list->head)) != NULL) {
            SIMPLEQ_REMOVE_HEAD(&g_wtp_device_list->head, entries);
#ifdef CONFIG_PHONE_SERVICE_WTP
            free_wtp_call_data(entry->data);
#endif
            free(entry);
        }
    }
    SIMPLEQ_INIT(&g_wtp_device_list->head); // 重置队列头
    g_wtp_device_list->next_id = 1; // 重置ID计数器

    ret = tapi_wtp_modify_discovery(enable, modify_wtp_discovery_done, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "%s:modify wtp discovery fail,ret=%d", __func__, ret);
    }
    return ret;
}

static int telephonytool_cmd_set_wtp_local_info(char* pargs)
{
    char dst[3][MAX_INPUT_ARGS_LEN];
    int cnt;
    wtp_local_t* local_info = (wtp_local_t*)malloc(sizeof(wtp_local_t));
    int ret = 0;

    if (strlen(pargs) == 0) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    cnt = split_input(dst, 3, pargs, " ");
    if (cnt != 3) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }

    local_info->name = strdup(dst[0]);
    local_info->number1 = strdup(dst[1]);
    local_info->number2 = strdup(dst[2]);
    local_info->position = (wtp_data_t*)malloc(sizeof(wtp_data_t) + sizeof(uint8_t) * 2);

    local_info->position->length = 2;
    local_info->position->type = 5;
    local_info->position->rfu = 6;
    local_info->position->value[0] = 1234 & 0xFF;
    local_info->position->value[1] = (1234 >> 8) & 0xFF;

    ret = tapi_wtp_set_local_info(local_info, set_wtp_local_info_done, NULL);

    if (ret < 0) {
        syslog(LOG_ERR, "%s:set wtp local info fail,ret=%d", __func__, ret);
    }

    free(local_info->position);
    free(local_info->number2);
    free(local_info->number1);
    free(local_info->name);
    free(local_info);
    return ret;
}

static int telephonytool_cmd_unregister_wtp_callback(char* pargs)
{
    int ret = 0;

    printf("%s\n", __func__);
    ret = tapi_client_wtp_unregister_cb(unregister_wtp_callback_done, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "%s:unregister wtp callback fail,ret=%d", __func__, ret);
    }
    return ret;
}

static const wtp_callbacks_t cb_info = {
    .size = sizeof(wtp_callbacks_t),
    .connection_state_changed_cb = wtp_call_conn_cb,
    .discovery_state_changed_cb = wtp_call_discovery_cb,
    .visibility_changed_cb = wtp_call_visibility_cb,
    .transport_requested_cb = wtp_call_requested_cb,
    .device_found_cb = wtp_call_device_found_cb,
    .remote_info_changed_cb = wtp_call_remote_info_update_cb,
};

static int telephonytool_cmd_register_wtp_callback(char* pargs)
{
    int ret = 0;

    printf("%s\n", __func__);

    ret = tapi_client_wtp_register_cb(&cb_info, register_wtp_callback_done, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "%s:register wtp callback fail,ret=%d", __func__, ret);
    }
    return ret;
}
#endif

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
}

static void register_esim_callback_done(tapi_async_result* result)
{
    printf("%s,%d\n", __func__, result->status);
}

static tele_callbacks_t esim_cb_info = {
    .radio_state_change_cb = esim_radio_state_change_cb,
    .operator_status_changed_cb = esim_operator_status_changed_cb,
    .operator_name_changed_cb = esim_operator_name_changed_cb,
    .network_reg_state_changed_cb = esim_network_reg_state_changed_cb,
    .strength_changed_cb = esim_network_strength_changed_cb,
    .modem_status_changed_cb = esim_modem_status_changed_cb,
    .radio_power_changed_cb = esim_radio_power_changed_cb,
    .call_state_changed_cb = esim_call_state_changed_cb,
};

static int telephonytool_cmd_register_esim_callback(char* pargs)
{
    int ret = 0;

    printf("%s\n", __func__);

    ret = tapi_client_register_callbacks(esim_cb_info, register_esim_callback_done, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "%s:register esim callback fail,ret=%d", __func__, ret);
    }
    return ret;
}

static void unregister_esim_callback_done(tapi_async_result* result)
{
    printf("%s,%d\n", __func__, result->status);
}

static int telephonytool_cmd_unregister_esim_callback(char* pargs)
{
    int ret = 0;

    printf("%s\n", __func__);

    ret = tapi_client_unregister_callbacks(unregister_esim_callback_done, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "%s:unregister esim callback fail,ret=%d", __func__, ret);
    }
    return ret;
}

static void esim_set_radio_power_done(tapi_async_result* result)
{
    printf("%s,%d\n", __func__, result->status);
}

static int telephonytool_cmd_set_esim_radio_power(char* pargs)
{
    char dst[3][MAX_INPUT_ARGS_LEN];
    int cnt;
    int ret = 0;
    bool enable;

    if (strlen(pargs) == 0) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    cnt = split_input(dst, 1, pargs, " ");
    if (cnt != 1) {
        syslog(LOG_ERR, "%s:parameter num is not correct", __func__);
        return -EINVAL;
    }
    enable = atoi(dst[0]);

    printf("%s\n", __func__);

    ret = tapi_client_set_radio_power(enable, esim_set_radio_power_done, NULL);
    if (ret < 0) {
        syslog(LOG_ERR, "%s:set esim radio power fail,ret=%d", __func__, ret);
    }
    return ret;
}

static struct phoneservicetool_cmd_s g_telephony_phoneservice_cmds[] = {
/*phone service command*/
#ifdef CONFIG_PHONE_SERVICE_WTP
    { "phone-register-wtp", PHONE_SERVICE_CMD,
        telephonytool_cmd_register_wtp_callback,
        "register wtp cb function(enter example :phone-register-wtp)" },
    { "phone-unregister-wtp", PHONE_SERVICE_CMD,
        telephonytool_cmd_unregister_wtp_callback,
        "unregister wtp cb function(enter example :phone-unregister-wtp)" },
    { "phone-set-wtp-local-info", PHONE_SERVICE_CMD,
        telephonytool_cmd_set_wtp_local_info,
        "set device local info for wtp call(enter example :phone-set-wtp-local-info telephonytool 10010 10011"
        "[device_name][phone_number1][phone_number2]" },
    { "phone-modify-wtp-discovery", PHONE_SERVICE_CMD,
        telephonytool_cmd_modify_wtp_discovery,
        "enable/disable discovery(enter example :phone-modify-wtp-discovery 0 [state:0-disable,1-enable" },
    { "phone-modify-wtp-visibility", PHONE_SERVICE_CMD,
        telephonytool_cmd_modify_wtp_visibility,
        "enable/disable visibility(enter example :phone-modify-wtp-visibility 0"
        "[state:0-disable,1-enable" },
    { "phone-set-audio-type", PHONE_SERVICE_CMD,
        telephonytool_cmd_set_audio_type,
        "set audio type for call(enter example :phone-set-audio-type 1"
        "[audio type:0-speaker,1-headphones]" },
#endif
    { "phone-dial-call", PHONE_SERVICE_CMD,
        telephonytool_cmd_dial_call,
        "dial a call(enter example :phone-dial-call 1 1"
        "[call type:0-esim/hf,1-wtp][id:phone number or wtp device id(match found device)]"
        "[hide_call_id, 0:show 1:hide, it is used when type=0]" },
    { "phone-hangup-call", PHONE_SERVICE_CMD,
        telephonytool_cmd_hangup_call,
        "hangup a call(enter example :phone-hangup-call 1"
        "[call type:0-esim/hf,1-wtp]"
        "[call_id, /ril_0/voicecall01 it is used when type=0]" },
    { "phone-hangup-all-call", PHONE_SERVICE_CMD,
        telephonytool_cmd_hangup_all_call,
        "hangup all call(enter example :phone-hangup-all-call 0"
        "[call type:0-esim/hf]" },
    { "phone-answer-call", PHONE_SERVICE_CMD,
        telephonytool_cmd_answer_call, "answer a call(enter example :phone-answer-call 1"
                                       "[call type:0-esim/hf,1-wtp]"
                                       "[call_id, /ril_0/voicecall01 it is used when type=0]" },
    { "phone-reject-call", PHONE_SERVICE_CMD,
        telephonytool_cmd_reject_call,
        "reject a call(enter example :phone-reject-call 1"
        "[call type:0-esim/hf,1-wtp]"
        "[call_id, /ril_0/voicecall01 it is used when type=0]" },
    { "phone-register-esim", PHONE_SERVICE_CMD,
        telephonytool_cmd_register_esim_callback,
        "register esim cb function(enter example :phone-register-esim)" },
    { "phone-unregister-esim", PHONE_SERVICE_CMD,
        telephonytool_cmd_unregister_esim_callback,
        "unregister esim cb function(enter example :phone-unregister-esim)" },
    { "phone-set-radio-power", PHONE_SERVICE_CMD,
        telephonytool_cmd_set_esim_radio_power,
        "set esim radio power function(enter example :phone-set-radio-power 0[state:0-disable,1-enable])" },
    { "phone-release-and-answer", PHONE_SERVICE_CMD,
        telephonytool_cmd_release_and_answer,
        "release_and_answer esim call(enter example :phone-release-and-answer)" },
    { "phone-hold-and-answer", PHONE_SERVICE_CMD,
        telephonytool_cmd_hold_and_answer,
        "hold_and_answer esim call(enter example :phone-hold-and-answer)" },
    { "phone-hold-call", PHONE_SERVICE_CMD,
        telephonytool_cmd_hold,
        "hold esim call(enter example :phone-hold-call 0[hold/unhold:1-hold,0-unhold])" },
    { "phone-merge-call", PHONE_SERVICE_CMD,
        telephonytool_cmd_merge,
        "merge esim call(enter example :phone-merge-call)" },
    { "phone-send-tones", PHONE_SERVICE_CMD,
        telephonytool_cmd_send_tones,
        "send tones(enter example :phone-send-tones 11[dtmf])" },
    { 0 },
};

void show_tapi_phoneservice_cmd(void)
{
    int i;

    for (i = 0; g_telephony_phoneservice_cmds[i].cmd; i++) {
        printf("%-35s %s\n", g_telephony_phoneservice_cmds[i].cmd,
            g_telephony_phoneservice_cmds[i].help);
    }
    printf("\n");
}

bool execute_phone_service_cmd(char* cmd, char* arg)
{
    int i;
    bool find_flag = false;

    for (i = 0; g_telephony_phoneservice_cmds[i].cmd; i++) {
        if (strcmp(cmd, g_telephony_phoneservice_cmds[i].cmd) == 0) {
            find_flag = true;
            if (g_telephony_phoneservice_cmds[i].pfunc(arg) < 0)
                printf("cmd:%s input parameter:%s invalid \n", cmd, arg);

            return find_flag;
        }
    }

    return find_flag;
}

int phone_client_init(void)
{
    g_wtp_device_list = malloc(sizeof(MLinkedList));
    if (g_wtp_device_list) {
        SIMPLEQ_INIT(&g_wtp_device_list->head);
        g_wtp_device_list->next_id = 1;
    } else {
        printf("%s:create linkedlist fail\n", __func__);
        return -errno;
    }
    if (tapi_start_phone_service_client(uv_default_loop(), NULL, false) < 0) {
        printf("error:phone service client init fail\n");
        return -errno;
    }
    return 0;
}

void phone_client_clean(void)
{
    tapi_stop_phone_service_client();
    if (g_wtp_device_list) {
        MListNode* entry = NULL;

        while ((entry = SIMPLEQ_FIRST(&g_wtp_device_list->head)) != NULL) {
            SIMPLEQ_REMOVE_HEAD(&g_wtp_device_list->head, entries);
#ifdef CONFIG_PHONE_SERVICE_WTP
            free_wtp_call_data(entry->data);
#endif
            free(entry);
        }
        free(g_wtp_device_list);
        g_wtp_device_list = NULL;
    }
#ifdef CONFIG_PHONE_SERVICE_WTP
    free_wtp_call_data(g_incoming_wtp_call);
    g_incoming_wtp_call = NULL;
    free_wtp_call_data(g_connected_wtp_call);
    g_connected_wtp_call = NULL;
#endif
}
