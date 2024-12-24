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
#include "tapi_ss.h"

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

typedef struct {
    char* old_passwd;
    char* new_passwd;
} cb_change_passwd_param;

typedef struct {
    char* name;
    char* value;
    char* fac;
} call_barring_lock;

typedef struct {
    char* key;
    char* value;
    char* pin2;
} cb_request_param;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void set_clir_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    tapi_clir_status status;
    const char* clir_status;

    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        return;
    }

    if (param->result == NULL) {
        tapi_log_error("invalid clir value in %s !!", __func__);
        return;
    }

    status = param->result->arg2;
    clir_status = tapi_utils_clir_status_to_string(status);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &clir_status);
}

static void set_call_waiting_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    int enable;

    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        return;
    }

    if (param->result == NULL) {
        tapi_log_error("invalid call waiting value in %s!!", __func__);
        return;
    }

    enable = param->result->arg2;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &enable);
}

static void query_call_forwarding_option_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    int option;
    int cls;

    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        return;
    }

    if (param->result == NULL) {
        tapi_log_error("invalid call forwarding option in %s!!", __func__);
        return;
    }

    option = param->result->arg1;
    cls = param->result->arg2;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &option);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &cls);
}

static void set_call_forwarding_option_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    int option;
    int cls;
    char* number;

    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        return;
    }

    if (param->result == NULL) {
        tapi_log_error("invalid call forwarding option in %s!!", __func__);
        return;
    }

    option = param->result->arg1;
    cls = param->result->arg2;
    number = param->result->data;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &option);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &cls);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &number);
}

static void cb_request_param_append(DBusMessageIter* iter, void* user_data)
{
    cb_request_param* set_property_param;
    tapi_async_handler* param;
    DBusMessageIter value;

    param = user_data;
    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        return;
    }

    if (param->result == NULL) {
        tapi_log_error("param result in %s is null", __func__);
        return;
    }

    set_property_param = param->result->data;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &set_property_param->key);

    dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &value);
    dbus_message_iter_append_basic(&value, DBUS_TYPE_STRING, &set_property_param->value);
    dbus_message_iter_close_container(iter, &value);

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &set_property_param->pin2);
}

static void cb_change_passwd_append(DBusMessageIter* iter, void* user_data)
{
    cb_change_passwd_param* change_passwd_param;
    tapi_async_handler* param;
    char* old_passwd;
    char* new_passwd;

    param = user_data;
    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        return;
    }

    if (param->result == NULL) {
        tapi_log_error("param result in %s is null", __func__);
        return;
    }

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
    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        return;
    }

    if (param->result == NULL) {
        tapi_log_error("param result in %s is null", __func__);
        return;
    }

    disable_all_cb_param = param->result->data;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &disable_all_cb_param);
}

static void disable_all_incoming_param_append(DBusMessageIter* iter, void* user_data)
{
    char* disable_all_incoming_param;
    tapi_async_handler* param;

    param = user_data;
    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        return;
    }

    if (param->result == NULL) {
        tapi_log_error("param result in %s is null", __func__);
        return;
    }

    disable_all_incoming_param = param->result->data;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &disable_all_incoming_param);
}

static void disable_all_outgoing_param_append(DBusMessageIter* iter, void* user_data)
{
    char* disable_all_outgoing_param;
    tapi_async_handler* param;

    param = user_data;
    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        return;
    }

    if (param->result == NULL) {
        tapi_log_error("param result in %s is null", __func__);
        return;
    }

    disable_all_outgoing_param = param->result->data;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &disable_all_outgoing_param);
}

static void ss_initiate_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param;
    char* ss_initiate_param;

    param = user_data;
    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        return;
    }

    if (param->result == NULL) {
        tapi_log_error("param result in %s is null", __func__);
        return;
    }

    ss_initiate_param = param->result->data;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &ss_initiate_param);
}

static void send_ussd_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param;
    char* send_ussd_param;

    param = user_data;
    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        return;
    }

    if (param->result == NULL) {
        tapi_log_error("param result in %s is null", __func__);
        return;
    }

    send_ussd_param = param->result->data;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &send_ussd_param);
}

