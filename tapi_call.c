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
#include <string.h>

#include "tapi.h"
#include "tapi_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NEW_VOICE_CALL_DBUS_PROXY 1
#define RELEASE_VOICE_CALL_DBUS_PROXY 2

#define START_PLAY_DTMF 1
#define STOP_PLAY_DTMF 2

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

typedef struct {
    char number[MAX_PHONE_NUMBER_LENGTH + 1];
    int hide_callerid;
} call_param;

typedef struct {
    int length;
    void* participants;
} ims_conference_param;

typedef struct {
    char* path;
    char* number;
} call_deflect_param;

typedef struct {
    unsigned char digit;
    int flag;
} call_dtmf_param;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int decode_voice_call_info(DBusMessageIter* iter, tapi_call_info* call_info);
static int call_manager_property_changed(DBusConnection* connection, DBusMessage* message,
    void* user_data);
static int call_state_changed(DBusConnection* connection, DBusMessage* message,
    void* user_data);
static int tapi_call_signal_ecc_list_change(DBusMessage* message, tapi_async_handler* handler);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int call_strcpy(char* dst, const char* src, int dst_size)
{
    if (dst == NULL || src == NULL || strlen(src) > dst_size) {
        return -EINVAL;
    }

    strcpy(dst, src);
    return 0;
}

static bool is_valid_dtmf_char(char c)
{
    if ((c >= '0' && c <= '9') || c == '*' || c == '#') {
        return true;
    }

    return false;
}

static void call_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* handler = user_data;
    char* hide_callerid_str;
    call_param* param;
    char* number;

    if (handler == NULL || handler->result == NULL || handler->result->data == NULL) {
        tapi_log_error("invalid dial request argument !!");
        return;
    }

    param = (call_param*)handler->result->data;

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

    free(param);
}

static void answer_hangup_param_append(DBusMessageIter* iter, void* user_data)
{
    char* path = user_data;

    if (path == NULL) {
        tapi_log_error("invalid answer_hangup request argument !!");
        return;
    }

    dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
}

static void dtmf_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* handler = user_data;
    call_dtmf_param* param;
    unsigned char digit;
    int flag;

    if (handler == NULL || handler->result == NULL || handler->result->data == NULL) {
        tapi_log_error("invalid dtmf request argument !!");
        return;
    }

    param = (call_dtmf_param*)handler->result->data;

    digit = param->digit;
    flag = param->flag;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_BYTE, &digit);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &flag);

    free(param);
}

static void deflect_param_append_0(DBusMessageIter* iter, void* user_data)
{
    call_deflect_param* param = user_data;
    char *path, *number;

    if (param == NULL) {
        tapi_log_error("invalid deflect request argument !!");
        return;
    }

    path = param->path;
    number = param->number;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &number);

    free(param);
}

static void separate_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    char* path = param->result->data;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
}

static void tone_param_append(DBusMessageIter* iter, void* user_data)
{
    char* param = user_data;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &param);
}

static void conference_param_append(DBusMessageIter* iter, void* user_data)
{
    ims_conference_param* ims_conference_participants;
    DBusMessageIter array;
    char** participants;

    ims_conference_participants = user_data;
    if (ims_conference_participants == NULL)
        return;

    participants = (char**)ims_conference_participants->participants;

    dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING, &array);

    for (int i = 0; i < ims_conference_participants->length; i++) {
        dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING, &participants[i]);
    }

    dbus_message_iter_close_container(iter, &array);
}

static int tapi_call_default_voicecall_slot_change(DBusMessage* message, tapi_async_handler* handler)
{
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter, var;
    const char* property;
    const char* slot;

    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (!dbus_message_iter_init(message, &iter)) {
        return false;
    }

    dbus_message_iter_get_basic(&iter, &property);
    dbus_message_iter_next(&iter);
    dbus_message_iter_recurse(&iter, &var);

    if (!strcmp(property, "VoiceCallSlot")) {
        dbus_message_iter_get_basic(&var, &slot);
        ar->arg2 = tapi_utils_get_slot_id(slot);
        ar->status = OK;
        cb(ar);
    }

    return true;
}

static int ring_back_tone_change(DBusMessage* message, tapi_async_handler* handler)
{
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter;

    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (is_call_signal_message(message, &iter, DBUS_TYPE_INT32)) {
        dbus_message_iter_get_basic(&iter, &ar->arg2);

        ar->status = OK;
        cb(ar);
    }

    return true;
}

