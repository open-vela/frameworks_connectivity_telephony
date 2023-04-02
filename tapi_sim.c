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
#include "tapi_sim.h"

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

typedef struct {
    int session_id;
    char* pdu;
    unsigned int len;
} sim_transmit_apdu_param;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int sim_state_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data);
/* This method should be called if the data field needs to be recycled. */
static void sim_event_data_free(void* user_data);
static void change_pin_param_append(DBusMessageIter* iter, void* user_data);
static void method_call_complete(DBusMessage* message, void* user_data);
static void enter_pin_param_append(DBusMessageIter* iter, void* user_data);
static void reset_pin_param_append(DBusMessageIter* iter, void* user_data);
static void lock_pin_param_append(DBusMessageIter* iter, void* user_data);
static void unlock_pin_param_append(DBusMessageIter* iter, void* user_data);
static void open_channel_param_append(DBusMessageIter* iter, void* user_data);
static void close_channel_param_append(DBusMessageIter* iter, void* user_data);
static void transmit_apdu_param_append(DBusMessageIter* iter, void* user_data);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int sim_state_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter, var, arr, entry;
    sim_lock_state* sim_lock;
    sim_state_result ss;
    char* property;
    int index;

    if (handler == NULL)
        return 0;

    ar = handler->result;
    if (ar == NULL)
        return 0;

    cb = handler->cb_function;
    if (cb == NULL)
        return 0;

    if (ar->msg_id != MSG_SIM_STATE_CHANGE_IND) {
        return 0;
    }

    if (dbus_message_iter_init(message, &iter) == false)
        return 0;

    dbus_message_iter_get_basic(&iter, &property);
    dbus_message_iter_next(&iter);

    dbus_message_iter_recurse(&iter, &var);

    sim_lock = NULL;
    if (strcmp(property, "Present") == 0) {
        dbus_message_iter_get_basic(&var, &ss.value);
        goto done;
    } else if (strcmp(property, "PinRequired") == 0) {
        dbus_message_iter_get_basic(&var, &ss.data);
        goto done;
    } else if (strcmp(property, "LockedPins") == 0) {
        sim_lock = malloc(sizeof(sim_lock_state));
        if (sim_lock == NULL) {
            return 0;
        }

        index = 0;
        dbus_message_iter_recurse(&var, &arr);
        while (dbus_message_iter_get_arg_type(&arr) == DBUS_TYPE_STRING) {
            dbus_message_iter_get_basic(&arr, &sim_lock->sim_pwd_type[index++]);
            dbus_message_iter_next(&arr);

            if (index >= MAX_SIM_PWD_TYPE)
                break;
        }

        ss.data = sim_lock;
        ar->arg2 = index;

        goto done;
    } else if (strcmp(property, "Retries") == 0) {
        sim_lock = malloc(sizeof(sim_lock_state));
        if (sim_lock == NULL) {
            return 0;
        }

        index = 0;
        dbus_message_iter_recurse(&var, &arr);
        if (dbus_message_iter_get_arg_type(&arr) == DBUS_TYPE_ARRAY) {
            dbus_message_iter_recurse(&arr, &entry);

            while (dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_INVALID) {
                if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_DICT_ENTRY) {
                    dbus_message_iter_get_basic(&entry, &sim_lock->sim_pwd_type[index++]);
                    dbus_message_iter_next(&entry);

                    dbus_message_iter_get_basic(&entry, &sim_lock->retry_count[index++]);
                }

                if (index >= MAX_SIM_PWD_TYPE)
                    break;

                dbus_message_iter_next(&arr);
                dbus_message_iter_recurse(&arr, &entry);
            }
        }

        ss.data = sim_lock;
        ar->arg2 = index;

        goto done;
    }

    return 1;

done:
    ss.name = property;
    ar->data = &ss;
    ar->status = OK;

    cb(ar);

    if (sim_lock != NULL)
        free(sim_lock);

    return 1;
}

static int sim_uicc_app_enabled_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter, var;
    const char* property;
    int enabled;

    if (handler == NULL)
        return 0;

    ar = handler->result;
    if (ar == NULL)
        return 0;

    cb = handler->cb_function;
    if (cb == NULL)
        return 0;

    if (ar->msg_id != MSG_SIM_UICC_APP_ENABLED_CHANGE_IND) {
        return 0;
    }

    if (dbus_message_iter_init(message, &iter) == false)
        return 0;

    dbus_message_iter_get_basic(&iter, &property);
    dbus_message_iter_next(&iter);

    dbus_message_iter_recurse(&iter, &var);
    if (strcmp(property, "UiccActive") == 0) {
        dbus_message_iter_get_basic(&var, &enabled);
        ar->status = OK;
        ar->arg2 = enabled;
        cb(ar);
    }

    return 1;
}