static void enable_fdn_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param;
    char* passwd;

    param = user_data;
    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        return;
    }

    if (param->result == NULL) {
        tapi_log_error("param result in %s is null", __func__);
        return;
    }

    passwd = param->result->data;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &passwd);
}

static void method_call_complete(DBusMessage* message, void* user_data)
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
        tapi_log_error("error from message in %s, %s: %s", __func__, err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
    }

    ar->status = OK;

done:
    cb(ar);
}

static void fill_cf_condition_info(const char* prop, DBusMessageIter* iter,
    tapi_call_forward_info* cf)
{
    const char* value_str;

    if (strcmp(prop, "Status") == 0) {
        dbus_message_iter_get_basic(iter, &cf->status);
    } else if (strcmp(prop, "Cls") == 0) {
        dbus_message_iter_get_basic(iter, &cf->cls);
    } else if (strcmp(prop, "Number") == 0) {
        dbus_message_iter_get_basic(iter, &value_str);

        if (strlen(value_str) <= MAX_PHONE_NUMBER_LENGTH)
            strcpy(cf->phone_number.number, value_str);
    } else if (strcmp(prop, "Time") == 0) {
        dbus_message_iter_get_basic(iter, &cf->time);
    }
}

static void call_forwarding_query_complete(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter args, list;
    DBusError err;
    tapi_call_forward_info* cf_condition;

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

    cf_condition = malloc(sizeof(tapi_call_forward_info));
    if (cf_condition == NULL) {
        tapi_log_error("cf_condition in %s is null", __func__);
        return;
    }

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("error from mesage in %s, %s: %s", __func__, err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_init(message, &args) == false) {
        tapi_log_error("dbus message iter init fail in %s", __func__);
        ar->status = ERROR;
        goto done;
    }
    dbus_message_iter_recurse(&args, &list);

    while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry, value;
        const char* name;

        dbus_message_iter_recurse(&list, &entry);
        dbus_message_iter_get_basic(&entry, &name);
        dbus_message_iter_next(&entry);

        dbus_message_iter_recurse(&entry, &value);
        fill_cf_condition_info(name, &value, cf_condition);

        dbus_message_iter_next(&list);
    }

    ar->status = OK;
    ar->data = cf_condition;

done:
    cb(ar);
    free(cf_condition);
}

static void ss_initiate_complete(DBusMessage* message, void* user_data)
{
    tapi_ss_initiate_info* info = NULL;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    tapi_async_function cb;
    const char* name;
    DBusMessageIter iter, var;
    DBusError err;

    handler = user_data;
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
        tapi_log_error("error from message in %s, %s: %s", __func__, err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        tapi_log_error("message iter init failed in %s", __func__);
        ar->status = ERROR;
        goto done;
    }

    info = malloc(sizeof(tapi_ss_initiate_info));
    if (info == NULL) {
        tapi_log_error("info in %s is null", __func__);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
        tapi_log_error("message iter type is not string in %s", __func__);
        ar->status = ERROR;
        goto done;
    }

    dbus_message_iter_get_basic(&iter, &name);
    dbus_message_iter_next(&iter);

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT) {
        tapi_log_error("message iter type is not variant in %s", __func__);
        ar->status = ERROR;
        goto done;
    }

    dbus_message_iter_recurse(&iter, &var);
    if (strcmp(name, "USSD")) {
        tapi_log_error("reponse is not ussd str in %s", __func__);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_get_arg_type(&var) != DBUS_TYPE_STRING) {
        tapi_log_error("get ussd str failed in %s", __func__);
        ar->status = ERROR;
        goto done;
    }

    dbus_message_iter_get_basic(&var, &info->ussd_response);
    ar->data = info;
    ar->status = OK;

done:
    cb(ar);
    if (info != NULL)
        free(info);
}

static void ss_send_ussd_cb(DBusMessage* message, void* user_data)
{
    DBusMessageIter iter;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusError err;
    char* response;

    handler = user_data;
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
        tapi_log_error("error from message in %s, %s: %s", __func__, err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        tapi_log_error("message iter init failed in %s", __func__);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
        tapi_log_error("message iter type is not string in %s", __func__);
        ar->status = ERROR;
        goto done;
    }

    dbus_message_iter_get_basic(&iter, &response);

    ar->data = response;
    ar->status = OK;

done:
    cb(ar);
}

