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

#include <stdio.h>

#include "tapi_cbs.h"
#include "tapi_internal.h"

static bool proxy_get_bool(GDBusProxy* proxy, const char* property)
{
    DBusMessageIter iter;
    dbus_bool_t value;

    if (!g_dbus_proxy_get_property(proxy, property, &iter)) {
        tapi_log_error("get property fail in %s", __func__);
        return false;
    }

    dbus_message_iter_get_basic(&iter, &value);
    return value;
}

static char* proxy_get_string(GDBusProxy* proxy, const char* property)
{
    DBusMessageIter iter;
    char* str;

    if (!g_dbus_proxy_get_property(proxy, property, &iter)) {
        tapi_log_error("get property fail in %s", __func__);
        return NULL;
    }

    dbus_message_iter_get_basic(&iter, &str);
    return str;
}

static int unsol_cbs_message(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    const char* member;
    DBusMessageIter iter, list;
    DBusMessageIter entry, result;
    char* text;
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    char *name, *value;
    dbus_bool_t value_t = false;
    tapi_cbs_message* cbs_message;

    if (NULL == handler) {
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

    if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL) {
        tapi_log_error("message type is not signal in %s", __func__);
        return 0;
    }

    if (!dbus_message_iter_init(message, &iter)) {
        tapi_log_error("message iter init fail in %s", __func__);
        return 0;
    }

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
        tapi_log_error("message iter type is not string in %s", __func__);
        return 0;
    }

    dbus_message_iter_get_basic(&iter, &text);
    member = dbus_message_get_member(message);
    if (strcmp(member, "IncomingBroadcast") == 0) {
        cbs_message = calloc(1, sizeof(tapi_cbs_message));
        if (cbs_message == NULL) {
            tapi_log_error("cbs_message in %s is null", __func__);
            return 0;
        }

        cbs_message->text = text;
        dbus_message_iter_next(&iter);
        dbus_message_iter_get_basic(&iter, &cbs_message->channel);

        ar->data = cbs_message;
        ar->status = OK;
        cb(ar);
        free(cbs_message);
    } else if (strcmp(member, "EmergencyBroadcast") == 0) {
        tapi_cbs_emergency_message* cbs_emergency_message = calloc(1, sizeof(tapi_cbs_emergency_message));
        if (cbs_emergency_message == NULL) {
            tapi_log_error("cbs_emergency_message in %s is null", __func__);
            return 0;
        }

        cbs_emergency_message->text = text;
        dbus_message_iter_next(&iter);
        dbus_message_iter_recurse(&iter, &list);

        while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_DICT_ENTRY) {
            dbus_message_iter_recurse(&list, &entry);
            dbus_message_iter_get_basic(&entry, &name);

            dbus_message_iter_next(&entry);
            dbus_message_iter_recurse(&entry, &result);
            dbus_message_iter_get_basic(&result, &value);

            if (strcmp(name, "EmergencyType") == 0) {
                dbus_message_iter_get_basic(&result, &cbs_emergency_message->emergency_type);
            } else if (strcmp(name, "EmergencyAlert") == 0) {
                dbus_message_iter_get_basic(&result, &value_t);
                cbs_emergency_message->emergency_alert = value_t;
            } else if (strcmp(name, "Popup") == 0) {
                dbus_message_iter_get_basic(&result, &value_t);
                cbs_emergency_message->popup = value_t;
            }
            dbus_message_iter_next(&list);
        }

        ar->data = cbs_emergency_message;
        ar->status = OK;
        cb(ar);
        free(cbs_emergency_message);
    }

    return 1;
}

int tapi_sms_set_cell_broadcast_power_on(tapi_context context, int slot_id, bool state)
{
    dbus_context* ctx = context;
    dbus_bool_t cbs_state = state;
    GDBusProxy* proxy;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id in %s", __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CBS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!g_dbus_proxy_set_property_basic(proxy, "Powered",
            DBUS_TYPE_BOOLEAN, &cbs_state, NULL, NULL, NULL)) {
        tapi_log_error("set property failed in %s", __func__);
        return -EINVAL;
    }

    return OK;
}

int tapi_sms_get_cell_broadcast_power_on(tapi_context context, int slot_id, bool* state)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id in %s", __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CBS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EISCONN;
    }

    *state = proxy_get_bool(proxy, "Powered");
    return OK;
}

int tapi_sms_set_cell_broadcast_topics(tapi_context context, int slot_id, char* topics)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id in %s", __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    if (topics == NULL) {
        tapi_log_error("topics in %s is null", __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CBS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!g_dbus_proxy_set_property_basic(proxy, "Topics",
            DBUS_TYPE_STRING, &topics, NULL, NULL, NULL)) {
        tapi_log_error("set property failed in %s", __func__);
        return -EINVAL;
    }

    return OK;
}

int tapi_sms_get_cell_broadcast_topics(tapi_context context, int slot_id, char** topics)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id in %s", __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CBS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EISCONN;
    }

    *topics = proxy_get_string(proxy, "Topics");
    return OK;
}

int tapi_cbs_register(tapi_context context, int slot_id, tapi_indication_msg msg, void* user_obj,
    tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* user_data;
    tapi_async_result* ar;
    const char* path;
    int watch_id = 0;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id in %s", __func__);
        return -EINVAL;
    }

    if (msg < MSG_INCOMING_CBS_IND || msg > MSG_EMERGENCY_CBS_IND) {
        tapi_log_error("invalid msg_id in %s, msg_id: %d", __func__, (int)msg);
        return -EINVAL;
    }

    path = tapi_utils_get_modem_path(slot_id);
    if (path == NULL) {
        tapi_log_error("no available modem ...\n");
        return -EIO;
    }

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL) {
        tapi_log_error("user_data in %s is null", __func__);
        return -ENOMEM;
    }

    user_data->cb_function = p_handle;
    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        free(user_data);
        return -ENOMEM;
    }
    user_data->result = ar;
    ar->msg_id = msg;
    ar->msg_type = INDICATION;
    ar->arg1 = slot_id;
    ar->user_obj = user_obj;

    switch (msg) {
    case MSG_INCOMING_CBS_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection, OFONO_SERVICE, path,
            OFONO_CELL_BROADCAST_INTERFACE, "IncomingBroadcast",
            unsol_cbs_message, user_data, handler_free);
        break;
    case MSG_EMERGENCY_CBS_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection, OFONO_SERVICE, path,
            OFONO_CELL_BROADCAST_INTERFACE, "EmergencyBroadcast",
            unsol_cbs_message, user_data, handler_free);
        break;
    default:
        break;
    }

    if (watch_id == 0) {
        tapi_log_error("add signal watch failed in %s, msg_id: %d", __func__, (int)msg);
        handler_free(user_data);
        return -EINVAL;
    }

    return watch_id;
}