/* This method should be called if the data field needs to be recycled. */
static void sim_event_data_free(void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    void* data;

    if (handler != NULL) {
        ar = handler->result;
        if (ar != NULL) {
            data = ar->data;
            if (data != NULL)
                free(data);

            free(ar);
        }

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
        tapi_log_error("%s error %s: %s \n", __func__, err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
    }

    ar->status = OK;

done:
    cb(ar);
}

static void open_logical_channel_cb(DBusMessage* message, void* user_data)
{
    DBusMessageIter iter;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusError err;

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

    dbus_message_iter_get_basic(&iter, &ar->arg2); /*session id*/

    ar->status = OK;

done:
    cb(ar);
}

static void transmit_apdu_cb(DBusMessage* message, void* user_data)
{
    DBusMessageIter iter;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusError err;
    char* response;

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

    dbus_message_iter_get_basic(&iter, &response);

    ar->data = response;
    ar->status = OK;

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

static void open_channel_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    char* aid;

    if (param == NULL || param->result == NULL) {
        tapi_log_error("invalid argument!");
        return;
    }

    aid = param->result->data;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &aid);
}

static void close_channel_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    int session_id;

    if (param == NULL || param->result == NULL) {
        tapi_log_error("invalid argument!");
        return;
    }

    session_id = param->result->arg2;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &session_id);
}

static void transmit_apdu_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    sim_transmit_apdu_param* transmit_apdu_param;
    int session_id;
    unsigned int len;
    char* pdu;

    if (param == NULL || param->result == NULL) {
        tapi_log_error("invalid argument!");
        return;
    }

    transmit_apdu_param = param->result->data;
    session_id = transmit_apdu_param->session_id;
    pdu = transmit_apdu_param->pdu;
    len = transmit_apdu_param->len;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &session_id);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &pdu);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &len);
}

static void transmit_apdu_basic_channel_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    sim_transmit_apdu_param* transmit_apdu_param;
    unsigned int len;
    char* pdu;

    if (param == NULL || param->result == NULL) {
        tapi_log_error("invalid argument!");
        return;
    }

    transmit_apdu_param = param->result->data;
    pdu = transmit_apdu_param->pdu;
    len = transmit_apdu_param->len;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &pdu);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &len);
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

        *out = result;
        return OK;
    }

    return -EINVAL;
}

int tapi_sim_get_sim_state(tapi_context context, int slot_id, int* out)
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

    if (g_dbus_proxy_get_property(proxy, "SimState", &iter)) {
        dbus_message_iter_get_basic(&iter, out);

        return OK;
    }

    return -EINVAL;
}

int tapi_sim_get_sim_iccid(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    bool has_icc_card;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    has_icc_card = false;
    tapi_sim_has_icc_card(context, slot_id, &has_icc_card);

    if (has_icc_card) {
        proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
        if (proxy == NULL) {
            tapi_log_error("no available proxy ...\n");
            return -EIO;
        }

        if (g_dbus_proxy_get_property(proxy, "CardIdentifier", &iter)) {
            dbus_message_iter_get_basic(&iter, out);
            return OK;
        }
    }

    return -EINVAL;
}

int tapi_sim_get_sim_operator(tapi_context context, int slot_id, int length, char* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    bool has_icc_card;
    char* mcc;
    char* mnc;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (out == NULL || length < (MAX_MCC_LENGTH + MAX_MNC_LENGTH + 1)) {
        return -EINVAL;
    }

    has_icc_card = false;
    tapi_sim_has_icc_card(context, slot_id, &has_icc_card);

    if (has_icc_card) {
        proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
        if (proxy == NULL) {
            tapi_log_error("no available proxy ...\n");
            return -EIO;
        }

        mcc = NULL;
        if (g_dbus_proxy_get_property(proxy, "MobileCountryCode", &iter)) {
            dbus_message_iter_get_basic(&iter, &mcc);
        }

        mnc = NULL;
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

        return OK;
    }

    return -EINVAL;
}