static void query_fdn_cb(DBusMessage* message, void* user_data)
{
    DBusMessageIter iter;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusError err;

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
        tapi_log_error("error from message in %s, %s: %s", __func__, err.name, err.message);
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

static void query_call_waiting_cb(DBusMessage* message, void* user_data)
{
    DBusMessageIter iter;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusError err;

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
        tapi_log_error("error from message in %s, %s: %s", __func__, err.name, err.message);
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

static void query_clir_cb(DBusMessage* message, void* user_data)
{
    DBusMessageIter iter;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusError err;

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
        tapi_log_error("error from message in %s, %s: %s", __func__, err.name, err.message);
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

static void ss_event_data_free(void* user_data)
{
    tapi_async_handler* handler;
    tapi_async_result* ar;

    handler = user_data;
    if (handler) {
        ar = handler->result;
        if (ar) {
            if (ar->data)
                free(ar->data);
            free(ar);
        }

        free(handler);
    }
}

static int call_barring_property_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    DBusMessageIter iter, list, entry, value;
    tapi_call_barring_info* cb_value = NULL;
    tapi_async_handler* handler;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusError err;

    handler = user_data;
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

    if (ar->msg_id != MSG_CALL_BARRING_PROPERTY_CHANGE_IND) {
        tapi_log_error("msg id in %s is invalid", __func__);
        return false;
    }

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("error from message in %s, %s: %s", __func__, err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        tapi_log_error("message iter init failed in %s", __func__);
        ar->status = ERROR;
        goto done;
    }

    dbus_message_iter_recurse(&iter, &list);

    cb_value = malloc(sizeof(tapi_call_barring_info));
    if (cb_value == NULL) {
        tapi_log_error("cb_value in %s is null", __func__);
        ar->status = ERROR;
        goto done;
    }

    while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_DICT_ENTRY) {
        dbus_message_iter_recurse(&list, &entry);
        dbus_message_iter_get_basic(&entry, &cb_value->service_type);

        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &value);

        if (strcmp(cb_value->service_type, "VoiceIncoming") == 0) {
            dbus_message_iter_get_basic(&value, &cb_value->value);
            ar->data = cb_value;
            ar->status = OK;
            goto done;
        } else if (strcmp(cb_value->service_type, "VoiceOutgoing") == 0) {
            dbus_message_iter_get_basic(&value, &cb_value->value);
            ar->data = cb_value;
            ar->status = OK;
            goto done;
        }

        dbus_message_iter_next(&list);
    }

done:
    cb(ar);
    if (cb_value != NULL)
        free(cb_value);

    return true;
}

static int ussd_state_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessageIter iter, var;
    DBusError err;
    const char* property;
    char* ussd_state;

    handler = user_data;
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

    if (ar->msg_id != MSG_USSD_PROPERTY_CHANGE_IND) {
        tapi_log_error("msg id in %s is invalid", __func__);
        return false;
    }

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("error from message in %s, %s: %s", __func__, err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        tapi_log_error("message iter init failed in %s", __func__);
        ar->status = ERROR;
        goto done;
    }

    dbus_message_iter_get_basic(&iter, &property);
    dbus_message_iter_next(&iter);

    dbus_message_iter_recurse(&iter, &var);

    if (strcmp(property, "State") == 0) {
        dbus_message_iter_get_basic(&var, &ussd_state);
        ar->data = ussd_state;
        ar->status = OK;
        goto done;
    }

done:
    cb(ar);
    return true;
}

