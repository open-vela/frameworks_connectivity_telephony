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

#include "tapi.h"
#include "tapi_internal.h"
#include "tapi_manager.h"
#include "tapi_sms.h"

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

typedef struct {
    char* number;
    char* text;
} message_param;

typedef struct {
    char* dest_addr;
    unsigned int port;
    char* data;
} data_message_param;

/****************************************************************************
 * Private Function
 ****************************************************************************/

static int decode_message_info(DBusMessageIter* iter, tapi_message_info* message_info);
static int unsol_sms_message(DBusConnection* connection, DBusMessage* message, void* user_data);
static void message_list_query_complete(DBusMessage* message, void* user_data);

static char* strdup0(const char* str)
{
    if (str != NULL)
        return strdup(str);

    return NULL;
}

static void message_free(void* user_data)
{
    message_param* message = user_data;
    if (message == NULL)
        return;

    free(message->number);
    message->number = NULL;
    free(message->text);
    message->text = NULL;
    free(message);
}

static void data_message_free(void* user_data)
{
    data_message_param* message = user_data;
    if (message == NULL)
        return;

    free(message->dest_addr);
    message->dest_addr = NULL;
    free(message->data);
    message->data = NULL;
    free(message);
}

static void send_message_param_append(DBusMessageIter* iter, void* user_data)
{
    message_param* msg_param;
    tapi_async_handler* param;

    param = user_data;
    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        return;
    }

    if (param->result == NULL) {
        tapi_log_error("invalid message argument in %s!", __func__);
        return;
    }

    msg_param = param->result->data;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &msg_param->number);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &msg_param->text);

    message_free(msg_param);
    param->result->data = NULL;
}

static void send_data_message_param_append(DBusMessageIter* iter, void* user_data)
{
    data_message_param* message;
    tapi_async_handler* param;

    param = user_data;
    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        return;
    }

    if (param->result == NULL) {
        tapi_log_error("invalid message argument in %s!", __func__);
        return;
    }

    message = param->result->data;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &message->dest_addr);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &message->port);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &message->data);

    data_message_free(message);
    param->result->data = NULL;
}

static void copy_message_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_message_info* message_info = user_data;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &message_info->sender);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &message_info->text);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &message_info->sms_type);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &message_info->sent_time);
}

static void delete_message_param_append(DBusMessageIter* iter, void* user_data)
{
    char* param = user_data;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &param);
}

static void send_sms_callback(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessageIter iter;
    DBusError err;
    char* uuid;

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

    if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_OBJECT_PATH) {
        dbus_message_iter_get_basic(&iter, &uuid);
        ar->data = uuid;
        ar->status = OK;
    }

done:
    cb(ar);
}

static void message_info_free(void* user_data)
{
    tapi_message_info* message_info = user_data;

    free(message_info->text);
    free(message_info->sender);
    free(message_info->sent_time);
    free(message_info);
}

