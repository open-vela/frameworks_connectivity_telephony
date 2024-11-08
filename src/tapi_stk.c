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
#include "tapi_stk.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STKAGENT_ERROR "org.ofono.stkagent.Error"
#define OFONO_ERROR_INTERFACE "org.ofono.Error"

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/
typedef struct {
    unsigned char item;
    char* agent_id;
} stk_select_item_param;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* This method should be called if the data field needs to be recycled. */
static void stk_event_data_free(void* user_data);
static void method_call_complete(DBusMessage* message, void* user_data);
static bool stk_agent_dbus_pending_reply(DBusConnection* conn,
    DBusMessage** msg, DBusMessage* reply);
static void stk_agent_register_param_append(DBusMessageIter* iter, void* user_data);
static void stk_select_item_param_append(DBusMessageIter* iter, void* user_data);
static DBusMessage* stk_agent_error_invalid_args(DBusMessage* msg);
static DBusMessage* stk_agent_error_failed(DBusMessage* msg);
static DBusMessage* stk_agent_error_not_implemented(DBusMessage* msg);
static DBusMessage* stk_agent_error_end_session(DBusMessage* msg);
static DBusMessage* stk_agent_error_go_back(DBusMessage* msg);
static DBusMessage* stk_agent_error_busy(DBusMessage* msg);
static DBusMessage* stk_agent_release(DBusConnection* conn,
    DBusMessage* msg, void* user_data);
static DBusMessage* stk_agent_cancel(DBusConnection* conn,
    DBusMessage* msg, void* user_data);
static DBusMessage* stk_agent_show_information(DBusConnection* conn,
    DBusMessage* msg, void* user_data);
static DBusMessage* stk_agent_handle_request_digit(DBusConnection* conn,
    DBusMessage* msg, void* user_data);
static DBusMessage* stk_agent_handle_request_key(DBusConnection* conn,
    DBusMessage* msg, void* user_data);
static DBusMessage* stk_agent_handle_request_confirmation(DBusConnection* conn,
    DBusMessage* msg, void* user_data);
static DBusMessage* stk_agent_handle_request_input(DBusConnection* conn,
    DBusMessage* msg, void* user_data);
static DBusMessage* stk_agent_handle_request_digits(DBusConnection* conn,
    DBusMessage* msg, void* user_data);
static DBusMessage* stk_agent_handle_play_tone(DBusConnection* conn,
    DBusMessage* msg, void* user_data);
static DBusMessage* stk_agent_handle_loop_tone(DBusConnection* conn,
    DBusMessage* msg, void* user_data);
static DBusMessage* stk_agent_handle_request_selection(DBusConnection* conn,
    DBusMessage* msg, void* user_data);
static DBusMessage* stk_agent_handle_request_quick_digit(DBusConnection* conn,
    DBusMessage* msg, void* user_data);
static DBusMessage* stk_agent_confirm_call_setup(DBusConnection* conn,
    DBusMessage* msg, void* user_data);
static DBusMessage* stk_agent_display_action_information(DBusConnection* conn,
    DBusMessage* msg, void* user_data);
static DBusMessage* stk_agent_confirm_launch_browser(DBusConnection* conn,
    DBusMessage* msg, void* user_data);
static DBusMessage* stk_agent_display_action(DBusConnection* conn,
    DBusMessage* msg, void* user_data);
static DBusMessage* stk_agent_confirm_open_channel(DBusConnection* conn,
    DBusMessage* msg, void* user_data);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const GDBusMethodTable agent_methods[] = {
    { GDBUS_METHOD("AgentRelease", NULL, NULL, stk_agent_release) },
    { GDBUS_ASYNC_METHOD("ConfirmCallSetup",
        GDBUS_ARGS({ "info", "s" }, { "icon_id", "y" }),
        GDBUS_ARGS({ "confirm", "b" }), stk_agent_confirm_call_setup) },
    { GDBUS_ASYNC_METHOD("ConfirmLaunchBrowser",
        GDBUS_ARGS({ "info", "s" }, { "icon_id", "y" }, { "url", "s" }),
        GDBUS_ARGS({ "confirm", "b" }), stk_agent_confirm_launch_browser) },
    { GDBUS_ASYNC_METHOD("ConfirmOpenChannel",
        GDBUS_ARGS({ "info", "s" }, { "icon_id", "y" }),
        GDBUS_ARGS({ "confirm", "b" }), stk_agent_confirm_open_channel) },
    { GDBUS_ASYNC_METHOD("ConfirmRequest",
        GDBUS_ARGS({ "alpha", "s" }, { "icon_id", "y" }),
        GDBUS_ARGS({ "confirmation", "b" }), stk_agent_handle_request_confirmation) },
    { GDBUS_ASYNC_METHOD("DisplayAction",
        GDBUS_ARGS({ "info", "s" }, { "icon_id", "y" }),
        NULL, stk_agent_display_action) },
    { GDBUS_ASYNC_METHOD("DisplayActionInformation",
        GDBUS_ARGS({ "info", "s" }, { "icon_id", "y" }),
        NULL, stk_agent_display_action_information) },
    { GDBUS_ASYNC_METHOD("RequestLoopTone",
        GDBUS_ARGS({ "tone", "s" }, { "info", "s" }, { "icon_id", "y" }),
        NULL, stk_agent_handle_loop_tone) },
    { GDBUS_ASYNC_METHOD("RequestPlayTone",
        GDBUS_ARGS({ "tone", "s" }, { "info", "s" }, { "icon_id", "y" }),
        NULL, stk_agent_handle_play_tone) },
    { GDBUS_ASYNC_METHOD("RequestQuickDigit",
        GDBUS_ARGS({ "alpha", "s" }, { "icon_id", "y" }),
        GDBUS_ARGS({ "digit", "s" }), stk_agent_handle_request_quick_digit) },
    { GDBUS_ASYNC_METHOD("RequestSelection",
        GDBUS_ARGS({ "title", "s" }, { "icon_id", "y" },
            { "items", "a(sy)" }, { "default", "n" }),
        GDBUS_ARGS({ "selection", "y" }), stk_agent_handle_request_selection) },
    { GDBUS_ASYNC_METHOD("RequestWithDigit",
        GDBUS_ARGS({ "alpha", "s" }, { "icon_id", "y" }),
        GDBUS_ARGS({ "digit", "s" }), stk_agent_handle_request_digit) },
    { GDBUS_ASYNC_METHOD("RequestWithDigits",
        GDBUS_ARGS({ "alpha", "s" }, { "icon_id", "y" },
            { "default", "s" }, { "min_len", "y" },
            { "max_len", "y" }, { "hide_typing", "b" }),
        GDBUS_ARGS({ "digits", "s" }), stk_agent_handle_request_digits) },
    { GDBUS_ASYNC_METHOD("RequestWithInput",
        GDBUS_ARGS({ "alpha", "s" }, { "icon_id", "y" },
            { "default", "s" }, { "min_len", "y" },
            { "max_len", "y" }, { "hide_typing", "b" }),
        GDBUS_ARGS({ "input", "s" }), stk_agent_handle_request_input) },
    { GDBUS_ASYNC_METHOD("RequestWithKey",
        GDBUS_ARGS({ "alpha", "s" }, { "icon_id", "y" }),
        GDBUS_ARGS({ "key", "s" }), stk_agent_handle_request_key) },
    { GDBUS_ASYNC_METHOD("ShowInformation",
        GDBUS_ARGS({ "info", "s" }, { "icon_id", "y" }, { "urgent", "b" }),
        NULL, stk_agent_show_information) },
    { GDBUS_NOREPLY_METHOD("Cancel", NULL, NULL, stk_agent_cancel) },
    {},
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* This method should be called if the data field needs to be recycled. */
static void stk_event_data_free(void* user_data)
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
        tapi_log_error("error from message in %s, error %s: %s",
            __func__, err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
    }

    ar->status = OK;

done:
    cb(ar);
}

