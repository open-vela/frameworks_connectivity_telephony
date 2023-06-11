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

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int object_filter(GDBusProxy* proxy)
{
    const char* interface = g_dbus_proxy_get_interface(proxy);
    if (interface == NULL)
        return false;

    // ss related interface skip get properties
    if ((strcmp(interface, OFONO_CALL_BARRING_INTERFACE) == 0)
        || (strcmp(interface, OFONO_CALL_FORWARDING_INTERFACE) == 0)
        || (strcmp(interface, OFONO_CALL_SETTINGS_INTERFACE) == 0)) {
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

static void get_dbus_proxy(dbus_context* ctx)
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

    ctx->dbus_proxy_manager = g_dbus_proxy_new(
        ctx->client, OFONO_MANAGER_PATH, OFONO_MANAGER_INTERFACE);

    for (int i = 0; i < CONFIG_ACTIVE_MODEM_COUNT; i++) {
        for (int j = 0; j < DBUS_PROXY_MAX_COUNT; j++) {
            ctx->dbus_proxy[i][j] = g_dbus_proxy_new(
                ctx->client, tapi_utils_get_modem_path(i), dbus_proxy_server[j]);
        }

        list_initialize(&ctx->call_proxy_list[i]);
    }
}

static void release_dbus_proxy(dbus_context* ctx)
{
    tapi_dbus_call_proxy* call_proxy;
    tapi_dbus_call_proxy* tmp;

    g_dbus_proxy_unref(ctx->dbus_proxy_manager);
    for (int i = 0; i < CONFIG_ACTIVE_MODEM_COUNT; i++) {
        for (int j = 0; j < DBUS_PROXY_MAX_COUNT; j++) {
            g_dbus_proxy_unref(ctx->dbus_proxy[i][j]);
        }

        list_for_every_entry_safe(&ctx->call_proxy_list[i], call_proxy, tmp,
            tapi_dbus_call_proxy, node)
        {
            g_dbus_proxy_unref(call_proxy->dbus_proxy);

            list_delete(&call_proxy->node);
            free(call_proxy);
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

    if (handler == NULL)
        return;

    ar = handler->result;
    if (ar == NULL)
        return;

    cb = handler->cb_function;
    if (cb == NULL)
        return;

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("%s: %s\n", err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
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

    if (handler == NULL)
        return;

    ar = handler->result;
    if (ar == NULL)
        return;

    cb = handler->cb_function;
    if (cb == NULL)
        return;

    info = malloc(sizeof(modem_activity_info));
    if (info == NULL) {
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
        ar->status = ERROR;
        goto done;
    }

    dbus_message_iter_recurse(&iter, &array);
    dbus_message_iter_get_fixed_array(&array, &activity_info, &length);

    if (length != MAX_TX_TIME_ARRAY_LEN + 3) {
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

    if (handler == NULL)
        return;

    ar = handler->result;
    if (ar == NULL)
        return;

    cb = handler->cb_function;
    if (cb == NULL)
        return;

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

static void modem_status_query_done(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter;
    DBusError err;

    if (handler == NULL)
        return;

    ar = handler->result;
    if (ar == NULL)
        return;

    cb = handler->cb_function;
    if (cb == NULL)
        return;

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("%s: %s\n", err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        ar->status = ERROR;
        goto done;
    }

    dbus_message_iter_get_basic(&iter, &ar->arg2);

    ar->status = OK;

done:
    cb(ar);
}

static int radio_state_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter, var;
    const char* property;

    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (dbus_message_iter_init(message, &iter) == false)
        return false;

    dbus_message_iter_get_basic(&iter, &property);
    dbus_message_iter_next(&iter);

    dbus_message_iter_recurse(&iter, &var);
    if (strcmp(property, "RadioState") == 0) {
        dbus_message_iter_get_basic(&var, &ar->arg2);
        ar->status = OK;
        cb(ar);
    }

    return true;
}

static int phone_state_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter args;

    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (dbus_message_iter_init(message, &args) == false)
        goto done;

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

    if (handler == NULL)
        return false;

    if ((ar = handler->result) == NULL || (cb = handler->cb_function) == NULL)
        return false;

    if (dbus_message_iter_init(message, &iter) == false) {
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

    if (handler == NULL)
        return 0;

    ar = handler->result;
    if (ar == NULL)
        return 0;

    cb = handler->cb_function;
    if (cb == NULL)
        return 0;

    ar->status = OK;
    cb(ar);

    return 1;
}

static void oem_ril_request_raw_param_append(DBusMessageIter* iter, void* user_data)
{
    oem_ril_request_data* oem_ril_req_raw_param;
    tapi_async_handler* param;
    DBusMessageIter array;
    unsigned char* oem_req;
    int i;

    param = user_data;
    if (param == NULL || param->result == NULL)
        return;

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
    if (handler == NULL)
        return;

    if ((ar = handler->result) == NULL || (cb = handler->cb_function) == NULL)
        return;

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("%s: %s\n", err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
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
    if (param == NULL || param->result == NULL)
        return;

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
    if (handler == NULL)
        return;

    if ((ar = handler->result) == NULL || (cb = handler->cb_function) == NULL)
        return;

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("%s: %s\n", err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        ar->status = ERROR;
        goto done;
    }

    num = 0;
    dbus_message_iter_recurse(&iter, &array);
    while (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_STRING) {
        if (num == MAX_OEM_RIL_RESP_STRINGS_LENTH) {
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

    if (cbd == NULL)
        return;

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
    u_int32_t state = MODEM_STATE_POWER_OFF;

    if (strcmp("ModemState", name) != 0)
        return;

    dbus_message_iter_get_basic(iter, &state);
    tapi_log_info("%s - modem_state : %d", __func__, state);

    if (state == MODEM_STATE_ALIVE)
        get_dbus_proxy(ctx);
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

    g_dbus_client_set_proxy_handlers(client, object_add, object_remove,
        object_filter, NULL, NULL);

    client_ready_cb_data* cbd = malloc(sizeof(client_ready_cb_data));
    if (cbd == NULL) {
        g_dbus_client_unref(client);
        goto error;
    }

    cbd->client_name = client_name;
    cbd->context = ctx;
    cbd->user_data = user_data;
    cbd->callback = callback;
    if (!g_dbus_client_set_ready_watch(client, on_dbus_client_ready, cbd)) {
        g_dbus_client_unref(client);
        free(cbd);
        goto error;
    }

    ctx->connection = connection;
    ctx->client = client;
    ctx->client_ready = false;
    snprintf(ctx->name, sizeof(ctx->name), "%s", client_name);
    get_dbus_proxy(ctx);

    for (int i = 0; i < CONFIG_ACTIVE_MODEM_COUNT; i++) {
        g_dbus_proxy_set_property_watch(ctx->dbus_proxy[i][DBUS_PROXY_MODEM],
            on_modem_property_change, ctx);
    }

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

    for (int i = 0; i < CONFIG_ACTIVE_MODEM_COUNT; i++) {
        g_dbus_proxy_remove_property_watch(ctx->dbus_proxy[i][DBUS_PROXY_MODEM], NULL);
    }

    release_dbus_proxy(ctx);
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
    handler->result = ar;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy,
            "GetModems", NULL, modem_list_query_done, handler, handler_free)) {
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

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (!ctx->client_ready)
        return -EAGAIN;

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
    rat = tapi_utils_network_mode_to_string(mode);

    if (!g_dbus_proxy_set_property_basic(proxy,
            "TechnologyPreference", DBUS_TYPE_STRING, &rat,
            property_set_done, handler, handler_free)) {
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

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (!ctx->client_ready)
        return -EAGAIN;

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_RADIO];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "TechnologyPreference", &iter)) {
        dbus_message_iter_get_basic(&iter, &result);

        *out = tapi_utils_network_mode_from_string(result);
        return OK;
    }

    return -EINVAL;
}

int tapi_send_modem_power(tapi_context context, int slot_id, bool state)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    int value = state;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (!ctx->client_ready)
        return -EAGAIN;

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!g_dbus_proxy_set_property_basic(proxy,
            "Powered", DBUS_TYPE_BOOLEAN, &value, NULL, NULL, NULL))
        return -EINVAL;

    return OK;
}

int tapi_get_imei(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (!ctx->client_ready)
        return -EAGAIN;

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Serial", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    return -EINVAL;
}

int tapi_get_imeisv(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (!ctx->client_ready)
        return -EAGAIN;

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "SoftwareVersionNumber", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    return -EINVAL;
}

int tapi_get_modem_manufacturer(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (!ctx->client_ready)
        return -EAGAIN;

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Manufacturer", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    return -EINVAL;
}

int tapi_get_modem_model(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (!ctx->client_ready)
        return -EAGAIN;

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Model", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    return -EINVAL;
}

int tapi_get_modem_revision(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (!ctx->client_ready)
        return -EAGAIN;

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Revision", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    return -EINVAL;
}

int tapi_get_phone_state(tapi_context context, int slot_id, tapi_phone_state* state)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (!ctx->client_ready)
        return -EAGAIN;

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "PhoneStatus", &iter)) {
        dbus_message_iter_get_basic(&iter, state);
        return OK;
    }

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

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (!ctx->client_ready)
        return -EAGAIN;

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

    if (!g_dbus_proxy_set_property_basic(proxy, "Online", DBUS_TYPE_BOOLEAN,
            &value, property_set_done, handler, handler_free)) {
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

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (!ctx->client_ready)
        return -EAGAIN;

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Online", &iter)) {
        dbus_message_iter_get_basic(&iter, &result);

        *out = result;
        return OK;
    }

    return -EINVAL;
}

