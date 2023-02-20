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

#include <stdio.h>

#include "tapi_internal.h"
#include "tapi_manager.h"
#include "tapi_sms.h"

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

typedef struct {
    char* number;
    char* text;
} message_param;

typedef struct {
    char* dest_addr;
    unsigned char port;
    char* data;
} data_message_param;

/****************************************************************************
 * Private Function
 ****************************************************************************/

static int decode_message_info(DBusMessageIter* iter, tapi_message_info* message_info);
static int unsol_sms_message(DBusConnection* connection, DBusMessage* message, void* user_data);
static void message_list_query_complete(DBusMessage* message, void* user_data);

static char* strdup0(const char* str)
{
    if (str != NULL)
        return strdup(str);

    return NULL;
}

static void send_message_param_append(DBusMessageIter* iter, void* user_data)
{
    message_param* param = user_data;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &param->number);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &param->text);
}

static void send_data_message_param_append(DBusMessageIter* iter, void* user_data)
{
    data_message_param* message = user_data;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &message->dest_addr);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_BYTE, &message->port);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &message->data);
}

static void copy_message_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_message_info* message_info = user_data;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &message_info->sender);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &message_info->text);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &message_info->sms_type);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &message_info->sent_time);
}

static void delete_message_param_append(DBusMessageIter* iter, void* user_data)
{
    char* param = user_data;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &param);
}

static void message_free(void* user_data)
{
    message_param* message = user_data;

    free(message->number);
    free(message->text);
    free(message);
}

static void data_message_free(void* user_data)
{
    data_message_param* message = user_data;

    free(message->dest_addr);
    free(message->data);
    free(message);
}

static void message_info_free(void* user_data)
{
    tapi_message_info* message_info = user_data;

    free(message_info->text);
    free(message_info->sender);
    free(message_info->sent_time);
    free(message_info);
}

static void message_event_free(void* user_data)
{
    tapi_async_handler* handler;
    tapi_async_result* ar;

    handler = user_data;
    if (handler != NULL) {
        ar = handler->result;
        if (ar != NULL)
            free(ar);
        free(handler);
    }
}

static char* proxy_get_string(GDBusProxy* proxy, const char* property)
{
    DBusMessageIter iter;
    char* str;

    if (!g_dbus_proxy_get_property(proxy, property, &iter))
        return NULL;

    dbus_message_iter_get_basic(&iter, &str);
    return str;
}

static int unsol_sms_message(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    DBusMessageIter iter, list;
    DBusMessageIter entry, result;
    const char* sender;
    const char* member;
    char* text;
    char *name, *value;
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    tapi_message_info* message_info;

    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL)
        return false;

    sender = dbus_message_get_sender(message);
    if (sender == NULL)
        goto done;

    if (!dbus_message_iter_init(message, &iter))
        goto done;

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING)
        goto done;

    dbus_message_iter_get_basic(&iter, &text);
    member = dbus_message_get_member(message);

    message_info = NULL;
    if (strcmp(member, "IncomingMessage") == 0
        || strcmp(member, "ImmediateMessage") == 0) {
        dbus_message_iter_next(&iter);
        dbus_message_iter_recurse(&iter, &list);

        message_info = malloc(sizeof(tapi_message_info));
        if (message_info == NULL) {
            goto done;
        }
        message_info->text = text;

        if (strcmp(member, "IncomingMessage") == 0) {
            message_info->sms_type = 1;
        } else if (strcmp(member, "ImmediateMessage") == 0) {
            message_info->sms_type = 2;
        }

        while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_DICT_ENTRY) {
            dbus_message_iter_recurse(&list, &entry);
            dbus_message_iter_get_basic(&entry, &name);

            dbus_message_iter_next(&entry);

            dbus_message_iter_recurse(&entry, &result);
            dbus_message_iter_get_basic(&result, &value);

            if (strcmp(name, "LocalSentTime") == 0) {
                dbus_message_iter_get_basic(&result, &message_info->local_sent_time);
            } else if (strcmp(name, "SentTime") == 0) {
                dbus_message_iter_get_basic(&result, &message_info->sent_time);
            } else if (strcmp(name, "Sender") == 0) {
                message_info->sender = value;
            }
            dbus_message_iter_next(&list);
        }

        ar->data = message_info;
        ar->status = OK;
    }

