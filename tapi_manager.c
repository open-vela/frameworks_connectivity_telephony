/*
 * Copyright (C) 2023 Xiaomi Corporation
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ofono/dfx.h>
#include <stdio.h>

#include "tapi_internal.h"
#include "tapi_manager.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_DBUS_INIT_RETRY_COUNT 6
#define MAX_DBUS_INIT_RETRY_INTERVAL_MS 500

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

typedef struct {
    dbus_context* context;
    const char* client_name;
    void* user_data;
    tapi_client_ready_function callback;
} client_ready_cb_data;

typedef struct {
    int length;
    void* oem_req;
} oem_ril_request_data;

typedef struct {
    bool enable;
    int module_mask;
    int from_event_id;
    int to_event_id;
} abnormal_event_data;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int object_filter(GDBusProxy* proxy)
{
    const char* interface = g_dbus_proxy_get_interface(proxy);
    if (interface == NULL) {
        tapi_log_error("interface in %s is null", __func__);
        return false;
    }

    // ss and sms related interface skip get properties
    if ((strcmp(interface, OFONO_CALL_BARRING_INTERFACE) == 0)
        || (strcmp(interface, OFONO_CALL_FORWARDING_INTERFACE) == 0)
        || (strcmp(interface, OFONO_CALL_SETTINGS_INTERFACE) == 0)
        || (strcmp(interface, OFONO_MESSAGE_MANAGER_INTERFACE) == 0)) {
        return true;
    }

    return false;
}

static void object_add(GDBusProxy* proxy, void* user_data)
{
}

static void object_remove(GDBusProxy* proxy, void* user_data)
{
}

static void get_persistent_dbus_proxy(dbus_context* ctx)
{
    ctx->dbus_proxy_manager = g_dbus_proxy_new(
        ctx->client, OFONO_MANAGER_PATH, OFONO_MANAGER_INTERFACE);

    for (int i = 0; i < CONFIG_MODEM_ACTIVE_COUNT; i++) {
        ctx->dbus_proxy[i][0] = g_dbus_proxy_new(
            ctx->client, tapi_utils_get_modem_path(i), OFONO_MODEM_INTERFACE);
    }
}

static void release_persistent_dbus_proxy(dbus_context* ctx)
{
    g_dbus_proxy_unref(ctx->dbus_proxy_manager);

    for (int i = 0; i < CONFIG_MODEM_ACTIVE_COUNT; i++) {
        g_dbus_proxy_unref(ctx->dbus_proxy[i][0]);
    }
}

static void get_mutable_dbus_proxy(dbus_context* ctx)
{
    const char* dbus_proxy_server[] = {
        OFONO_MODEM_INTERFACE,
        OFONO_RADIO_SETTINGS_INTERFACE,
        OFONO_VOICECALL_MANAGER_INTERFACE,
        OFONO_SIM_MANAGER_INTERFACE,
        OFONO_STK_INTERFACE,
        OFONO_CONNECTION_MANAGER_INTERFACE,
        OFONO_MESSAGE_MANAGER_INTERFACE,
        OFONO_CELL_BROADCAST_INTERFACE,
        OFONO_NETWORK_REGISTRATION_INTERFACE,
        OFONO_NETMON_INTERFACE,
        OFONO_CALL_BARRING_INTERFACE,
        OFONO_CALL_FORWARDING_INTERFACE,
        OFONO_SUPPLEMENTARY_SERVICES_INTERFACE,
        OFONO_CALL_SETTINGS_INTERFACE,
        OFONO_IMS_INTERFACE,
        OFONO_PHONEBOOK_INTERFACE,
    };

    for (int i = 0; i < CONFIG_MODEM_ACTIVE_COUNT; i++) {
        for (int j = 1; j < DBUS_PROXY_MAX_COUNT; j++) {
            if (!is_interface_supported(dbus_proxy_server[j])) {
                ctx->dbus_proxy[i][j] = NULL;
                continue;
            }

            ctx->dbus_proxy[i][j] = g_dbus_proxy_new(
                ctx->client, tapi_utils_get_modem_path(i), dbus_proxy_server[j]);
        }
    }
}

static void release_mutable_dbus_proxy(dbus_context* ctx)
{
    for (int i = 0; i < CONFIG_MODEM_ACTIVE_COUNT; i++) {
        for (int j = 1; j < DBUS_PROXY_MAX_COUNT; j++) {
            if (ctx->dbus_proxy[i][j] != NULL) {
                g_dbus_proxy_unref(ctx->dbus_proxy[i][j]);
                ctx->dbus_proxy[i][j] = NULL;
            }
        }
    }
}

static void modem_list_query_done(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter args, list;
    DBusError err;
    char* result[MAX_MODEM_COUNT];
    int index;

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return;
    }

    ar = handler->result;
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return;
    }

    cb = handler->cb_function;
    if (cb == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return;
    }

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("%s: %s\n", err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_has_signature(message, "a(oa{sv})") == false) {
        tapi_log_error("message signature in %s is invalid", __func__);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_init(message, &args) == false) {
        tapi_log_error("message iter init failed in %s", __func__);
        ar->status = ERROR;
        goto done;
    }
    dbus_message_iter_recurse(&args, &list);

    index = 0;
    while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_STRUCT) {
        DBusMessageIter entry;
        dbus_message_iter_recurse(&list, &entry);
        dbus_message_iter_get_basic(&entry, &result[index++]);
        if (index >= MAX_MODEM_COUNT)
            break;
        dbus_message_iter_next(&list);
    }
    ar->status = OK;
    ar->arg1 = index; // modem count;
    ar->data = result;

done:
    cb(ar);
}

static void modem_activity_info_query_done(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter, array;
    DBusError err;
    modem_activity_info* info;
    int activity_info[MAX_TX_TIME_ARRAY_LEN + 3];
    int length;

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return;
    }

    ar = handler->result;
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return;
    }

    cb = handler->cb_function;
    if (cb == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return;
    }

    info = malloc(sizeof(modem_activity_info));
    if (info == NULL) {
        tapi_log_error("info in %s is null", __func__);
        return;
    }

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("%s: %s\n", err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        tapi_log_error("message iter init failed in %s", __func__);
        ar->status = ERROR;
        goto done;
    }

    dbus_message_iter_recurse(&iter, &array);
    dbus_message_iter_get_fixed_array(&array, &activity_info, &length);

    if (length != MAX_TX_TIME_ARRAY_LEN + 3) {
        tapi_log_error("length in %s is invalid", __func__);
        ar->status = ERROR;
        goto done;
    }

    info->sleep_time = activity_info[0];
    info->idle_time = activity_info[1];

    for (int i = 0; i < MAX_TX_TIME_ARRAY_LEN; i++) {
        info->tx_time[i] = activity_info[i + 2];
    }

    info->rx_time = activity_info[7];

    ar->status = OK;
    ar->data = info;

done:
    cb(ar);
    if (info != NULL)
        free(info);
}

static void enable_or_disable_modem_done(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusError err;

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return;
    }

    ar = handler->result;
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return;
    }

    cb = handler->cb_function;
    if (cb == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return;
    }

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("%s: %s\n", err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
    }

    ar->status = OK;

done:
    cb(ar);
}

static void enable_modem_abnormal_event_done(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusError err;
    DBusMessageIter iter;

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return;
    }

    ar = handler->result;
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return;
    }

    ar->status = OK;
    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("%s: %s\n", err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
    } else {
        if (dbus_message_iter_init(message, &iter) == false) {
            tapi_log_error("message iter init failed in %s", __func__);
            ar->status = ERROR;
        }

        dbus_message_iter_get_basic(&iter, &ar->arg2);
        tapi_log_info("%s:%d", __func__, ar->arg2);
    }

    cb = handler->cb_function;
    if (cb != NULL) {
        cb(ar);
    }
}

static void modem_status_query_done(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter;
    DBusError err;

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return;
    }

    ar = handler->result;
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return;
    }

    cb = handler->cb_function;
    if (cb == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return;
    }

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("%s: %s\n", err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        tapi_log_error("message iter init failed in %s", __func__);
        ar->status = ERROR;
        goto done;
    }

    dbus_message_iter_get_basic(&iter, &ar->arg2);

    ar->status = OK;

done:
    cb(ar);
}

static int airplane_mode_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter, var;
    const char* property;

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return false;
    }

    ar = handler->result;
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return 0;
    }

    if (ar->msg_id != MSG_AIRPLANE_MODE_CHANGE_IND) {
        tapi_log_error("invalid message id in %s, msg_id: %d", __func__, (int)ar->msg_id);
        return 0;
    }

    cb = handler->cb_function;
    if (cb == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return 0;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        tapi_log_error("message iter init failed in %s", __func__);
        return 0;
    }

    dbus_message_iter_get_basic(&iter, &property);
    dbus_message_iter_next(&iter);

    dbus_message_iter_recurse(&iter, &var);
    if (strcmp(property, "Online") == 0) {
        dbus_message_iter_get_basic(&var, &ar->arg2);
        ar->status = OK;
        cb(ar);
    }

    return 1;
}

static int radio_state_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter, var;
    const char* property;

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return 0;
    }

    ar = handler->result;
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return 0;
    }

    if (ar->msg_id != MSG_RADIO_STATE_CHANGE_IND) {
        tapi_log_error("invalid message id in %s, msg_id: %d", __func__, (int)ar->msg_id);
        return 0;
    }

    cb = handler->cb_function;
    if (cb == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return 0;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        tapi_log_error("message iter init failed in %s", __func__);
        return 0;
    }

    dbus_message_iter_get_basic(&iter, &property);
    dbus_message_iter_next(&iter);

    dbus_message_iter_recurse(&iter, &var);
    if (strcmp(property, "RadioState") == 0) {
        dbus_message_iter_get_basic(&var, &ar->arg2);
        ar->status = OK;
        cb(ar);
    }

    return 1;
}

static int phone_state_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter args;

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return false;
    }

    ar = handler->result;
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return false;
    }

    cb = handler->cb_function;
    if (cb == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return false;
    }

    if (dbus_message_iter_init(message, &args) == false) {
        tapi_log_error("message iter init failed in %s", __func__);
        ar->status = ERROR;
        goto done;
    }

    ar->status = OK;
    dbus_message_iter_get_basic(&args, &ar->arg2);

done:
    cb(ar);
    return true;
}

static int process_oem_hook_raw_indication(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    DBusMessageIter iter, array;
    tapi_async_result* ar;
    tapi_async_function cb;
    unsigned char* response;
    int num;

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return false;
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return false;
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return false;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        tapi_log_error("message iter init failed in %s", __func__);
        ar->status = ERROR;
        goto done;
    }

    dbus_message_iter_recurse(&iter, &array);
    dbus_message_iter_get_fixed_array(&array, &response, &num);

    ar->data = response;
    ar->arg2 = num;
    ar->status = OK;

done:
    cb(ar);
    return true;
}

static int modem_restart(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return 0;
    }

    ar = handler->result;
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return 0;
    }

    cb = handler->cb_function;
    if (cb == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return 0;
    }

    ar->status = OK;
    cb(ar);

    return 1;
}

static int device_info_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return false;
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return false;
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return false;
    }

    ar->status = OK;
    cb(ar);

    return true;
}

static int modem_ecc_list_change(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter, list;
    DBusMessageIter array;
    char* key = NULL;
    int index = 0;
    ecc_info ecc_list[MAX_ECC_LIST_SIZE] = { 0 };

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return 0;
    }

    ar = handler->result;
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return 0;
    }
    ar->msg_id = MSG_ECC_LIST_CHANGE_IND;

    cb = handler->cb_function;
    if (cb == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return 0;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        tapi_log_error("message iter init failed in %s", __func__);
        return 0;
    }

    dbus_message_iter_get_basic(&iter, &key);
    dbus_message_iter_next(&iter);

    if (strcmp(key, "EmergencyNumbers") == 0) {
        dbus_message_iter_recurse(&iter, &list);
        dbus_message_iter_recurse(&list, &array);

        while (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_STRING) {
            char* str;
            char* ptr;

            dbus_message_iter_get_basic(&array, &str);
            ecc_list[index].ecc_num = str;
            dbus_message_iter_next(&array);

            if (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_STRING) {
                dbus_message_iter_get_basic(&array, &str);
                ptr = strtok(str, ",");
                ecc_list[index].category = atoi(ptr);
                ptr = strtok(NULL, ",");
                ecc_list[index].condition = atoi(ptr);
                syslog(LOG_DEBUG, "tapi_call_property_change info:%s,%d,%d",
                    ecc_list[index].ecc_num, ecc_list[index].category,
                    ecc_list[index].condition);
            }

            index++;

            if (index >= MAX_ECC_LIST_SIZE)
                break;

            dbus_message_iter_next(&array);
        }

        ar->status = OK;
        ar->data = ecc_list;
        ar->arg2 = index;
        cb(ar);
    }

    return true;
}

static int modem_state_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter, var;
    const char* property;

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return 0;
    }

    ar = handler->result;
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return 0;
    }

    if (ar->msg_id != MSG_MODEM_STATE_CHANGE_IND) {
        tapi_log_error("msg_id is invalid in %s", __func__);
        return 0;
    }

    cb = handler->cb_function;
    if (cb == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return 0;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        tapi_log_error("message iter init failed in %s", __func__);
        return 0;
    }

    dbus_message_iter_get_basic(&iter, &property);
    dbus_message_iter_next(&iter);

    dbus_message_iter_recurse(&iter, &var);
    if (strcmp(property, "ModemState") == 0) {
        dbus_message_iter_get_basic(&var, &ar->arg2);
        ar->status = OK;
        cb(ar);
    }

    return 1;
}

static void enable_modem_abnormal_event_param_append(DBusMessageIter* iter, void* user_data)
{
    abnormal_event_data* abnormal_event_param;
    tapi_async_handler* param;
    int enable;
    int module_mask;
    int from_event_id;
    int to_event_id;

    param = user_data;
    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        return;
    }

    if (param->result == NULL) {
        tapi_log_error("result in %s is null", __func__);
        return;
    }

    abnormal_event_param = param->result->data;
    enable = abnormal_event_param->enable ? 1 : 0;
    module_mask = abnormal_event_param->module_mask;
    from_event_id = abnormal_event_param->from_event_id;
    to_event_id = abnormal_event_param->to_event_id;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &enable);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &module_mask);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &from_event_id);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &to_event_id);

    free(abnormal_event_param);
}

static void oem_ril_request_raw_param_append(DBusMessageIter* iter, void* user_data)
{
    oem_ril_request_data* oem_ril_req_raw_param;
    tapi_async_handler* param;
    DBusMessageIter array;
    unsigned char* oem_req;
    int i;

    param = user_data;
    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        return;
    }

    if (param->result == NULL) {
        tapi_log_error("result in %s is null", __func__);
        return;
    }

    oem_ril_req_raw_param = param->result->data;
    oem_req = (unsigned char*)oem_ril_req_raw_param->oem_req;

    dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE_AS_STRING, &array);

    for (i = 0; i < oem_ril_req_raw_param->length; i++) {
        dbus_message_iter_append_basic(&array, DBUS_TYPE_BYTE, &oem_req[i]);
    }

    dbus_message_iter_close_container(iter, &array);

    free(oem_ril_req_raw_param);
}

static void atom_command_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* handler = user_data;
    int atom = handler->result->arg1;
    int command = handler->result->arg2;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &atom);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &command);
}

static void oem_ril_request_raw_cb(DBusMessage* message, void* user_data)
{
    DBusMessageIter iter, array;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusError err;
    unsigned char* response;
    int num;

    handler = user_data;
    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return;
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return;
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return;
    }

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("%s: %s\n", err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        tapi_log_error("message iter init failed in %s", __func__);
        ar->status = ERROR;
        goto done;
    }

    dbus_message_iter_recurse(&iter, &array);
    dbus_message_iter_get_fixed_array(&array, &response, &num);

    ar->data = response;
    ar->arg2 = num;
    ar->status = OK;

done:
    cb(ar);
}

static void oem_ril_request_strings_param_append(DBusMessageIter* iter, void* user_data)
{
    oem_ril_request_data* oem_ril_req_strings_param;
    tapi_async_handler* param;
    DBusMessageIter array;
    char** oem_req;
    int i;

    param = user_data;
    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        return;
    }

    if (param->result == NULL) {
        tapi_log_error("result in %s is null", __func__);
        return;
    }

    oem_ril_req_strings_param = param->result->data;
    oem_req = (char**)oem_ril_req_strings_param->oem_req;

    dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING, &array);

    for (i = 0; i < oem_ril_req_strings_param->length; i++) {
        dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING, &oem_req[i]);
    }

    dbus_message_iter_close_container(iter, &array);

    free(oem_ril_req_strings_param);
}

static void oem_ril_request_strings_cb(DBusMessage* message, void* user_data)
{
    char* response[MAX_OEM_RIL_RESP_STRINGS_LENTH];
    DBusMessageIter iter, array;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusError err;
    int num;

    handler = user_data;
    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return;
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return;
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return;
    }

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("%s: %s\n", err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        tapi_log_error("message iter init failed in %s", __func__);
        ar->status = ERROR;
        goto done;
    }

    num = 0;
    dbus_message_iter_recurse(&iter, &array);
    while (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_STRING) {
        if (num == MAX_OEM_RIL_RESP_STRINGS_LENTH) {
            tapi_log_error("too many strings in %s", __func__);
            ar->status = ERROR;
            goto done;
        }

        dbus_message_iter_get_basic(&array, &response[num++]);
        dbus_message_iter_next(&array);
    }

    ar->data = response;
    ar->arg2 = num;
    ar->status = OK;

done:
    cb(ar);
}

static void on_dbus_client_ready(GDBusClient* client, void* user_data)
{
    client_ready_cb_data* cbd = user_data;
    dbus_context* ctx;
    tapi_client_ready_function cb;

    if (cbd == NULL) {
        tapi_log_error("cbd in %s is null", __func__);
        return;
    }

    ctx = cbd->context;
    if (ctx != NULL) {
        ctx->client_ready = true;
    }

    cb = cbd->callback;
    if (cb != NULL)
        cb(cbd->client_name, cbd->user_data);

    free(cbd);
}

static void on_modem_property_change(GDBusProxy* proxy, const char* name,
    DBusMessageIter* iter, void* user_data)
{
    dbus_context* ctx = user_data;
    int new_state = MODEM_STATE_POWER_OFF;
    int modem_id = 0;

    if (strcmp("ModemState", name) != 0)
        return;

    modem_id = get_modem_id_by_proxy(ctx, proxy);
    dbus_message_iter_get_basic(iter, &new_state);
    tapi_log_info("%s - from %d to %d", __func__, ctx->modem_state[modem_id], new_state);

    if (ctx->modem_state[modem_id] == MODEM_STATE_AWARE && new_state == MODEM_STATE_ALIVE) {
        tapi_log_info("%s - refresh dbus_proxy ", __func__);
        release_mutable_dbus_proxy(ctx);
        get_mutable_dbus_proxy(ctx);
    }

    ctx->modem_state[modem_id] = new_state;
}

static int tapi_modem_register(tapi_context context,
    int slot_id, tapi_indication_msg msg, void* user_obj, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    const char* modem_path;
    int watch_id;
    tapi_async_handler* handler;
    tapi_async_result* ar;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    modem_path = tapi_utils_get_modem_path(slot_id);
    if (modem_path == NULL) {
        tapi_log_error("no available modem ...\n");
        return -EIO;
    }

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        tapi_log_error("no memory for handler in %s", __func__);
        return -ENOMEM;
    }

    handler->cb_function = p_handle;
    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("no memory for async result in %s", __func__);
        free(handler);
        return -ENOMEM;
    }
    handler->result = ar;
    ar->msg_id = msg;
    ar->msg_type = INDICATION;
    ar->arg1 = slot_id;
    ar->user_obj = user_obj;

    switch (msg) {
    case MSG_RADIO_STATE_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_MODEM_INTERFACE,
            "PropertyChanged", radio_state_changed, handler, handler_free);
        break;
    case MSG_PHONE_STATE_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_VOICECALL_MANAGER_INTERFACE,
            "PhoneStatusChanged", phone_state_changed, handler, handler_free);
        break;
    case MSG_OEM_HOOK_RAW_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_MODEM_INTERFACE,
            "OemHookIndication", process_oem_hook_raw_indication, handler, handler_free);
        break;
    case MSG_MODEM_RESTART_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_MODEM_INTERFACE,
            "ModemRestart", modem_restart, handler, handler_free);
        break;
    case MSG_AIRPLANE_MODE_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_MODEM_INTERFACE,
            "PropertyChanged", airplane_mode_changed, handler, handler_free);
        break;
    case MSG_DEVICE_INFO_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_MODEM_INTERFACE,
            "DeviceInfoChanged", device_info_changed, handler, handler_free);
        break;
    case MSG_MODEM_STATE_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_MODEM_INTERFACE,
            "PropertyChanged", modem_state_changed, handler, handler_free);
        break;
    case MSG_MODEM_ECC_LIST_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_MODEM_INTERFACE,
            "PropertyChanged", modem_ecc_list_change, handler, handler_free);
        break;
    default:
        handler_free(handler);
        return -EINVAL;
    }

    if (watch_id == 0) {
        tapi_log_error("watch id is invalid in %s, msg_id: %d", __func__, (int)msg);
        handler_free(handler);
        return -EINVAL;
    }

    return watch_id;
}
static void system_dbus_disconnected(DBusConnection* conn, void* user_data)
{
    tapi_client_ready_function callback = (tapi_client_ready_function)user_data;
    tapi_log_error("DBusConnection %p has disconnected!", conn);
    callback(NULL, NULL);
}
/****************************************************************************
 * Public Functions
 ****************************************************************************/

