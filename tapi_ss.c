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
#include "tapi_ss.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void cb_change_passwd_append(DBusMessageIter* iter, void* user_data)
{
    cb_change_passwd_param* change_passwd_param;
    tapi_async_handler* param;
    char* old_passwd;
    char* new_passwd;

    param = user_data;
    if (param == NULL || param->result == NULL)
        return;

    change_passwd_param = param->result->data;
    old_passwd = change_passwd_param->old_passwd;
    new_passwd = change_passwd_param->new_passwd;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &old_passwd);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &new_passwd);
}

static void disable_all_cb_param_append(DBusMessageIter* iter, void* user_data)
{
    char* disable_all_cb_param;
    tapi_async_handler* param;

    param = user_data;
    if (param == NULL || param->result == NULL)
        return;

    disable_all_cb_param = param->result->data;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &disable_all_cb_param);
}

static void disable_all_incoming_param_append(DBusMessageIter* iter, void* user_data)
{
    char* disable_all_incoming_param;
    tapi_async_handler* param;

    param = user_data;
    if (param == NULL || param->result == NULL)
        return;

    disable_all_incoming_param = param->result->data;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &disable_all_incoming_param);
}

static void disable_all_outgoing_param_append(DBusMessageIter* iter, void* user_data)
{
    char* disable_all_outgoing_param;
    tapi_async_handler* param;

    param = user_data;
    if (param == NULL || param->result == NULL)
        return;

    disable_all_outgoing_param = param->result->data;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &disable_all_outgoing_param);
}

static void disable_cf_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param;
    char* disable_cf_param;

    param = user_data;
    if (param == NULL || param->result == NULL)
        return;

    disable_cf_param = param->result->data;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &disable_cf_param);
}

static void fill_call_forwarding(const char* prop,
    DBusMessageIter* iter, tapi_call_forwarding_info* cf)
{
    if (strcmp(prop, "VoiceUnconditional") == 0) {
        const char* value;
        dbus_message_iter_get_basic(iter, &value);
        snprintf(cf->phone_number, sizeof(cf->phone_number), "%s", value);
    } else if (strcmp(prop, "VoiceBusy") == 0) {
        const char* value;
        dbus_message_iter_get_basic(iter, &value);
        snprintf(cf->voice_busy, sizeof(cf->voice_busy), "%s", value);
    } else if (strcmp(prop, "VoiceNoReply") == 0) {
        const char* value;
        dbus_message_iter_get_basic(iter, &value);
        snprintf(cf->voice_no_reply, sizeof(cf->voice_no_reply), "%s", value);
    } else if (strcmp(prop, "VoiceNoReplyTimeout") == 0) {
        dbus_message_iter_get_basic(iter, &cf->voice_no_reply_timeout);
    } else if (strcmp(prop, "VoiceNotReachable") == 0) {
        const char* value;
        dbus_message_iter_get_basic(iter, &value);
        snprintf(cf->voice_not_reachable, sizeof(cf->voice_not_reachable), "%s", value);
    } else if (strcmp(prop, "ForwardingFlagOnSim") == 0) {
        dbus_message_iter_get_basic(iter, &cf->forwarding_flag_on_sim);
    }
}

static void ss_set_property_complete(const DBusError* error, void* user_data)
{
    tapi_async_handler* handler;
    tapi_async_result* ar;
    tapi_async_function cb;

    handler = user_data;
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

static void user_data_free(void* user_data)
{
    tapi_async_handler* handler;
    tapi_async_result* ar;

    handler = user_data;
    if (handler) {
        ar = handler->result;
        if (ar)
            free(ar);
        free(handler);
    }
}

static int call_barring_property_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler;
    DBusMessageIter iter, list;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusError err;

    handler = user_data;
    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (ar->msg_id != MSG_CALL_BARRING_PROPERTY_CHANGE_IND) {
        return false;
    }

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("%s: %s\n", err.name, err.message);
        dbus_error_free(&err);
        goto done;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        goto done;
    }

    dbus_message_iter_recurse(&iter, &list);
    while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry, value;
        const char* property;

        dbus_message_iter_recurse(&list, &entry);
        dbus_message_iter_get_basic(&entry, &property);

        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &value);

        if (strcmp(property, "VoiceIncoming") == 0) {
            char* voiceincoming;
            dbus_message_iter_get_basic(&value, &voiceincoming);
            ar->data = &voiceincoming;
            ar->status = OK;
        } else if (strcmp(property, "VoiceOutgoing") == 0) {
            char* voiceoutgoing;
            dbus_message_iter_get_basic(&value, &voiceoutgoing);
            ar->data = &voiceoutgoing;
            ar->status = OK;
        }

        dbus_message_iter_next(&list);
    }