static int unsol_sms_message(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    DBusMessageIter iter, list;
    DBusMessageIter entry, result;
    const char* sender;
    const char* member;
    char* text;
    char *name, *value;
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    tapi_message_info* message_info = NULL;

    if (handler == NULL) {
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

    sender = dbus_message_get_sender(message);
    if (sender == NULL) {
        tapi_log_error("sender in %s is null", __func__);
        ar->status = ERROR;
        goto done;
    }

    if (!dbus_message_iter_init(message, &iter)) {
        tapi_log_error("message iter init failed in %s", __func__);
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
        tapi_log_error("message iter get arg type failed in %s", __func__);
        ar->status = ERROR;
        goto done;
    }

    dbus_message_iter_get_basic(&iter, &text);
    member = dbus_message_get_member(message);

    if (strcmp(member, "IncomingMessage") == 0
        || strcmp(member, "ImmediateMessage") == 0) {
        dbus_message_iter_next(&iter);
        dbus_message_iter_recurse(&iter, &list);

        message_info = calloc(1, sizeof(tapi_message_info));
        if (message_info == NULL) {
            tapi_log_error("message_info in %s is null", __func__);
            ar->status = ERROR;
            goto done;
        }
        message_info->text = text;

        if (strcmp(member, "IncomingMessage") == 0) {
            message_info->sms_type = 1;
        } else if (strcmp(member, "ImmediateMessage") == 0) {
            message_info->sms_type = 2;
        }

        while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_DICT_ENTRY) {
            dbus_message_iter_recurse(&list, &entry);
            dbus_message_iter_get_basic(&entry, &name);

            dbus_message_iter_next(&entry);

            dbus_message_iter_recurse(&entry, &result);
            dbus_message_iter_get_basic(&result, &value);

            if (strcmp(name, "LocalSentTime") == 0) {
                dbus_message_iter_get_basic(&result, &message_info->local_sent_time);
            } else if (strcmp(name, "SentTime") == 0) {
                dbus_message_iter_get_basic(&result, &message_info->sent_time);
            } else if (strcmp(name, "Sender") == 0) {
                message_info->sender = value;
            }
            dbus_message_iter_next(&list);
        }

        ar->data = message_info;
        ar->status = OK;
    } else if (strcmp(member, "StatusReportMessage") == 0) {
        if (strcmp(text, "0") && strcmp(text, "1")) {
            ar->status = ERROR;
            goto done;
        }

        ar->status = OK;
        ar->data = text;
    }

done:
    cb(ar);
    if (message_info != NULL)
        free(message_info);

    return 1;
}

static int sms_property_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter, var;
    const char* property;
    const char* slot;
    int delivered;

    if (handler == NULL) {
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

    if (dbus_message_iter_init(message, &iter) == false) {
        tapi_log_error("message iter init failed in %s", __func__);
        return 0;
    }

    dbus_message_iter_get_basic(&iter, &property);
    dbus_message_iter_next(&iter);
    dbus_message_iter_recurse(&iter, &var);

    if ((ar->msg_id == MSG_DEFAULT_SMS_SLOT_CHANGED_IND)
        && !strcmp(property, "SmsSlot")) {
        dbus_message_iter_get_basic(&var, &slot);
        ar->arg2 = tapi_utils_get_slot_id(slot);
        ar->status = OK;
        cb(ar);
    } else if ((ar->msg_id == MSG_SMS_REPORT_SWITCH_CHANGED_IND)
        && !strcmp(property, "UseDeliveryReports")) {
        dbus_message_iter_get_basic(&var, &delivered);
        if (delivered != 0 && delivered != 1) {
            ar->status = ERROR;
        } else {
            ar->status = OK;
            ar->arg2 = delivered;
        }

        cb(ar);
    }

    return 1;
}

static int decode_message_info(DBusMessageIter* iter, tapi_message_info* message_info)
{
    DBusMessageIter subArrayIter;

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

        if (strcmp(key, "Type") == 0) {
            dbus_message_iter_get_basic(&value, &ret);
            message_info->sms_type = ret;
        } else if (strcmp(key, "To") == 0) {
            dbus_message_iter_get_basic(&value, &result);
            message_info->sender = strdup0(result);
        } else if (strcmp(key, "Text") == 0) {
            dbus_message_iter_get_basic(&value, &result);
            message_info->text = strdup0(result);
        } else if (strcmp(key, "Date") == 0) {
            dbus_message_iter_get_basic(&value, &result);
            message_info->sent_time = strdup0(result);
            message_info->local_sent_time = strdup0(result);
        }

        dbus_message_iter_next(&subArrayIter);
    }

    return 1;
}

static void message_list_query_complete(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_message_info message_list[MAX_MESSAGE_LIST_COUNT];
    DBusMessageIter args, list;
    tapi_async_function cb;
    tapi_async_result* ar;
    int message_count = 0;
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
    ar->status = ERROR;

    cb = handler->cb_function;
    if (cb == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return;
    }

    // start to handle response message.
    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("error from message in %s, %s: %s", __func__, err.name, err.message);
        dbus_error_free(&err);
        goto done;
    }

    if (dbus_message_has_signature(message, "a(oa{sv})") == false) {
        tapi_log_error("message signature is invalid in %s", __func__);
        goto done;
    }

    if (dbus_message_iter_init(message, &args) == false) {
        tapi_log_error("message iter init failed in %s", __func__);
        goto done;
    }

    dbus_message_iter_recurse(&args, &list);

    while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_STRUCT) {
        DBusMessageIter entry;

        dbus_message_iter_recurse(&list, &entry);
        if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_OBJECT_PATH) {

            decode_message_info(&entry, message_list + message_count);
            message_count++;
        }

        dbus_message_iter_next(&list);
    }

    ar->arg2 = message_count;
    ar->status = OK;
    ar->data = message_list;