tapi_context tapi_open(const char* client_name,
    tapi_client_ready_function callback, void* user_data)
{
    DBusConnection* connection;
    GDBusClient* client;
    dbus_context* ctx;
    DBusError err;
    int retry_round = 0;
    int slot_id = 0;
#ifdef CONFIG_MODEM_ABNORMAL_EVENT
    bool enable = true;
#else
    bool enable = false;
#endif
    int module_mask = 63;
    int from_event_id = 0;
    int to_event_id = 0;

    ctx = malloc(sizeof(dbus_context));
    if (ctx == NULL) {
        tapi_log_error("context malloc failed! \n");
        return NULL;
    }

    client_ready_cb_data* cbd = malloc(sizeof(client_ready_cb_data));
    if (cbd == NULL) {
        tapi_log_error("client callback malloc failed! \n");
        free(ctx);
        return NULL;
    }

    while (1) {
        connection = g_dbus_setup_private(DBUS_BUS_SYSTEM, NULL, NULL);
        if (connection == NULL) {
            tapi_log_error("dbus connection init error \n");

            if (retry_round++ < MAX_DBUS_INIT_RETRY_COUNT) {
                usleep(MAX_DBUS_INIT_RETRY_INTERVAL_MS * 1000);
                continue;
            }

            tapi_log_error("max retry times, giving up! \n");
            goto error;
        }

        g_dbus_set_disconnect_function(connection, system_dbus_disconnected, callback, NULL);

        dbus_error_init(&err);
        dbus_request_name(connection, client_name, &err);
        if (dbus_error_is_set(&err)) {
            tapi_log_error("%s error %s: %s \n", __func__, err.name, err.message);
            dbus_error_free(&err);
            goto error;
        }

        client = g_dbus_client_new(connection, OFONO_SERVICE, OFONO_MANAGER_PATH);
        if (client == NULL) {
            tapi_log_error("client create failed! \n");
            goto error;
        }

        break;
    }

    g_dbus_client_set_proxy_handlers(client, object_add, object_remove,
        object_filter, NULL, NULL);

    cbd->client_name = client_name;
    cbd->context = ctx;
    cbd->user_data = user_data;
    cbd->callback = callback;
    if (!g_dbus_client_set_ready_watch(client, on_dbus_client_ready, cbd)) {
        tapi_log_error("set ready watch failed! \n");
        g_dbus_client_unref(client);
        goto error;
    }

    ctx->connection = connection;
    ctx->client = client;
    ctx->client_ready = false;
    ctx->logging_over_miwear_cb = NULL;
    snprintf(ctx->name, sizeof(ctx->name), "%s", client_name);
    get_persistent_dbus_proxy(ctx);
    get_mutable_dbus_proxy(ctx);

    for (int i = 0; i < CONFIG_MODEM_ACTIVE_COUNT; i++) {
        ctx->modem_state[i] = MODEM_STATE_POWER_OFF;
        g_dbus_proxy_set_property_watch(ctx->dbus_proxy[i][DBUS_PROXY_MODEM],
            on_modem_property_change, ctx);
    }

    tapi_enable_modem_abnormal_event(ctx, slot_id, enable, 0, module_mask, from_event_id, to_event_id, NULL);

    return ctx;

error:
    free(ctx);
    free(cbd);
    if (connection != NULL) {
        dbus_connection_close(connection);
        dbus_connection_unref(connection);
    }

    tapi_log_error("dbus connection open error \n");
    return NULL;
}