int tapi_sim_get_sim_operator_name(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    bool has_icc_card;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    has_icc_card = false;
    tapi_sim_has_icc_card(context, slot_id, &has_icc_card);

    if (has_icc_card) {
        proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
        if (proxy == NULL) {
            tapi_log_error("no available proxy ...\n");
            return -EIO;
        }

        if (g_dbus_proxy_get_property(proxy, "ServiceProviderName", &iter)) {
            dbus_message_iter_get_basic(&iter, out);
            return OK;
        }
    }

    return -EINVAL;
}

int tapi_sim_get_subscriber_id(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    bool has_icc_card;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    has_icc_card = false;
    tapi_sim_has_icc_card(context, slot_id, &has_icc_card);

    if (has_icc_card) {
        proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
        if (proxy == NULL) {
            tapi_log_error("no available proxy ...\n");
            return -EIO;
        }

        if (g_dbus_proxy_get_property(proxy, "SubscriberIdentity", &iter)) {
            dbus_message_iter_get_basic(&iter, out);
            return OK;
        }
    }

    return -EINVAL;
}

int tapi_sim_register(tapi_context context, int slot_id,
    tapi_indication_msg msg, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    const char* modem_path;
    int watch_id = 0;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || msg < MSG_SIM_STATE_CHANGE_IND || msg > MSG_SIM_UICC_APP_ENABLED_CHANGE_IND) {
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
    case MSG_SIM_STATE_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_SIM_MANAGER_INTERFACE,
            "PropertyChanged", sim_state_changed, handler, user_data_free);
        break;
    case MSG_SIM_UICC_APP_ENABLED_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_SIM_MANAGER_INTERFACE,
            "PropertyChanged", sim_uicc_app_enabled_changed, handler, user_data_free);
        break;
    default:
        break;
    }

    if (watch_id == 0) {
        user_data_free(handler);
        return -EINVAL;
    }

    return watch_id;
}

int tapi_sim_unregister(tapi_context context, int watch_id)
{
    dbus_context* ctx = context;

    if (ctx == NULL || watch_id <= 0)
        return -EINVAL;

    if (!g_dbus_remove_watch(ctx->connection, watch_id))
        return -EINVAL;

    return OK;
}

int tapi_sim_change_pin(tapi_context context, int slot_id,
    int event_id, char* pin_type, char* old_pin, char* new_pin, tapi_async_function p_handle)
{
    sim_pin_param* change_pin_param;
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    tapi_async_result* ar;
    GDBusProxy* proxy;
    bool has_icc_card;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || pin_type == NULL || old_pin == NULL || new_pin == NULL) {
        return -EINVAL;
    }

    has_icc_card = false;
    tapi_sim_has_icc_card(context, slot_id, &has_icc_card);

    if (has_icc_card) {
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

        if (!g_dbus_proxy_method_call(proxy, "ChangePin",
                change_pin_param_append, method_call_complete, user_data, sim_event_data_free)) {
            sim_event_data_free(user_data);
            return -EINVAL;
        }

        return OK;
    }

    return -EINVAL;
}

int tapi_sim_enter_pin(tapi_context context, int slot_id,
    int event_id, char* pin_type, char* pin, tapi_async_function p_handle)
{
    sim_pin_param* enter_pin_param;
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    tapi_async_result* ar;
    GDBusProxy* proxy;
    bool has_icc_card;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || pin_type == NULL || pin == NULL) {
        return -EINVAL;
    }

    has_icc_card = false;
    tapi_sim_has_icc_card(context, slot_id, &has_icc_card);

    if (has_icc_card) {
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

        if (!g_dbus_proxy_method_call(proxy, "EnterPin",
                enter_pin_param_append, method_call_complete, user_data, sim_event_data_free)) {
            sim_event_data_free(user_data);
            return -EINVAL;
        }

        return OK;
    }

    return -EINVAL;
}

int tapi_sim_reset_pin(tapi_context context, int slot_id,
    int event_id, char* puk_type, char* puk, char* new_pin, tapi_async_function p_handle)
{
    sim_reset_pin_param* reset_pin_param;
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    tapi_async_result* ar;
    GDBusProxy* proxy;
    bool has_icc_card;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || puk_type == NULL || puk == NULL || new_pin == NULL) {
        return -EINVAL;
    }

    has_icc_card = false;
    tapi_sim_has_icc_card(context, slot_id, &has_icc_card);

    if (has_icc_card) {
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

        if (!g_dbus_proxy_method_call(proxy, "ResetPin",
                reset_pin_param_append, method_call_complete, user_data, sim_event_data_free)) {
            sim_event_data_free(user_data);
            return -EINVAL;
        }

        return OK;
    }

    return -EINVAL;
}