static int
call_manager_property_changed(DBusConnection* connection, DBusMessage* message,
    void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    int msg_id;

    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    msg_id = ar->msg_id;
    if (dbus_message_is_signal(message, OFONO_VOICECALL_MANAGER_INTERFACE,
            "PropertyChanged")
        && msg_id == MSG_ECC_LIST_CHANGE_IND) {
        return tapi_call_signal_ecc_list_change(message, handler);
    } else if (dbus_message_is_signal(message, OFONO_VOICECALL_MANAGER_INTERFACE,
                   "RingBackTone")
        && msg_id == MSG_CALL_RING_BACK_TONE_IND) {
        return ring_back_tone_change(message, handler);
    } else if (dbus_message_is_signal(message, OFONO_MANAGER_INTERFACE, "PropertyChanged")
        && msg_id == MSG_DEFAULT_VOICECALL_SLOT_CHANGE_IND) {
        return tapi_call_default_voicecall_slot_change(message, handler);
    }

    return true;
}

static int call_state_changed(DBusConnection* connection, DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter;
    tapi_call_info voicecall;

    if (handler == NULL)
        return 0;

    ar = handler->result;
    if (ar == NULL)
        return 0;

    cb = handler->cb_function;
    if (cb == NULL)
        return 0;

    if (!dbus_message_iter_init(message, &iter))
        return 0;

    if (decode_voice_call_info(&iter, &voicecall)) {
        ar->data = &voicecall;
        ar->status = OK;
        cb(ar);
        return 1;
    }

    return 0;
}

static void dial_call_callback(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessageIter iter;
    DBusError err;
    char* call_id;

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

    if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_OBJECT_PATH) {
        dbus_message_iter_get_basic(&iter, &call_id);
        ar->data = call_id;
        ar->status = OK;
    }

done:
    cb(ar);
}

static void play_dtmf_callback(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusError err;

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
    }

    cb(ar);
}

static void merge_call_complete(DBusMessage* message, void* user_data)
{
    char* call_path_list[MAX_CONFERENCE_PARTICIPANT_COUNT];
    tapi_async_handler* handler = user_data;
    DBusMessageIter iter, list;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusError err;
    int index = 0;

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

    if (dbus_message_has_signature(message, "ao") == false) {
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        ar->status = ERROR;
        goto done;
    }

    dbus_message_iter_recurse(&iter, &list);
    while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_OBJECT_PATH) {
        dbus_message_iter_get_basic(&list, &call_path_list[index++]);
        if (index >= MAX_CONFERENCE_PARTICIPANT_COUNT)
            break;

        dbus_message_iter_next(&list);
    }

    ar->data = call_path_list;
    ar->status = OK;
    ar->arg2 = index;

done:
    cb(ar);
}

static void call_list_query_complete(DBusMessage* message, void* user_data)
{
    tapi_call_info call_list[MAX_CALL_LIST_COUNT];
    tapi_async_handler* handler = user_data;
    DBusMessageIter args, list;
    tapi_async_function cb;
    tapi_async_result* ar;
    int call_count = 0;
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

            decode_voice_call_info(&entry, call_list + call_count);
            call_count++;
        }

        dbus_message_iter_next(&list);
    }

    ar->arg2 = call_count;
    ar->status = OK;
    ar->data = call_list;

done:
    cb(ar);
}

static int tapi_call_signal_ecc_list_change(DBusMessage* message, tapi_async_handler* handler)
{
    DBusMessageIter iter, list, array;
    tapi_async_result* ar;
    tapi_async_function cb;
    char* ecc_list[MAX_ECC_LIST_SIZE];
    char* key;
    int index = 0;

    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (is_call_signal_message(message, &iter, DBUS_TYPE_STRING)) {
        dbus_message_iter_get_basic(&iter, &key);
        dbus_message_iter_next(&iter);

        dbus_message_iter_recurse(&iter, &list);
        dbus_message_iter_recurse(&list, &array);

        while (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_STRING) {
            dbus_message_iter_get_basic(&array, &ecc_list[index++]);
            if (index >= MAX_ECC_LIST_SIZE)
                break;

            dbus_message_iter_next(&array);
        }

        ar->status = OK;
    }

    ar->data = ecc_list;
    ar->arg2 = index;
    cb(ar);

    return true;
}