done:
    cb(ar);
    while (--message_count >= 0) {
        free(message_list[message_count].sender);
        free(message_list[message_count].text);
        free(message_list[message_count].sent_time);
        free(message_list[message_count].local_sent_time);
    }
}

int tapi_sms_get_op_code(tapi_context context, int slot_id)
{
    char mcc[MAX_MCC_LENGTH + 1] = { 0 };
    char mnc[MAX_MNC_LENGTH + 1] = { 0 };
    int result;

    result = tapi_network_get_mcc(context, slot_id, mcc, sizeof(mcc));
    if (result != OK) {
        return OP_UNKNOW;
    }
    result = tapi_network_get_mnc(context, slot_id, mnc, sizeof(mnc));
    if (result != OK) {
        return OP_UNKNOW;
    }

    return get_op_code_base_mcc_mnc(mcc, mnc);
}

static void report_data_logging_for_sms(dbus_context* ctx, int slot_id, int sms_type,
    int direction, int fail_flag)
{
    char covered_plmn[MAX_MCC_LENGTH + MAX_MNC_LENGTH + 1] = { '\0' };
    int opcode = tapi_sms_get_op_code(ctx, slot_id);

    tapi_get_coverted_plmn(ctx, slot_id, covered_plmn);

#ifdef CONFIG_TELEPHONY_DFX
    OFONO_DFX_SMS_INFO(opcode, sms_type, direction, fail_flag, covered_plmn);
#else
    tapi_async_result* ar;
    char out_data[LOG_IND_BUF_SIZE] = { 0 };

    if (ctx->logging_over_cb != NULL) {
        ar = calloc(1, sizeof(tapi_async_result));
        if (ar == NULL) {
            tapi_log_error("Memory allocation failed");
        } else {
            snprintf(out_data, LOG_IND_BUF_SIZE, "%s,%d,%d,%d,%d,%s",
                "SMS_INFO", opcode, sms_type, direction, fail_flag, covered_plmn);
            ar->status = OK;
            ar->data = out_data;
            ctx->logging_over_cb(ar);
            free(ar);
        }
    }
#endif
}

