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

#include "tapi_call.h"
#include "tapi_internal.h"
#include <dbus/dbus.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

typedef struct {
    char number[MAX_PHONE_NUMBER_LENGTH];
    int hide_callerid;
} call_param;

typedef struct {
    char path[MAX_CALL_ID_LENGTH];
    void* out;
} conference_param;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int decode_voice_call_info(DBusMessageIter* iter, tapi_call_info* call_info);
static int call_manager_property_changed(DBusConnection* connection, DBusMessage* message,
    void* user_data);
static int tapi_call_signal_call_added(DBusMessage* message, tapi_async_handler* handler);
static int tapi_call_signal_normal(DBusMessage* message, tapi_async_handler* handler, int msg_type);
static int tapi_call_signal_ecc_list_change(DBusMessage* message, tapi_async_handler* handler);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void call_event_free(void* user_data)
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

static void call_param_append(DBusMessageIter* iter, void* user_data)
{
    char* number;
    char* hide_callerid_str;

    call_param* param = user_data;
    if (param == NULL) {
        tapi_log_error("invalid dial request argument !!");
        return;
    }

    switch (param->hide_callerid) {
    case 1:
        hide_callerid_str = "enabled";
        break;
    case 2:
        hide_callerid_str = "disabled";
        break;
    default:
        hide_callerid_str = "default";
        break;
    }

    number = param->number;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &number);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &hide_callerid_str);
}

static void deflect_param_append(DBusMessageIter* iter, void* user_data)
{
    char* param = user_data;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &param);
}

static void separate_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    conference_param* conf_param = param->result->data;

    char* path = conf_param->path;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
}

static void tone_param_append(DBusMessageIter* iter, void* user_data)
{
    char* param = user_data;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &param);
}

static int tapi_swap_call(tapi_context context, int slot_id)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    return g_dbus_proxy_method_call(proxy, "SwapCalls", NULL, NULL, NULL, NULL);
}

static int
call_manager_property_changed(DBusConnection* connection, DBusMessage* message,
    void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    const char* member;
    const char* interface;
    const char* path;
    const char* sender;
    const char* dest;
    const char* signature;
    int msg_id;
    int ret;

    tapi_log_debug("call_manager_property_changed \n");
    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL)
        return false;

    tapi_log_debug("msg_id: %d, msg_status: %d \n", ar->msg_id, ar->status);

    member = dbus_message_get_member(message);
    interface = dbus_message_get_interface(message);
    path = dbus_message_get_path(message);
    sender = dbus_message_get_sender(message);
    dest = dbus_message_get_destination(message);
    signature = dbus_message_get_signature(message);
    tapi_log_debug("path: %s, interface: %s, sender: %s, dest: %s, member: %s, signature: %s",
        path, interface, sender, dest, member, signature);

    msg_id = ar->msg_id;
    ret = false;

    //signal messsge
    if (dbus_message_is_signal(message, OFONO_VOICECALL_MANAGER_INTERFACE,
            "CallAdded")
        && msg_id == MSG_CALL_ADD_MESSAGE_IND) {
        return tapi_call_signal_call_added(message, handler);
    } else if (dbus_message_is_signal(message, OFONO_VOICECALL_MANAGER_INTERFACE,
                   "CallRemoved")
        && msg_id == MSG_CALL_REMOVE_MESSAGE_IND) {
        return tapi_call_signal_normal(message, handler, DBUS_TYPE_OBJECT_PATH);
    } else if (dbus_message_is_signal(message, OFONO_VOICECALL_MANAGER_INTERFACE,
                   "Forwarded")
        && msg_id == MSG_CALL_FORWARDED_MESSAGE_IND) {
        return tapi_call_signal_normal(message, handler, DBUS_TYPE_STRING);
    } else if (dbus_message_is_signal(message, OFONO_VOICECALL_MANAGER_INTERFACE,
                   "BarringActive")
        && msg_id == MSG_CALL_BARRING_ACTIVE_MESSAGE_IND) {
        return tapi_call_signal_normal(message, handler, DBUS_TYPE_STRING);
    } else if (dbus_message_is_signal(message, OFONO_VOICECALL_MANAGER_INTERFACE,
                   "PropertyChanged")
        && msg_id == MSG_ECC_LIST_CHANGE_IND) {
        return tapi_call_signal_ecc_list_change(message, handler);
    }
    return ret;
}

