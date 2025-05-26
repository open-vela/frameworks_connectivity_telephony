/*
 * Copyright (C) 2025 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "tapi_phone.h"
#include "conn_xpc_client.h"
#include "conn_xpc_client_msg.h"
#include "tapi_common.h"
#include "tapi_xpc.h"
#include <uv.h>

#define PHONE_SERVICE_SOCKET_PATH "telephony/phone_service"
#define COLON_NUM 5

int string_to_bt_address(bt_address_t* addr, const char* str)
{
    int colon_count = 0;
    int result = 0;

    if (addr == NULL || str == NULL) {
        tapi_log_error("%s: addr or str is null", __func__);
        return -1;
    }

    if (strlen(str) < BT_ADDR_STR_LENGTH) {
        tapi_log_error("%s: str len is not correct", __func__);
        return -1;
    }

    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == ':') {
            colon_count++;
        }
    }
    if (colon_count != COLON_NUM) {
        tapi_log_error("%s: str format is not correct", __func__);
        return -1;
    }

    result = sscanf(str, "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX",
        &addr->addr[0], &addr->addr[1], &addr->addr[2],
        &addr->addr[3], &addr->addr[4], &addr->addr[5]);

    if (result != BT_ADDR_LENGTH) {
        tapi_log_error("%s: parsing bt addr fail", __func__);
        return -1;
    }

    return 0;
}

tapi_async_handler* create_aync_handler(int msg_id, tapi_async_function async_cb, void* user_obj)
{
    tapi_async_handler* handler;
    tapi_async_result* ar;

    tapi_log_info("%s", __func__);

    handler = calloc(1, sizeof(tapi_async_handler));
    if (handler == NULL) {
        tapi_log_error("%s:calloc tapi async handler fail", __func__);
        return NULL;
    }

    handler->cb_function = async_cb;
    ar = calloc(1, sizeof(tapi_async_result));
    if (ar == NULL) {
        free(handler);
        tapi_log_error("%s:calloc tapi async result fail", __func__);
        return NULL;
    }
    handler->result = ar;
    ar->msg_id = msg_id;
    ar->user_obj = user_obj;

    return handler;
}

static wtp_xpc_data_t* covert_wtp_data_to_xpc_data_in_client(tapi_wtp_call_data_t* wtp_data, int msg_id, tapi_async_function async_cb, void* user_obj)
{
    wtp_xpc_data_t* xpc_info;
    tapi_async_handler* handler;
    int ret;

    tapi_log_info("%s", __func__);
    if (wtp_data == NULL || wtp_data->remote_bt_addr == NULL) {
        tapi_log_error("%s: wtp_data or wtp_data->remote is NULL", __func__);
        return NULL;
    }

    xpc_info = (wtp_xpc_data_t*)calloc(1, sizeof(wtp_xpc_data_t) + sizeof(uint8_t) * wtp_data->other_info_len);
    if (xpc_info == NULL) {
        tapi_log_error("%s: calloc failed", __func__);
        return NULL;
    }

    ret = string_to_bt_address(&xpc_info->device_info.addr, wtp_data->remote_bt_addr);
    if (ret != 0) {
        tapi_log_error("%s: remote_bt_addr is null", __func__);
        free(xpc_info);
        return NULL;
    }

    // copy other info
    xpc_info->other_info_len = wtp_data->other_info_len;
    if (wtp_data->other_info_len > 0) {
        memcpy(xpc_info->value, wtp_data->value, sizeof(uint8_t) * wtp_data->other_info_len);
    }

    handler = create_aync_handler(msg_id, async_cb, user_obj);
    if (handler == NULL) {
        tapi_log_error("%s:aync handler create fail", __func__);
        free(xpc_info);
        return NULL;
    }
    xpc_info->user_data = handler;

    return xpc_info;
}

static void free_wtp_remote_in_client(wtp_remote_t* remote)
{
    if (remote == NULL) {
        return;
    }
    if (remote->addr != NULL) {
        free(remote->addr);
        remote->addr = NULL;
    }
    if (remote->name != NULL) {
        free(remote->name);
        remote->name = NULL;
    }
    if (remote->number1 != NULL) {
        free(remote->number1);
        remote->number1 = NULL;
    }
    if (remote->number2 != NULL) {
        free(remote->number2);
        remote->number2 = NULL;
    }
    if (remote->position != NULL) {
        free(remote->position);
        remote->position = NULL;
    }
    free(remote);
}

static wtp_remote_t* covert_xpc_device_to_wtp_remote_in_client(wtp_xpc_device_t* xpc_device)
{
    wtp_remote_t* remote;

    if (xpc_device == NULL) {
        tapi_log_error("%s:xpc_device is NULL", __func__);
        return NULL;
    }

    tapi_log_info("%s", __func__);
    remote = (wtp_remote_t*)calloc(1, sizeof(wtp_remote_t));
    if (remote == NULL) {
        tapi_log_error("%s:calloc wtp_remote_t fail", __func__);
        return NULL;
    }
    remote->addr_type = xpc_device->addr_type;
    remote->signal = xpc_device->signal;
    remote->rfu = xpc_device->rfu;
    remote->addr = calloc(1, sizeof(bt_address_t));
    if (remote->addr == NULL) {
        tapi_log_error("%s:calloc addr fail", __func__);
        free_wtp_remote_in_client(remote);
        return NULL;
    }
    memcpy(remote->addr->addr, xpc_device->addr.addr, BT_ADDR_LENGTH);
    if (xpc_device->name[0] != '\0') {
        size_t len = strlen(xpc_device->name) + 1;
        remote->name = (char*)calloc(len, sizeof(char));
        if (remote->name == NULL) {
            tapi_log_error("%s:calloc name fail", __func__);
            free_wtp_remote_in_client(remote);
            return NULL;
        }
        strcpy(remote->name, xpc_device->name);
    } else {
        remote->name = NULL;
    }

    if (xpc_device->number1[0] != '\0') {
        size_t len = strlen(xpc_device->number1) + 1;
        remote->number1 = (char*)calloc(len, sizeof(char));
        if (remote->number1 == NULL) {
            tapi_log_error("%s:calloc number1 fail", __func__);
            free_wtp_remote_in_client(remote);
            return NULL;
        }
        strcpy(remote->number1, xpc_device->number1);
    } else {
        remote->number1 = NULL;
    }

    if (xpc_device->number2[0] != '\0') {
        size_t len = strlen(xpc_device->number2) + 1;
        remote->number2 = (char*)calloc(len, sizeof(char));
        if (remote->number2 == NULL) {
            tapi_log_error("%s:calloc number2 fail", __func__);
            free_wtp_remote_in_client(remote);
            return NULL;
        }
        strcpy(remote->number2, xpc_device->number2);
    } else {
        remote->number2 = NULL;
    }

    uint16_t len = xpc_device->position[0] | (xpc_device->position[1] << 8);
    if (len > 0) { // position will be null if position length is 0
        remote->position = (wtp_data_t*)calloc(1, sizeof(wtp_data_t) + sizeof(uint8_t) * len);
        if (remote->position == NULL) {
            tapi_log_error("%s:calloc position fail", __func__);
            free_wtp_remote_in_client(remote);
            return NULL;
        }
        remote->position->length = len;
        remote->position->type = xpc_device->position[2];
        remote->position->rfu = xpc_device->position[3];
        memcpy(remote->position->value, xpc_device->position + 4, remote->position->length);
    } else {
        remote->position = NULL;
    }

    return remote;
}

static void free_handler_in_client(void* user_data)
{
    tapi_async_handler* handler;

    tapi_log_info("%s", __func__);
    handler = (tapi_async_handler*)user_data;
    if (handler != NULL) {
        if (handler->result != NULL) {
            free(handler->result);
        }
        free(handler);
    }
}

wtp_xpc_device_local_t* covert_wtp_local_to_xpc_local_in_client(wtp_local_t* wtp_local, int msg_id, tapi_async_function async_cb, void* user_obj)
{
    wtp_xpc_device_local_t* xpc_local;
    tapi_async_handler* handler;

    if (wtp_local == NULL) {
        tapi_log_error("%s:wtp_local is NULL", __func__);
        return NULL;
    }
    tapi_log_info("%s", __func__);
    xpc_local = (wtp_xpc_device_local_t*)calloc(1, sizeof(wtp_xpc_device_local_t));
    if (xpc_local == NULL) {
        tapi_log_error("%s:calloc xpc local fail", __func__);
        return NULL;
    }
    if (wtp_local->name != NULL) {
        strncpy(xpc_local->name, wtp_local->name, WTP_NAME_LEN_MAX);
    }
    if (wtp_local->number1 != NULL) {
        strncpy(xpc_local->number1, wtp_local->number1, WTP_PHONE_NUMBER_LEN_MAX);
    }
    if (wtp_local->number2 != NULL) {
        strncpy(xpc_local->number2, wtp_local->number2, WTP_PHONE_NUMBER_LEN_MAX);
    }
    if (wtp_local->position != NULL) {
        memcpy(xpc_local->position, wtp_local->position, sizeof(wtp_data_t) + sizeof(uint8_t) * wtp_local->position->length);
    }

    handler = create_aync_handler(msg_id, async_cb, user_obj);
    if (handler == NULL) {
        tapi_log_error("%s:aync handler create fail", __func__);
        free(xpc_local);
        return NULL;
    }
    xpc_local->user_data = handler;

    return xpc_local;
}

int tapi_client_wtp_unregister_cb(tapi_async_function async_cb, void* user_obj)
{
    int ret = 0;
    wtp_xpc_unregister_callback_t xpc_cb;
    tapi_async_handler* handler;

    conn_xpc_module_msg_t* module_msg = conn_xpc_module_msg_alloc(PHONE_SERVICE_WTP,
        PHONE_SERVICE_WTP_UNREGISTER_CALLBACK, sizeof(wtp_xpc_unregister_callback_t));
    if (!module_msg) {
        tapi_log_error("%s:malloc module msg fail", __func__);
        return -1;
    }
    handler = create_aync_handler(PHONE_SERVICE_WTP_UNREGISTER_CALLBACK, async_cb, user_obj);
    if (handler == NULL) {
        tapi_log_error("%s:aync handler create fail", __func__);
        return -1;
    }
    xpc_cb.user_data = handler;
    memcpy(module_msg->xpc_msg.value, &xpc_cb, sizeof(wtp_xpc_unregister_callback_t));
    ret = conn_xpc_client_send(module_msg);
    if (ret != 0) {
        tapi_log_error("%s:send xpc message fail", __func__);
        free_handler_in_client(handler);
    }
    return ret; // 0 success,other fail
}

int tapi_client_wtp_register_cb(const wtp_callbacks_t* cb_list, tapi_async_function async_cb, void* user_obj)
{
    int ret = 0;
    wtp_xpc_callbacks_t xpc_cb;
    tapi_async_handler* handler;

    conn_xpc_module_msg_t* module_msg = conn_xpc_module_msg_alloc(PHONE_SERVICE_WTP,
        PHONE_SERVICE_WTP_REGISTER_CALLBACK, sizeof(wtp_xpc_callbacks_t));
    if (!module_msg) {
        tapi_log_error("%s:malloc module msg fail", __func__);
        return -1;
    }
    xpc_cb.wtp_callback.size = cb_list->size;
    xpc_cb.wtp_callback.connection_state_changed_cb = cb_list->connection_state_changed_cb;
    xpc_cb.wtp_callback.discovery_state_changed_cb = cb_list->discovery_state_changed_cb;
    xpc_cb.wtp_callback.visibility_changed_cb = cb_list->visibility_changed_cb;
    xpc_cb.wtp_callback.transport_requested_cb = cb_list->transport_requested_cb;
    xpc_cb.wtp_callback.device_found_cb = cb_list->device_found_cb;
    xpc_cb.wtp_callback.remote_info_changed_cb = cb_list->remote_info_changed_cb;
    handler = create_aync_handler(PHONE_SERVICE_WTP_REGISTER_CALLBACK, async_cb, user_obj);
    if (handler == NULL) {
        tapi_log_error("%s:aync handler create fail", __func__);
        return -1;
    }
    xpc_cb.user_data = handler;
    memcpy(module_msg->xpc_msg.value, &xpc_cb, sizeof(wtp_xpc_callbacks_t));
    ret = conn_xpc_client_send(module_msg);
    if (ret != 0) {
        tapi_log_error("%s:send xpc message fail", __func__);
        free_handler_in_client(xpc_cb.user_data);
    }
    return ret; // 0 success,other fail
}

int tapi_wtp_set_local_info(wtp_local_t* local, tapi_async_function async_cb, void* user_obj)
{
    int ret = 0;
    wtp_xpc_device_local_t* xpc_local;
    conn_xpc_module_msg_t* module_msg;

    xpc_local = covert_wtp_local_to_xpc_local_in_client(local, PHONE_SERVICE_WTP_SET_LOCAL_INFO, async_cb, user_obj);
    if (xpc_local == NULL) {
        tapi_log_error("%s:covert wtp local to xpc local fail", __func__);
        return -1;
    }
    module_msg = conn_xpc_module_msg_alloc(PHONE_SERVICE_WTP,
        PHONE_SERVICE_WTP_SET_LOCAL_INFO, sizeof(wtp_xpc_device_local_t));
    if (!module_msg) {
        tapi_log_error("%s:malloc module msg fail", __func__);
        free_handler_in_client(xpc_local);
        free(xpc_local);
        return -1;
    }
    memcpy(module_msg->xpc_msg.value, xpc_local, sizeof(wtp_xpc_device_local_t));

    ret = conn_xpc_client_send(module_msg);
    if (ret != 0) {
        tapi_log_error("%s:send xpc message fail", __func__);
        free_handler_in_client(xpc_local->user_data);
    }
    free(xpc_local);
    return ret; // 0 success,other fail
}

int tapi_client_set_audio_type(int type, tapi_async_function async_cb, void* user_obj)
{
    int ret = 0;
    wtp_audio_type_t xpc_audio_data;
    conn_xpc_module_msg_t* module_msg;
    tapi_async_handler* handler;

    module_msg = conn_xpc_module_msg_alloc(PHONE_SERVICE_WTP,
        PHONE_SERVICE_WTP_SET_AUDIO_TYPE, sizeof(wtp_audio_type_t));
    if (!module_msg) {
        tapi_log_error("%s:malloc module msg fail", __func__);
        return -1;
    }
    xpc_audio_data.type = type;
    handler = create_aync_handler(PHONE_SERVICE_WTP_SET_AUDIO_TYPE, async_cb, user_obj);
    if (handler == NULL) {
        tapi_log_error("%s:aync handler create fail", __func__);
        return -1;
    }
    xpc_audio_data.user_data = handler;

    memcpy(module_msg->xpc_msg.value, &xpc_audio_data, sizeof(wtp_audio_type_t));
    ret = conn_xpc_client_send(module_msg);
    if (ret != 0) {
        tapi_log_error("%s:send xpc message fail", __func__);
        free_handler_in_client(xpc_audio_data.user_data);
    }
    return ret; // 0 success,other fail
}

int tapi_wtp_modify_discovery(bool enable, tapi_async_function async_cb, void* user_obj)
{
    int ret = 0;
    conn_xpc_module_msg_t* module_msg;
    wtp_discovery_t xpc_data;
    tapi_async_handler* handler;

    module_msg = conn_xpc_module_msg_alloc(PHONE_SERVICE_WTP,
        PHONE_SERVICE_WTP_MODIFY_DISCOVERY, sizeof(wtp_discovery_t));
    if (!module_msg) {
        tapi_log_error("%s:malloc module msg fail", __func__);
        return -1;
    }

    xpc_data.enable = enable;
    handler = create_aync_handler(PHONE_SERVICE_WTP_MODIFY_DISCOVERY, async_cb, user_obj);
    if (handler == NULL) {
        tapi_log_error("%s:aync handler create fail", __func__);
        return -1;
    }
    xpc_data.user_data = handler;
    memcpy(module_msg->xpc_msg.value, &xpc_data, sizeof(wtp_discovery_t));
    ret = conn_xpc_client_send(module_msg);
    if (ret != 0) {
        tapi_log_error("%s:send xpc message fail", __func__);
        free_handler_in_client(xpc_data.user_data);
    }
    return ret; // 0 success,other fail
}

int tapi_wtp_modify_visibility(bool enable, tapi_async_function async_cb, void* user_obj)
{
    int ret = 0;
    conn_xpc_module_msg_t* module_msg;
    wtp_visibility_t xpc_data;
    tapi_async_handler* handler;

    module_msg = conn_xpc_module_msg_alloc(PHONE_SERVICE_WTP,
        PHONE_SERVICE_WTP_MODIFY_VISIBILITY, sizeof(wtp_visibility_t));
    if (!module_msg) {
        tapi_log_error("%s:malloc module msg fail", __func__);
        return -1;
    }

    xpc_data.enable = enable;
    handler = create_aync_handler(PHONE_SERVICE_WTP_MODIFY_VISIBILITY, async_cb, user_obj);
    if (handler == NULL) {
        tapi_log_error("%s:aync handler create fail", __func__);
        return -1;
    }
    xpc_data.user_data = handler;
    memcpy(module_msg->xpc_msg.value, &xpc_data, sizeof(wtp_visibility_t));
    ret = conn_xpc_client_send(module_msg);
    if (ret != 0) {
        tapi_log_error("%s:send xpc message fail", __func__);
        free_handler_in_client(xpc_data.user_data);
    }
    return ret; // 0 success,other fail
}

int tapi_dial_call(tapi_call_data_t call_data, tapi_async_function async_cb, void* user_obj)
{
    int ret = 0;
    wtp_xpc_data_t* xpc_data;
    conn_xpc_module_msg_t* module_msg;

    xpc_data = covert_wtp_data_to_xpc_data_in_client(call_data.wtp_info, PHONE_SERVICE_WTP_DIAL, async_cb, user_obj);
    if (xpc_data == NULL) {
        tapi_log_error("%s,device info fail", __func__);
        return -1;
    }
    module_msg = conn_xpc_module_msg_alloc(PHONE_SERVICE_WTP,
        PHONE_SERVICE_WTP_DIAL, sizeof(wtp_xpc_data_t) + sizeof(uint8_t) * xpc_data->other_info_len);
    if (!module_msg) {
        tapi_log_error("%s,malloc module msg fail", __func__);
        free(xpc_data);
        return -1;
    }
    memcpy(module_msg->xpc_msg.value, xpc_data, sizeof(wtp_xpc_data_t) + sizeof(uint8_t) * xpc_data->other_info_len);
    ret = conn_xpc_client_send(module_msg);
    if (ret != 0) {
        tapi_log_error("%s:send xpc message fail", __func__);
        free_handler_in_client(xpc_data->user_data);
    }
    free(xpc_data);
    return ret; // 0 success,other fail
}

int tapi_hangup_call(tapi_call_data_t call_data, tapi_async_function async_cb, void* user_obj)
{
    int ret = 0;
    wtp_xpc_data_t* xpc_data;
    conn_xpc_module_msg_t* module_msg;

    xpc_data = covert_wtp_data_to_xpc_data_in_client(call_data.wtp_info, PHONE_SERVICE_WTP_HANGUP, async_cb, user_obj);
    if (xpc_data == NULL) {
        tapi_log_error("%s,device info fail", __func__);
        return -1;
    }
    module_msg = conn_xpc_module_msg_alloc(PHONE_SERVICE_WTP,
        PHONE_SERVICE_WTP_HANGUP, sizeof(wtp_xpc_data_t) + sizeof(uint8_t) * xpc_data->other_info_len);
    if (!module_msg) {
        tapi_log_error("malloc module msg fail");
        free(xpc_data);
        return -1;
    }
    memcpy(module_msg->xpc_msg.value, xpc_data, sizeof(wtp_xpc_data_t) + sizeof(uint8_t) * xpc_data->other_info_len);
    ret = conn_xpc_client_send(module_msg);
    if (ret != 0) {
        tapi_log_error("%s:send xpc message fail", __func__);
        free_handler_in_client(xpc_data->user_data);
    }
    free(xpc_data);
    return ret; // 0 success,other fail
}

int tapi_answer_call(tapi_call_data_t call_data, tapi_async_function async_cb, void* user_obj)
{
    int ret = 0;
    wtp_xpc_data_t* xpc_data;
    conn_xpc_module_msg_t* module_msg;

    xpc_data = covert_wtp_data_to_xpc_data_in_client(call_data.wtp_info, PHONE_SERVICE_WTP_ANSWER, async_cb, user_obj);
    if (xpc_data == NULL) {
        tapi_log_error("%s,device info fail", __func__);
        return -1;
    }
    module_msg = conn_xpc_module_msg_alloc(PHONE_SERVICE_WTP,
        PHONE_SERVICE_WTP_ANSWER, sizeof(wtp_xpc_data_t) + sizeof(uint8_t) * xpc_data->other_info_len);
    if (!module_msg) {
        tapi_log_error("malloc module msg fail");
        free(xpc_data);
        return -1;
    }
    memcpy(module_msg->xpc_msg.value, xpc_data, sizeof(wtp_xpc_data_t) + sizeof(uint8_t) * xpc_data->other_info_len);
    ret = conn_xpc_client_send(module_msg);
    if (ret != 0) {
        tapi_log_error("%s:send xpc message fail", __func__);
        free_handler_in_client(xpc_data->user_data);
    }
    free(xpc_data);
    return ret; // 0 success,other fail
}

int tapi_reject_call(tapi_call_data_t call_data, tapi_async_function async_cb, void* user_obj)
{
    int ret = 0;
    wtp_xpc_data_t* xpc_data;
    conn_xpc_module_msg_t* module_msg;

    xpc_data = covert_wtp_data_to_xpc_data_in_client(call_data.wtp_info, PHONE_SERVICE_WTP_REJECT, async_cb, user_obj);
    if (xpc_data == NULL) {
        tapi_log_error("%s,device info fail", __func__);
        return -1;
    }
    module_msg = conn_xpc_module_msg_alloc(PHONE_SERVICE_WTP,
        PHONE_SERVICE_WTP_REJECT, sizeof(wtp_xpc_data_t) + sizeof(uint8_t) * xpc_data->other_info_len);
    if (!module_msg) {
        tapi_log_error("malloc module msg fail");
        free(xpc_data);
        return -1;
    }
    memcpy(module_msg->xpc_msg.value, xpc_data, sizeof(wtp_xpc_data_t) + sizeof(uint8_t) * xpc_data->other_info_len);
    ret = conn_xpc_client_send(module_msg);
    if (ret != 0) {
        tapi_log_error("%s:send xpc message fail", __func__);
        free_handler_in_client(xpc_data->user_data);
    }
    free(xpc_data);
    return ret; // 0 success,other fail
}

int32_t phone_service_wtp_client_dispatch(uv_stream_t* handle, conn_xpc_msg_t* xpc_msg)
{
    tapi_async_handler* handler;

    tapi_log_info("%s,%d", __func__, xpc_msg->msg_type);

    if (xpc_msg->msg_type >= PHONE_SERVICE_WTP_MAX) {
        tapi_log_error("%s:unexpected msg", __func__);
        return -1;
    }

    if (xpc_msg->msg_type == PHONE_SERVICE_WTP_CONN_STATE_CHANGED) {
        if (xpc_msg->len != sizeof(wtp_conn_state_xpc_data_t)) {
            tapi_log_error("%s:unexpected msg len", __func__);
            return -1;
        }
        wtp_conn_state_xpc_data_t* data = (wtp_conn_state_xpc_data_t*)xpc_msg->value;
        wtp_remote_t* remote = covert_xpc_device_to_wtp_remote_in_client(&data->device_info);
        if (data->func_cb != NULL) {
            data->func_cb(NULL, remote, data->state, data->reason, &data->param);
        }
        free_wtp_remote_in_client(remote);
    } else if (xpc_msg->msg_type == PHONE_SERVICE_WTP_DISCOVERY_STATE_CHANGED) {
        if (xpc_msg->len != sizeof(wtp_discovery_state_xpc_data_t)) {
            tapi_log_error("%s:unexpected msg len", __func__);
            return -1;
        }
        wtp_discovery_state_xpc_data_t* data = (wtp_discovery_state_xpc_data_t*)xpc_msg->value;
        if (data->func_cb != NULL) {
            data->func_cb(NULL, data->started, data->reason);
        }
    } else if (xpc_msg->msg_type == PHONE_SERVICE_WTP_VISIBILITY_STATE_CHANGED) {
        if (xpc_msg->len != sizeof(wtp_visibility_state_xpc_data_t)) {
            tapi_log_error("%s:unexpected msg len", __func__);
            return -1;
        }
        wtp_visibility_state_xpc_data_t* data = (wtp_visibility_state_xpc_data_t*)xpc_msg->value;
        if (data->func_cb != NULL) {
            data->func_cb(NULL, data->visible, data->reason);
        }
    } else if (xpc_msg->msg_type == PHONE_SERVICE_WTP_TRANSPORT_REQUESTED) {
        if (xpc_msg->len != sizeof(wtp_requested_xpc_data_t)) {
            tapi_log_error("%s:unexpected msg len", __func__);
            return -1;
        }
        wtp_requested_xpc_data_t* data = (wtp_requested_xpc_data_t*)xpc_msg->value;
        wtp_remote_t* remote = covert_xpc_device_to_wtp_remote_in_client(&data->device_info);
        if (data->func_cb != NULL) {
            data->func_cb(NULL, remote, &data->param);
        }
        free_wtp_remote_in_client(remote);
    } else if (xpc_msg->msg_type == PHONE_SERVICE_WTP_DEVICE_FOUND) {
        if (xpc_msg->len != sizeof(wtp_found_xpc_data_t)) {
            tapi_log_error("%s:unexpected msg len", __func__);
            return -1;
        }
        wtp_found_xpc_data_t* data = (wtp_found_xpc_data_t*)xpc_msg->value;
        wtp_remote_t* remote = covert_xpc_device_to_wtp_remote_in_client(&data->device_info);
        if (data->func_cb != NULL) {
            data->func_cb(NULL, remote, &data->param);
        }
        free_wtp_remote_in_client(remote);
    } else if (xpc_msg->msg_type == PHONE_SERVICE_WTP_REMOTE_INFO_CHANGED) {
        if (xpc_msg->len != sizeof(wtp_remote_info_xpc_data_t)) {
            tapi_log_error("%s:unexpected msg len", __func__);
            return -1;
        }
        wtp_remote_info_xpc_data_t* data = (wtp_remote_info_xpc_data_t*)xpc_msg->value;
        wtp_remote_t* remote = covert_xpc_device_to_wtp_remote_in_client(&data->device_info);
        if (data->func_cb != NULL) {
            data->func_cb(NULL, remote);
        }
        free_wtp_remote_in_client(remote);
    } else {
        common_resp_t* resp = (common_resp_t*)xpc_msg->value;
        if (xpc_msg->len != sizeof(common_resp_t)) {
            tapi_log_error("%s:unexpected msg len", __func__);
            return -1;
        }
        if (resp->aync_handler != NULL) {
            handler = resp->aync_handler;
        } else {
            tapi_log_error("%s:resp->aync_handler == NULL", __func__);
            return -1;
        }
        if (handler->result != NULL) {
            handler->result->status = resp->ret;
        } else {
            tapi_log_error("%s:handler->result == NULL", __func__);
            free_handler_in_client(handler);
            return -1;
        }
        if (handler->cb_function != NULL) {
            handler->cb_function(handler->result);
        } else {
            tapi_log_error("%s:handler->cb_function == NULL", __func__);
        }
        free_handler_in_client(handler);
    }
    return 0;
}

int tapi_start_phone_service_client(uv_loop_t* loop, phone_client_status_cb cb, void* user_data)
{
    tapi_log_info("%s", __func__);
    if (start_conn_xpc_client(loop, PHONE_SERVICE_SOCKET_PATH, user_data) < 0) {
        tapi_log_error("start xpc client failed\n");
        return -1;
    }
    if (cb != NULL) {
        register_conn_xpc_client_pipe_status_cb(cb);
    }
    // register wtp dispatch func
    register_conn_xpc_client_service(PHONE_SERVICE_WTP, phone_service_wtp_client_dispatch);
    return 0;
}

void tapi_stop_phone_service_client()
{
    unregister_conn_xpc_client_service(PHONE_SERVICE_WTP);
    stop_conn_xpc_client();
}