static int decode_voice_call_info(DBusMessageIter* iter, tapi_call_info* call_info)
{
    DBusMessageIter subArrayIter;
    char* property;

    memset(call_info, 0, sizeof(tapi_call_info));

    dbus_message_iter_get_basic(iter, &property);
    if (call_strcpy(call_info->call_id, property, MAX_CALL_ID_LENGTH) != 0) {
        tapi_log_debug("decode_voice_call_info: path is too long: %s", property);
        return -EIO;
    }

    dbus_message_iter_next(iter);
    dbus_message_iter_recurse(iter, &subArrayIter);

    while (dbus_message_iter_get_arg_type(&subArrayIter) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry, value;
        unsigned char val;
        const char* key;
        char* result;
        int ret;

        dbus_message_iter_recurse(&subArrayIter, &entry);
        dbus_message_iter_get_basic(&entry, &key);

        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &value);

        if (strcmp(key, "State") == 0) {
            dbus_message_iter_get_basic(&value, &result);
            call_info->state = tapi_utils_call_status_from_string(result);
        } else if (strcmp(key, "LineIdentification") == 0) {
            dbus_message_iter_get_basic(&value, &result);
            call_strcpy(call_info->lineIdentification, result, MAX_CALL_LINE_ID_LENGTH);
        } else if (strcmp(key, "IncomingLine") == 0) {
            dbus_message_iter_get_basic(&value, &result);
            call_strcpy(call_info->incoming_line, result, MAX_CALL_INCOMING_LINE_LENGTH);
        } else if (strcmp(key, "Name") == 0) {
            dbus_message_iter_get_basic(&value, &result);
            call_strcpy(call_info->name, result, MAX_CALL_NAME_LENGTH);
        } else if (strcmp(key, "StartTime") == 0) {
            dbus_message_iter_get_basic(&value, &result);
            call_strcpy(call_info->start_time, result, MAX_CALL_START_TIME_LENGTH);
        } else if (strcmp(key, "Multiparty") == 0) {
            dbus_message_iter_get_basic(&value, &ret);
            call_info->multiparty = ret;
        } else if (strcmp(key, "RemoteHeld") == 0) {
            dbus_message_iter_get_basic(&value, &ret);
            call_info->remote_held = ret;
        } else if (strcmp(key, "RemoteMultiparty") == 0) {
            dbus_message_iter_get_basic(&value, &ret);
            call_info->remote_multiparty = ret;
        } else if (strcmp(key, "Information") == 0) {
            dbus_message_iter_get_basic(&value, &result);
            call_strcpy(call_info->info, result, MAX_CALL_INFO_LENGTH);
        } else if (strcmp(key, "Icon") == 0) {
            dbus_message_iter_get_basic(&value, &val);
            call_info->icon = val;
        } else if (strcmp(key, "Emergency") == 0) {
            dbus_message_iter_get_basic(&value, &ret);
            call_info->is_emergency_number = ret;
        } else if (strcmp(key, "DisconnectReason") == 0) {
            dbus_message_iter_get_basic(&value, &ret);
            call_info->disconnect_reason = ret;
        }

        dbus_message_iter_next(&subArrayIter);
    }

    return true;
}

static int tapi_register_manager_call_signal(tapi_context context, int slot_id, char* interface,
    tapi_indication_msg msg, void* user_obj, tapi_async_function p_handle,
    GDBusSignalFunction function)
{
    tapi_async_handler* handler;
    dbus_context* ctx = context;
    tapi_async_result* ar;
    const char* member;
    const char* modem_path;
    int watch_id;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    member = get_call_signal_member(msg);
    if (member == NULL) {
        tapi_log_error("no signal member found ...\n");
        return -EINVAL;
    }

    modem_path = tapi_utils_get_modem_path(slot_id);
    if (modem_path == NULL) {
        tapi_log_error("no available modem ...\n");
        return -EIO;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        return -ENOMEM;
    }
    ar->msg_id = msg;
    ar->msg_type = INDICATION;
    ar->arg1 = slot_id;
    ar->user_obj = user_obj;

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        free(ar);
        return -ENOMEM;
    }
    handler->cb_function = p_handle;
    handler->result = ar;

    switch (msg) {
    case MSG_DEFAULT_VOICECALL_SLOT_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, OFONO_MANAGER_PATH, interface,
            member, function, handler, handler_free);
        break;
    default:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, interface, member, function, handler, handler_free);
        break;
    }

    if (watch_id == 0) {
        handler_free(handler);
        return -EINVAL;
    }

    return watch_id;
}