static int call_property_changed(DBusConnection* connection, DBusMessage* message,
    void* user_data)
{
    int ret = false;
    DBusMessageIter iter;
    const char* path;
    tapi_call_property* call_property;
    int msg_id;
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;

    tapi_log_debug("call_property_changed \n");
    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    msg_id = ar->msg_id;
    if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL)
        return false;

    call_property = ar->data;
    if (call_property == NULL)
        return false;

    path = dbus_message_get_path(message);
    if (strcmp(path, call_property->call_path) != 0) {
        tapi_log_debug("call_property_changed user_data path: %s, signal path: %s",
            call_property->call_path, path);
        return false;
    }

    if (dbus_message_is_signal(message, OFONO_VOICECALL_INTERFACE, "DisconnectReason")
        && msg_id == MSG_CALL_DISCONNECTED_REASON_MESSAGE_IND) {
        if (tapi_is_call_signal_message(message, &iter, DBUS_TYPE_STRING)) {
            char* reason_str; //local,remote,network
            dbus_message_iter_get_basic(&iter, &reason_str);
            ar->data = reason_str;
            ar->status = OK;
            tapi_log_debug("DisconnectReason %s", reason_str);
        }
        ret = true;
    } else if (dbus_message_is_signal(message, OFONO_VOICECALL_INTERFACE, "PropertyChanged")
        && msg_id == MSG_CALL_PROPERTY_CHANGED_MESSAGE_IND) {
        if (tapi_is_call_signal_message(message, &iter, DBUS_TYPE_STRING)) {
            DBusMessageIter value_iter;
            char* key;
            dbus_message_iter_get_basic(&iter, &key);
            dbus_message_iter_next(&iter);
            strcpy(call_property->key, key);

            if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT)
                return false;

            dbus_message_iter_recurse(&iter, &value_iter);
            dbus_message_iter_get_basic(&value_iter, &call_property->value);

            ar->data = call_property;
            ar->status = OK;
            tapi_log_debug("call PropertyChanged [%s, %s]", call_property->key,
                (char*)call_property->value);
        }
        ret = true;
    }

    if (ret)
        cb(ar);

    return true;
}

static void merge_call_complete(DBusMessage* message, void* user_data)
{
    DBusMessageIter iter, list;
    DBusError err;
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    int index;
    conference_param* conf_param;
    char** call_path_list;

    tapi_log_debug("merge_call_complete \n");
    if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_ERROR) {
        const char* dbus_error = dbus_message_get_error_name(message);
        tapi_log_error("merge_call_complete ERROR: %s", dbus_error);
        return;
    }

    if (handler == NULL)
        return;

    ar = handler->result;
    if (ar == NULL)
        return;

    cb = handler->cb_function;
    if (cb == NULL)
        return;

    ar->status = OK;
    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("%s: %s\n", err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_has_signature(message, "ao") == false) {
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        ar->status = ERROR;
        goto done;
    }

    dbus_message_iter_recurse(&iter, &list);
    index = 0;
    conf_param = ar->data;
    call_path_list = (char**)conf_param->out;
    while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_OBJECT_PATH) {
        char* property;
        dbus_message_iter_get_basic(&list, &property); // object path
        tapi_log_debug("merge_call_complete path %s", property);
        call_path_list[index] = strdup(property);
        index++;
        dbus_message_iter_next(&list);
    }

    free(conf_param);
    ar->arg2 = index;

done:
    cb(ar);
}

static void call_list_query_complete(DBusMessage* message, void* user_data)
{
    DBusMessageIter args, list;
    DBusError err;
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    int index;
    tapi_call_list* call_list_head;

    if (handler == NULL)
        return;

    ar = handler->result;
    if (ar == NULL)
        return;

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

    //create calllist
    call_list_head = NULL;
    index = 0;
    while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_STRUCT) {
        DBusMessageIter entry;

        dbus_message_iter_recurse(&list, &entry);
        if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_OBJECT_PATH) {
            tapi_call_info* voicecall = malloc(sizeof(tapi_call_info));
            decode_voice_call_info(&entry, voicecall);
            call_list_head = tapi_add_call_to_list(call_list_head, voicecall);
        }

        dbus_message_iter_next(&list);
        index++;
    }