static bool stk_agent_dbus_pending_reply(DBusConnection* conn,
    DBusMessage** msg, DBusMessage* reply)
{
    if (conn == NULL) {
        tapi_log_error("conn in %s is null", __func__);
        return false;
    }

    if (msg == NULL) {
        tapi_log_error("msg in %s is null", __func__);
        return false;
    }

    if (reply == NULL) {
        tapi_log_error("reply in %s is null", __func__);
        return false;
    }

    g_dbus_send_message(conn, reply);

    dbus_message_unref(*msg);
    *msg = NULL;

    return true;
}

static void stk_agent_register_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    char* agent_id;

    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        return;
    }

    if (param->result == NULL) {
        tapi_log_error("param result in %s is null", __func__);
        return;
    }

    agent_id = param->result->data;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &agent_id);
}

static void stk_select_item_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    stk_select_item_param* select_item_param;
    unsigned char item;
    char* agent_id;

    if (param == NULL) {
        tapi_log_error("param in %s is null", __func__);
        return;
    }

    if (param->result == NULL) {
        tapi_log_error("param result in %s is null", __func__);
        return;
    }

    select_item_param = param->result->data;
    item = select_item_param->item;
    agent_id = select_item_param->agent_id;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_BYTE, &item);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &agent_id);
}

static DBusMessage* stk_agent_error_invalid_args(DBusMessage* msg)
{
    return g_dbus_create_error(msg, STKAGENT_ERROR ".InvalidArguments",
        "Invalid arguments provided");
}

static DBusMessage* stk_agent_error_failed(DBusMessage* msg)
{
    return g_dbus_create_error(msg, STKAGENT_ERROR ".Failed",
        "Operation failed");
}

static DBusMessage* stk_agent_error_not_implemented(DBusMessage* msg)
{
    return g_dbus_create_error(msg, OFONO_ERROR_INTERFACE ".NotImplemented",
        "Implementation not provided");
}

static DBusMessage* stk_agent_error_end_session(DBusMessage* msg)
{
    return g_dbus_create_error(msg, OFONO_ERROR_INTERFACE ".EndSession",
        "End Session Request");
}

static DBusMessage* stk_agent_error_go_back(DBusMessage* msg)
{
    return g_dbus_create_error(msg, OFONO_ERROR_INTERFACE ".GoBack",
        "Go Back Request");
}

static DBusMessage* stk_agent_error_busy(DBusMessage* msg)
{
    return g_dbus_create_error(msg, OFONO_ERROR_INTERFACE ".Busy",
        "UI Busy");
}

static DBusMessage* stk_agent_release(DBusConnection* conn,
    DBusMessage* msg, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessage* reply;
    dbus_context* ctx;

    tapi_log_info("stk agent release method is called\n");

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    reply = dbus_message_new_method_return(msg);
    ctx = ar->data;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_failed(msg);
        goto done;
    }

    if (ctx->pending) {
        dbus_message_unref(ctx->pending);
        ctx->pending = NULL;
    }

    ar->status = OK;
    ar->data = NULL;
    ar->msg_id = MSG_STK_AGENT_RELEASE_IND;

done:
    cb(ar);

    return reply;
}

static DBusMessage* stk_agent_cancel(DBusConnection* conn,
    DBusMessage* msg, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessage* reply;
    dbus_context* ctx;

    tapi_log_info("stk agent cancel method is called\n");

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    reply = NULL;
    ctx = ar->data;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_failed(msg);
        goto done;
    }

    if (ctx->pending) {
        tapi_log_error("context in %s is busy", __func__);
        dbus_message_unref(ctx->pending);
        ctx->pending = NULL;
    }

    ar->status = OK;
    ar->data = NULL;
    ar->msg_id = MSG_STK_AGENT_CANCEL_IND;

done:
    cb(ar);

    return reply;
}

static DBusMessage* stk_agent_show_information(DBusConnection* conn,
    DBusMessage* msg, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_stk_display_info_params* params;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessage* reply;
    dbus_context* ctx;

    tapi_log_info("stk agent display text method is called\n");

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    ar->msg_id = MSG_STK_AGENT_DISPLAY_TEXT_IND;

    params = NULL;
    reply = NULL;
    ctx = ar->data;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_failed(msg);
        goto done;
    }

    if (ctx->pending) {
        tapi_log_error("context in %s is busy", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_busy(msg);
        goto done;
    }

    params = malloc(sizeof(tapi_stk_display_info_params));
    if (params == NULL) {
        tapi_log_error("params in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_not_implemented(msg);
        goto done;
    }

    if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &params->info,
            DBUS_TYPE_BYTE, &params->icon_id,
            DBUS_TYPE_BOOLEAN, &params->urgent,
            DBUS_TYPE_INVALID)) {
        tapi_log_error("failed to get args in %s", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_invalid_args(msg);
        goto done;
    }

    ctx->pending = dbus_message_ref(msg);
    ar->data = params;
    ar->status = OK;

done:
    cb(ar);
    if (params != NULL) {
        free(params);
    }

    return reply;
}