done:
    cb(ar);
    if (message_info != NULL)
        free(message_info);

    return true;
}

static int decode_message_info(DBusMessageIter* iter, tapi_message_info* message_info)
{
    DBusMessageIter subArrayIter;

    dbus_message_iter_next(iter);
    dbus_message_iter_recurse(iter, &subArrayIter);

    while (dbus_message_iter_get_arg_type(&subArrayIter) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry, value;
        const char* key;
        char* result;
        int ret;

        dbus_message_iter_recurse(&subArrayIter, &entry);
        dbus_message_iter_get_basic(&entry, &key);

        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &value);

        if (strcmp(key, "Type") == 0) {
            dbus_message_iter_get_basic(&value, &ret);
            message_info->sms_type = ret;
        } else if (strcmp(key, "To") == 0) {
            dbus_message_iter_get_basic(&value, &result);
            message_info->sender = strdup0(result);
        } else if (strcmp(key, "Text") == 0) {
            dbus_message_iter_get_basic(&value, &result);
            message_info->text = strdup0(result);
        } else if (strcmp(key, "Date") == 0) {
            dbus_message_iter_get_basic(&value, &result);
            message_info->sent_time = strdup0(result);
            message_info->local_sent_time = strdup0(result);
        }

        dbus_message_iter_next(&subArrayIter);
    }

    return true;
}

static void message_list_query_complete(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_message_info message_list[MAX_MESSAGE_LIST_COUNT];
    DBusMessageIter args, list;
    tapi_async_function cb;
    tapi_async_result* ar;
    int message_count = 0;
    DBusError err;

    if (handler == NULL)
        return;

    ar = handler->result;
    if (ar == NULL)
        return;
    ar->status = ERROR;

    cb = handler->cb_function;
    if (cb == NULL)
        return;

    // start to handle response message.
    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("%s: %s\n", err.name, err.message);
        dbus_error_free(&err);
        goto done;
    }

    if (dbus_message_has_signature(message, "a(oa{sv})") == false)
        goto done;

    if (dbus_message_iter_init(message, &args) == false)
        goto done;

    dbus_message_iter_recurse(&args, &list);

    while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_STRUCT) {
        DBusMessageIter entry;

        dbus_message_iter_recurse(&list, &entry);
        if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_OBJECT_PATH) {

            decode_message_info(&entry, message_list + message_count);
            message_count++;
        }

        dbus_message_iter_next(&list);
    }

    ar->arg2 = message_count;
    ar->status = OK;
    ar->data = message_list;

done:
    cb(ar);
    while (--message_count >= 0) {
        free(message_list[message_count].sender);
        free(message_list[message_count].text);
        free(message_list[message_count].sent_time);
        free(message_list[message_count].local_sent_time);
    }
}

int tapi_sms_send_message(tapi_context context, int slot_id,
    char* number, char* text)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    message_param* message;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (number == NULL || text == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SMS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    message = calloc(1, sizeof(message_param));
    if (message == NULL)
        return -EINVAL;

    message->number = strdup0(number);
    message->text = strdup0(text);

    if (!g_dbus_proxy_method_call(proxy, "SendMessage",
            send_message_param_append, NULL, message, message_free)) {
        message_free(message);
        return -EINVAL;
    }

    return OK;
}

int tapi_sms_send_data_message(tapi_context context, int slot_id,
    char* dest_addr, unsigned char port, char* text)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    data_message_param* data_message;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (dest_addr == NULL || text == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SMS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    data_message = calloc(1, sizeof(data_message_param));
    if (data_message == NULL)
        return -ENOMEM;

    data_message->dest_addr = strdup0(dest_addr);
    data_message->data = strdup0(text);
    data_message->port = port;

    if (!g_dbus_proxy_method_call(proxy, "SendDataMessage",
            send_data_message_param_append, NULL, data_message, data_message_free)) {
        data_message_free(data_message);
        return -EINVAL;
    }

    return OK;
}

