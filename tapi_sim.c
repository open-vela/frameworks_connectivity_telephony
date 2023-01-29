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
#include <gdbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <tapi.h>

#include "tapi_internal.h"

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

typedef struct {
    char* pin_type;
    char* old_pin;
    char* new_pin;
} sim_pin_param;

typedef struct {
    char* puk_type;
    char* puk;
    char* new_pin;
} sim_reset_pin_param;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int sim_state_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data);
static void user_data_free(void* user_data);
static void change_pin_param_append(DBusMessageIter* iter, void* user_data);
static void method_call_complete(DBusMessage* message, void* user_data);
static void enter_pin_param_append(DBusMessageIter* iter, void* user_data);
static void reset_pin_param_append(DBusMessageIter* iter, void* user_data);
static void lock_pin_param_append(DBusMessageIter* iter, void* user_data);
static void unlock_pin_param_append(DBusMessageIter* iter, void* user_data);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int sim_state_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    int msg_id;

    tapi_log_debug(" %s \n", __func__);

    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    msg_id = ar->msg_id;
    if (!dbus_message_is_signal(message, OFONO_SIM_MANAGER_INTERFACE, "PropertyChanged")
        || msg_id != MSG_SIM_STATE_CHANGE_IND) {
        return false;
    }

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

static void change_pin_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    sim_pin_param* change_pin_param;
    char* pin_type;
    char* old_pin;
    char* new_pin;

    if (param == NULL || param->result == NULL) {
        tapi_log_error("invalid pin argument!");
        return;
    }

    change_pin_param = param->result->data;
    pin_type = change_pin_param->pin_type;
    old_pin = change_pin_param->old_pin;
    new_pin = change_pin_param->new_pin;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &pin_type);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &old_pin);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &new_pin);
}

static void method_call_complete(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter;
    DBusError err;

    tapi_log_debug(" %s  \n", __func__);

    if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_ERROR) {
        const char* dbus_error = dbus_message_get_error_name(message);
        tapi_log_error("%s error: %s \n", __func__, dbus_error);
        return;
    }

    handler = user_data;
    if (handler == NULL)
        return;

    ar = handler->result;
    if (ar == NULL)
        return;
    ar->status = OK;

    cb = handler->cb_function;
    if (cb == NULL)
        return;

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("%s error %s: %s \n", __func__, err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        ar->status = ERROR;
        goto done;
    }

done:
    cb(ar);
}

static void enter_pin_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    sim_pin_param* enter_pin_param;
    char* pin_type;
    char* pin;

    if (param == NULL || param->result == NULL) {
        tapi_log_error("invalid pin argument!");
        return;
    }

    enter_pin_param = param->result->data;
    pin_type = enter_pin_param->pin_type;
    pin = enter_pin_param->new_pin;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &pin_type);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &pin);
}

static void reset_pin_param_append(DBusMessageIter* iter, void* user_data)
{
    sim_reset_pin_param* reset_pin_param;
    tapi_async_handler* param = user_data;
    char* puk_type;
    char* new_pin;
    char* puk;

    if (param == NULL || param->result == NULL) {
        tapi_log_error("invalid pin argument!");
        return;
    }

    reset_pin_param = param->result->data;
    puk_type = reset_pin_param->puk_type;
    puk = reset_pin_param->puk;
    new_pin = reset_pin_param->new_pin;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &puk_type);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &puk);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &new_pin);
}

static void lock_pin_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    sim_pin_param* lock_pin_param;
    char* pin_type;
    char* pin;

    if (param == NULL || param->result == NULL) {
        tapi_log_error("invalid pin argument!");
        return;
    }

    lock_pin_param = param->result->data;
    pin_type = lock_pin_param->pin_type;
    pin = lock_pin_param->new_pin;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &pin_type);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &pin);
}

static void unlock_pin_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    sim_pin_param* unlock_pin_param;
    char* pin_type;
    char* pin;

    if (param == NULL || param->result == NULL) {
        tapi_log_error("invalid pin argument!");
        return;
    }

    unlock_pin_param = param->result->data;
    pin_type = unlock_pin_param->pin_type;
    pin = unlock_pin_param->new_pin;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &pin_type);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &pin);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int tapi_sim_has_icc_card(tapi_context context, int slot_id, bool* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    int result;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Present", &iter)) {
        dbus_message_iter_get_basic(&iter, &result);
    }

    *out = result;

    return 0;
}

int tapi_sim_get_sim_iccid(tapi_context context, int slot_id, char** out)
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

    if (g_dbus_proxy_get_property(proxy, "CardIdentifier", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
    }

    return 0;
}

int tapi_sim_get_sim_operator(tapi_context context, int slot_id, int length, char* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    char* mcc;
    char* mnc;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (out == NULL || length < (MAX_MCC_LENGTH + MAX_MNC_LENGTH + 1)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "MobileCountryCode", &iter)) {
        dbus_message_iter_get_basic(&iter, &mcc);
    }

    if (g_dbus_proxy_get_property(proxy, "MobileNetworkCode", &iter)) {
        dbus_message_iter_get_basic(&iter, &mnc);
    }

    if (mcc == NULL || mnc == NULL)
        return -EIO;

    for (int i = 0; i < MAX_MCC_LENGTH; i++)
        *out++ = *mcc++;
    for (int j = 0; j < MAX_MNC_LENGTH; j++)
        *out++ = *mnc++;
    *out = '\0';

    return 0;
}

int tapi_sim_get_sim_operator_name(tapi_context context, int slot_id, char** out)
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

    if (g_dbus_proxy_get_property(proxy, "ServiceProviderName", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
    }

    return 0;
}

