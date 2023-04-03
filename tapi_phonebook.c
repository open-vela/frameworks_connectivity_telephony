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
#include "tapi_phonebook.h"

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

typedef struct {
    int fdn_idx;
    char* tag;
    char* number;
    char* pin2;
} fdn_record_param;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* This method should be called if the data field needs to be recycled. */
static void phonebook_event_data_free(void* user_data);
static void load_adn_entries_cb(DBusMessage* message, void* user_data);
static void load_fdn_entries_cb(DBusMessage* message, void* user_data);
static void insert_fdn_record_append(DBusMessageIter* iter, void* user_data);
static void insert_fdn_record_cb(DBusMessage* message, void* user_data);
static void delete_fdn_record_append(DBusMessageIter* iter, void* user_data);
static void update_fdn_record_append(DBusMessageIter* iter, void* user_data);
static void method_call_complete(DBusMessage* message, void* user_data);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* This method should be called if the data field needs to be recycled. */
static void phonebook_event_data_free(void* user_data)
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

static void load_adn_entries_cb(DBusMessage* message, void* user_data)
{
    DBusMessageIter iter;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusError err;
    char* entries;

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

    dbus_message_iter_get_basic(&iter, &entries);

    ar->data = entries;
    ar->status = OK;

done:
    cb(ar);
}

static void load_fdn_entries_cb(DBusMessage* message, void* user_data)
{
    fdn_entry entries[MAX_FDN_RECORD_LENGTH];
    DBusMessageIter iter, arr, entry;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusError err;
    int index;

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

    dbus_message_iter_recurse(&iter, &arr);
    if (dbus_message_iter_get_arg_type(&arr) == DBUS_TYPE_ARRAY) {
        dbus_message_iter_recurse(&arr, &entry);

        index = 0;
        while (dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_INVALID) {
            if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_STRUCT) {
                dbus_message_iter_get_basic(&entry, &entries[index].fdn_idx);
                dbus_message_iter_next(&entry);

                dbus_message_iter_get_basic(&entry, &entries[index].tag);
                dbus_message_iter_next(&entry);

                dbus_message_iter_get_basic(&entry, &entries[index].number);

                index++;
            }

            if (index >= MAX_FDN_RECORD_LENGTH)
                break;

            dbus_message_iter_next(&arr);
            dbus_message_iter_recurse(&arr, &entry);
        }

        ar->arg2 = index;
    }

    ar->data = entries;
    ar->status = OK;

done:
    cb(ar);
}

static void insert_fdn_record_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    fdn_record_param* insert_fdn_record;
    char* name;
    char* number;
    char* pin2;

    if (param == NULL || param->result == NULL) {
        tapi_log_error("invalid pin argument!");
        return;
    }

    insert_fdn_record = param->result->data;
    name = insert_fdn_record->tag;
    number = insert_fdn_record->number;
    pin2 = insert_fdn_record->pin2;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &name);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &number);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &pin2);
}

static void insert_fdn_record_cb(DBusMessage* message, void* user_data)
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

    dbus_message_iter_get_basic(&iter, &ar->arg2); /*fdn index*/

    ar->status = OK;

done:
    cb(ar);
}

static void delete_fdn_record_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    fdn_record_param* delete_fdn_record;
    int fdn_idx;
    char* pin2;

    if (param == NULL || param->result == NULL) {
        tapi_log_error("invalid pin argument!");
        return;
    }

    delete_fdn_record = param->result->data;
    pin2 = delete_fdn_record->pin2;
    fdn_idx = delete_fdn_record->fdn_idx;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &pin2);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &fdn_idx);
}

static void update_fdn_record_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* param = user_data;
    fdn_record_param* update_fdn_record;
    char* new_number;
    char* new_name;
    int fdn_idx;
    char* pin2;

    if (param == NULL || param->result == NULL) {
        tapi_log_error("invalid pin argument!");
        return;
    }

    update_fdn_record = param->result->data;
    new_name = update_fdn_record->tag;
    new_number = update_fdn_record->number;
    pin2 = update_fdn_record->pin2;
    fdn_idx = update_fdn_record->fdn_idx;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &new_name);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &new_number);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &pin2);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &fdn_idx);
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

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int tapi_phonebook_load_adn_entries(tapi_context context,
    int slot_id, int event_id, tapi_async_function p_handle)
{
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_PHONEBOOK];
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

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL) {
        free(ar);
        return -ENOMEM;
    }
    user_data->cb_function = p_handle;
    user_data->result = ar;

    if (!g_dbus_proxy_method_call(proxy, "Import", NULL,
            load_adn_entries_cb, user_data, handler_free)) {
        handler_free(user_data);
        return -EINVAL;
    }

    return OK;
}