static int manage_call_proxy_method(tapi_context context, int slot_id, const char* member)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id) || member == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!g_dbus_proxy_method_call(proxy, member, NULL, no_operate_callback, NULL, NULL)) {
        return -EINVAL;
    }

    return OK;
}

static int manager_conference(tapi_context context, int slot_id,
    const char* member, ims_conference_param* param)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id) || param == NULL) {
        return -EINVAL;
    }

    if (param->length <= 0 || param->length > MAX_IMS_CONFERENCE_CALLS) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!g_dbus_proxy_method_call(proxy, member, conference_param_append,
            no_operate_callback, param, free)) {
        return -EINVAL;
    }

    return OK;
}

static int call_play_dtmf(tapi_context context, int slot_id, unsigned char digit, int flag,
    int event_id, tapi_async_function p_handle)
{
    tapi_async_handler* handler;
    dbus_context* ctx = context;
    call_dtmf_param* param;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    if (flag == START_PLAY_DTMF && !is_valid_dtmf_char(digit)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    param = malloc(sizeof(call_dtmf_param));
    if (param == NULL)
        return -ENOMEM;

    param->digit = digit;
    param->flag = flag;

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(param);
        return -ENOMEM;
    }
    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->data = param;

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        free(param);
        free(ar);
        return -ENOMEM;
    }
    handler->result = ar;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "PlayDtmf", dtmf_param_append,
            play_dtmf_callback, handler, handler_free)) {
        free(param);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int tapi_call_dial(tapi_context context, int slot_id, char* number, int hide_callerid,
    int event_id, tapi_async_function p_handle)
{
    tapi_async_handler* handler;
    dbus_context* ctx = context;
    tapi_async_result* ar;
    call_param* param;
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

    snprintf(param->number, sizeof(param->number), "%s", number);
    param->hide_callerid = hide_callerid;

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(param);
        return -ENOMEM;
    }
    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->data = param;

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        free(param);
        free(ar);
        return -ENOMEM;
    }
    handler->result = ar;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "Dial", call_param_append,
            dial_call_callback, handler, handler_free)) {
        free(param);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_call_release_and_swap(tapi_context context, int slot_id)
{
    return manage_call_proxy_method(context, slot_id, "ReleaseAndSwap");
}

int tapi_call_release_and_answer(tapi_context context, int slot_id)
{
    return manage_call_proxy_method(context, slot_id, "ReleaseAndAnswer");
}

int tapi_call_hold_and_answer(tapi_context context, int slot_id)
{
    return manage_call_proxy_method(context, slot_id, "HoldAndAnswer");
}

int tapi_call_hold_call(tapi_context context, int slot_id)
{
    return manage_call_proxy_method(context, slot_id, "SwapCalls");
}

int tapi_call_unhold_call(tapi_context context, int slot_id)
{
    return manage_call_proxy_method(context, slot_id, "SwapCalls");
}

int tapi_call_transfer(tapi_context context, int slot_id)
{
    return manage_call_proxy_method(context, slot_id, "Transfer");
}

int tapi_call_hangup_all_calls(tapi_context context, int slot_id)
{
    return manage_call_proxy_method(context, slot_id, "HangupAll");
}

int tapi_call_get_call_by_state(tapi_context context, int slot_id,
    int state, tapi_call_info* call_list, int size, tapi_call_info* info)
{
    dbus_context* ctx = context;
    int index = 0;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || call_list == NULL || info == NULL) {
        return -EINVAL;
    }

    for (int i = 0; i < size; i++) {
        if (call_list[i].state == state) {
            memcpy(info + index, call_list + i, sizeof(tapi_call_info));
            index++;
        }
    }

    return index;
}