bool tapi_sms_set_service_center_address(tapi_context context, int slot_id, char* number)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (number == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SMS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!g_dbus_proxy_set_property_basic(proxy, "ServiceCenterAddress",
            DBUS_TYPE_STRING, &number, NULL, NULL, NULL)) {
        return -EINVAL;
    }

    return OK;
}

int tapi_sms_get_service_center_address(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SMS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    *out = proxy_get_string(proxy, "ServiceCenterAddress");
    return OK;
}

int tapi_sms_get_all_messages_from_sim(tapi_context context, int slot_id,
    tapi_message_list* list, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* user_data;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL)
        return -ENOMEM;

    user_data->cb_function = p_handle;
    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(user_data);
        return -ENOMEM;
    }

    user_data->result = ar;
    ar->arg1 = slot_id;
    ar->data = list;

    if (!g_dbus_proxy_method_call(proxy, "GetAllMessagesFromSim", NULL,
            message_list_query_complete, user_data, message_event_free)) {
        message_event_free(user_data);
        return -EINVAL;
    }

    return OK;
}

int tapi_sms_copy_message_to_sim(tapi_context context, int slot_id,
    char* number, char* text, char* send_time, int type)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_message_info* message_info;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (number == NULL || text == NULL || send_time == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SMS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    message_info = calloc(1, sizeof(tapi_message_info));
    if (message_info == NULL)
        return -ENOMEM;

    message_info->text = strdup0(text);
    message_info->sender = strdup0(number);
    message_info->sent_time = strdup0(send_time);
    message_info->sms_type = type;

    if (!g_dbus_proxy_method_call(proxy, "InsertMessageToSim", copy_message_param_append,
            NULL, message_info, message_info_free)) {
        message_info_free(message_info);
        return -EINVAL;
    }

    return OK;
}

int tapi_sms_delete_message_from_sim(tapi_context context, int slot_id, int index)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (index < 0) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SMS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!g_dbus_proxy_method_call(proxy, "DeleteMessageFromSim",
            delete_message_param_append, NULL, (void*)index, NULL)) {
        return -EINVAL;
    }

    return OK;
}

int tapi_sms_register(tapi_context context, int slot_id,
    tapi_indication_msg msg_type,
    tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    char* path;
    tapi_async_handler* user_data;
    tapi_async_result* ar;
    int watch_id;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || msg_type < MSG_INCOMING_MESSAGE_IND || msg_type > MSG_STATUS_REPORT_MESSAGE_IND) {
        return -EINVAL;
    }

    path = tapi_utils_get_modem_path(slot_id);
    if (path == NULL) {
        tapi_log_error("no available modem ...\n");
        return -EIO;
    }

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL)
        return -ENOMEM;

    user_data->cb_function = p_handle;
    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(user_data);
        free(path);
        return -ENOMEM;
    }

    user_data->result = ar;
    ar->msg_id = msg_type;
    ar->arg1 = slot_id;

    switch (msg_type) {
    case MSG_INCOMING_MESSAGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection, OFONO_SERVICE, path,
            OFONO_MESSAGE_MANAGER_INTERFACE, "IncomingMessage",
            unsol_sms_message, user_data, message_event_free);
        break;
    case MSG_IMMEDIATE_MESSAGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection, OFONO_SERVICE, path,
            OFONO_MESSAGE_MANAGER_INTERFACE, "ImmediateMessage",
            unsol_sms_message, user_data, message_event_free);
        break;
    case MSG_STATUS_REPORT_MESSAGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection, OFONO_SERVICE, path,
            OFONO_MESSAGE_MANAGER_INTERFACE, "StatusReportMessage",
            unsol_sms_message, user_data, message_event_free);
        break;
    default:
        break;
    }

    if (watch_id == 0) {
        message_event_free(user_data);
        return -EINVAL;
    }

    return watch_id;
}

int tapi_sms_unregister(tapi_context context, int watch_id)
{
    dbus_context* ctx = context;
    if (ctx == NULL || watch_id <= 0)
        return -EINVAL;

    return g_dbus_remove_watch(ctx->connection, watch_id);
}