int tapi_sms_send_message(tapi_context context, int slot_id, int sms_id,
    char* number, char* text, int event_id, tapi_async_function p_handle)
{
    tapi_async_handler* handler;
    tapi_async_result* ar;
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    message_param* message;

    report_data_logging_for_sms(ctx, slot_id,
        OFONO_SMS_TYPE_UNKNOW, OFONO_SMS_SEND, OFONO_SMS_NORMAL);

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id in %s", __func__);
        return -EINVAL;
    }

    if (number == NULL) {
        tapi_log_error("number in %s is null", __func__);
        return -EINVAL;
    }

    if (text == NULL) {
        tapi_log_error("text in %s is null", __func__);
        return -EINVAL;
    }

    proxy = get_dbus_proxy_by_type(ctx, slot_id, DBUS_PROXY_SMS);
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    message = calloc(1, sizeof(message_param));
    if (message == NULL) {
        tapi_log_error("message in %s is null", __func__);
        return -EINVAL;
    }

    message->number = strdup0(number);
    message->text = strdup0(text);

    ar = calloc(1, sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        message_free(message);
        return -ENOMEM;
    }
    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->arg2 = sms_id;
    ar->data = message;

    handler = calloc(1, sizeof(tapi_async_handler));
    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        message_free(message);
        free(ar);
        return -ENOMEM;
    }
    handler->result = ar;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "SendMessage",
            send_message_param_append, send_sms_callback, handler, handler_free)) {
        tapi_log_error("method call failed in %s", __func__);
        report_data_logging_for_sms(ctx, slot_id, OFONO_CS_SMS,
            OFONO_SMS_SEND, OFONO_SMS_FAIL);
        message_free(message);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_sms_send_data_message(tapi_context context, int slot_id, int sms_id,
    char* dest_addr, unsigned int port, char* text, int event_id, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusProxy* proxy;
    data_message_param* data_message;

    report_data_logging_for_sms(ctx, slot_id, OFONO_SMS_TYPE_UNKNOW,
        OFONO_SMS_SEND, OFONO_SMS_NORMAL);

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id in %s", __func__);
        return -EINVAL;
    }

    if (dest_addr == NULL) {
        tapi_log_error("dest_addr in %s is null", __func__);
        return -EINVAL;
    }

    if (text == NULL) {
        tapi_log_error("text in %s is null", __func__);
        return -EINVAL;
    }

    proxy = get_dbus_proxy_by_type(ctx, slot_id, DBUS_PROXY_SMS);
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    data_message = calloc(1, sizeof(data_message_param));
    if (data_message == NULL) {
        tapi_log_error("data_message in %s is null", __func__);
        return -ENOMEM;
    }

    data_message->dest_addr = strdup0(dest_addr);
    data_message->data = strdup0(text);
    data_message->port = port;

    ar = calloc(1, sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        data_message_free(data_message);
        return -ENOMEM;
    }

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->arg2 = sms_id;
    ar->data = data_message;

    handler = calloc(1, sizeof(tapi_async_handler));
    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        data_message_free(data_message);
        free(ar);
        return -ENOMEM;
    }

    handler->result = ar;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "SendDataMessage",
            send_data_message_param_append, send_sms_callback, handler, handler_free)) {
        tapi_log_error("method call failed in %s", __func__);
        report_data_logging_for_sms(ctx, slot_id, OFONO_IMS_SMS,
            OFONO_SMS_SEND, OFONO_SMS_FAIL);
        data_message_free(data_message);
        handler_free(handler);
        return -EINVAL;
    }

    return OK;
}

bool tapi_sms_set_service_center_address(tapi_context context, int slot_id, char* number)
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
        tapi_log_error("client is not ready in %s", __func__);
        return -EAGAIN;
    }

    if (number == NULL) {
        tapi_log_error("number in %s is null", __func__);
        return -EINVAL;
    }

    proxy = get_dbus_proxy_by_type(ctx, slot_id, DBUS_PROXY_SMS);
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (!g_dbus_proxy_set_property_basic(proxy, "ServiceCenterAddress",
            DBUS_TYPE_STRING, &number, NULL, NULL, NULL)) {
        tapi_log_error("set property failed in %s", __func__);
        return -EINVAL;
    }

    return OK;
}

int tapi_sms_get_service_center_address(tapi_context context, int slot_id, char* out, int length)
{
    dbus_context* ctx = context;
    char* value = NULL;
    GDBusProxy* proxy;

    if (ctx == NULL || out == NULL || length <= 0) {
        tapi_log_error("context or out is null or length <= 0 in %s", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id in %s", __func__);
        return -EINVAL;
    }

    proxy = get_dbus_proxy_by_type(ctx, slot_id, DBUS_PROXY_SMS);
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (!g_dbus_proxy_get_property_basic(proxy, "ServiceCenterAddress", &value)) {
        tapi_log_error("get property failed in %s", __func__);
        return -EINVAL;
    }

    if (value != NULL) {
        strlcpy(out, value, length);
    } else {
        out[0] = '\0';
    }

    return OK;
}

int tapi_sms_enable_delivery_report(tapi_context context, int slot_id, bool enable)
{
    dbus_context* ctx = context;
    int value = enable;
    GDBusProxy* proxy;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = get_dbus_proxy_by_type(ctx, slot_id, DBUS_PROXY_SMS);
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (!g_dbus_proxy_set_property_basic(proxy, "UseDeliveryReports",
            DBUS_TYPE_BOOLEAN, &value, NULL, NULL, NULL)) {
        tapi_log_error("set property failed in %s", __func__);
        return -EINVAL;
    }

    return OK;
}

int tapi_sms_get_delivery_report_status(tapi_context context, int slot_id, bool* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    int result;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = get_dbus_proxy_by_type(ctx, slot_id, DBUS_PROXY_SMS);
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (g_dbus_proxy_get_property_basic(proxy, "UseDeliveryReports", &result)) {
        *out = result;
        return OK;
    }

    tapi_log_error("get property failed in %s", __func__);
    return -EINVAL;
}

int tapi_sms_get_all_messages_from_sim(tapi_context context, int slot_id,
    tapi_message_list* list, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* user_data;
    tapi_async_result* ar;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id in %s", __func__);
        return -EINVAL;
    }

    proxy = get_dbus_proxy_by_type(ctx, slot_id, DBUS_PROXY_CALL);
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    user_data = calloc(1, sizeof(tapi_async_handler));
    if (user_data == NULL) {
        tapi_log_error("user data in %s is null", __func__);
        return -ENOMEM;
    }

    user_data->cb_function = p_handle;
    ar = calloc(1, sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        free(user_data);
        return -ENOMEM;
    }

    user_data->result = ar;
    ar->arg1 = slot_id;
    ar->data = list;

    if (!g_dbus_proxy_method_call(proxy, "GetAllMessagesFromSim", NULL,
            message_list_query_complete, user_data, handler_free)) {
        tapi_log_error("method call failed in %s", __func__);
        handler_free(user_data);
        return -EINVAL;
    }

    return OK;
}

