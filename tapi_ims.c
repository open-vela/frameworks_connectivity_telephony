/****************************************************************************
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "tapi_ims.h"
#include "tapi_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IMS_REGISTER_ENABLE 1
#define IMS_REGISTER_DISABLE 0

/****************************************************************************
 * Private Function
 ****************************************************************************/

static int tapi_ims_bitmask(int bit_mask, int bit_field, bool enabled)
{
    if (enabled)
        return bit_mask | bit_field;
    else
        return bit_mask & ~bit_field;
}

static void set_ss_param_append(DBusMessageIter* iter, void* user_data)
{
    int* param = user_data;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, param);
}

static int tapi_ims_enable(tapi_context context, int slot_id, int state)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    char* member;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id in %s", __func__);
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_IMS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (state == IMS_REGISTER_ENABLE)
        member = "Register";
    else if (state == IMS_REGISTER_DISABLE)
        member = "Unregister";
    else {
        tapi_log_error("invalid state in %s", __func__);
        return -EINVAL;
    }

    if (!g_dbus_proxy_method_call(proxy, member, NULL, no_operate_callback, NULL, NULL)) {
        tapi_log_error("call method failed in %s", __func__);
        return -EIO;
    }

    return OK;
}

static int ims_registration_changed(DBusConnection* connection, DBusMessage* message,
    void* user_data)
{
    tapi_async_handler* handler = user_data;
    DBusMessageIter iter, value_iter;
    tapi_async_function cb;
    tapi_async_result* ar;
    handler = user_data;
    int val;
    int msg_id;
    char* key;

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

    msg_id = handler->result->msg_id;
    if (dbus_message_is_signal(message, OFONO_IMS_INTERFACE,
            "PropertyChanged")
        && msg_id == MSG_IMS_REGISTRATION_MESSAGE_IND) {

        if (dbus_message_iter_init(message, &iter) == FALSE) {
            tapi_log_error("message iter init fail in %s", __func__);
            handler->result->status = ERROR;
            goto done;
        }

        dbus_message_iter_get_basic(&iter, &key);
        dbus_message_iter_next(&iter);

        handler->result->status = ERROR;

        dbus_message_iter_recurse(&iter, &value_iter);
        dbus_message_iter_get_basic(&value_iter, &val);

        if (strcmp("Registered", key) == 0)
            ar->arg1 = IMS_REG;
        else if (strcmp("VoiceCapable", key) == 0)
            ar->arg1 = VOICE_CAP;
        else if (strcmp("SmsCapable", key) == 0)
            ar->arg1 = SMS_CAP;

        ar->arg2 = val;
        ar->status = OK;

        cb(ar);
    }

done:
    return true;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int tapi_ims_turn_on(tapi_context context, int slot_id)
{
    return tapi_ims_enable(context, slot_id, IMS_REGISTER_ENABLE);
}

int tapi_ims_turn_off(tapi_context context, int slot_id)
{
    return tapi_ims_enable(context, slot_id, IMS_REGISTER_DISABLE);
}

int tapi_ims_set_service_status(tapi_context context, int slot_id, int capability)
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

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_IMS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!g_dbus_proxy_method_call(proxy, "SetCapability", set_ss_param_append,
            no_operate_callback, &capability, NULL)) {
        tapi_log_error("call method failed in %s", __func__);
        return -EIO;
    }

    return OK;
}

int tapi_ims_register_registration_change(tapi_context context, int slot_id, void* user_obj,
    tapi_async_function p_handle)
{
    tapi_async_handler* handler;
    dbus_context* ctx = context;
    tapi_async_result* ar;
    const char* path;
    int watch_id;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id in %s", __func__);
        return -EINVAL;
    }

    path = tapi_utils_get_modem_path(slot_id);
    if (path == NULL) {
        tapi_log_error("no available modem ...\n");
        return -EIO;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        tapi_log_error("async result in %s is null", __func__);
        return -ENOMEM;
    }

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL) {
        tapi_log_error("async handler in %s is null", __func__);
        free(ar);
        return -ENOMEM;
    }

    handler->cb_function = p_handle;
    handler->result = ar;

    ar->msg_id = MSG_IMS_REGISTRATION_MESSAGE_IND;
    ar->msg_type = INDICATION;
    ar->arg1 = slot_id;
    ar->user_obj = user_obj;

    watch_id = g_dbus_add_signal_watch(ctx->connection,
        OFONO_SERVICE, path, OFONO_IMS_INTERFACE, "PropertyChanged",
        ims_registration_changed, handler, handler_free);

    if (watch_id == 0) {
        tapi_log_error("add signal watch failed in %s", __func__);
        handler_free(handler);
        return -EINVAL;
    }

    return watch_id;
}