int tapi_close(tapi_context context)
{
    dbus_context* ctx = context;
    if (ctx == NULL) {
        tapi_log_error("dbus connection close error \n");
        return -EINVAL;
    }

    for (int i = 0; i < CONFIG_MODEM_ACTIVE_COUNT; i++) {
        g_dbus_proxy_remove_property_watch(ctx->dbus_proxy[i][DBUS_PROXY_MODEM], NULL);
    }

    release_persistent_dbus_proxy(ctx);
    release_mutable_dbus_proxy(ctx);
    g_dbus_client_unref(ctx->client);
    dbus_connection_close(ctx->connection);
    dbus_connection_unref(ctx->connection);

    free(context);
    return OK;
}

int tapi_query_modem_list(tapi_context context, int event_id, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;

    proxy = ctx->dbus_proxy_manager;
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...");
        return -EIO;
    }

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return -ENOMEM;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        free(handler);
        return -ENOMEM;
    }

    ar->msg_id = event_id;
    handler->result = ar;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy,
            "GetModems", NULL, modem_list_query_done, handler, handler_free)) {
        tapi_log_error("method call failed in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

bool tapi_is_feature_supported(tapi_feature_type feature)
{
    switch (feature) {
    case FEATURE_VOICE:
        return true;
    case FEATURE_DATA:
        return true;
    case FEATURE_SMS:
        return true;
    case FEATURE_IMS:
        return true;
    default:
        break;
    }

    return false;
}

int tapi_set_pref_net_mode(tapi_context context,
    int slot_id, int event_id, tapi_pref_net_mode mode, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    const char* rat;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_RADIO];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return -ENOMEM;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        free(handler);
        return -ENOMEM;
    }
    handler->result = ar;

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    handler->cb_function = p_handle;
    ar->data = "TechnologyPreference";
    rat = tapi_utils_network_mode_to_string(mode);

    if (!g_dbus_proxy_set_property_basic(proxy,
            "TechnologyPreference", DBUS_TYPE_STRING, &rat,
            property_set_done, handler, handler_free)) {
        tapi_log_error("set property failed in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_get_pref_net_mode(tapi_context context, int slot_id, tapi_pref_net_mode* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    char* result;
    DBusMessageIter iter;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_RADIO];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "TechnologyPreference", &iter)) {
        dbus_message_iter_get_basic(&iter, &result);

        *out = tapi_utils_network_mode_from_string(result);
        return OK;
    }

    tapi_log_error("get property failed in %s", __func__);
    return -EINVAL;
}