static DBusMessage* stk_agent_handle_request_digit(DBusConnection* conn,
    DBusMessage* msg, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_stk_request_key_params* params;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessage* reply;
    dbus_context* ctx;

    tapi_log_info("stk agent request digit method is called\n");

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    ar->msg_id = MSG_STK_AGENT_REQUEST_DIGIT_IND;

    params = NULL;
    reply = NULL;
    ctx = ar->data;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_failed(msg);
        goto done;
    }

    if (ctx->pending) {
        tapi_log_error("context in %s is busy", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_busy(msg);
        goto done;
    }

    params = malloc(sizeof(tapi_stk_request_key_params));
    if (params == NULL) {
        tapi_log_error("params in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_not_implemented(msg);
        goto done;
    }

    if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &params->alpha,
            DBUS_TYPE_BYTE, &params->icon_id,
            DBUS_TYPE_INVALID)) {
        tapi_log_error("failed to get args in %s", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_invalid_args(msg);
        goto done;
    }

    ctx->pending = dbus_message_ref(msg);
    ar->data = params;
    ar->status = OK;

done:
    cb(ar);
    if (params != NULL) {
        free(params);
    }

    return reply;
}

static DBusMessage* stk_agent_handle_request_key(DBusConnection* conn,
    DBusMessage* msg, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_stk_request_key_params* params;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessage* reply;
    dbus_context* ctx;

    tapi_log_info("stk agent request key method is called\n");

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    ar->msg_id = MSG_STK_AGENT_REQUEST_KEY_IND;

    params = NULL;
    reply = NULL;
    ctx = ar->data;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_failed(msg);
        goto done;
    }

    if (ctx->pending) {
        tapi_log_error("context in %s is busy", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_busy(msg);
        goto done;
    }

    params = malloc(sizeof(tapi_stk_request_key_params));
    if (params == NULL) {
        tapi_log_error("params in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_not_implemented(msg);
        goto done;
    }

    if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &params->alpha,
            DBUS_TYPE_BYTE, &params->icon_id,
            DBUS_TYPE_INVALID)) {
        tapi_log_error("failed to get args in %s", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_invalid_args(msg);
        goto done;
    }

    ctx->pending = dbus_message_ref(msg);
    ar->data = params;
    ar->status = OK;

done:
    cb(ar);
    if (params != NULL) {
        free(params);
    }

    return reply;
}

static DBusMessage* stk_agent_handle_request_confirmation(DBusConnection* conn,
    DBusMessage* msg, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_stk_request_key_params* params;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessage* reply;
    dbus_context* ctx;

    tapi_log_info("stk agent request confirmation is method\n");

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    ar->msg_id = MSG_STK_AGENT_REQUEST_CONFIRMATION_IND;

    params = NULL;
    reply = NULL;
    ctx = ar->data;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_failed(msg);
        goto done;
    }

    if (ctx->pending) {
        tapi_log_error("context in %s is busy", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_busy(msg);
        goto done;
    }

    params = malloc(sizeof(tapi_stk_request_key_params));
    if (params == NULL) {
        tapi_log_error("params in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_not_implemented(msg);
        goto done;
    }

    if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &params->alpha,
            DBUS_TYPE_BYTE, &params->icon_id,
            DBUS_TYPE_INVALID)) {
        tapi_log_error("failed to get args in %s", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_invalid_args(msg);
        goto done;
    }

    ctx->pending = dbus_message_ref(msg);
    ar->data = params;
    ar->status = OK;

done:
    cb(ar);
    if (params != NULL) {
        free(params);
    }

    return reply;
}

static DBusMessage* stk_agent_handle_request_input(DBusConnection* conn,
    DBusMessage* msg, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_stk_request_input_params* params;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessage* reply;
    dbus_context* ctx;

    tapi_log_info("stk agent request input method is called\n");

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    ar->msg_id = MSG_STK_AGENT_REQUEST_INPUT_IND;

    params = NULL;
    reply = NULL;
    ctx = ar->data;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_failed(msg);
        goto done;
    }

    if (ctx->pending) {
        tapi_log_error("context in %s is busy", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_busy(msg);
        goto done;
    }

    params = malloc(sizeof(tapi_stk_request_input_params));
    if (params == NULL) {
        tapi_log_error("params in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_not_implemented(msg);
        goto done;
    }

    if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &params->alpha,
            DBUS_TYPE_BYTE, &params->icon_id,
            DBUS_TYPE_STRING, &params->def_input,
            DBUS_TYPE_BYTE, &params->min_len,
            DBUS_TYPE_BYTE, &params->max_len,
            DBUS_TYPE_BOOLEAN, &params->hide_typing,
            DBUS_TYPE_INVALID)) {
        tapi_log_error("failed to get args in %s", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_invalid_args(msg);
        goto done;
    }

    ctx->pending = dbus_message_ref(msg);
    ar->data = params;
    ar->status = OK;

done:
    cb(ar);
    if (params != NULL) {
        free(params);
    }

    return reply;
}

static DBusMessage* stk_agent_handle_request_digits(DBusConnection* conn,
    DBusMessage* msg, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_stk_request_input_params* params;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessage* reply;
    dbus_context* ctx;

    tapi_log_info("stk agent request input digits is called\n");

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    ar->msg_id = MSG_STK_AGENT_REQUEST_DIGITS_IND;

    params = NULL;
    reply = NULL;
    ctx = ar->data;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_failed(msg);
        goto done;
    }

    if (ctx->pending) {
        tapi_log_error("context in %s is busy", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_busy(msg);
        goto done;
    }

    params = malloc(sizeof(tapi_stk_request_input_params));
    if (params == NULL) {
        tapi_log_error("params in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_not_implemented(msg);
        goto done;
    }

    if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &params->alpha,
            DBUS_TYPE_BYTE, &params->icon_id,
            DBUS_TYPE_STRING, &params->def_input,
            DBUS_TYPE_BYTE, &params->min_len,
            DBUS_TYPE_BYTE, &params->max_len,
            DBUS_TYPE_BOOLEAN, &params->hide_typing,
            DBUS_TYPE_INVALID)) {
        tapi_log_error("failed to get args in %s", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_invalid_args(msg);
        goto done;
    }

    ctx->pending = dbus_message_ref(msg);
    ar->data = params;
    ar->status = OK;

done:
    cb(ar);
    if (params != NULL) {
        free(params);
    }

    return reply;
}