int tapi_get_radio_state(tapi_context context, int slot_id, tapi_radio_state* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    int result;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (!ctx->client_ready)
        return -EAGAIN;

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_MODEM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!g_dbus_proxy_get_property(proxy, "RadioState", &iter))
        return -EINVAL;

    dbus_message_iter_get_basic(&iter, &result);
    *out = result;
    return OK;
}

int tapi_get_msisdn_number(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter, var_elem;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (!ctx->client_ready)
        return -EAGAIN;

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!g_dbus_proxy_get_property(proxy, "SubscriberNumbers", &iter))
        return -EIO;

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

    return -EINVAL;
}

int tapi_get_modem_activity_info(tapi_context context, int slot_id,
    int event_id, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

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
    handler->result = ar;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "GetModemActivityInfo", NULL,
            modem_activity_info_query_done, handler, handler_free)) {
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

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || oem_req == NULL || length <= 0) {
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

    oem_ril_req = malloc(sizeof(oem_ril_request_data));
    if (oem_ril_req == NULL) {
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

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || oem_req == NULL || length <= 0) {
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

    oem_ril_req_param = malloc(sizeof(oem_ril_request_data));
    if (oem_ril_req_param == NULL) {
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

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    handler->result = ar;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, enable ? "EnableModem" : "DisableModem",
            NULL, enable_or_disable_modem_done, handler, handler_free)) {
        handler_free(handler);
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

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    handler->result = ar;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "GetModemStatus", NULL,
            modem_status_query_done, handler, handler_free)) {
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_register(tapi_context context,
    int slot_id, tapi_indication_msg msg, void* user_obj, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    const char* modem_path;
    int watch_id;
    tapi_async_handler* handler;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || msg < MSG_RADIO_STATE_CHANGE_IND || msg > MSG_MODEM_RESTART_IND) {
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
    default:
        break;
    }

    if (watch_id == 0) {
        handler_free(handler);
        return -EINVAL;
    }

    return watch_id;
}

int tapi_unregister(tapi_context context, int watch_id)
{
    dbus_context* ctx = context;
    if (ctx == NULL || watch_id <= 0)
        return -EINVAL;

    if (!g_dbus_remove_watch(ctx->connection, watch_id))
        return -EINVAL;

    return OK;
}

int tapi_handle_command(tapi_context context, int slot_id, int atom, int command)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

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

    ar->arg1 = atom;
    ar->arg2 = command;
    handler->result = ar;

    if (!g_dbus_proxy_method_call(proxy, "HandleCommand", atom_command_param_append,
            no_operate_callback, handler, handler_free)) {
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

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (!ctx->client_ready)
        return -EAGAIN;

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

    if (!g_dbus_proxy_set_property_basic(proxy, "FastDormancy", DBUS_TYPE_BOOLEAN,
            &value, property_set_done, handler, handler_free)) {
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}