int tapi_send_modem_power(tapi_context context, int slot_id, bool state)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    int value = state;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (!g_dbus_proxy_set_property_basic(proxy,
            "Powered", DBUS_TYPE_BOOLEAN, &value, NULL, NULL, NULL)) {
        tapi_log_error("set property failed in %s", __func__);
        return -EINVAL;
    }

    return OK;
}

int tapi_get_imei(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Serial", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    tapi_log_error("get property failed in %s", __func__);
    return -EINVAL;
}

int tapi_get_imeisv(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "SoftwareVersionNumber", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    tapi_log_error("get property failed in %s", __func__);
    return -EINVAL;
}

int tapi_get_modem_manufacturer(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Manufacturer", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    tapi_log_error("get property failed in %s", __func__);
    return -EINVAL;
}

int tapi_get_modem_model(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Model", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    tapi_log_error("get property failed in %s", __func__);
    return -EINVAL;
}

int tapi_get_modem_revision(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Revision", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    tapi_log_error("get property failed in %s", __func__);
    return -EINVAL;
}

int tapi_get_phone_state(tapi_context context, int slot_id, tapi_phone_state* state)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    int new_state = 0;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "PhoneStatus", &iter)) {
        dbus_message_iter_get_basic(&iter, &new_state);
        *state = new_state;
        return OK;
    }

    tapi_log_error("get property failed in %s", __func__);
    return -EINVAL;
}