static DBusMessage* stk_agent_handle_play_tone(DBusConnection* conn,
    DBusMessage* msg, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_stk_play_tone_params* params;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessage* reply;
    dbus_context* ctx;

    tapi_log_info("stk agent play tone method is called\n");

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    ar->msg_id = MSG_STK_AGENT_PLAY_TONE_IND;

    params = NULL;
    reply = NULL;
    ctx = ar->data;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_failed(msg);
        goto done;
    }

    if (ctx->pending) {
        tapi_log_error("context in %s is busy", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_busy(msg);
        goto done;
    }

    params = malloc(sizeof(tapi_stk_play_tone_params));
    if (params == NULL) {
        tapi_log_error("params in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_not_implemented(msg);
        goto done;
    }

    if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &params->tone,
            DBUS_TYPE_STRING, &params->info,
            DBUS_TYPE_BYTE, &params->icon_id,
            DBUS_TYPE_INVALID)) {
        tapi_log_error("failed to get args in %s", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_invalid_args(msg);
        goto done;
    }

    ctx->pending = dbus_message_ref(msg);
    ar->data = params;
    ar->status = OK;

done:
    cb(ar);
    if (params != NULL) {
        free(params);
    }

    return reply;
}

static DBusMessage* stk_agent_handle_loop_tone(DBusConnection* conn,
    DBusMessage* msg, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_stk_play_tone_params* params;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessage* reply;
    dbus_context* ctx;

    tapi_log_info("stk agent loop tone method is called\n");

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    ar->msg_id = MSG_STK_AGENT_LOOP_TONE_IND;

    params = NULL;
    reply = NULL;
    ctx = ar->data;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_failed(msg);
        goto done;
    }

    if (ctx->pending) {
        tapi_log_error("context in %s is busy", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_busy(msg);
        goto done;
    }

    params = malloc(sizeof(tapi_stk_play_tone_params));
    if (params == NULL) {
        tapi_log_error("params in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_not_implemented(msg);
        goto done;
    }

    if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &params->tone,
            DBUS_TYPE_STRING, &params->info,
            DBUS_TYPE_BYTE, &params->icon_id,
            DBUS_TYPE_INVALID)) {
        tapi_log_error("failed to get args in %s", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_invalid_args(msg);
        goto done;
    }

    ctx->pending = dbus_message_ref(msg);
    ar->data = params;
    ar->status = OK;

done:
    cb(ar);
    if (params != NULL) {
        free(params);
    }

    return reply;
}

