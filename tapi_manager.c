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

#include <dbus/dbus.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <gdbus.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include <tapi.h>

#include "tapi_internal.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void object_add(GDBusProxy* proxy, void* user_data)
{
}

static void object_remove(GDBusProxy* proxy, void* user_data)
{
}

static void get_dbus_proxy(dbus_context* ctx)
{
    const char* dbus_proxy_server[DBUS_PROXY_MAX_COUNT] = {
        OFONO_MODEM_INTERFACE,
        OFONO_RADIO_SETTINGS_INTERFACE,
        OFONO_VOICECALL_MANAGER_INTERFACE,
        OFONO_SIM_MANAGER_INTERFACE,
    };

    ctx->dbus_proxy_manager = g_dbus_proxy_new(
        ctx->client, OFONO_MANAGER_PATH, OFONO_MANAGER_INTERFACE);

    for (int i = 0; i < CONFIG_ACTIVE_MODEM_COUNT; i++) {
        for (int j = 0; j < DBUS_PROXY_MAX_COUNT; j++) {
            ctx->dbus_proxy[i][j] = g_dbus_proxy_new(
                ctx->client, tapi_utils_get_modem_path(i), dbus_proxy_server[j]);
        }
    }
}

static void release_dbus_proxy(dbus_context* ctx)
{
    for (int i = 0; i < CONFIG_ACTIVE_MODEM_COUNT; i++) {
        for (int j = 0; j < DBUS_PROXY_MAX_COUNT; j++) {
            g_dbus_proxy_unref(ctx->dbus_proxy[i][j]);
        }
    }
}

static void modem_property_set_done(const DBusError* error, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;

    tapi_log_debug("modem_property_set_done \n");
    if (handler == NULL)
        return;

    ar = handler->result;
    if (ar == NULL)
        return;

    cb = handler->cb_function;
    if (cb == NULL)
        return;

    if (dbus_error_is_set(error)) {
        tapi_log_error("%s: %s\n", error->name, error->message);
        ar->status = ERROR;
    } else {
        ar->status = OK;
    }

    cb(ar);
}

static void modem_list_query_done(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter args, list;
    DBusError err;
    char** result;
    int index;

    tapi_log_debug("modem_list_query_done \n");
    if (handler == NULL)
        return;

    ar = handler->result;
    if (ar == NULL)
        return;

    cb = handler->cb_function;
    if (cb == NULL)
        return;

    result = ar->data;
    ar->status = OK;

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == TRUE) {
        tapi_log_error("%s: %s\n", err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
    }

    if (dbus_message_has_signature(message, "a(oa{sv})") == false)
        goto done;
    if (dbus_message_iter_init(message, &args) == false)
        goto done;
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
    ar->arg1 = index; // modem count;

done:
    cb(ar);
}

static int radio_state_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;

    tapi_log_debug("radio_state_changed \n");
    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (!dbus_message_is_signal(message, OFONO_MODEM_INTERFACE, "PropertyChanged")
        || ar->msg_id != MSG_RADIO_STATE_CHANGE_IND) {
        return false;
    }

    cb(ar);
    return true;
}

static int phone_state_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter args;

    tapi_log_debug("phone_state_changed \n");
    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (!dbus_message_is_signal(message, OFONO_VOICECALL_MANAGER_INTERFACE, "PhoneStatusChanged")
        || ar->msg_id != MSG_PHONE_STATE_CHANGE_IND) {
        return false;
    }

    if (dbus_message_has_signature(message, "i") == false)
        return false;
    if (dbus_message_iter_init(message, &args) == false)
        return false;
    dbus_message_iter_get_basic(&args, &ar->arg2);

    cb(ar);
    return true;
}

static void user_data_free(void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;

    if (handler != NULL) {
        ar = handler->result;
        if (ar != NULL)
            free(ar);
        free(handler);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

tapi_context tapi_open(const char* client_name)
{
    DBusConnection* connection;
    GDBusClient* client;
    dbus_context* ctx;
    DBusError err;

    connection = g_dbus_setup_private(DBUS_BUS_SYSTEM, NULL, NULL);
    if (connection == NULL) {
        tapi_log_error("dbus connection init error \n");
        return NULL;
    }

    dbus_error_init(&err);
    dbus_request_name(connection, client_name, &err);
    if (dbus_error_is_set(&err)) {
        goto error;
    }

    client = g_dbus_client_new(connection, OFONO_SERVICE, OFONO_MANAGER_PATH);
    if (client == NULL) {
        goto error;
    }

    // fill tapi contexts .
    ctx = malloc(sizeof(dbus_context));
    if (ctx == NULL) {
        g_dbus_client_unref(client);
        goto error;
    }

    g_dbus_client_set_proxy_handlers(client, object_add, object_remove, NULL, NULL);

    ctx->connection = connection;
    ctx->client = client;
    memset(ctx->name, 0, sizeof(ctx->name));
    sprintf(ctx->name, "%s", client_name);
    get_dbus_proxy(ctx);

    return ctx;

error:
    dbus_connection_close(connection);
    dbus_connection_unref(connection);
    tapi_log_error("dbus client init error \n");
    return NULL;
}

int tapi_close(tapi_context context)
{
    dbus_context* ctx = context;
    if (ctx == NULL) {
        tapi_log_error("dbus connection close error \n");
        return -EINVAL;
    }

    release_dbus_proxy(ctx);
    g_dbus_remove_all_watches(ctx->connection);
    g_dbus_client_unref(ctx->client);
    dbus_connection_close(ctx->connection);
    dbus_connection_unref(ctx->connection);

    free(context);
    return 0;
}

int tapi_query_modem_list(tapi_context context,
    int event_id, char* list[], tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;

    proxy = ctx->dbus_proxy_manager;
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        return -ENOMEM;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(handler);
        return -ENOMEM;
    }

    ar->msg_id = event_id;
    ar->data = list;
    handler->result = ar;
    handler->cb_function = p_handle;

    return g_dbus_proxy_method_call(proxy,
        "GetModems", NULL, modem_list_query_done, handler, user_data_free);
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

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_RADIO];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL)
        return -ENOMEM;

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(handler);
        return -ENOMEM;
    }
    handler->result = ar;

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    handler->cb_function = p_handle;
    ar->data = "TechnologyPreference";
    rat = tapi_pref_network_mode_to_string(mode);

    g_dbus_proxy_set_property_basic(proxy,
        "TechnologyPreference", DBUS_TYPE_STRING, &rat,
        modem_property_set_done, handler, user_data_free);

    return 0;
}