int tapi_set_radio_power(tapi_context context,
    int slot_id, int event_id, bool state, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    int value = state;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return -ENOMEM;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        free(handler);
        return -ENOMEM;
    }
    handler->result = ar;

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_set_property_basic(proxy, "Online", DBUS_TYPE_BOOLEAN,
            &value, property_set_done, handler, handler_free)) {
        tapi_log_error("set property failed in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_get_radio_power(tapi_context context, int slot_id, bool* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    int result;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Online", &iter)) {
        dbus_message_iter_get_basic(&iter, &result);

        *out = result;
        return OK;
    }

    tapi_log_error("get property failed in %s", __func__);
    return -EINVAL;
}

int tapi_get_radio_state(tapi_context context, int slot_id, tapi_radio_state* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    int result;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (!g_dbus_proxy_get_property(proxy, "RadioState", &iter)) {
        tapi_log_error("get property failed in %s", __func__);
        return -EINVAL;
    }

    dbus_message_iter_get_basic(&iter, &result);
    *out = result;
    return OK;
}

int tapi_get_msisdn_number(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter, var_elem;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (!g_dbus_proxy_get_property(proxy, "SubscriberNumbers", &iter)) {
        tapi_log_error("get property iter failed in %s", __func__);
        return -EIO;
    }

    if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
        dbus_message_iter_recurse(&iter, &var_elem);
        while (dbus_message_iter_get_arg_type(&var_elem) != DBUS_TYPE_INVALID) {
            if (dbus_message_iter_get_arg_type(&var_elem) == DBUS_TYPE_STRING) {
                dbus_message_iter_get_basic(&var_elem, out);
                break;
            }
        }

        return OK;
    }

    tapi_log_error("get property failed in %s", __func__);
    return -EINVAL;
}