done:
    cb(ar);
    return true;
}

static int call_forwarding_property_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_call_forwarding_info* info = NULL;
    tapi_async_handler* handler;
    DBusMessageIter iter, list;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusError err;

    handler = user_data;
    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (ar->msg_id != MSG_CALL_FORWARDING_PROPERTY_CHANGE_IND) {
        return false;
    }

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("%s: %s\n", err.name, err.message);
        dbus_error_free(&err);
        goto done;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        goto done;
    }

    dbus_message_iter_recurse(&iter, &list);

    info = malloc(sizeof(tapi_call_forwarding_info));
    if (info == NULL)
        goto done;

    while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry, value;
        const char* key;

        dbus_message_iter_recurse(&list, &entry);
        dbus_message_iter_get_basic(&entry, &key);

        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &value);

        fill_call_forwarding(key, &value, info);

        dbus_message_iter_next(&list);
    }

    ar->data = info;
    ar->status = OK;

done:
    cb(ar);
    if (info != NULL)
        free(info);

    return OK;
}

static int ussd_state_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessageIter iter;
    DBusError err;

    handler = user_data;
    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (ar->msg_id != MSG_USSD_PROPERTY_CHANGE_IND) {
        return false;
    }

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("%s: %s\n", err.name, err.message);
        dbus_error_free(&err);
        goto done;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        goto done;
    }

    while (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry, value;
        const char* property;

        dbus_message_iter_recurse(&iter, &entry);
        dbus_message_iter_get_basic(&entry, &property);

        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &value);

        if (strcmp(property, "State") == 0) {
            dbus_message_iter_get_basic(&value, &ar->arg2);
            ar->status = OK;
            goto done;
        }

        dbus_message_iter_next(&iter);
    }

done:
    cb(ar);
    return true;
}

static bool is_ussd_signal_message(DBusMessage* message,
    DBusMessageIter* iter, int msg_type)
{
    if (!dbus_message_iter_init(message, iter)) {
        tapi_log_error("ussd_signal message has no param");
        return false;
    }

    if (dbus_message_iter_get_arg_type(iter) != msg_type) {
        tapi_log_error("ussd_signal param is not right");
        return false;
    }

    return true;
}

static int ussd_notification_received(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessageIter iter;

    handler = user_data;
    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (is_ussd_signal_message(message, &iter, DBUS_TYPE_STRING)) {
        char* notification_message;
        dbus_message_iter_get_basic(&iter, &notification_message);
        ar->data = notification_message;
        ar->status = OK;
    }

    cb(ar);
    return true;
}