int tapi_get_pref_net_mode(tapi_context context, int slot_id, tapi_pref_net_mode* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    char* result;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_RADIO];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "TechnologyPreference", &iter)) {
        dbus_message_iter_get_basic(&iter, &result);
    }
    tapi_pref_network_mode_from_string(result, out);

    return 0;
}

int tapi_reboot_modem(tapi_context context, int slot_id)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    int state;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    // Power Off
    state = false;
    g_dbus_proxy_set_property_basic(proxy,
        "Powered", DBUS_TYPE_BOOLEAN, &state, NULL, NULL, NULL);
    // Power on
    state = true;
    g_dbus_proxy_set_property_basic(proxy,
        "Powered", DBUS_TYPE_BOOLEAN, &state, NULL, NULL, NULL);

    return 0;
}

int tapi_get_imei(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Serial", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
    }

    return 0;
}

int tapi_get_imeisv(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "SoftwareVersionNumber", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
    }

    return 0;
}

int tapi_get_modem_manufacturer(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Manufacturer", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
    }

    return 0;
}

int tapi_get_modem_model(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Model", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
    }

    return 0;
}

int tapi_get_modem_revision(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Revision", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
    }

    return 0;
}

int tapi_get_phone_state(tapi_context context, int slot_id, tapi_phone_state* state)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "PhoneStatus", &iter)) {
        dbus_message_iter_get_basic(&iter, state);
    }

    return 0;
}

int tapi_set_radio_power(tapi_context context,
    int slot_id, int event_id, bool state, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    int value = state;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL)
        return -ENOMEM;

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(handler);
        return -ENOMEM;
    }
    handler->result = ar;

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    handler->cb_function = p_handle;

    g_dbus_proxy_set_property_basic(proxy,
        "Online", DBUS_TYPE_BOOLEAN, &value,
        modem_property_set_done, handler, user_data_free);

    return 0;
}

int tapi_get_radio_power(tapi_context context, int slot_id, bool* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    int result;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Online", &iter)) {
        dbus_message_iter_get_basic(&iter, &result);
    }

    *out = result;
    return 0;
}

int tapi_get_radio_state(tapi_context context, int slot_id, tapi_radio_state* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    int result;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    result = false;
    if (g_dbus_proxy_get_property(proxy, "Powered", &iter)) {
        dbus_message_iter_get_basic(&iter, &result);
    }

    if (!result) {
        *out = RADIO_STATE_UNAVAILABLE;
        return 0;
    }
    if (g_dbus_proxy_get_property(proxy, "Emergency", &iter)) {
        dbus_message_iter_get_basic(&iter, &result);
    }
    if (result) {
        *out = RADIO_STATE_EMERGENCY_ONLY;
        return 0;
    }
    if (g_dbus_proxy_get_property(proxy, "Online", &iter)) {
        dbus_message_iter_get_basic(&iter, &result);
    }
    if (result) {
        *out = RADIO_STATE_ON;
    } else {
        *out = RADIO_STATE_OFF;
    }

    return 0;
}

int tapi_get_msisdn_number(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!g_dbus_proxy_get_property(proxy, "SubscriberNumbers", &iter))
        return -EIO;

    if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
        DBusMessageIter var_elem;
        dbus_message_iter_recurse(&iter, &var_elem);
        while (dbus_message_iter_get_arg_type(&var_elem) != DBUS_TYPE_INVALID) {
            if (dbus_message_iter_get_arg_type(&var_elem) == DBUS_TYPE_STRING) {
                dbus_message_iter_get_basic(&var_elem, out);
                break;
            }
        }
    }

    return 0;
}

int tapi_register(tapi_context context,
    int slot_id, tapi_indication_msg msg, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    char* modem_path;
    int watch_id;
    tapi_async_handler* handler;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || msg < MSG_RADIO_STATE_CHANGE_IND || msg > MSG_PHONE_STATE_CHANGE_IND) {
        return -EINVAL;
    }

    modem_path = tapi_utils_get_modem_path(slot_id);
    if (modem_path == NULL) {
        tapi_log_error("no available modem ...\n");
        return -EIO;
    }

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL)
        return -ENOMEM;

    handler->cb_function = p_handle;
    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(handler);
        return -ENOMEM;
    }
    handler->result = ar;
    ar->msg_id = msg;
    ar->arg1 = slot_id;

    switch (msg) {
    case MSG_RADIO_STATE_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_MODEM_INTERFACE,
            "PropertyChanged", radio_state_changed, handler, user_data_free);
        break;
    case MSG_PHONE_STATE_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_VOICECALL_MANAGER_INTERFACE,
            "PhoneStatusChanged", phone_state_changed, handler, user_data_free);
        break;
    default:
        break;
    }

    return watch_id;
}

int tapi_unregister(tapi_context context, int watch_id)
{
    dbus_context* ctx = context;
    if (ctx == NULL || watch_id <= 0)
        return -EINVAL;

    return g_dbus_remove_watch(ctx->connection, watch_id);
}