int tapi_sms_copy_message_to_sim(tapi_context context, int slot_id,
    char* number, char* text, char* send_time, int type)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_message_info* message_info;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id in %s", __func__);
        return -EINVAL;
    }

    if (number == NULL) {
        tapi_log_error("number in %s is null", __func__);
        return -EINVAL;
    }

    if (text == NULL) {
        tapi_log_error("text in %s is null", __func__);
        return -EINVAL;
    }

    if (send_time == NULL) {
        tapi_log_error("send_time in %s is null", __func__);
        return -EINVAL;
    }

    proxy = get_dbus_proxy_by_type(ctx, slot_id, DBUS_PROXY_SMS);
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    message_info = calloc(1, sizeof(tapi_message_info));
    if (message_info == NULL) {
        tapi_log_error("message_info in %s is null", __func__);
        return -ENOMEM;
    }

    message_info->text = strdup0(text);
    message_info->sender = strdup0(number);
    message_info->sent_time = strdup0(send_time);
    message_info->sms_type = type;

    if (!g_dbus_proxy_method_call(proxy, "InsertMessageToSim", copy_message_param_append,
            no_operate_callback, message_info, message_info_free)) {
        tapi_log_error("method call failed in %s", __func__);
        message_info_free(message_info);
        return -EINVAL;
    }

    return OK;
}

int tapi_sms_delete_message_from_sim(tapi_context context, int slot_id, int index)
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

    if (index < 0) {
        tapi_log_error("invalid index in %s", __func__);
        return -EINVAL;
    }

    proxy = get_dbus_proxy_by_type(ctx, slot_id, DBUS_PROXY_SMS);
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (!g_dbus_proxy_method_call(proxy, "DeleteMessageFromSim",
            delete_message_param_append, no_operate_callback, (void*)(intptr_t)index, NULL)) {
        tapi_log_error("method call failed in %s", __func__);
        return -EINVAL;
    }

    return OK;
}