static int ussd_request_received(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessageIter iter;

    handler = user_data;
    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (is_ussd_signal_message(message, &iter, DBUS_TYPE_STRING)) {
        char* request_message;
        dbus_message_iter_get_basic(&iter, &request_message);
        ar->data = request_message;
        ar->status = OK;
    }

    cb(ar);
    return true;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

// Call Barring
int tapi_ss_request_call_barring(tapi_context context, int slot_id, int event_id,
    char* facility, char* pin2, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;
    char* value[2];
    int temp;
    int len;
    char* key;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_BARRING];
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

    ar->arg1 = slot_id;
    ar->msg_id = event_id;
    handler->cb_function = p_handle;

    tapi_call_barring_lock tapi_call_barring_info[] = {
        { "AllOutgoing", "all", "AO" },
        { "InternationalOutgoing", "international", "OI" },
        { "InternationalOutgoingExceptHome", "internationalnothome", "OX" },
        { "AllIncoming", "always", "AI" },
        { "IncomingWhenRoaming", "whenroaming", "IR" },
    };

    len = sizeof(tapi_call_barring_info) / sizeof(tapi_call_barring_lock);
    for (temp = 0; temp < len; temp++) {
        if (strcmp(tapi_call_barring_info[temp].fac, facility) == 0) {
            break;
        }
    }

    if (strcmp(facility, "AI") == 0 || strcmp(facility, "IR") == 0) {
        key = "VoiceIncoming";
    } else {
        key = "VoiceOutgoing";
    }

    value[0] = tapi_call_barring_info[temp].value;
    value[1] = pin2;

    return g_dbus_proxy_set_property_array(proxy, key, DBUS_TYPE_STRING,
        value, sizeof(value) / sizeof(char*), ss_set_property_complete, handler, user_data_free);
}

int tapi_ss_query_call_barring_info(tapi_context context, int slot_id, const char* service_type, char** out)
{
    dbus_context* ctx = context;
    DBusMessageIter iter;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_BARRING];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, service_type, &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    return ERROR;
}

int tapi_ss_change_call_barring_password(tapi_context context, int slot_id, int event_id,
    char* old_passwd, char* new_passwd, tapi_async_function p_handle)
{
    cb_change_passwd_param* param;
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_BARRING];
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

    param = malloc(sizeof(cb_change_passwd_param));
    if (param == NULL) {
        free(handler);
        free(ar);
        return -ENOMEM;
    }

    param->old_passwd = old_passwd;
    param->new_passwd = new_passwd;

    ar->arg1 = slot_id;
    handler->cb_function = p_handle;
    ar->msg_id = event_id;
    ar->data = param;

    return g_dbus_proxy_method_call(proxy, "ChangePassword", cb_change_passwd_append,
        NULL, handler, user_data_free);
}

int tapi_ss_disable_all_call_barrings(tapi_context context, int slot_id, int event_id,
    char* passwd, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_BARRING];
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
    ar->arg1 = slot_id;
    ar->msg_id = event_id;
    ar->data = passwd;
    handler->cb_function = p_handle;

    return g_dbus_proxy_method_call(proxy, "DisableAll", disable_all_cb_param_append,
        NULL, handler, user_data_free);
}

int tapi_ss_disable_all_incoming(tapi_context context, int slot_id,
    int event_id, char* passwd, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_BARRING];
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
    ar->arg1 = slot_id;
    ar->msg_id = event_id;
    ar->data = passwd;
    handler->cb_function = p_handle;

    return g_dbus_proxy_method_call(proxy, "DisableAllIncoming",
        disable_all_incoming_param_append, NULL, handler, user_data_free);
}

int tapi_ss_disable_all_outgoing(tapi_context context, int slot_id,
    int event_id, char* passwd, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_BARRING];
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
    ar->arg1 = slot_id;
    ar->data = passwd;
    ar->msg_id = event_id;
    handler->cb_function = p_handle;

    return g_dbus_proxy_method_call(proxy, "DisableAllOutgoing",
        disable_all_outgoing_param_append, NULL, handler, user_data_free);
}

// Call Forwarding
int tapi_ss_request_call_forwarding(tapi_context context, int slot_id, int event_id,
    const char* cf_type, char* value, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;
    int value_type;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_FORWARDING];
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

    ar->arg1 = slot_id;
    ar->msg_id = event_id;
    handler->cb_function = p_handle;

    if (strcmp(cf_type, "VoiceNoReplyTimeout") == 0)
        value_type = DBUS_TYPE_UINT32;
    else if (strcmp(cf_type, "ForwardingFlagOnSim") == 0)
        value_type = DBUS_TYPE_BOOLEAN;
    else
        value_type = DBUS_TYPE_STRING;

    return g_dbus_proxy_set_property_basic(proxy, cf_type, value_type,
        &value, ss_set_property_complete, handler, user_data_free);
}