int tapi_get_modem_activity_info(tapi_context context, int slot_id,
    int event_id, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return -ENOMEM;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        free(handler);
        return -ENOMEM;
    }
    handler->result = ar;

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    handler->result = ar;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "GetModemActivityInfo", NULL,
            modem_activity_info_query_done, handler, handler_free)) {
        tapi_log_error("get property failed in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_invoke_oem_ril_request_raw(tapi_context context, int slot_id, int event_id,
    unsigned char oem_req[], int length, tapi_async_function p_handle)
{
    oem_ril_request_data* oem_ril_req;
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (oem_req == NULL) {
        tapi_log_error("oem_req in %s is null", __func__);
        return -EINVAL;
    }

    if (length <= 0) {
        tapi_log_error("length in %s is invalid", __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return -ENOMEM;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        free(handler);
        return -ENOMEM;
    }
    handler->result = ar;

    oem_ril_req = malloc(sizeof(oem_ril_request_data));
    if (oem_ril_req == NULL) {
        tapi_log_error("oem_ril_req in %s is null", __func__);
        free(handler);
        free(ar);
        return -ENOMEM;
    }
    oem_ril_req->length = length;
    oem_ril_req->oem_req = oem_req;

    ar->arg1 = slot_id;
    ar->msg_id = event_id;
    ar->data = oem_ril_req;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "OemRequestRaw", oem_ril_request_raw_param_append,
            oem_ril_request_raw_cb, handler, handler_free)) {
        tapi_log_error("method call failed in %s", __func__);
        handler_free(handler);
        free(oem_ril_req);
        return -EINVAL;
    }

    return OK;
}

int tapi_invoke_oem_ril_request_strings(tapi_context context, int slot_id, int event_id,
    char* oem_req[], int length, tapi_async_function p_handle)
{
    oem_ril_request_data* oem_ril_req_param;
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (oem_req == NULL) {
        tapi_log_error("oem_req in %s is null", __func__);
        return -EINVAL;
    }

    if (length <= 0) {
        tapi_log_error("length in %s is invalid", __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return -ENOMEM;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        free(handler);
        return -ENOMEM;
    }
    handler->result = ar;

    oem_ril_req_param = malloc(sizeof(oem_ril_request_data));
    if (oem_ril_req_param == NULL) {
        tapi_log_error("oem_ril_req_param in %s is null", __func__);
        free(handler);
        free(ar);
        return -ENOMEM;
    }
    oem_ril_req_param->length = length;
    oem_ril_req_param->oem_req = oem_req;

    ar->arg1 = slot_id;
    ar->msg_id = event_id;
    ar->data = oem_ril_req_param;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "OemRequestStrings", oem_ril_request_strings_param_append,
            oem_ril_request_strings_cb, handler, handler_free)) {
        tapi_log_error("method call failed in %s", __func__);
        handler_free(handler);
        free(oem_ril_req_param);
        return -EINVAL;
    }

    return OK;
}

int tapi_enable_modem(tapi_context context, int slot_id,
    int event_id, bool enable, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return -ENOMEM;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        free(handler);
        return -ENOMEM;
    }

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    handler->result = ar;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, enable ? "EnableModem" : "DisableModem",
            NULL, enable_or_disable_modem_done, handler, handler_free)) {
        tapi_log_error("method call failed in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_enable_modem_abnormal_event(tapi_context context, int slot_id, bool enable,
    int event_id, int module_mask, int from_event_id, int to_event_id, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    abnormal_event_data* user_data;
    GDBusProxy* proxy;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return -ENOMEM;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        free(handler);
        return -ENOMEM;
    }

    user_data = malloc(sizeof(abnormal_event_data));
    if (user_data == NULL) {
        tapi_log_error("user_data in %s is null", __func__);
        free(handler);
        free(ar);
        return -ENOMEM;
    }
    user_data->enable = enable;
    user_data->module_mask = module_mask;
    user_data->from_event_id = from_event_id;
    user_data->to_event_id = to_event_id;

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->data = user_data;
    handler->result = ar;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "EnableModemAbnormalEvent",
            enable_modem_abnormal_event_param_append, enable_modem_abnormal_event_done,
            handler, handler_free)) {
        tapi_log_error("method call failed in %s", __func__);
        handler_free(handler);
        free(user_data);
        return -EINVAL;
    }

    return OK;
}