int tapi_sms_register(tapi_context context, int slot_id,
    tapi_indication_msg msg_type, void* user_obj, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    const char* path;
    tapi_async_handler* user_data;
    tapi_async_result* ar;
    int watch_id = 0;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id in %s", __func__);
        return -EINVAL;
    }

    if (msg_type < MSG_INCOMING_MESSAGE_IND || msg_type > MSG_SMS_REPORT_SWITCH_CHANGED_IND) {
        tapi_log_error("invalid msg type in %s, msg_type: %d", __func__, msg_type);
        return -EINVAL;
    }

    path = tapi_utils_get_modem_path(slot_id);
    if (path == NULL) {
        tapi_log_error("no available modem in %s", __func__);
        return -EIO;
    }

    user_data = calloc(1, sizeof(tapi_async_handler));
    if (user_data == NULL) {
        tapi_log_error("user data in %s is null", __func__);
        return -ENOMEM;
    }

    user_data->cb_function = p_handle;
    ar = calloc(1, sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        free(user_data);
        return -ENOMEM;
    }

    user_data->result = ar;
    ar->msg_id = msg_type;
    ar->msg_type = INDICATION;
    ar->arg1 = slot_id;
    ar->user_obj = user_obj;

    switch (msg_type) {
    case MSG_INCOMING_MESSAGE_IND:
        watch_id = dbus_client_add_signal_watch(ctx->client, OFONO_SERVICE, path,
            OFONO_MESSAGE_MANAGER_INTERFACE, "IncomingMessage",
            unsol_sms_message, user_data, handler_free);
        break;
    case MSG_IMMEDIATE_MESSAGE_IND:
        watch_id = dbus_client_add_signal_watch(ctx->client, OFONO_SERVICE, path,
            OFONO_MESSAGE_MANAGER_INTERFACE, "ImmediateMessage",
            unsol_sms_message, user_data, handler_free);
        break;
    case MSG_STATUS_REPORT_MESSAGE_IND:
        watch_id = dbus_client_add_signal_watch(ctx->client, OFONO_SERVICE, path,
            OFONO_MESSAGE_MANAGER_INTERFACE, "StatusReportMessage",
            unsol_sms_message, user_data, handler_free);
        break;
    case MSG_DEFAULT_SMS_SLOT_CHANGED_IND:
        watch_id = dbus_client_add_signal_watch(ctx->client,
            OFONO_SERVICE, OFONO_MANAGER_PATH, OFONO_MANAGER_INTERFACE,
            "PropertyChanged", sms_property_changed, user_data, handler_free);
        break;
    case MSG_SMS_REPORT_SWITCH_CHANGED_IND:
        watch_id = dbus_client_add_signal_watch(ctx->client, OFONO_SERVICE, path,
            OFONO_MESSAGE_MANAGER_INTERFACE, "PropertyChanged", sms_property_changed, user_data, handler_free);
    default:
        break;
    }

    if (watch_id == 0) {
        tapi_log_error("watch id is 0 in %s, msg_id: %d", __func__, (int)msg_type);
        handler_free(user_data);
        return -EINVAL;
    }

    return watch_id;
}

int tapi_sms_set_default_slot(tapi_context context, int slot_id)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    const char* modem_path;

    proxy = ctx->dbus_proxy_manager;
    if (proxy == NULL) {
        tapi_log_error("proxy in %s is null", __func__);
        return -EIO;
    }

    if (!ctx->client_ready) {
        tapi_log_error("client is not ready in %s", __func__);
        return -EAGAIN;
    }

    if (!tapi_is_valid_slotid(slot_id) && slot_id != -1) {
        tapi_log_error("invalid slot id in %s", __func__);
        return -EINVAL;
    }

    modem_path = tapi_utils_get_modem_path(slot_id);
    if (!g_dbus_proxy_set_property_basic(proxy,
            "SmsSlot", DBUS_TYPE_STRING, &modem_path, NULL, NULL, NULL)) {
        tapi_log_error("set property failed in %s", __func__);
        return -EINVAL;
    }

    return OK;
}

int tapi_sms_get_default_slot(tapi_context context, int* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    char* modem_path;

    proxy = ctx->dbus_proxy_manager;
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (!ctx->client_ready) {
        tapi_log_error("client is not ready in %s", __func__);
        return -EAGAIN;
    }

    if (g_dbus_proxy_get_property_basic(proxy, "SmsSlot", &modem_path)) {
        *out = tapi_utils_get_slot_id(modem_path);
        return OK;
    }

    tapi_log_error("get property failed in %s", __func__);
    return -EINVAL;
}

int tapi_sms_unregister(tapi_context context, int watch_id)
{
    dbus_context* ctx = context;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (watch_id <= 0) {
        tapi_log_error("invalid watch id in %s, watch_id: %d", __func__, watch_id);
        return -EINVAL;
    }

    if (!dbus_client_remove_watch(ctx->client, watch_id)) {
        tapi_log_error("remove watch failed in %s", __func__);
        return -EINVAL;
    }

    return OK;
}