static bool is_ussd_signal_message(DBusMessage* message,
    DBusMessageIter* iter, int msg_type)
{
    if (!dbus_message_iter_init(message, iter)) {
        tapi_log_error("message iter init failed in %s", __func__);
        return false;
    }

    if (dbus_message_iter_get_arg_type(iter) != msg_type) {
        tapi_log_error("arg type in %s is invalid", __func__);
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

    if (is_ussd_signal_message(message, &iter, DBUS_TYPE_STRING)) {
        char* notification_message;
        dbus_message_iter_get_basic(&iter, &notification_message);
        ar->data = notification_message;
        ar->status = OK;
    } else {
        ar->status = ERROR;
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

    if (is_ussd_signal_message(message, &iter, DBUS_TYPE_STRING)) {
        char* request_message;
        dbus_message_iter_get_basic(&iter, &request_message);
        ar->data = request_message;
        ar->status = OK;
    } else {
        ar->status = ERROR;
    }

    cb(ar);
    return true;
}

static void report_data_logging_for_ss(dbus_context* ctx, char* type, char* fail_reason)
{
#if defined(CONFIG_OFONO_DATA_LOG_OVER_MIWEAR) \
    && (!defined(CONFIG_DFX_EVENT) || (!defined(CONFIG_DFX)))
    tapi_async_result* temp_ar;
    char out_data[MIWEAR_LOG_IND_BUF_SIZE] = { 0 };

    if (ctx->logging_over_miwear_cb != NULL) {
        temp_ar = malloc(sizeof(tapi_async_result));
        if (temp_ar == NULL) {
            tapi_log_error("Memory allocation failed");
        } else {
            snprintf(out_data, MIWEAR_LOG_IND_BUF_SIZE, "%s,%s,%s", "SS_INFO",
                type, fail_reason);
            temp_ar->status = OK;
            temp_ar->data = out_data;
            ctx->logging_over_miwear_cb(temp_ar);
            free(temp_ar);
        }
    }
#else
    OFONO_DFX_SS_INFO(type, fail_reason);
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int tapi_ss_initiate_service(tapi_context context, int slot_id, int event_id,
    char* command, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    report_data_logging_for_ss(ctx, "ss:ussd:request", "NA");

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    if (command == NULL) {
        tapi_log_error("command in %s is null", __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SS];
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

    ar->arg1 = slot_id;
    ar->data = command;
    ar->msg_id = event_id;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "Initiate", ss_initiate_param_append,
            ss_initiate_complete, handler, handler_free)) {
        report_data_logging_for_ss(ctx, "ss:ussd:request", "dbus method fail");
        tapi_log_error("method call fail in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

// Call Barring
int tapi_ss_request_call_barring(tapi_context context, int slot_id, int event_id,
    tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    report_data_logging_for_ss(ctx, "ss:request callbarring", "NA");

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_BARRING];
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
    ar->arg1 = slot_id;
    ar->msg_id = event_id;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "GetProperties",
            NULL, method_call_complete, handler, handler_free)) {
        report_data_logging_for_ss(ctx, "ss:request callbarring", "dbus method fail");
        tapi_log_error("method call fail in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_ss_set_call_barring_option(tapi_context context, int slot_id, int event_id,
    char* facility, char* pin2, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    cb_request_param* param;
    tapi_async_result* ar;
    GDBusProxy* proxy;
    int temp;
    int len;
    char* key;

    report_data_logging_for_ss(ctx, "ss:set callbarring", "NA");

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    if (facility == NULL) {
        tapi_log_error("facility in %s is null", __func__);
        return -EINVAL;
    }

    if (pin2 == NULL) {
        tapi_log_error("pin2 in %s is null", __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_BARRING];
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

    param = malloc(sizeof(cb_request_param));
    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        free(handler);
        free(ar);
        return -ENOMEM;
    }

    ar->arg1 = slot_id;
    ar->msg_id = event_id;
    handler->cb_function = p_handle;

    call_barring_lock call_barring_info[] = {
        { "AllOutgoing", "all", "AO" },
        { "InternationalOutgoing", "international", "OI" },
        { "InternationalOutgoingExceptHome", "internationalnothome", "OX" },
        { "AllIncoming", "always", "AI" },
        { "IncomingWhenRoaming", "whenroaming", "IR" },
    };

    len = sizeof(call_barring_info) / sizeof(call_barring_lock);
    for (temp = 0; temp < len; temp++) {
        if (strcmp(call_barring_info[temp].fac, facility) == 0) {
            break;
        }
    }

    if (strcmp(facility, "AI") == 0 || strcmp(facility, "IR") == 0) {
        key = "VoiceIncoming";
    } else {
        key = "VoiceOutgoing";
    }

    param->key = key;
    param->value = call_barring_info[temp].value;
    param->pin2 = pin2;
    ar->data = param;

    if (!g_dbus_proxy_method_call(proxy, "SetProperty", cb_request_param_append,
            method_call_complete, handler, ss_event_data_free)) {
        report_data_logging_for_ss(ctx, "ss:set callbarring", "dbus method fail");
        tapi_log_error("method call fail in %s", __func__);
        ss_event_data_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_ss_get_call_barring_option(tapi_context context, int slot_id, const char* service_type, char** out)
{
    dbus_context* ctx = context;
    DBusMessageIter iter;
    GDBusProxy* proxy;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    if (service_type == NULL) {
        tapi_log_error("service_type in %s is null", __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_BARRING];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, service_type, &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    tapi_log_error("get property failed in %s", __func__);
    return -EINVAL;
}

int tapi_ss_change_call_barring_password(tapi_context context, int slot_id, int event_id,
    char* old_passwd, char* new_passwd, tapi_async_function p_handle)
{
    cb_change_passwd_param* param;
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    report_data_logging_for_ss(ctx, "ss:set callbarring:change password", "NA");

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    if (old_passwd == NULL) {
        tapi_log_error("old_passwd in %s is null", __func__);
        return -EINVAL;
    }

    if (new_passwd == NULL) {
        tapi_log_error("new_passwd in %s is null", __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_BARRING];
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

    param = malloc(sizeof(cb_change_passwd_param));
    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
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

    if (!g_dbus_proxy_method_call(proxy, "ChangePassword", cb_change_passwd_append,
            method_call_complete, handler, ss_event_data_free)) {
        report_data_logging_for_ss(ctx, "ss:set callbarring:change password", "dbus method fail");
        tapi_log_error("method call fail in %s", __func__);
        ss_event_data_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_ss_disable_all_call_barrings(tapi_context context, int slot_id, int event_id,
    char* passwd, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    report_data_logging_for_ss(ctx, "ss:set callbarring:disable all", "NA");

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    if (passwd == NULL) {
        tapi_log_error("passwd in %s is null", __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_BARRING];
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
    ar->arg1 = slot_id;
    ar->msg_id = event_id;
    ar->data = passwd;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "DisableAll", disable_all_cb_param_append,
            method_call_complete, handler, handler_free)) {
        report_data_logging_for_ss(ctx, "ss:set callbarring:disable all", "dbus method fail");
        tapi_log_error("method call fail in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_ss_disable_all_incoming(tapi_context context, int slot_id,
    int event_id, char* passwd, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    report_data_logging_for_ss(ctx, "ss:set callbarring:disable all incoming", "NA");

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    if (passwd == NULL) {
        tapi_log_error("passwd in %s is null", __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_BARRING];
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
    ar->arg1 = slot_id;
    ar->msg_id = event_id;
    ar->data = passwd;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "DisableAllIncoming",
            disable_all_incoming_param_append, method_call_complete, handler, handler_free)) {
        report_data_logging_for_ss(ctx, "ss:set callbarring:disable all incoming", "dbus method fail");
        tapi_log_error("method call fail in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_ss_disable_all_outgoing(tapi_context context, int slot_id,
    int event_id, char* passwd, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    report_data_logging_for_ss(ctx, "ss:set callbarring:disable all outgoing", "NA");

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    if (passwd == NULL) {
        tapi_log_error("passwd in %s is null", __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_BARRING];
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
    ar->arg1 = slot_id;
    ar->data = passwd;
    ar->msg_id = event_id;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "DisableAllOutgoing",
            disable_all_outgoing_param_append, method_call_complete, handler, handler_free)) {
        report_data_logging_for_ss(ctx, "ss:set callbarring:disable all outgoing", "dbus method fail");
        tapi_log_error("method call fail in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

// Call Forwarding
int tapi_ss_query_call_forwarding_option(tapi_context context, int slot_id, int event_id,
    tapi_call_forward_option option, tapi_call_forward_class cls, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    report_data_logging_for_ss(ctx, "ss:query call forwarding", "NA");

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    if (option < CF_REASON_UNCONDITIONAL || option > CF_REASON_NOT_REACHABLE) {
        tapi_log_error("option %d in %s is invalid", option, __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_FORWARDING];
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
    ar->arg1 = option;
    ar->arg2 = cls;
    ar->msg_id = event_id;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "GetCallForwarding",
            query_call_forwarding_option_append, call_forwarding_query_complete,
            handler, handler_free)) {
        report_data_logging_for_ss(ctx, "ss:query call forwarding", "dbus method fail");
        tapi_log_error("method call fail in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_ss_set_call_forwarding_option(tapi_context context, int slot_id, int event_id,
    tapi_call_forward_option option, tapi_call_forward_class cls, char* number,
    tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    report_data_logging_for_ss(ctx, "ss:set call forwarding", "NA");

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    if (number == NULL) {
        tapi_log_error("number in %s is null", __func__);
        return -EINVAL;
    }

    if (option < CF_REASON_UNCONDITIONAL || option > CF_REASON_NOT_REACHABLE) {
        tapi_log_error("option %d in %s is invalid", option, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_FORWARDING];
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
    ar->arg1 = option;
    ar->arg2 = cls;
    ar->msg_id = event_id;
    ar->data = number;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "SetCallForwarding",
            set_call_forwarding_option_append, method_call_complete, handler, handler_free)) {
        tapi_log_error("method call fail in %s", __func__);
        report_data_logging_for_ss(ctx, "ss:set call forwarding", "dbus method fail");
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

// USSD
int tapi_get_ussd_state(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    DBusMessageIter iter;
    GDBusProxy* proxy;

    report_data_logging_for_ss(ctx, "ss:ussd:get state", "NA");

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "State", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    tapi_log_error("get property fail in %s", __func__);
    report_data_logging_for_ss(ctx, "ss:ussd:get state", "dbus method fail");
    return -EINVAL;
}

int tapi_ss_send_ussd(tapi_context context, int slot_id, int event_id, char* reply,
    tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    report_data_logging_for_ss(ctx, "ss:ussd:response", "NA");

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    if (reply == NULL) {
        tapi_log_error("reply in %s is null", __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SS];
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
    ar->arg1 = slot_id;
    ar->msg_id = event_id;
    ar->data = reply;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "Respond", send_ussd_param_append,
            ss_send_ussd_cb, handler, handler_free)) {
        report_data_logging_for_ss(ctx, "ss:ussd:response", "dbus method fail");
        tapi_log_error("method call fail in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_ss_cancel_ussd(tapi_context context, int slot_id, int event_id,
    tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    report_data_logging_for_ss(ctx, "ss:ussd:cancel", "NA");

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SS];
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
    ar->arg1 = slot_id;
    ar->msg_id = event_id;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "Cancel", NULL,
            method_call_complete, handler, handler_free)) {
        report_data_logging_for_ss(ctx, "ss:ussd:cancel", "dbus method fail");
        tapi_log_error("method call fail in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

// Call Waiting
int tapi_ss_set_call_waiting(tapi_context context, int slot_id, int event_id, bool enable,
    tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    report_data_logging_for_ss(ctx, "ss:set call waiting", "NA");

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_SETTING];
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
    ar->arg1 = slot_id;
    ar->arg2 = enable;
    ar->msg_id = event_id;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "SetCallWaiting",
            set_call_waiting_append, method_call_complete, handler, handler_free)) {
        tapi_log_error("method call fail in %s", __func__);
        report_data_logging_for_ss(ctx, "ss:set call waiting", "dbus method fail");
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_ss_get_call_waiting(tapi_context context, int slot_id, int event_id,
    tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    report_data_logging_for_ss(ctx, "ss:get call waiting", "NA");

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_SETTING];
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
    ar->arg1 = slot_id;
    ar->msg_id = event_id;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "GetCallWaiting", NULL,
            query_call_waiting_cb, handler, handler_free)) {
        tapi_log_error("method call fail in %s", __func__);
        report_data_logging_for_ss(ctx, "ss:get call waiting", "dbus method fail");
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

// Calling Line Presentation
int tapi_ss_get_calling_line_presentation_info(tapi_context context, int slot_id,
    char** out)
{
    dbus_context* ctx = context;
    DBusMessageIter iter;
    GDBusProxy* proxy;

    report_data_logging_for_ss(ctx, "ss:get clip", "NA");

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_SETTING];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "CallingLinePresentation", &iter)) {
        report_data_logging_for_ss(ctx, "ss:get clip", "dbus method fail");
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    tapi_log_error("get property failed in %s", __func__);
    return -EINVAL;
}

// Calling Line Restriction
int tapi_ss_set_calling_line_restriction(tapi_context context, int slot_id, int event_id,
    tapi_clir_status state, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    report_data_logging_for_ss(ctx, "ss:set clir", "NA");

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_SETTING];
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
    ar->arg1 = slot_id;
    ar->arg2 = state;
    ar->msg_id = event_id;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "SetClir",
            set_clir_append, method_call_complete, handler, handler_free)) {
        tapi_log_error("method call fail in %s", __func__);
        report_data_logging_for_ss(ctx, "ss:set clir", "dbus method fail");
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_ss_get_calling_line_restriction_info(tapi_context context, int slot_id,
    int event_id, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    report_data_logging_for_ss(ctx, "ss:get clir", "NA");

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL_SETTING];
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
    ar->arg1 = slot_id;
    ar->msg_id = event_id;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "GetClir", NULL,
            query_clir_cb, handler, handler_free)) {
        tapi_log_error("method call fail in %s", __func__);
        report_data_logging_for_ss(ctx, "ss:get clir", "dbus method fail");
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_ss_enable_fdn(tapi_context context, int slot_id, int event_id,
    bool enable, char* passwd, tapi_async_function p_handle)
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
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    if (passwd == NULL) {
        tapi_log_error("passwd in %s is null", __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
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
    ar->arg1 = slot_id;
    ar->msg_id = event_id;
    ar->data = passwd;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, enable ? "EnableFdn" : "DisableFdn",
            enable_fdn_param_append, method_call_complete, handler, handler_free)) {
        tapi_log_error("method call fail in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_ss_query_fdn(tapi_context context, int slot_id, int event_id,
    tapi_async_function p_handle)
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
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_SIM];
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
    ar->arg1 = slot_id;
    ar->msg_id = event_id;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "QueryFdn", NULL,
            query_fdn_cb, handler, handler_free)) {
        tapi_log_error("method call fail in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_ss_register(tapi_context context,
    int slot_id, tapi_indication_msg msg, void* user_obj, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    const char* modem_path;
    int watch_id = 0;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("slot id %d in %s is invalid", slot_id, __func__);
        return -EINVAL;
    }

    if (msg < MSG_CALL_BARRING_PROPERTY_CHANGE_IND || msg > MSG_USSD_PROPERTY_CHANGE_IND) {
        tapi_log_error("msg %d in %s is invalid", msg, __func__);
        return -EINVAL;
    }

    modem_path = tapi_utils_get_modem_path(slot_id);
    if (modem_path == NULL) {
        tapi_log_error("no available modem in %s", __func__);
        return -EIO;
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
    ar->msg_id = msg;
    ar->msg_type = INDICATION;
    ar->arg1 = slot_id;
    ar->user_obj = user_obj;

    switch (msg) {
    case MSG_CALL_BARRING_PROPERTY_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_CALL_BARRING_INTERFACE,
            "PropertyChanged", call_barring_property_changed, handler, handler_free);
        break;
    case MSG_USSD_PROPERTY_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_SUPPLEMENTARY_SERVICES_INTERFACE,
            "PropertyChanged", ussd_state_changed, handler, handler_free);
        break;
    case MSG_USSD_NOTIFICATION_RECEIVED_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_SUPPLEMENTARY_SERVICES_INTERFACE,
            "NotificationReceived", ussd_notification_received, handler, handler_free);
        break;
    case MSG_USSD_REQUEST_RECEIVED_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_SUPPLEMENTARY_SERVICES_INTERFACE,
            "RequestReceived", ussd_request_received, handler, handler_free);
        break;
    default:
        break;
    }

    if (watch_id == 0) {
        tapi_log_error("add signal watch fail in %s, msg_id: %d", __func__, (int)msg);
        handler_free(handler);
        return -EINVAL;
    }

    return watch_id;
}

int tapi_ss_unregister(tapi_context context, int watch_id)
{
    dbus_context* ctx = context;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (watch_id <= 0) {
        tapi_log_error("watch id %d in %s is invalid", watch_id, __func__);
        return -EINVAL;
    }

    if (!g_dbus_remove_watch(ctx->connection, watch_id)) {
        tapi_log_error("remove signal watch fail in %s, watch_id: %d", __func__, watch_id);
        return -EINVAL;
    }

    return OK;
}