int tapi_get_modem_status(tapi_context context, int slot_id,
    int event_id, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return -ENOMEM;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        free(handler);
        return -ENOMEM;
    }

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    handler->result = ar;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "GetModemStatus", NULL,
            modem_status_query_done, handler, handler_free)) {
        tapi_log_error("method call failed in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_get_modem_status_sync(tapi_context context, int slot_id, tapi_modem_state* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    int state = 0;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "ModemState", &iter)) {
        dbus_message_iter_get_basic(&iter, &state);
        *out = state;
        return OK;
    }

    tapi_log_error("get property failed in %s", __func__);
    return -EINVAL;
}

int tapi_data_log_ind(DBusConnection* connection, DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter;
    char* out_data;

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return 0;
    }

    ar = handler->result;
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return 0;
    }

    cb = handler->cb_function;
    if (cb == NULL) {
        tapi_log_error("callback function in %s is null", __func__);
        return 0;
    }

    if (!dbus_message_iter_init(message, &iter)) {
        tapi_log_error("message iterator init failed in %s", __func__);
        return 0;
    }

    if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING) {
        dbus_message_iter_get_basic(&iter, &out_data);
        ar->data = out_data;
        ar->status = OK;
        cb(ar);
        return 1;
    }

    tapi_log_error("get data loging failed in %s", __func__);
    return 0;
}

static int tapi_manager_register_data_loging(tapi_context context,
    int slot_id, void* user_obj, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    int watch_id;
    tapi_async_handler* handler;
    tapi_async_result* ar;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return -ENOMEM;
    }

    handler->cb_function = p_handle;
    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        free(handler);
        return -ENOMEM;
    }
    handler->result = ar;
    ar->msg_id = MSG_DATA_LOGING_IND;
    ar->msg_type = INDICATION;
    ar->arg1 = slot_id;
    ar->user_obj = user_obj;

    watch_id = g_dbus_add_signal_watch(ctx->connection,
        OFONO_SERVICE, OFONO_MANAGER_PATH, OFONO_MANAGER_INTERFACE,
        "DataLogInd", tapi_data_log_ind, handler, handler_free);

    if (watch_id == 0) {
        tapi_log_error("add signal watch failed in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }
    ctx->logging_over_miwear_cb = p_handle;

    return watch_id;
}

static int trigger_modem_load_ecc_list(tapi_context context, int slot_id)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    syslog(LOG_DEBUG, "load modem ecc list");

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (!g_dbus_proxy_method_call(proxy, "LoadModemEccList", NULL,
            no_operate_callback, NULL, NULL)) {
        tapi_log_error("load modem ecc list command send fail");
        return -EINVAL;
    }

    return 1;
}

int tapi_register(tapi_context context,
    int slot_id, tapi_indication_msg msg, void* user_obj, tapi_async_function p_handle)
{
    int voicecall_watch_id = 0;
    int modem_watch_id = 0;

    switch (msg) {
    case MSG_CALL_STATE_CHANGE_IND:
        return tapi_call_register_call_state_change(
            context, slot_id, user_obj, p_handle);
    case MSG_CALL_RING_BACK_TONE_IND:
        return tapi_call_register_ringback_tone_change(
            context, slot_id, user_obj, p_handle);
    case MSG_ECC_LIST_CHANGE_IND:
        voicecall_watch_id = tapi_call_register_emergency_list_change(
            context, slot_id, user_obj, p_handle);
        modem_watch_id = tapi_modem_register(context, slot_id, MSG_MODEM_ECC_LIST_CHANGE_IND,
            user_obj, p_handle);
        trigger_modem_load_ecc_list(context, slot_id);
        if (voicecall_watch_id < 0 || modem_watch_id < 0) {
            tapi_log_error("register ecc list change:%d,%d", voicecall_watch_id, modem_watch_id);
            return voicecall_watch_id;
        }
        return voicecall_watch_id;
    case MSG_DEFAULT_VOICECALL_SLOT_CHANGE_IND:
        return tapi_call_register_default_voicecall_slot_change(
            context, user_obj, p_handle);
    case MSG_NETWORK_STATE_CHANGE_IND:
    case MSG_VOICE_REGISTRATION_STATE_CHANGE_IND:
    case MSG_CELLINFO_CHANGE_IND:
    case MSG_SIGNAL_STRENGTH_CHANGE_IND:
    case MSG_NITZ_STATE_CHANGE_IND:
        return tapi_network_register(context, slot_id, msg, user_obj, p_handle);
    case MSG_DATA_ENABLED_CHANGE_IND:
    case MSG_DATA_REGISTRATION_STATE_CHANGE_IND:
    case MSG_DATA_NETWORK_TYPE_CHANGE_IND:
    case MSG_DATA_CONNECTION_STATE_CHANGE_IND:
    case MSG_DEFAULT_DATA_SLOT_CHANGE_IND:
        return tapi_data_register(context, slot_id, msg, user_obj, p_handle);
    case MSG_SIM_STATE_CHANGE_IND:
    case MSG_SIM_UICC_APP_ENABLED_CHANGE_IND:
        return tapi_sim_register(context, slot_id, msg, user_obj, p_handle);
    case MSG_INCOMING_MESSAGE_IND:
    case MSG_IMMEDIATE_MESSAGE_IND:
    case MSG_STATUS_REPORT_MESSAGE_IND:
        return tapi_sms_register(context, slot_id, msg, user_obj, p_handle);
    case MSG_INCOMING_CBS_IND:
    case MSG_EMERGENCY_CBS_IND:
        return tapi_cbs_register(context, slot_id, msg, user_obj, p_handle);
    case MSG_RADIO_STATE_CHANGE_IND:
    case MSG_PHONE_STATE_CHANGE_IND:
    case MSG_OEM_HOOK_RAW_IND:
    case MSG_MODEM_RESTART_IND:
    case MSG_AIRPLANE_MODE_CHANGE_IND:
    case MSG_DEVICE_INFO_CHANGE_IND:
    case MSG_MODEM_STATE_CHANGE_IND:
        return tapi_modem_register(context, slot_id, msg, user_obj, p_handle);
    case MSG_IMS_REGISTRATION_MESSAGE_IND:
        return tapi_ims_register_registration_change(context, slot_id, user_obj, p_handle);
    case MSG_DATA_LOGING_IND:
        return tapi_manager_register_data_loging(context, slot_id, user_obj, p_handle);
    default:
        break;
    }

    return 0;
}