static DBusMessage* stk_agent_handle_request_selection(DBusConnection* conn,
    DBusMessage* msg, void* user_data)
{
    tapi_stk_request_selection_params* params;
    tapi_async_handler* handler = user_data;
    DBusMessageIter iter, array, entry;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessage* reply;
    dbus_context* ctx;
    DBusError err;
    unsigned char icon_id;
    char* text;
    int index;

    tapi_log_info("stk agent request selection method is called\n");

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    ar->msg_id = MSG_STK_AGENT_REQUEST_SELECTION_IND;

    params = NULL;
    reply = NULL;
    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, msg) == true) {
        tapi_log_error("error from message in %s, %s: %s", __func__, err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        reply = stk_agent_error_failed(msg);
        goto done;
    }

    if (dbus_message_iter_init(msg, &iter) == false) {
        tapi_log_error("dbus message iter init failed in %s", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_invalid_args(msg);
        goto done;
    }

    ctx = ar->data;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_failed(msg);
        goto done;
    }

    if (ctx->pending) {
        tapi_log_error("context in %s is busy", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_busy(msg);
        goto done;
    }

    params = malloc(sizeof(tapi_stk_request_selection_params));
    if (params == NULL) {
        tapi_log_error("params in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_not_implemented(msg);
        goto done;
    }

    dbus_message_iter_get_basic(&iter, &params->alpha);
    dbus_message_iter_next(&iter);

    dbus_message_iter_get_basic(&iter, &params->icon_id);
    dbus_message_iter_next(&iter);

    index = 0;
    dbus_message_iter_recurse(&iter, &array);
    if (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_ARRAY) {

        dbus_message_iter_recurse(&array, &entry);

        while (dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_INVALID) {

            if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_STRUCT) {

                dbus_message_iter_get_basic(&entry, &text);
                dbus_message_iter_next(&entry);

                dbus_message_iter_get_basic(&entry, &icon_id);

                snprintf(params->items[index++].text, sizeof(params->items[index++].text), "%s", text);
                params->items[index++].icon_id = icon_id;
            }

            if (index >= MAX_STK_MAIN_MENU_LENGTH)
                break;

            dbus_message_iter_next(&array);
            dbus_message_iter_recurse(&array, &entry);
        }
    }
    dbus_message_iter_next(&iter);

    dbus_message_iter_get_basic(&iter, &params->default_item);

    ctx->pending = dbus_message_ref(msg);
    ar->data = params;
    ar->status = OK;

done:
    cb(ar);
    if (params != NULL) {
        free(params);
    }

    return reply;
}

static DBusMessage* stk_agent_handle_request_quick_digit(DBusConnection* conn,
    DBusMessage* msg, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_stk_request_key_params* params;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessage* reply;
    dbus_context* ctx;

    tapi_log_info("stk agent request quick digit method is called\n");

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    ar->msg_id = MSG_STK_AGENT_REQUEST_QUICK_DIGIT_IND;

    params = NULL;
    reply = NULL;
    ctx = ar->data;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_failed(msg);
        goto done;
    }

    if (ctx->pending) {
        tapi_log_error("context in %s is busy", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_busy(msg);
        goto done;
    }

    params = malloc(sizeof(tapi_stk_request_key_params));
    if (params == NULL) {
        tapi_log_error("params in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_not_implemented(msg);
        goto done;
    }

    if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &params->alpha,
            DBUS_TYPE_BYTE, &params->icon_id,
            DBUS_TYPE_INVALID)) {
        tapi_log_error("dbus message get args failed in %s", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_invalid_args(msg);
        goto done;
    }

    ctx->pending = dbus_message_ref(msg);
    ar->data = params;
    ar->status = OK;

done:
    cb(ar);
    if (params != NULL) {
        free(params);
    }

    return reply;
}

static DBusMessage* stk_agent_confirm_call_setup(DBusConnection* conn,
    DBusMessage* msg, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_stk_request_key_params* params;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessage* reply;
    dbus_context* ctx;

    tapi_log_info("stk agent confirm call setup method is called\n");

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    ar->msg_id = MSG_STK_AGENT_CONFIRM_CALL_SETUP_IND;

    params = NULL;
    reply = NULL;
    ctx = ar->data;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_failed(msg);
        goto done;
    }

    if (ctx->pending) {
        tapi_log_error("context in %s is busy", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_busy(msg);
        goto done;
    }

    params = malloc(sizeof(tapi_stk_request_key_params));
    if (params == NULL) {
        tapi_log_error("params in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_not_implemented(msg);
        goto done;
    }

    if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &params->alpha,
            DBUS_TYPE_BYTE, &params->icon_id,
            DBUS_TYPE_INVALID)) {
        tapi_log_error("dbus message get args failed in %s", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_invalid_args(msg);
        goto done;
    }

    ctx->pending = dbus_message_ref(msg);
    ar->data = params;
    ar->status = OK;

done:
    cb(ar);
    if (params != NULL) {
        free(params);
    }

    return reply;
}

static DBusMessage* stk_agent_display_action_information(DBusConnection* conn,
    DBusMessage* msg, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_stk_display_info_params* params;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessage* reply;
    dbus_context* ctx;

    tapi_log_info("stk agent display action information method is called\n");

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    ar->msg_id = MSG_STK_AGENT_DISPLAY_ACTION_INFORMATION_IND;

    params = NULL;
    reply = NULL;
    ctx = ar->data;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_failed(msg);
        goto done;
    }

    if (ctx->pending) {
        tapi_log_error("context in %s is busy", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_busy(msg);
        goto done;
    }

    params = malloc(sizeof(tapi_stk_display_info_params));
    if (params == NULL) {
        tapi_log_error("params in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_not_implemented(msg);
        goto done;
    }

    if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &params->info,
            DBUS_TYPE_BYTE, &params->icon_id,
            DBUS_TYPE_INVALID)) {
        tapi_log_error("dbus message get args failed in %s", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_invalid_args(msg);
        goto done;
    }

    ctx->pending = dbus_message_ref(msg);
    ar->data = params;
    ar->status = OK;

done:
    cb(ar);
    if (params != NULL) {
        free(params);
    }

    return reply;
}

static DBusMessage* stk_agent_confirm_launch_browser(DBusConnection* conn,
    DBusMessage* msg, void* user_data)
{
    tapi_stk_confirm_launch_browser_params* params;
    tapi_async_handler* handler = user_data;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessage* reply;
    dbus_context* ctx;

    tapi_log_info("stk agent confirm launch browser method is called\n");

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    ar->msg_id = MSG_STK_AGENT_CONFIRM_LAUNCH_BROWSER_IND;

    params = NULL;
    reply = NULL;
    ctx = ar->data;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_failed(msg);
        goto done;
    }

    if (ctx->pending) {
        tapi_log_error("context in %s is busy", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_busy(msg);
        goto done;
    }

    params = malloc(sizeof(tapi_stk_confirm_launch_browser_params));
    if (params == NULL) {
        tapi_log_error("params in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_not_implemented(msg);
        goto done;
    }

    if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &params->info,
            DBUS_TYPE_BYTE, &params->icon_id,
            DBUS_TYPE_STRING, &params->url,
            DBUS_TYPE_INVALID)) {
        tapi_log_error("dbus message get args failed in %s", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_invalid_args(msg);
        goto done;
    }

    ctx->pending = dbus_message_ref(msg);
    ar->data = params;
    ar->status = OK;

done:
    cb(ar);
    if (params != NULL) {
        free(params);
    }

    return reply;
}

static DBusMessage* stk_agent_display_action(DBusConnection* conn,
    DBusMessage* msg, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_stk_display_info_params* params;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessage* reply;
    dbus_context* ctx;

    tapi_log_info("stk agent display action method is called\n");

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    ar->msg_id = MSG_STK_AGENT_DISPLAY_ACTION_IND;

    params = NULL;
    reply = NULL;
    ctx = ar->data;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_failed(msg);
        goto done;
    }

    if (ctx->pending) {
        tapi_log_error("context in %s is busy", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_busy(msg);
        goto done;
    }

    params = malloc(sizeof(tapi_stk_display_info_params));
    if (params == NULL) {
        tapi_log_error("params in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_not_implemented(msg);
        goto done;
    }

    if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &params->info,
            DBUS_TYPE_BYTE, &params->icon_id,
            DBUS_TYPE_INVALID)) {
        tapi_log_error("dbus message get args failed in %s", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_invalid_args(msg);
        goto done;
    }

    ctx->pending = dbus_message_ref(msg);
    ar->data = params;
    ar->status = OK;

done:
    cb(ar);
    if (params != NULL) {
        free(params);
    }

    return reply;
}

static DBusMessage* stk_agent_confirm_open_channel(DBusConnection* conn,
    DBusMessage* msg, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_stk_request_key_params* params;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusMessage* reply;
    dbus_context* ctx;

    tapi_log_info("stk agent confirm open channel method is called\n");

    if (handler == NULL) {
        tapi_log_error("handler in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((ar = handler->result) == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    if ((cb = handler->cb_function) == NULL) {
        tapi_log_error("callback in %s is null", __func__);
        return stk_agent_error_not_implemented(msg);
    }

    ar->msg_id = MSG_STK_AGENT_CONFIRM_OPEN_CHANNEL_IND;

    params = NULL;
    reply = NULL;
    ctx = ar->data;
    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_failed(msg);
        goto done;
    }

    if (ctx->pending) {
        tapi_log_error("context in %s is busy", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_busy(msg);
        goto done;
    }

    params = malloc(sizeof(tapi_stk_request_key_params));
    if (params == NULL) {
        tapi_log_error("params in %s is null", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_not_implemented(msg);
        goto done;
    }

    if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &params->alpha,
            DBUS_TYPE_BYTE, &params->icon_id,
            DBUS_TYPE_INVALID)) {
        tapi_log_error("dbus message get args failed in %s", __func__);
        ar->status = ERROR;
        reply = stk_agent_error_invalid_args(msg);
        goto done;
    }

    ctx->pending = dbus_message_ref(msg);
    ar->data = params;
    ar->status = OK;

done:
    cb(ar);
    if (params != NULL) {
        free(params);
    }

    return reply;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int tapi_stk_agent_interface_register(tapi_context context, int slot_id,
    char* agent_id, tapi_async_function p_handle)
{
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    tapi_async_result* ar;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (agent_id == NULL) {
        tapi_log_error("agent_id in %s is null", __func__);
        return -EINVAL;
    }

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL) {
        tapi_log_error("user_data in %s is null", __func__);
        return -ENOMEM;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        free(user_data);
        return -ENOMEM;
    }
    ar->arg1 = slot_id;
    ar->data = ctx;
    user_data->result = ar;
    user_data->cb_function = p_handle;

    tapi_log_debug("starting stk agent interface in %s,  slot : %d, agent id : %s",
        __func__, slot_id, agent_id);
    if (!g_dbus_register_interface(ctx->connection, agent_id, OFONO_SIM_APP_INTERFACE,
            agent_methods, NULL, NULL, user_data, handler_free)) {
        tapi_log_error("Unable to register stk agent %s\n", agent_id);
        return -EINVAL;
    }

    return OK;
}

int tapi_stk_agent_interface_unregister(tapi_context context, char* agent_id)
{
    dbus_context* ctx = context;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (agent_id == NULL) {
        tapi_log_error("agent_id in %s is null", __func__);
        return -EINVAL;
    }

    tapi_log_debug("stopping stk agent interface in %s, agent id : %s", __func__, agent_id);
    if (!g_dbus_unregister_interface(ctx->connection, agent_id,
            OFONO_SIM_APP_INTERFACE)) {
        tapi_log_error("Unable to unregister stk agent %s in %s", agent_id, __func__);
        return -EINVAL;
    }

    return OK;
}

int tapi_stk_default_agent_interface_register(tapi_context context, int slot_id,
    tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    char* agent_path;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    agent_path = NULL;
    if (slot_id == SLOT_ID_1) {
        agent_path = STK_AGENT_DEFAULT_ID_0;
    } else if (slot_id == SLOT_ID_2) {
        agent_path = STK_AGENT_DEFAULT_ID_1;
    }

    return tapi_stk_agent_interface_register(context, slot_id, agent_path, p_handle);
}

int tapi_stk_default_agent_interface_unregister(tapi_context context, int slot_id)
{
    dbus_context* ctx = context;
    char* agent_path;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    agent_path = NULL;
    if (slot_id == SLOT_ID_1) {
        agent_path = STK_AGENT_DEFAULT_ID_0;
    } else if (slot_id == SLOT_ID_2) {
        agent_path = STK_AGENT_DEFAULT_ID_1;
    }

    return tapi_stk_agent_interface_unregister(context, agent_path);
}

int tapi_stk_agent_register(tapi_context context, int slot_id,
    int event_id, char* agent_id, tapi_async_function p_handle)
{
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_result* ar;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (agent_id == NULL) {
        tapi_log_error("agent_id in %s is null", __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_STK];
    if (proxy == NULL) {
        tapi_log_error("proxy in %s is null", __func__);
        return -EIO;
    }

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL) {
        tapi_log_error("user_data in %s is null", __func__);
        return -ENOMEM;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        free(user_data);
        return -ENOMEM;
    }

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->data = agent_id;
    user_data->result = ar;
    user_data->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "RegisterAgent", stk_agent_register_param_append,
            method_call_complete, user_data, handler_free)) {
        tapi_log_error("dbus method call fail in %s", __func__);
        handler_free(user_data);
        return -EINVAL;
    }

    return OK;
}

