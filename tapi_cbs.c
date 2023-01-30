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

#include <dbus/dbus.h>
#include <errno.h>
#include <stdlib.h>

#include "tapi_cbs.h"
#include "tapi_internal.h"
#include <stdio.h>
#include <string.h>

static bool proxy_get_bool(GDBusProxy* proxy, const char* property)
{
    DBusMessageIter iter;
    dbus_bool_t value;

    if (!g_dbus_proxy_get_property(proxy, property, &iter))
        return false;

    dbus_message_iter_get_basic(&iter, &value);

    return value;
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

static void cbs_event_free(void* user_data)
{
    tapi_async_handler* handler;
    tapi_async_result* ar;

    tapi_log_debug("cbs_event_free %p", user_data);
    handler = user_data;
    if (handler != NULL) {
        ar = handler->result;
        if (ar != NULL)
            free(ar);
        free(handler);
    }
}

static gboolean unsol_cbs_message(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    const char* sender;
    const char* member;
    DBusMessageIter iter, list;
    DBusMessageIter entry, result;
    char* text;
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    char *name, *value;
    tapi_cbs_message* cbs_message;

    if (NULL == handler)
        return FALSE;

    ar = handler->result;
    if (ar == NULL)
        return FALSE;

    cb = handler->cb_function;
    if (cb == NULL)
        return FALSE;

    tapi_log_debug("unsol_cbs_message start \n");

    if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL)
        return FALSE;

    sender = dbus_message_get_sender(message);
    if (sender == NULL)
        return FALSE;

    if (!dbus_message_iter_init(message, &iter))
        return FALSE;

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING)
        return FALSE;

    dbus_message_iter_get_basic(&iter, &text);
    member = dbus_message_get_member(message);
    if (strcmp(member, "IncomingBroadcast") == 0) {
        cbs_message = calloc(1, sizeof(tapi_cbs_message));
        if (cbs_message == NULL)
            return FALSE;

        cbs_message->text = text;
        dbus_message_iter_next(&iter);
        dbus_message_iter_get_basic(&iter, &cbs_message->channel);

        ar->data = cbs_message;
        tapi_log_debug("%s, text : %s channel: %hu\n", __func__, text, cbs_message->channel);
    } else if (strcmp(member, "EmergencyBroadcast") == 0) {
        tapi_cbs_emergency_message* cbs_emergency_message = calloc(1, sizeof(tapi_cbs_emergency_message));
        tapi_log_debug("EmergencyBroadcast %s\n", text);
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
                tapi_log_debug("EmergencyBroadcast emergency_type: %s \n", cbs_emergency_message->emergency_type);
            } else if (strcmp(name, "EmergencyAlert") == 0) {
                dbus_message_iter_get_basic(&result, &cbs_emergency_message->emergency_alert);
                tapi_log_debug("EmergencyBroadcast emergency_alert: %d \n", cbs_emergency_message->emergency_alert);
            } else if (strcmp(name, "Popup") == 0) {
                dbus_message_iter_get_basic(&result, &cbs_emergency_message->popup);
                tapi_log_debug("EmergencyBroadcast popup: %d \n", cbs_emergency_message->popup);
            }
            dbus_message_iter_next(&list);
        }
        ar->data = cbs_emergency_message;
    }

    cb(handler->result);

    return TRUE;
}

int tapi_sms_set_cell_broadcast_power_on(tapi_context context, int slot_id, bool state)
{
    dbus_context* ctx = context;
    dbus_bool_t cbs_state = state;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CBS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EISCONN;
    }

    g_dbus_proxy_set_property_basic(proxy, "Powered",
        DBUS_TYPE_BOOLEAN, &cbs_state,
        NULL, NULL, NULL);
    return 0;
}

int tapi_sms_get_cell_broadcast_power_on(tapi_context context, int slot_id, bool* state)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CBS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EISCONN;
    }

    *state = proxy_get_bool(proxy, "Powered");

    return 0;
}

int tapi_sms_set_cell_broadcast_topics(tapi_context context, int slot_id, char* topics)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (topics == NULL) {
        return -EIO;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CBS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EISCONN;
    }

    g_dbus_proxy_set_property_basic(proxy, "Topics",
        DBUS_TYPE_STRING, &topics,
        NULL, NULL, NULL);

    return 0;
}

int tapi_sms_get_cell_broadcast_topics(tapi_context context, int slot_id, char** topics)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CBS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EISCONN;
    }

    *topics = proxy_get_string(proxy, "Topics");

    return 0;
}

int tapi_cbs_register(tapi_context context, int slot_id, tapi_indication_msg msg,
    tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    char* path;
    tapi_async_handler* user_data;
    tapi_async_result* ar;
    int watch_id;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || msg < MSG_TAPI_INCOMING_CBS_IND || msg > MSG_TAPI_EMERGENCY_CBS_IND) {
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
        return -ENOMEM;
    }
    user_data->result = ar;
    ar->msg_id = msg;
    ar->arg1 = slot_id;

    switch (msg) {
    case MSG_INCOMING_MESSAGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection, OFONO_SERVICE, path,
            OFONO_CELL_BROADCAST_INTERFACE, "IncomingBroadcast",
            unsol_cbs_message, user_data, cbs_event_free);
        break;
    case MSG_TAPI_EMERGENCY_CBS_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection, OFONO_SERVICE, path,
            OFONO_CELL_BROADCAST_INTERFACE, "EmergencyBroadcast",
            unsol_cbs_message, user_data, cbs_event_free);
        break;
    default:
        break;
    }

    return watch_id;
}