done:
    tapi_log_debug("tapi_call_get_all_calls size: %d", index);
    ar->data = call_list_head;
    cb(ar);
}

static int tapi_call_signal_call_added(DBusMessage* message, tapi_async_handler* handler)
{
    int ret = false;
    DBusMessageIter iter;
    tapi_async_result* ar;
    tapi_async_function cb;

    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (tapi_is_call_signal_message(message, &iter, DBUS_TYPE_OBJECT_PATH)) {
        //TODO: it can free data by app
        tapi_call_info* voicecall = malloc(sizeof(tapi_call_info));
        if (voicecall == NULL)
            return false;

        if (decode_voice_call_info(&iter, voicecall)) {
            ar->data = voicecall;
            ar->status = OK;
            cb(ar);
        }

        ret = true;
        tapi_log_debug("CallAdded Message add call %s", voicecall->call_path);
        dbus_free(voicecall);
    }
    return ret;
}

static int tapi_call_signal_normal(DBusMessage* message, tapi_async_handler* handler, int msg_type)
{
    int ret = false;
    DBusMessageIter iter;
    tapi_async_result* ar;
    tapi_async_function cb;

    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (tapi_is_call_signal_message(message, &iter, msg_type)) {
        char* info;
        dbus_message_iter_get_basic(&iter, &info);
        ar->data = info;
        ar->status = OK;
        cb(ar);

        ret = true;
        tapi_log_debug("call singal message id: %d, data: %s", ar->msg_id, info);
    }

    return ret;
}

static int tapi_call_signal_ecc_list_change(DBusMessage* message, tapi_async_handler* handler)
{
    int ret = false;
    DBusMessageIter iter;
    tapi_async_result* ar;
    tapi_async_function cb;

    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (tapi_is_call_signal_message(message, &iter, DBUS_TYPE_STRING)) {
        char* key;
        dbus_message_iter_get_basic(&iter, &key);
        dbus_message_iter_next(&iter);

        DBusMessageIter list, array;
        dbus_message_iter_recurse(&iter, &list);
        dbus_message_iter_recurse(&list, &array);

        int index = 0;
        char* out[MAX_CALL_PROPERTY_NAME_LENGTH];
        while (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_STRING) {
            char* value;
            dbus_message_iter_get_basic(&array, &value);
            dbus_message_iter_next(&array);

            tapi_log_debug("tapi_call_signal_ecc_list_change %s", value);
            out[index++] = strdup(value);
        }

        ar->data = out;
        ar->arg1 = index;
        ar->status = OK;
        cb(ar);

        ret = true;
    }
    return ret;
}