int tapi_sim_register_sim_state_change(tapi_context context, int slot_id,
    tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    char* modem_path;
    int watch_id;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
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
    ar->msg_id = MSG_SIM_STATE_CHANGE_IND;
    ar->arg1 = slot_id;

    watch_id = g_dbus_add_signal_watch(ctx->connection,
        OFONO_SERVICE, modem_path, OFONO_SIM_MANAGER_INTERFACE,
        "PropertyChanged", sim_state_changed, handler, user_data_free);

    return watch_id;
}

int tapi_sim_unregister_sim_state_change(tapi_context context, int watch_id)
{
    dbus_context* ctx = context;

    if (ctx == NULL || watch_id <= 0)
        return -EINVAL;

    return g_dbus_remove_watch(ctx->connection, watch_id);
}

int tapi_sim_change_pin(tapi_context context, int slot_id,
    int event_id, char* pin_type, char* old_pin, char* new_pin, tapi_async_function p_handle)
{
    sim_pin_param* change_pin_param;
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || pin_type == NULL || old_pin == NULL || new_pin == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    change_pin_param = malloc(sizeof(sim_pin_param));
    if (change_pin_param == NULL) {
        return -ENOMEM;
    }
    change_pin_param->pin_type = pin_type;
    change_pin_param->old_pin = old_pin;
    change_pin_param->new_pin = new_pin;

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(change_pin_param);
        return -ENOMEM;
    }
    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->data = change_pin_param;

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL) {
        free(change_pin_param);
        free(ar);
        return -ENOMEM;
    }
    user_data->cb_function = p_handle;
    user_data->result = ar;

    return g_dbus_proxy_method_call(proxy, "ChangePin",
        change_pin_param_append, method_call_complete, user_data, user_data_free);
}

int tapi_sim_enter_pin(tapi_context context, int slot_id,
    int event_id, char* pin_type, char* pin, tapi_async_function p_handle)
{
    sim_pin_param* enter_pin_param;
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || pin_type == NULL || pin == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    enter_pin_param = malloc(sizeof(sim_pin_param));
    if (enter_pin_param == NULL) {
        return -ENOMEM;
    }
    enter_pin_param->pin_type = pin_type;
    enter_pin_param->new_pin = pin;

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(enter_pin_param);
        return -ENOMEM;
    }
    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->data = enter_pin_param;

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL) {
        free(enter_pin_param);
        free(ar);
        return -ENOMEM;
    }
    user_data->cb_function = p_handle;
    user_data->result = ar;

    return g_dbus_proxy_method_call(proxy, "EnterPin",
        enter_pin_param_append, method_call_complete, user_data, user_data_free);
}

int tapi_sim_reset_pin(tapi_context context, int slot_id,
    int event_id, char* puk_type, char* puk, char* new_pin, tapi_async_function p_handle)
{
    sim_reset_pin_param* reset_pin_param;
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || puk_type == NULL || puk == NULL || new_pin == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    reset_pin_param = malloc(sizeof(sim_pin_param));
    if (reset_pin_param == NULL) {
        return -ENOMEM;
    }
    reset_pin_param->puk_type = puk_type;
    reset_pin_param->puk = puk;
    reset_pin_param->new_pin = new_pin;

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(reset_pin_param);
        return -ENOMEM;
    }
    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->data = reset_pin_param;

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL) {
        free(reset_pin_param);
        free(ar);
        return -ENOMEM;
    }
    user_data->cb_function = p_handle;
    user_data->result = ar;

    return g_dbus_proxy_method_call(proxy, "ResetPin",
        reset_pin_param_append, method_call_complete, user_data, user_data_free);
}

int tapi_sim_lock_pin(tapi_context context, int slot_id,
    int event_id, char* pin_type, char* pin, tapi_async_function p_handle)
{
    sim_pin_param* lock_pin_param;
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || pin_type == NULL || pin == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    lock_pin_param = malloc(sizeof(sim_pin_param));
    if (lock_pin_param == NULL) {
        return -ENOMEM;
    }
    lock_pin_param->pin_type = pin_type;
    lock_pin_param->new_pin = pin;

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(lock_pin_param);
        return -ENOMEM;
    }
    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->data = lock_pin_param;

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL) {
        free(lock_pin_param);
        free(ar);
        return -ENOMEM;
    }
    user_data->cb_function = p_handle;
    user_data->result = ar;

    return g_dbus_proxy_method_call(proxy, "LockPin",
        lock_pin_param_append, method_call_complete, user_data, user_data_free);
}

int tapi_sim_unlock_pin(tapi_context context, int slot_id,
    int event_id, char* pin_type, char* pin, tapi_async_function p_handle)
{
    sim_pin_param* unlock_pin_param;
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || pin_type == NULL || pin == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    unlock_pin_param = malloc(sizeof(sim_pin_param));
    if (unlock_pin_param == NULL) {
        return -ENOMEM;
    }
    unlock_pin_param->pin_type = pin_type;
    unlock_pin_param->new_pin = pin;

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(unlock_pin_param);
        return -ENOMEM;
    }
    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->data = unlock_pin_param;

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL) {
        free(unlock_pin_param);
        free(ar);
        return -ENOMEM;
    }
    user_data->cb_function = p_handle;
    user_data->result = ar;

    return g_dbus_proxy_method_call(proxy, "UnlockPin",
        unlock_pin_param_append, method_call_complete, user_data, user_data_free);
}