int tapi_sim_lock_pin(tapi_context context, int slot_id,
    int event_id, char* pin_type, char* pin, tapi_async_function p_handle)
{
    sim_pin_param* lock_pin_param;
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_result* ar;
    bool has_icc_card;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || pin_type == NULL || pin == NULL) {
        return -EINVAL;
    }

    has_icc_card = false;
    tapi_sim_has_icc_card(context, slot_id, &has_icc_card);

    if (has_icc_card) {
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

        if (!g_dbus_proxy_method_call(proxy, "LockPin",
                lock_pin_param_append, method_call_complete, user_data, sim_event_data_free)) {
            sim_event_data_free(user_data);
            return -EINVAL;
        }

        return OK;
    }

    return -EINVAL;
}

int tapi_sim_unlock_pin(tapi_context context, int slot_id,
    int event_id, char* pin_type, char* pin, tapi_async_function p_handle)
{
    sim_pin_param* unlock_pin_param;
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_result* ar;
    bool has_icc_card;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || pin_type == NULL || pin == NULL) {
        return -EINVAL;
    }

    has_icc_card = false;
    tapi_sim_has_icc_card(context, slot_id, &has_icc_card);

    if (has_icc_card) {
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

        if (!g_dbus_proxy_method_call(proxy, "UnlockPin",
                unlock_pin_param_append, method_call_complete, user_data, sim_event_data_free)) {
            sim_event_data_free(user_data);
            return -EINVAL;
        }

        return OK;
    }

    return -EINVAL;
}

int tapi_sim_open_logical_channel(tapi_context context, int slot_id,
    int event_id, char* aid, tapi_async_function p_handle)
{
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    tapi_async_result* ar;
    GDBusProxy* proxy;
    bool has_icc_card;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || aid == NULL) {
        return -EINVAL;
    }

    has_icc_card = false;
    tapi_sim_has_icc_card(context, slot_id, &has_icc_card);

    if (has_icc_card) {
        proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
        if (proxy == NULL) {
            tapi_log_error("no available proxy ...\n");
            return -EIO;
        }

        ar = malloc(sizeof(tapi_async_result));
        if (ar == NULL) {
            return -ENOMEM;
        }
        ar->msg_id = event_id;
        ar->arg1 = slot_id;
        ar->data = aid;

        user_data = malloc(sizeof(tapi_async_handler));
        if (user_data == NULL) {
            free(ar);
            return -ENOMEM;
        }
        user_data->cb_function = p_handle;
        user_data->result = ar;

        if (!g_dbus_proxy_method_call(proxy, "OpenLogicalChannel", open_channel_param_append,
                open_logical_channel_cb, user_data, user_data_free)) {
            user_data_free(user_data);
            return -EINVAL;
        }

        return OK;
    }

    return -EINVAL;
}

int tapi_sim_close_logical_channel(tapi_context context, int slot_id,
    int event_id, int session_id, tapi_async_function p_handle)
{
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    tapi_async_result* ar;
    GDBusProxy* proxy;
    bool has_icc_card;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    has_icc_card = false;
    tapi_sim_has_icc_card(context, slot_id, &has_icc_card);

    if (has_icc_card) {
        proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
        if (proxy == NULL) {
            tapi_log_error("no available proxy ...\n");
            return -EIO;
        }

        ar = malloc(sizeof(tapi_async_result));
        if (ar == NULL) {
            return -ENOMEM;
        }
        ar->msg_id = event_id;
        ar->arg1 = slot_id;
        ar->arg2 = session_id;

        user_data = malloc(sizeof(tapi_async_handler));
        if (user_data == NULL) {
            free(ar);
            return -ENOMEM;
        }
        user_data->cb_function = p_handle;
        user_data->result = ar;

        if (!g_dbus_proxy_method_call(proxy, "CloseLogicalChannel", close_channel_param_append,
                method_call_complete, user_data, user_data_free)) {
            user_data_free(user_data);
            return -EINVAL;
        }

        return OK;
    }

    return -EINVAL;
}