int tapi_unregister(tapi_context context, int watch_id)
{
    dbus_context* ctx = context;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (watch_id <= 0) {
        tapi_log_error("invalid watch id %d", watch_id);
        return -EINVAL;
    }

    if (!g_dbus_remove_watch(ctx->connection, watch_id)) {
        tapi_log_error("remove watch failed in %s", __func__);
        return -EINVAL;
    }

    return OK;
}

int tapi_handle_command(tapi_context context, int slot_id, int atom, int command)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return -ENOMEM;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        free(handler);
        return -ENOMEM;
    }

    ar->arg1 = atom;
    ar->arg2 = command;
    handler->result = ar;

    if (!g_dbus_proxy_method_call(proxy, "HandleCommand", atom_command_param_append,
            no_operate_callback, handler, handler_free)) {
        tapi_log_error("method call failed in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_set_fast_dormancy(tapi_context context,
    int slot_id, int event_id, bool state, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    int value = state;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_RADIO];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return -ENOMEM;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        free(handler);
        return -ENOMEM;
    }
    handler->result = ar;

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_set_property_basic(proxy, "FastDormancy", DBUS_TYPE_BOOLEAN,
            &value, property_set_done, handler, handler_free)) {
        tapi_log_error("set property failed in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_get_phone_number(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    char* msisdn_number = NULL;
    char* subscriber_uri_number = NULL;
    int ret1, ret2;

    ret1 = tapi_get_msisdn_number(ctx, slot_id, &msisdn_number);
    if (ret1 == OK && msisdn_number != NULL) {
        *out = msisdn_number;
        tapi_log_info("get phone number from UICC.");
    } else {
        ret2 = tapi_ims_get_subscriber_uri_number(ctx, slot_id, &subscriber_uri_number);
        *out = (ret2 == OK && subscriber_uri_number != NULL) ? subscriber_uri_number : NULL;
        tapi_log_info("get phone number from IMS.");
    }

    return OK;
}

int tapi_get_carrier_config_bool(tapi_context context, int slot_id, char* key, bool* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    DBusMessageIter var_elem;
    int result = 0;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (key == NULL) {
        tapi_log_error("key in %s is null", __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (!g_dbus_proxy_get_property(proxy, "CarrierConfig", &iter)) {
        tapi_log_error("get property failed in %s", __func__);
        return -EINVAL;
    }

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
        tapi_log_error("get property failed in %s", __func__);
        return -EINVAL;
    }

    dbus_message_iter_recurse(&iter, &var_elem);

    while (dbus_message_iter_get_arg_type(&var_elem) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry, value;
        const char* prop_name;

        dbus_message_iter_recurse(&var_elem, &entry);
        dbus_message_iter_get_basic(&entry, &prop_name);

        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &value);

        if (strcmp(prop_name, key) == 0) {
            dbus_message_iter_get_basic(&value, &result);
            *out = result;

            return OK;
        }

        dbus_message_iter_next(&var_elem);
    }

    tapi_log_error("get property failed in %s", __func__);
    return -EINVAL;
}

int tapi_get_carrier_config_int(tapi_context context, int slot_id, char* key, int* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    DBusMessageIter var_elem;
    int result = 0;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (key == NULL) {
        tapi_log_error("key in %s is null", __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (!g_dbus_proxy_get_property(proxy, "CarrierConfig", &iter)) {
        tapi_log_error("get property iter failed in %s", __func__);
        return -EINVAL;
    }

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
        tapi_log_error("iter type in %s is invalid", __func__);
        return -EINVAL;
    }

    dbus_message_iter_recurse(&iter, &var_elem);

    while (dbus_message_iter_get_arg_type(&var_elem) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry, value;
        const char* prop_name;

        dbus_message_iter_recurse(&var_elem, &entry);
        dbus_message_iter_get_basic(&entry, &prop_name);

        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &value);

        if (strcmp(prop_name, key) == 0) {
            dbus_message_iter_get_basic(&value, &result);
            *out = result;

            return OK;
        }

        dbus_message_iter_next(&var_elem);
    }

    tapi_log_error("get property failed in %s", __func__);
    return -EINVAL;
}

int tapi_get_carrier_config_string(tapi_context context, int slot_id, char* key, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    DBusMessageIter var_elem;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (key == NULL) {
        tapi_log_error("key in %s is null", __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (!g_dbus_proxy_get_property(proxy, "CarrierConfig", &iter)) {
        tapi_log_error("get property iter failed in %s", __func__);
        return -EINVAL;
    }

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
        tapi_log_error("iter type in %s is invalid", __func__);
        return -EINVAL;
    }

    dbus_message_iter_recurse(&iter, &var_elem);

    while (dbus_message_iter_get_arg_type(&var_elem) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry, value;
        const char* prop_name;

        dbus_message_iter_recurse(&var_elem, &entry);
        dbus_message_iter_get_basic(&entry, &prop_name);

        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &value);

        if (strcmp(prop_name, key) == 0) {
            dbus_message_iter_get_basic(&value, out);
            return OK;
        }

        dbus_message_iter_next(&var_elem);
    }

    tapi_log_error("get property failed in %s", __func__);
    return -EINVAL;
}