int tapi_stk_agent_unregister(tapi_context context, int slot_id,
    int event_id, char* agent_id, tapi_async_function p_handle)
{
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_result* ar;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (agent_id == NULL) {
        tapi_log_error("agent_id in %s is null", __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_STK];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL) {
        tapi_log_error("user_data in %s is null", __func__);
        return -ENOMEM;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        free(user_data);
        return -ENOMEM;
    }

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->data = agent_id;
    user_data->result = ar;
    user_data->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "UnregisterAgent", stk_agent_register_param_append,
            method_call_complete, user_data, handler_free)) {
        tapi_log_error("dbus method call fail in %s", __func__);
        handler_free(user_data);
        return -EINVAL;
    }

    return OK;
}

int tapi_stk_default_agent_register(tapi_context context, int slot_id,
    int event_id, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    char* agent_path;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    agent_path = NULL;
    if (slot_id == SLOT_ID_1) {
        agent_path = STK_AGENT_DEFAULT_ID_0;
    } else if (slot_id == SLOT_ID_2) {
        agent_path = STK_AGENT_DEFAULT_ID_1;
    }

    return tapi_stk_agent_register(context, slot_id, event_id, agent_path, p_handle);
}

int tapi_stk_default_agent_unregister(tapi_context context, int slot_id,
    int event_id, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    char* agent_path;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    agent_path = NULL;
    if (slot_id == SLOT_ID_1) {
        agent_path = STK_AGENT_DEFAULT_ID_0;
    } else if (slot_id == SLOT_ID_2) {
        agent_path = STK_AGENT_DEFAULT_ID_1;
    }

    return tapi_stk_agent_unregister(context, slot_id, event_id, agent_path, p_handle);
}