int tapi_call_get_all_calls(tapi_context context, int slot_id, int event_id,
    tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

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
    ar->msg_id = event_id;
    ar->arg1 = slot_id;

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        free(ar);
        return -ENOMEM;
    }

    handler->cb_function = p_handle;
    handler->result = ar;

    if (!g_dbus_proxy_method_call(proxy, "GetCalls", NULL,
            call_list_query_complete, handler, handler_free)) {
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_call_merge_call(tapi_context context,
    int slot_id, int event_id, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

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

    ar->msg_id = event_id;
    ar->arg1 = slot_id;

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        free(ar);
        return -ENOMEM;
    }
    handler->result = ar;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "CreateMultiparty", NULL,
            merge_call_complete, handler, handler_free)) {
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_call_separate_call(tapi_context context,
    int slot_id, int event_id, char* call_id, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id) || !call_id) {
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
    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->data = call_id;

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        free(ar);
        return -ENOMEM;
    }
    handler->cb_function = p_handle;
    handler->result = ar;

    if (!g_dbus_proxy_method_call(proxy, "PrivateChat",
            separate_param_append, merge_call_complete, handler, handler_free)) {
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_call_hangup_multiparty(tapi_context context, int slot_id)
{
    return manage_call_proxy_method(context, slot_id, "multiparty_hangup");
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

    if (!g_dbus_proxy_method_call(proxy, "SendTones", tone_param_append,
            no_operate_callback, tones, NULL)) {
        return -EINVAL;
    }

    return OK;
}

int tapi_call_get_ecc_list(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    DBusMessageIter list;
    GDBusProxy* proxy;
    int index = 0;

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

    if (!g_dbus_proxy_get_property(proxy, "EmergencyNumbers", &list)) {
        return -EINVAL;
    }

    if (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_ARRAY) {
        DBusMessageIter var_elem;
        dbus_message_iter_recurse(&list, &var_elem);

        while (dbus_message_iter_get_arg_type(&var_elem) != DBUS_TYPE_INVALID) {
            if (dbus_message_iter_get_arg_type(&var_elem) == DBUS_TYPE_STRING)
                dbus_message_iter_get_basic(&var_elem, &out[index++]);

            if (index >= MAX_ECC_LIST_SIZE)
                break;

            dbus_message_iter_next(&var_elem);
        }
    }

    return index;
}

bool tapi_call_is_emergency_number(tapi_context context, char* number)
{
    for (int i = 0; i < CONFIG_ACTIVE_MODEM_COUNT; i++) {
        char* ecc_list[MAX_ECC_LIST_SIZE];
        int size = tapi_call_get_ecc_list(context, i, ecc_list);
        if (size <= 0) {
            tapi_log_error("tapi_is_emergency_number: get ecc list error\n");
            return false;
        }

        for (int j = 0; j < size; j++) {
            if (strcmp(number, ecc_list[j]) == 0) {
                tapi_log_debug("tapi_is_emergency_number:%s is emergency number\n", number);
                return true;
            }
        }
    }

    return false;
}

int tapi_call_register_emergency_list_change(tapi_context context, int slot_id, void* user_obj,
    tapi_async_function p_handle)
{
    if (context == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    return tapi_register_manager_call_signal(context, slot_id,
        OFONO_VOICECALL_MANAGER_INTERFACE, MSG_ECC_LIST_CHANGE_IND, user_obj,
        p_handle, call_manager_property_changed);
}

int tapi_call_register_ringback_tone_change(tapi_context context, int slot_id, void* user_obj,
    tapi_async_function p_handle)
{
    if (context == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    return tapi_register_manager_call_signal(context, slot_id,
        OFONO_VOICECALL_MANAGER_INTERFACE, MSG_CALL_RING_BACK_TONE_IND, user_obj,
        p_handle, call_manager_property_changed);
}

int tapi_call_register_default_voicecall_slot_change(tapi_context context, void* user_obj,
    tapi_async_function p_handle)
{
    if (context == NULL) {
        return -EINVAL;
    }

    return tapi_register_manager_call_signal(context, 0,
        OFONO_MANAGER_INTERFACE, MSG_DEFAULT_VOICECALL_SLOT_CHANGE_IND, user_obj,
        p_handle, call_manager_property_changed);
}

int tapi_call_dial_conferece(tapi_context context, int slot_id, char* participants[], int size)
{
    ims_conference_param* ims_conference_participants_param;
    int ret;

    ims_conference_participants_param = malloc(sizeof(ims_conference_param));
    if (ims_conference_participants_param == NULL) {
        return -ENOMEM;
    }

    ims_conference_participants_param->length = size;
    ims_conference_participants_param->participants = participants;

    ret = manager_conference(context, slot_id, "DialConference",
        ims_conference_participants_param);

    if (ret != OK) {
        free(ims_conference_participants_param);
    }

    return ret;
}

int tapi_call_invite_participants(tapi_context context, int slot_id,
    char* participants[], int size)
{
    ims_conference_param* ims_conference_participants_param;
    int ret;

    ims_conference_participants_param = malloc(sizeof(ims_conference_param));
    if (ims_conference_participants_param == NULL) {
        return -ENOMEM;
    }

    ims_conference_participants_param->length = size;
    ims_conference_participants_param->participants = participants;

    ret = manager_conference(context, slot_id, "InviteParticipants",
        ims_conference_participants_param);

    if (ret != OK) {
        free(ims_conference_participants_param);
    }

    return ret;
}

int tapi_call_register_call_state_change(tapi_context context, int slot_id,
    void* user_obj, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    const char* modem_path;
    int watch_id;
    tapi_async_handler* handler;
    tapi_async_result* ar;

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
    ar->arg1 = slot_id;
    ar->user_obj = user_obj;
    ar->msg_type = INDICATION;

    watch_id = g_dbus_add_signal_watch(ctx->connection,
        OFONO_SERVICE, modem_path, OFONO_VOICECALL_MANAGER_INTERFACE,
        "CallChanged", call_state_changed, handler, handler_free);

    if (watch_id == 0) {
        handler_free(handler);
        return -EINVAL;
    }

    return watch_id;
}

int tapi_call_answer_by_id(tapi_context context, int slot_id, char* call_id)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id) || call_id == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!g_dbus_proxy_method_call(proxy, "Answer", answer_hangup_param_append,
            no_operate_callback, call_id, NULL)) {
        return -EINVAL;
    }

    return OK;
}