int tapi_ims_get_registration(tapi_context context, int slot_id,
    tapi_ims_registration_info* ims_reg)
{
    dbus_context* ctx = context;
    DBusMessageIter iter;
    GDBusProxy* proxy;
    int val = 0;
    int cap_value = 0;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id in %s", __func__);
        return -EINVAL;
    }

    if (ims_reg == NULL) {
        tapi_log_error("ims registration info in %s is null", __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_IMS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!g_dbus_proxy_get_property(proxy, "Registered", &iter)) {
        tapi_log_error("get property failed in %s", __func__);
        return -EIO;
    }

    dbus_message_iter_get_basic(&iter, &val);
    ims_reg->reg_info = val;

    if (g_dbus_proxy_get_property(proxy, "VoiceCapable", &iter)) {
        dbus_message_iter_get_basic(&iter, &val);
        cap_value = tapi_ims_bitmask(cap_value, VOICE_CAPABLE_FLAG, val);
    }

    if (g_dbus_proxy_get_property(proxy, "SmsCapable", &iter)) {
        dbus_message_iter_get_basic(&iter, &val);
        cap_value = tapi_ims_bitmask(cap_value, SMS_CAPABLE_FLAG, val);
    }

    ims_reg->ext_info = cap_value;

    return OK;
}

int tapi_ims_is_registered(tapi_context context, int slot_id, bool* out)
{
    dbus_context* ctx = context;
    DBusMessageIter iter;
    GDBusProxy* proxy;
    int val = 0;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id in %s", __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_IMS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!g_dbus_proxy_get_property(proxy, "Registered", &iter)) {
        tapi_log_error("get property failed in %s", __func__);
        return -EIO;
    }

    dbus_message_iter_get_basic(&iter, &val);
    *out = val;

    return OK;
}

int tapi_ims_is_volte_available(tapi_context context, int slot_id, bool* out)
{
    dbus_context* ctx = context;
    int reg = 0, cap = 0;
    DBusMessageIter iter;
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
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_IMS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!g_dbus_proxy_get_property(proxy, "Registered", &iter)) {
        tapi_log_error("get property failed in %s", __func__);
        return -EIO;
    }

    dbus_message_iter_get_basic(&iter, &reg);

    if (!g_dbus_proxy_get_property(proxy, "VoiceCapable", &iter)) {
        tapi_log_error("get property failed in %s", __func__);
        return -EIO;
    }

    dbus_message_iter_get_basic(&iter, &cap);
    *out = reg && cap;

    return OK;
}

int tapi_ims_get_subscriber_uri_number(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    DBusMessageIter iter;
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
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_IMS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "SubscriberUriNumber", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    tapi_log_error("get property failed in %s", __func__);
    return -EINVAL;
}

int tapi_ims_get_enabled(tapi_context context, int slot_id, bool* out)
{
    dbus_context* ctx = context;
    DBusMessageIter iter;
    GDBusProxy* proxy;
    int is_enabled = 0;

    if (ctx == NULL) {
        tapi_log_error("context in %s is null", __func__);
        return -EINVAL;
    }

    if (!tapi_is_valid_slotid(slot_id)) {
        tapi_log_error("invalid slot id in %s", __func__);
        return -EINVAL;
    }

    if (!ctx->client_ready) {
        tapi_log_error("dbus client is not ready in %s", __func__);
        return -EAGAIN;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_IMS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "ImsSwitchStatus", &iter)) {
        dbus_message_iter_get_basic(&iter, &is_enabled);

        *out = is_enabled;
        return OK;
    }

    tapi_log_error("get property failed in %s", __func__);
    return -EINVAL;
}