int tapi_stk_select_item(tapi_context context, int slot_id,
    int event_id, unsigned char item, char* agent_id, tapi_async_function p_handle)
{
    stk_select_item_param* select_item_param;
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_result* ar;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (agent_id == NULL) {
        tapi_log_error("agent_id in %s is null", __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_STK];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    select_item_param = malloc(sizeof(stk_select_item_param));
    if (select_item_param == NULL) {
        tapi_log_error("select_item_param in %s is null", __func__);
        return -ENOMEM;
    }
    select_item_param->item = item;
    select_item_param->agent_id = agent_id;

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        free(select_item_param);
        return -ENOMEM;
    }
    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->data = select_item_param;

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL) {
        tapi_log_error("user_data in %s is null", __func__);
        free(select_item_param);
        free(ar);
        return -ENOMEM;
    }
    user_data->result = ar;
    user_data->cb_function = p_handle;

    tapi_log_info("tapi_stk_select_item item : %d, path : %s\n", item, agent_id);
    if (!g_dbus_proxy_method_call(proxy, "SelectItem", stk_select_item_param_append,
            method_call_complete, user_data, stk_event_data_free)) {
        tapi_log_error("dbus method call fail in %s", __func__);
        stk_event_data_free(user_data);
        return -EINVAL;
    }

    return OK;
}

int tapi_stk_get_idle_mode_text(tapi_context context, int slot_id, char** text)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

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

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_STK];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "IdleModeText", &iter)) {
        dbus_message_iter_get_basic(&iter, text);
        return OK;
    }

    tapi_log_error("dbus get property fail in %s", __func__);
    return ERROR;
}

int tapi_stk_get_idle_mode_icon(tapi_context context, int slot_id, char** icon)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

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

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_STK];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "IdleModeIcon", &iter)) {
        dbus_message_iter_get_basic(&iter, icon);
        return OK;
    }

    tapi_log_error("dbus get property fail in %s", __func__);
    return ERROR;
}

int tapi_stk_get_main_menu(tapi_context context, int slot_id, int length, tapi_stk_menu_item out[])
{
    dbus_context* ctx = context;
    DBusMessageIter array, entry;
    GDBusProxy* proxy;
    unsigned char icon_id;
    char* text;
    int index;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id %d in %s", slot_id, __func__);
        return -EINVAL;
    }

    if (length <= 0) {
        tapi_log_error("invalid length %d in %s", length, __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_STK];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (!g_dbus_proxy_get_property(proxy, "MainMenu", &array)) {
        tapi_log_error("dbus get property fail in %s", __func__);
        return -EIO;
    }

    index = 0;
    if (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_ARRAY) {

        dbus_message_iter_recurse(&array, &entry);

        while (dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_INVALID) {

            if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_STRUCT) {

                dbus_message_iter_get_basic(&entry, &text);
                dbus_message_iter_next(&entry);

                dbus_message_iter_get_basic(&entry, &icon_id);

                snprintf(out[index++].text, sizeof(out[index++].text), "%s", text);
                out[index++].icon_id = icon_id;
            }

            if (index >= length || index >= MAX_STK_MAIN_MENU_LENGTH)
                break;

            dbus_message_iter_next(&array);
            dbus_message_iter_recurse(&array, &entry);
        }
    }

    return index;
}

int tapi_stk_get_main_menu_title(tapi_context context, int slot_id, char** title)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

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

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_STK];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "MainMenuTitle", &iter)) {
        dbus_message_iter_get_basic(&iter, title);
        return OK;
    }

    tapi_log_error("dbus get property fail in %s", __func__);
    return ERROR;
}

int tapi_stk_get_main_menu_icon(tapi_context context, int slot_id, char** icon)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

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

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_STK];
    if (proxy == NULL) {
        tapi_log_error("no available proxy in %s", __func__);
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "MainMenuIcon", &iter)) {
        dbus_message_iter_get_basic(&iter, icon);
        return OK;
    }

    tapi_log_error("dbus get property fail in %s", __func__);
    return ERROR;
}

int tapi_stk_handle_agent_display_text(tapi_context context, tapi_stk_agent_operator_code op)
{
    dbus_context* ctx = context;
    DBusMessage* reply;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (ctx->connection == NULL) {
        tapi_log_error("connection in %s is null", __func__);
        return -EINVAL;
    }

    if (ctx->pending == NULL) {
        tapi_log_error("pending in %s is null", __func__);
        return -EINVAL;
    }

    switch (op) {
    case OP_CODE_CONFIRM_POSITIVE:
        reply = dbus_message_new_method_return(ctx->pending);
        break;
    case OP_CODE_CONFIRM_NEGATIVE:
        reply = stk_agent_error_end_session(ctx->pending);
        break;
    case OP_CODE_CONFIRM_BACKWARD:
        reply = stk_agent_error_go_back(ctx->pending);
        break;
    default:
        reply = stk_agent_error_failed(ctx->pending);
        break;
    }

    return stk_agent_dbus_pending_reply(ctx->connection, &ctx->pending, reply) ? OK : ERROR;
}

int tapi_stk_handle_agent_request_digit(tapi_context context,
    tapi_stk_agent_operator_code op, char* digit)
{
    return tapi_stk_handle_agent_request_key(context, op, digit);
}

int tapi_stk_handle_agent_request_key(tapi_context context,
    tapi_stk_agent_operator_code op, char* key)
{
    dbus_context* ctx = context;
    DBusMessage* reply;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (ctx->connection == NULL) {
        tapi_log_error("connection in %s is null", __func__);
        return -EINVAL;
    }

    if (ctx->pending == NULL) {
        tapi_log_error("pending in %s is null", __func__);
        return -EINVAL;
    }

    switch (op) {
    case OP_CODE_CONFIRM_POSITIVE:
        reply = dbus_message_new_method_return(ctx->pending);
        if (reply == NULL)
            break;

        dbus_message_append_args(reply,
            DBUS_TYPE_STRING, &key,
            DBUS_TYPE_INVALID);
        break;
    case OP_CODE_CONFIRM_NEGATIVE:
        reply = stk_agent_error_end_session(ctx->pending);
        break;
    case OP_CODE_CONFIRM_BACKWARD:
        reply = stk_agent_error_go_back(ctx->pending);
        break;
    default:
        reply = stk_agent_error_failed(ctx->pending);
        break;
    }

    return stk_agent_dbus_pending_reply(ctx->connection, &ctx->pending, reply) ? OK : ERROR;
}