static int decode_voice_call_info(DBusMessageIter* iter, tapi_call_info* call_info)
{
    char* property;
    DBusMessageIter subArrayIter;

    dbus_message_iter_get_basic(iter, &property);
    strcpy(call_info->call_path, property);

    tapi_log_debug("------------call info------------");
    tapi_log_debug("decode_voice_call_info : %s", property);

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

        if (strcmp(key, "State") == 0) {
            dbus_message_iter_get_basic(&value, &result);
            call_info->state = tapi_call_string_to_status(result);
            tapi_log_debug("call state: %d \n", call_info->state);
        } else if (strcmp(key, "LineIdentification") == 0) {
            dbus_message_iter_get_basic(&value, &result);
            strcpy(call_info->lineIdentification, result);
            tapi_log_debug("call LineIdentification: %s \n", result);
        } else if (strcmp(key, "IncomingLine") == 0) {
            dbus_message_iter_get_basic(&value, &result);
            strcpy(call_info->incoming_line, result);
            tapi_log_debug("call IncomingLine: %s \n", result);
        } else if (strcmp(key, "Name") == 0) {
            dbus_message_iter_get_basic(&value, &result);
            strcpy(call_info->name, result);
            tapi_log_debug("call Name: %s \n", call_info->name);
        } else if (strcmp(key, "StartTime") == 0) {
            dbus_message_iter_get_basic(&value, &result);
            strcpy(call_info->start_time, result);
            tapi_log_debug("call StartTime: %s \n", result);
        } else if (strcmp(key, "Multiparty") == 0) {
            dbus_message_iter_get_basic(&value, &ret);
            call_info->multiparty = ret;
            tapi_log_debug("call Multiparty: %d \n", ret);
        } else if (strcmp(key, "RemoteHeld") == 0) {
            dbus_message_iter_get_basic(&value, &ret);
            call_info->remote_held = ret;
            tapi_log_debug("call RemoteHeld: %d \n", ret);
        } else if (strcmp(key, "RemoteMultiparty") == 0) {
            dbus_message_iter_get_basic(&value, &ret);
            call_info->remote_multiparty = ret;
            tapi_log_debug("call RemoteMultiparty: %d \n", ret);
        } else if (strcmp(key, "Information") == 0) {
            dbus_message_iter_get_basic(&value, &result);
            strcpy(call_info->info, result);
            tapi_log_debug("call Information: %s \n", result);
        } else if (strcmp(key, "Icon") == 0) {
            unsigned char val;
            dbus_message_iter_get_basic(&value, &val);
            call_info->icon = val;
            tapi_log_debug("call Icon: %d \n", val);
        } else if (strcmp(key, "Emergency") == 0) {
            dbus_message_iter_get_basic(&value, &ret);
            call_info->is_emergency_number = ret;
            tapi_log_debug("call Emergency: %d \n", ret);
        }

        dbus_message_iter_next(&subArrayIter);
    }
    tapi_log_debug("------------call info end------------");
    return true;
}

int tapi_call_dial(tapi_context context, int slot_id, char* number, int hide_callerid)
{
    call_param* param;
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    param = malloc(sizeof(call_param));
    if (param == NULL) {
        return -ENOMEM;
    }
    memset(param->number, 0, sizeof(param->number));
    sprintf(param->number, "%s", number);
    param->hide_callerid = hide_callerid;

    return g_dbus_proxy_method_call(proxy, "Dial", call_param_append, NULL, param, free);
}

int tapi_call_hangup_call(tapi_context context, int slot_id, char* call_id)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id) || !call_id) {
        return -EINVAL;
    }

    proxy = g_dbus_proxy_new(
        ctx->client, call_id, OFONO_VOICECALL_INTERFACE);
    g_dbus_proxy_method_call(proxy, "Hangup", NULL, NULL, NULL, NULL);
    g_dbus_proxy_unref(proxy);

    return 0;
}

int tapi_release_and_answer(tapi_context context, int slot_id)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    return g_dbus_proxy_method_call(proxy, "ReleaseAndAnswer", NULL, NULL, NULL, NULL);
}

int tapi_hold_and_answer(tapi_context context, int slot_id)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    return g_dbus_proxy_method_call(proxy, "HoldAndAnswer", NULL, NULL, NULL, NULL);
}

int tapi_call_answer_call(tapi_context context, int slot_id, tapi_call_list* call_list)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    int error;
    char* call_path;
    int count;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id) || !call_list) {
        return -EINVAL;
    }

    count = tapi_get_call_count(call_list);
    tapi_log_debug("tapi_call_answer_call callcount:%d", count);
    if (count == 1) {
        call_path = call_list->data->call_path;
        proxy = g_dbus_proxy_new(ctx->client, call_path, OFONO_VOICECALL_INTERFACE);
        if (proxy == NULL) {
            tapi_log_error("ERROR to initialize GDBusProxy for " OFONO_VOICECALL_MANAGER_INTERFACE);
            return -ENODEV;
        }

        g_dbus_proxy_method_call(proxy, "Answer", NULL, NULL, NULL, NULL);
        g_dbus_proxy_unref(proxy);

        error = 0;
    } else if (count == 2) {
        error = tapi_hold_and_answer(context, slot_id);
    } else if (count > 2) {
        error = tapi_release_and_answer(context, slot_id);
    } else {
        error = EPERM;
    }

    return error;
}