int tapi_phonebook_load_fdn_entries(tapi_context context,
    int slot_id, int event_id, tapi_async_function p_handle)
{
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_PHONEBOOK];
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

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL) {
        free(ar);
        return -ENOMEM;
    }
    user_data->cb_function = p_handle;
    user_data->result = ar;

    if (!g_dbus_proxy_method_call(proxy, "ImportFdn", NULL,
            load_fdn_entries_cb, user_data, handler_free)) {
        handler_free(user_data);
        return -EINVAL;
    }

    return OK;
}

int tapi_phonebook_insert_fdn_entry(tapi_context context, int slot_id,
    int event_id, char* name, char* number, char* pin2, tapi_async_function p_handle)
{
    fdn_record_param* fdn_record;
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || name == NULL || number == NULL || pin2 == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_PHONEBOOK];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    fdn_record = malloc(sizeof(fdn_record_param));
    if (fdn_record == NULL) {
        return -ENOMEM;
    }
    fdn_record->tag = name;
    fdn_record->number = number;
    fdn_record->pin2 = pin2;

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(fdn_record);
        return -ENOMEM;
    }
    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->data = fdn_record;

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL) {
        free(fdn_record);
        free(ar);
        return -ENOMEM;
    }
    user_data->cb_function = p_handle;
    user_data->result = ar;

    if (!g_dbus_proxy_method_call(proxy, "InsertFdn", insert_fdn_record_append,
            insert_fdn_record_cb, user_data, phonebook_event_data_free)) {
        phonebook_event_data_free(user_data);
        return -EINVAL;
    }

    return OK;
}

int tapi_phonebook_delete_fdn_entry(tapi_context context, int slot_id,
    int event_id, int fdn_idx, char* pin2, tapi_async_function p_handle)
{
    fdn_record_param* fdn_record;
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || fdn_idx < 1 || pin2 == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_PHONEBOOK];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    fdn_record = malloc(sizeof(fdn_record_param));
    if (fdn_record == NULL) {
        return -ENOMEM;
    }
    fdn_record->fdn_idx = fdn_idx;
    fdn_record->pin2 = pin2;

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(fdn_record);
        return -ENOMEM;
    }
    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->data = fdn_record;

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL) {
        free(fdn_record);
        free(ar);
        return -ENOMEM;
    }
    user_data->cb_function = p_handle;
    user_data->result = ar;

    if (!g_dbus_proxy_method_call(proxy, "DeleteFdn", delete_fdn_record_append,
            method_call_complete, user_data, phonebook_event_data_free)) {
        phonebook_event_data_free(user_data);
        return -EINVAL;
    }

    return OK;
}

int tapi_phonebook_update_fdn_entry(tapi_context context, int slot_id, int event_id,
    int fdn_idx, char* new_name, char* new_number, char* pin2, tapi_async_function p_handle)
{
    tapi_async_handler* user_data;
    dbus_context* ctx = context;
    fdn_record_param* fdn_record;
    tapi_async_result* ar;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || fdn_idx < 1 || new_name == NULL
        || new_number == NULL || pin2 == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_PHONEBOOK];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    fdn_record = malloc(sizeof(fdn_record_param));
    if (fdn_record == NULL) {
        return -ENOMEM;
    }
    fdn_record->fdn_idx = fdn_idx;
    fdn_record->tag = new_name;
    fdn_record->number = new_number;
    fdn_record->pin2 = pin2;

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(fdn_record);
        return -ENOMEM;
    }
    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->data = fdn_record;

    user_data = malloc(sizeof(tapi_async_handler));
    if (user_data == NULL) {
        free(fdn_record);
        free(ar);
        return -ENOMEM;
    }
    user_data->cb_function = p_handle;
    user_data->result = ar;

    if (!g_dbus_proxy_method_call(proxy, "UpdateFdn", update_fdn_record_append,
            method_call_complete, user_data, phonebook_event_data_free)) {
        phonebook_event_data_free(user_data);
        return -EINVAL;
    }

    return OK;
}
