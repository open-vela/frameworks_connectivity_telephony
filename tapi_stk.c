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
 * Private Type Declarations
 ****************************************************************************/
typedef struct {
    unsigned char item;
    char* agent_id;
} stk_select_item_param;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void user_data_free(void* user_data);
/* This method should be called if the data field needs to be recycled. */
static void user_data_free2(void* user_data);
static void method_call_complete(DBusMessage* message, void* user_data);
static void stk_agent_register_param_append(DBusMessageIter* iter, void* user_data);
static void stk_select_item_param_append(DBusMessageIter* iter, void* user_data);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

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

/* This method should be called if the data field needs to be recycled. */
static void user_data_free2(void* user_data)
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

    if (handler == NULL)
        return;

    if ((ar = handler->result) == NULL || (cb = handler->cb_function) == NULL)
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

static void stk_agent_register_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    char* agent_id;

    if (param == NULL || param->result == NULL) {
        tapi_log_error("invalid pin argument!");
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

    if (param == NULL || param->result == NULL) {
        tapi_log_error("invalid stk input argument!");
        return;
    }

    select_item_param = param->result->data;
    item = select_item_param->item;
    agent_id = select_item_param->agent_id;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_BYTE, &item);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &agent_id);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int tapi_stk_agent_register(tapi_context context, int slot_id,
    int event_id, char* agent_id, tapi_async_function p_handle)
{
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || agent_id == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_STK];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL) {
        return -ENOMEM;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(user_data);
        return -ENOMEM;
    }

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->data = agent_id;
    user_data->result = ar;
    user_data->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "RegisterAgent", stk_agent_register_param_append,
            method_call_complete, user_data, user_data_free)) {
        tapi_log_error("failed to register agent\n");
        user_data_free(user_data);
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

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || agent_id == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_STK];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL) {
        return -ENOMEM;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(user_data);
        return -ENOMEM;
    }

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->data = agent_id;
    user_data->result = ar;
    user_data->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "UnregisterAgent", stk_agent_register_param_append,
            method_call_complete, user_data, user_data_free)) {
        tapi_log_error("failed to unregister agent\n");
        user_data_free(user_data);
        return -EINVAL;
    }

    return OK;
}

int tapi_stk_default_agent_register(tapi_context context, int slot_id,
    int event_id, tapi_async_function p_handle)
{
    return tapi_stk_agent_register(context, slot_id, event_id, STK_AGENT_DEFAULT_ID, p_handle);
}

int tapi_stk_default_agent_unregister(tapi_context context, int slot_id,
    int event_id, tapi_async_function p_handle)
{
    return tapi_stk_agent_unregister(context, slot_id, event_id, STK_AGENT_DEFAULT_ID, p_handle);
}

int tapi_stk_select_item(tapi_context context, int slot_id,
    int event_id, unsigned char item, char* agent_id, tapi_async_function p_handle)
{
    stk_select_item_param* select_item_param;
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || item >= 0 || agent_id == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_STK];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    select_item_param = malloc(sizeof(stk_select_item_param));
    if (select_item_param == NULL) {
        return -ENOMEM;
    }
    select_item_param->item = item;
    select_item_param->agent_id = agent_id;

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(select_item_param);
        return -ENOMEM;
    }
    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->data = select_item_param;

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL) {
        free(select_item_param);
        free(ar);
        return -ENOMEM;
    }
    user_data->result = ar;
    user_data->cb_function = p_handle;

    tapi_log_info("tapi_stk_select_item item : %d, path : %s\n", item, agent_id);
    if (!g_dbus_proxy_method_call(proxy, "SelectItem", stk_select_item_param_append,
            method_call_complete, user_data, user_data_free2)) {
        tapi_log_error("failed to select item\n");
        user_data_free2(user_data);
        return -EINVAL;
    }

    return OK;
}

int tapi_stk_get_idle_mode_text(tapi_context context, int slot_id, char** text)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_STK];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "IdleModeText", &iter)) {
        dbus_message_iter_get_basic(&iter, text);
        return OK;
    }

    return ERROR;
}

int tapi_stk_get_idle_mode_icon(tapi_context context, int slot_id, char** icon)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_STK];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "IdleModeIcon", &iter)) {
        dbus_message_iter_get_basic(&iter, icon);
        return OK;
    }

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

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || length <= 0) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_STK];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!g_dbus_proxy_get_property(proxy, "MainMenu", &array)) {
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

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_STK];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "MainMenuTitle", &iter)) {
        dbus_message_iter_get_basic(&iter, title);
        return OK;
    }

    return ERROR;
}

int tapi_stk_get_main_menu_icon(tapi_context context, int slot_id, char** icon)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_STK];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "MainMenuIcon", &iter)) {
        dbus_message_iter_get_basic(&iter, icon);
        return OK;
    }

    return ERROR;
}