int tapi_call_hold_call(tapi_context context, int slot_id)
{
    dbus_context* ctx = context;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    return tapi_swap_call(context, slot_id);
}

int tapi_call_unhold_call(tapi_context context, int slot_id)
{
    dbus_context* ctx = context;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    return tapi_swap_call(context, slot_id);
}

int tapi_call_transfer(tapi_context context, int slot_id)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    return g_dbus_proxy_method_call(proxy, "Transfer", NULL, NULL, NULL, NULL);
}

int tapi_call_deflect_call(tapi_context context, int slot_id, char* call_id, char* number)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id) || !call_id) {
        return -EINVAL;
    }

    proxy = g_dbus_proxy_new(ctx->client, call_id, OFONO_VOICECALL_INTERFACE);
    if (proxy == NULL) {
        tapi_log_error("ERROR to initialize GDBusProxy for " OFONO_VOICECALL_MANAGER_INTERFACE);
        return -ENODEV;
    }

    g_dbus_proxy_method_call(proxy, "Deflect", deflect_param_append, NULL, call_id, NULL);
    g_dbus_proxy_unref(proxy);

    return 0;
}

int tapi_call_hangup_all_calls(tapi_context context, int slot_id)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    return g_dbus_proxy_method_call(proxy, "HangupAll", NULL, NULL, NULL, NULL);
}

void tapi_call_tapi_get_call_by_state(tapi_context context, int slot_id,
    int state, tapi_call_list* call_list, tapi_call_info* info)
{
    dbus_context* ctx = context;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id) || !call_list || !info) {
        return;
    }

    tapi_get_call_by_state(call_list, state, info);
}

int tapi_call_get_all_calls(tapi_context context, int slot_id,
    tapi_call_list* list, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        return -ENOMEM;
    }
    ar->arg1 = slot_id;
    ar->data = list;

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        free(ar);
        return -ENOMEM;
    }
    handler->cb_function = p_handle;
    handler->result = ar;

    return g_dbus_proxy_method_call(proxy, "GetCalls", NULL,
        call_list_query_complete, handler, call_event_free);
}

int tapi_call_merge_call(tapi_context context,
    int slot_id, char** out, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    conference_param* conf_param;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    conf_param = malloc(sizeof(conference_param));
    if (conf_param == NULL) {
        return -ENOMEM;
    }
    conf_param->out = out;

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(conf_param);
        return -ENOMEM;
    }
    ar->msg_id = MSG_CALL_MERGE_IND;
    ar->arg1 = slot_id;
    ar->data = conf_param;

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        free(conf_param);
        free(ar);
        return -ENOMEM;
    }
    handler->result = ar;
    handler->cb_function = p_handle;

    return g_dbus_proxy_method_call(proxy, "CreateMultiparty", NULL,
        merge_call_complete, handler, call_event_free);
}

int tapi_call_separate_call(tapi_context context,
    int slot_id, char* call_id, char** out, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    conference_param* conf_param;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id) || !call_id) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    conf_param = malloc(sizeof(conference_param));
    if (conf_param == NULL) {
        return -ENOMEM;
    }
    memset(conf_param->path, 0, sizeof(conf_param->path));
    sprintf(conf_param->path, "%s", call_id);
    conf_param->out = out;

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(conf_param);
        return -ENOMEM;
    }
    ar->msg_id = MSG_CALL_SEPERATE_IND;
    ar->arg1 = slot_id;
    ar->data = conf_param;

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        free(conf_param);
        free(ar);
        return -ENOMEM;
    }
    handler->cb_function = p_handle;
    handler->result = ar;

    return g_dbus_proxy_method_call(proxy, "PrivateChat",
        separate_param_append, merge_call_complete, handler, call_event_free);
}

int tapi_call_hangup_multiparty(tapi_context context, int slot_id)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    return g_dbus_proxy_method_call(proxy, "multiparty_hangup",
        NULL, NULL, NULL, NULL);
}