int tapi_stk_handle_agent_request_confirmation(tapi_context context,
    tapi_stk_agent_operator_code op)
{
    dbus_context* ctx = context;
    DBusMessage* reply;
    int confirm;
    bool flag = false;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (ctx->connection == NULL) {
        tapi_log_error("connection in %s is null", __func__);
        return -EINVAL;
    }

    if (ctx->pending == NULL) {
        tapi_log_error("pending in %s is null", __func__);
        return -EINVAL;
    }

    switch (op) {
    case OP_CODE_CONFIRM_POSITIVE: // "Enter YES"
        flag = true;
    case OP_CODE_CONFIRM_NEGATIVE: // "Enter NO"
        confirm = flag ? 1 : 0;

        reply = dbus_message_new_method_return(ctx->pending);
        if (reply == NULL)
            break;

        dbus_message_append_args(reply,
            DBUS_TYPE_BOOLEAN, &confirm,
            DBUS_TYPE_INVALID);
        break;
    case OP_CODE_CONFIRM_BACKWARD:
        reply = stk_agent_error_go_back(ctx->pending);
        break;
    default:
        reply = stk_agent_error_failed(ctx->pending);
        break;
    }

    return stk_agent_dbus_pending_reply(ctx->connection, &ctx->pending, reply) ? OK : ERROR;
}

int tapi_stk_handle_agent_request_input(tapi_context context,
    tapi_stk_agent_operator_code op, char* input)
{
    dbus_context* ctx = context;
    DBusMessage* reply;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (ctx->connection == NULL) {
        tapi_log_error("connection in %s is null", __func__);
        return -EINVAL;
    }

    if (ctx->pending == NULL) {
        tapi_log_error("pending in %s is null", __func__);
        return -EINVAL;
    }

    switch (op) {
    case OP_CODE_CONFIRM_POSITIVE:
        reply = dbus_message_new_method_return(ctx->pending);
        if (reply == NULL)
            break;

        dbus_message_append_args(reply,
            DBUS_TYPE_STRING, &input,
            DBUS_TYPE_INVALID);
        break;
    case OP_CODE_CONFIRM_NEGATIVE:
        reply = stk_agent_error_end_session(ctx->pending);
        break;
    case OP_CODE_CONFIRM_BACKWARD:
        reply = stk_agent_error_go_back(ctx->pending);
        break;
    default:
        reply = stk_agent_error_failed(ctx->pending);
        break;
    }

    return stk_agent_dbus_pending_reply(ctx->connection, &ctx->pending, reply) ? OK : ERROR;
}

int tapi_stk_handle_agent_request_digits(tapi_context context,
    tapi_stk_agent_operator_code op, char* digits)
{
    return tapi_stk_handle_agent_request_input(context, op, digits);
}

int tapi_stk_handle_agent_play_tone(tapi_context context, tapi_stk_agent_operator_code op)
{
    dbus_context* ctx = context;
    DBusMessage* reply;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (ctx->connection == NULL) {
        tapi_log_error("connection in %s is null", __func__);
        return -EINVAL;
    }

    if (ctx->pending == NULL) {
        tapi_log_error("pending in %s is null", __func__);
        return -EINVAL;
    }

    switch (op) {
    case OP_CODE_CONFIRM_POSITIVE:
        reply = dbus_message_new_method_return(ctx->pending);
        break;
    case OP_CODE_CONFIRM_NEGATIVE:
        reply = stk_agent_error_end_session(ctx->pending);
        break;
    case OP_CODE_CONFIRM_BACKWARD:
        reply = stk_agent_error_go_back(ctx->pending);
        break;
    default:
        reply = stk_agent_error_failed(ctx->pending);
        break;
    }

    return stk_agent_dbus_pending_reply(ctx->connection, &ctx->pending, reply) ? OK : ERROR;
}

int tapi_stk_handle_agent_loop_tone(tapi_context context,
    tapi_stk_agent_operator_code op)
{
    return tapi_stk_handle_agent_play_tone(context, op);
}

int tapi_stk_handle_agent_request_selection(tapi_context context,
    tapi_stk_agent_operator_code op, unsigned char selection)
{
    dbus_context* ctx = context;
    DBusMessage* reply;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (ctx->connection == NULL) {
        tapi_log_error("connection in %s is null", __func__);
        return -EINVAL;
    }

    if (ctx->pending == NULL) {
        tapi_log_error("pending in %s is null", __func__);
        return -EINVAL;
    }

    switch (op) {
    case OP_CODE_CONFIRM_POSITIVE:
        reply = dbus_message_new_method_return(ctx->pending);
        if (reply == NULL)
            break;

        dbus_message_append_args(reply,
            DBUS_TYPE_BYTE, &selection,
            DBUS_TYPE_INVALID);
        break;
    case OP_CODE_CONFIRM_NEGATIVE:
        reply = stk_agent_error_end_session(ctx->pending);
        break;
    case OP_CODE_CONFIRM_BACKWARD:
        reply = stk_agent_error_go_back(ctx->pending);
        break;
    default:
        reply = stk_agent_error_failed(ctx->pending);
        break;
    }

    return stk_agent_dbus_pending_reply(ctx->connection, &ctx->pending, reply) ? OK : ERROR;
}

int tapi_stk_handle_agent_request_quick_digit(tapi_context context,
    tapi_stk_agent_operator_code op, char* digit)
{
    return tapi_stk_handle_agent_request_key(context, op, digit);
}

int tapi_stk_handle_agent_confirm_call_setup(tapi_context context,
    tapi_stk_agent_operator_code op)
{
    return tapi_stk_handle_agent_request_confirmation(context, op);
}

int tapi_stk_handle_agent_display_action_information(tapi_context context,
    tapi_stk_agent_operator_code op)
{
    return tapi_stk_handle_agent_display_text(context, op);
}

int tapi_stk_handle_agent_confirm_launch_browser(tapi_context context,
    tapi_stk_agent_operator_code op)
{
    return tapi_stk_handle_agent_request_confirmation(context, op);
}

int tapi_stk_handle_agent_display_action(tapi_context context,
    tapi_stk_agent_operator_code op)
{
    return tapi_stk_handle_agent_display_text(context, op);
}

int tapi_stk_handle_agent_confirm_open_channel(tapi_context context,
    tapi_stk_agent_operator_code op)
{
    return tapi_stk_handle_agent_request_confirmation(context, op);
}