int tapi_call_hangup_by_id(tapi_context context, int slot_id, char* call_id)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id) || call_id == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!g_dbus_proxy_method_call(proxy, "Hangup", answer_hangup_param_append,
            no_operate_callback, call_id, NULL)) {
        return -EINVAL;
    }

    return OK;
}

int tapi_call_deflect_by_id(tapi_context context, int slot_id, char* call_id, char* number)
{
    dbus_context* ctx = context;
    call_deflect_param* param;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || call_id == NULL || number == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_CALL];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    param = malloc(sizeof(call_deflect_param));
    if (param == NULL)
        return -ENOMEM;

    param->path = call_id;
    param->number = number;

    if (!g_dbus_proxy_method_call(proxy, "Deflect", deflect_param_append_0,
            no_operate_callback, param, free)) {
        free(param);
        return -EINVAL;
    }

    return OK;
}

int tapi_call_start_dtmf(tapi_context context, int slot_id, unsigned char digit,
    int event_id, tapi_async_function p_handle)
{
    return call_play_dtmf(context, slot_id, digit, START_PLAY_DTMF, event_id, p_handle);
}

int tapi_call_stop_dtmf(tapi_context context, int slot_id,
    int event_id, tapi_async_function p_handle)
{
    return call_play_dtmf(context, slot_id, 0, STOP_PLAY_DTMF, event_id, p_handle);
}

int tapi_call_set_default_slot(tapi_context context, int slot_id)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    const char* modem_path;

    proxy = ctx->dbus_proxy_manager;
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!ctx->client_ready)
        return -EAGAIN;

    if (!tapi_is_valid_slotid(slot_id) && slot_id != -1)
        return -EINVAL;

    modem_path = tapi_utils_get_modem_path(slot_id);

    if (!g_dbus_proxy_set_property_basic(proxy,
            "VoiceCallSlot", DBUS_TYPE_STRING, &modem_path, NULL, NULL, NULL)) {
        return -EINVAL;
    }

    return OK;
}

int tapi_call_get_default_slot(tapi_context context, int* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    char* modem_path;

    proxy = ctx->dbus_proxy_manager;
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!ctx->client_ready)
        return -EAGAIN;

    if (g_dbus_proxy_get_property(proxy, "VoiceCallSlot", &iter)) {
        dbus_message_iter_get_basic(&iter, &modem_path);
        *out = tapi_utils_get_slot_id(modem_path);
        return OK;
    }

    return -EINVAL;
}