int tapi_call_send_tones(void* context, int slot_id, char* tones)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    return g_dbus_proxy_method_call(proxy, "SendTones", tone_param_append, NULL, tones, NULL);
}

int tapi_call_get_ecc_list(tapi_context context, int slot_id, char** out, int* size)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter list;
    int index = 0;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id) || !size) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!g_dbus_proxy_get_property(proxy, "EmergencyNumbers", &list)) {
        return -EIO;
    }

    if (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_ARRAY) {
        DBusMessageIter var_elem;
        dbus_message_iter_recurse(&list, &var_elem);
        while (dbus_message_iter_get_arg_type(&var_elem) != DBUS_TYPE_INVALID) {
            if (dbus_message_iter_get_arg_type(&var_elem) == DBUS_TYPE_STRING)
                dbus_message_iter_get_basic(&var_elem, &out[index++]);
            if (index > *size)
                break;
            dbus_message_iter_next(&var_elem);
        }
    }

    return 0;
}

bool tapi_is_emergency_number(tapi_context context, char* number)
{
    for (int i = 0; i < CONFIG_ACTIVE_MODEM_COUNT; i++) {
        char* ecc_list[20];
        int size;
        if (tapi_call_get_ecc_list(context, i, ecc_list, &size) != 0) {
            tapi_log_error("tapi_is_emergency_number: get ecc list error\n");
            return false;
        }

        for (int j = 0; j < size; j++) {
            if (strcmp(number, ecc_list[j]) == 0) {
                tapi_log_debug("tapi_is_emergency_number:%s is emergency number\n", number);
                return false;
            }
        }
    }

    return true;
}

int tapi_register_call_signal(tapi_context context, int slot_id, char* path, char* interface,
    tapi_indication_msg msg, tapi_async_function p_handle, GDBusSignalFunction function)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    tapi_call_property* call_property;
    const char* member;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (path == NULL) {
        path = tapi_utils_get_modem_path(slot_id);
        ;
    }
    if (path == NULL) {
        tapi_log_error("no available modem ...\n");
        return -ENODEV;
    }

    member = tapi_get_call_signal_member(msg);
    if (member == NULL) {
        tapi_log_error("no signal member found ...\n");
        return -EINVAL;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        return -ENOMEM;
    }
    ar->msg_id = msg;
    ar->arg1 = slot_id;

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        free(ar);
        return -ENOMEM;
    }
    handler->cb_function = p_handle;
    handler->result = ar;

    if (msg == MSG_CALL_PROPERTY_CHANGED_MESSAGE_IND) {
        call_property = malloc(sizeof(tapi_call_property));
        if (call_property == NULL) {
            free(ar);
            free(handler);
            return -ENOMEM;
        }
        strcpy(call_property->call_path, path);
        ar->data = call_property;
    }

    return g_dbus_add_signal_watch(ctx->connection,
        OFONO_SERVICE, path, interface, member, function, handler, call_event_free);
}

int tapi_register_managercall_change(tapi_context context, int slot_id,
    tapi_indication_msg msg, tapi_async_function p_handle)
{
    if (context == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    return tapi_register_call_signal(context, slot_id, NULL,
        OFONO_VOICECALL_MANAGER_INTERFACE, msg,
        p_handle, call_manager_property_changed);
}

int tapi_register_call_info_change(tapi_context context, int slot_id, char* call_path,
    tapi_indication_msg msg,
    tapi_async_function p_handle)
{
    if (context == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    return tapi_register_call_signal(context, slot_id, call_path, OFONO_VOICECALL_INTERFACE,
        msg, p_handle, call_property_changed);
}

int tapi_register_emergencylist_change(tapi_context context, int slot_id, tapi_async_function p_handle)
{
    if (context == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    return tapi_register_call_signal(context, slot_id, NULL,
        OFONO_VOICECALL_MANAGER_INTERFACE, MSG_ECC_LIST_CHANGE_IND,
        p_handle, call_manager_property_changed);
}

void tapi_call_tapi_free_call_list(tapi_call_list* head)
{
    tapi_free_call_list(head);
}