int tapi_ss_query_call_forwarding_info(tapi_context context, int slot_id,
    const char* service_type, char** out)
{
    dbus_context* ctx = context;
    DBusMessageIter iter;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_FORWARDING];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, service_type, &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    return ERROR;
}

int tapi_ss_disable_call_forwarding(tapi_context context, int slot_id, int event_id,
    char* cf_type, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_FORWARDING];
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
    ar->arg1 = slot_id;
    ar->msg_id = event_id;
    ar->data = cf_type;
    handler->cb_function = p_handle;

    return g_dbus_proxy_method_call(proxy, "DisableAll",
        disable_cf_param_append, NULL, handler, user_data_free);
}

// USSD
int tapi_get_ussd_state(tapi_context context, int slot_id, tapi_ussd_state* out)
{
    dbus_context* ctx = context;
    DBusMessageIter iter;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "State", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    return ERROR;
}

int tapi_ss_cancel_ussd(tapi_context context, int slot_id, int event_id,
    tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SS];
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

    ar->arg1 = slot_id;
    ar->msg_id = event_id;
    handler->cb_function = p_handle;

    return g_dbus_proxy_method_call(proxy, "Cancel", NULL,
        NULL, handler, user_data_free);
}

// Call Waiting
int tapi_ss_request_call_wating(tapi_context context, int slot_id, int event_id, char* state,
    tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_SETTING];
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

    ar->arg1 = slot_id;
    ar->msg_id = event_id;
    handler->cb_function = p_handle;

    return g_dbus_proxy_set_property_basic(proxy, "VoiceCallWaiting", DBUS_TYPE_STRING,
        &state, ss_set_property_complete, handler, user_data_free);
}

int tapi_ss_query_call_wating(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    DBusMessageIter iter;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_SETTING];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "VoiceCallWaiting", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    return ERROR;
}

// Calling Line Presentation
int tapi_ss_query_calling_line_presentation_info(tapi_context context, int slot_id,
    tapi_clip_status* out)
{
    dbus_context* ctx = context;
    DBusMessageIter iter;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_SETTING];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "CallingLinePresentation", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    return ERROR;
}

// Calling Line Restriction
int tapi_ss_query_calling_line_restriction_info(tapi_context context, int slot_id,
    tapi_clir_status* out)
{
    dbus_context* ctx = context;
    DBusMessageIter iter;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_FORWARDING];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "CallingLineRestriction", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    return ERROR;
}

int tapi_ss_register(tapi_context context,
    int slot_id, tapi_indication_msg msg, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    char* modem_path;
    int watch_id;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || msg < MSG_CALL_BARRING_PROPERTY_CHANGE_IND || msg > MSG_USSD_PROPERTY_CHANGE_IND) {
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
    case MSG_CALL_BARRING_PROPERTY_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_CALL_BARRING_INTERFACE,
            "PropertyChanged", call_barring_property_changed, handler, user_data_free);
        break;
    case MSG_CALL_FORWARDING_PROPERTY_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_CALL_FORWARDING_INTERFACE,
            "PropertyChanged", call_forwarding_property_changed, handler, user_data_free);
        break;
    case MSG_USSD_PROPERTY_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_SUPPLEMENTARY_SERVICES_INTERFACE,
            "PropertyChanged", ussd_state_changed, handler, user_data_free);
        break;
    case MSG_USSD_NOTIFICATION_RECEIVED_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_SUPPLEMENTARY_SERVICES_INTERFACE,
            "NotificationReceived", ussd_notification_received, handler, user_data_free);
        break;
    case MSG_USSD_REQUEST_RECEIVED_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_SUPPLEMENTARY_SERVICES_INTERFACE,
            "RequestReceived", ussd_request_received, handler, user_data_free);
        break;
    default:
        break;
    }

    return watch_id;
}

int tapi_ss_unregister(tapi_context context, int watch_id)
{
    dbus_context* ctx = context;
    if (ctx == NULL || watch_id <= 0)
        return -EINVAL;

    return g_dbus_remove_watch(ctx->connection, watch_id);
}