int tapi_sim_transmit_apdu_logical_channel(tapi_context context, int slot_id,
    int event_id, int session_id, char* pdu, unsigned int len, tapi_async_function p_handle)
{
    sim_transmit_apdu_param* transmit_apdu_param;
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    tapi_async_result* ar;
    GDBusProxy* proxy;
    bool has_icc_card;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || pdu == NULL) {
        return -EINVAL;
    }

    has_icc_card = false;
    tapi_sim_has_icc_card(context, slot_id, &has_icc_card);

    if (has_icc_card) {
        proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
        if (proxy == NULL) {
            tapi_log_error("no available proxy ...\n");
            return -EIO;
        }

        transmit_apdu_param = malloc(sizeof(sim_transmit_apdu_param));
        if (transmit_apdu_param == NULL) {
            return -ENOMEM;
        }
        transmit_apdu_param->session_id = session_id;
        transmit_apdu_param->pdu = pdu;
        transmit_apdu_param->len = len;

        ar = malloc(sizeof(tapi_async_result));
        if (ar == NULL) {
            free(transmit_apdu_param);
            return -ENOMEM;
        }
        ar->msg_id = event_id;
        ar->arg1 = slot_id;
        ar->data = transmit_apdu_param;

        user_data = malloc(sizeof(tapi_async_handler));
        if (user_data == NULL) {
            free(transmit_apdu_param);
            free(ar);
            return -ENOMEM;
        }
        user_data->cb_function = p_handle;
        user_data->result = ar;

        if (!g_dbus_proxy_method_call(proxy, "TransmitApduLogicalChannel",
                transmit_apdu_param_append, transmit_apdu_cb, user_data, sim_event_data_free)) {
            sim_event_data_free(user_data);
            return -EINVAL;
        }

        return OK;
    }

    return -EINVAL;
}

int tapi_sim_transmit_apdu_basic_channel(tapi_context context, int slot_id,
    int event_id, char* pdu, unsigned int len, tapi_async_function p_handle)
{
    sim_transmit_apdu_param* transmit_apdu_param;
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    tapi_async_result* ar;
    GDBusProxy* proxy;
    bool has_icc_card;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id) || pdu == NULL) {
        return -EINVAL;
    }

    has_icc_card = false;
    tapi_sim_has_icc_card(context, slot_id, &has_icc_card);

    if (has_icc_card) {
        proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
        if (proxy == NULL) {
            tapi_log_error("no available proxy ...\n");
            return -EIO;
        }

        transmit_apdu_param = malloc(sizeof(sim_transmit_apdu_param));
        if (transmit_apdu_param == NULL) {
            return -ENOMEM;
        }
        transmit_apdu_param->pdu = pdu;
        transmit_apdu_param->len = len;

        ar = malloc(sizeof(tapi_async_result));
        if (ar == NULL) {
            free(transmit_apdu_param);
            return -ENOMEM;
        }
        ar->msg_id = event_id;
        ar->arg1 = slot_id;
        ar->data = transmit_apdu_param;

        user_data = malloc(sizeof(tapi_async_handler));
        if (user_data == NULL) {
            free(transmit_apdu_param);
            free(ar);
            return -ENOMEM;
        }
        user_data->cb_function = p_handle;
        user_data->result = ar;

        if (!g_dbus_proxy_method_call(proxy, "TransmitApduBasicChannel",
                transmit_apdu_basic_channel_param_append, transmit_apdu_cb, user_data, sim_event_data_free)) {
            sim_event_data_free(user_data);
            return -EINVAL;
        }

        return OK;
    }

    return -EINVAL;
}

int tapi_sim_get_uicc_enablement(tapi_context context, int slot_id, tapi_sim_uicc_app_state* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    bool has_icc_card;
    int result;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    has_icc_card = false;
    tapi_sim_has_icc_card(context, slot_id, &has_icc_card);

    if (has_icc_card) {
        proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
        if (proxy == NULL) {
            tapi_log_error("no available proxy ...\n");
            return -EIO;
        }

        if (!g_dbus_proxy_get_property(proxy, "UiccActive", &iter))
            return ERROR;

        dbus_message_iter_get_basic(&iter, &result);
        *out = result;
        return OK;
    }

    return -EINVAL;
}

int tapi_sim_set_uicc_enablement(tapi_context context,
    int slot_id, int event_id, int state, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    int value = state;
    bool has_icc_card;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    has_icc_card = false;
    tapi_sim_has_icc_card(context, slot_id, &has_icc_card);

    if (has_icc_card) {
        proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
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

        if (!g_dbus_proxy_set_property_basic(proxy, "UiccActive", DBUS_TYPE_INT32,
                &value, property_set_done, handler, user_data_free)) {
            user_data_free(handler);
            return -EINVAL;
        }

        return OK;
    }

    return -EINVAL;
}
