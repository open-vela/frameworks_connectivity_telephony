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

#include <dbus/dbus.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tapi.h"
#include "tapi_internal.h"
#include "tapi_network.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void parse_nitz(char* str, tapi_network_time* info)
{
    int index;
    int* p;
    char* pch;
    char* orig;

    if (str == NULL || info == NULL)
        return;

    p = &info->sec;
    orig = strdup(str);

    pch = strtok(orig, ",");
    index = 0;
    while (pch != NULL) {
        *p++ = atoi(pch);
        pch = strtok(NULL, ",");
        index++;
    }

    if (orig != NULL)
        free(orig);
}

static void fill_registration_info(const char* prop, DBusMessageIter* iter, tapi_registration_info* registration_info)
{
    if (strcmp(prop, "Status") == 0) {
        char* status;
        dbus_message_iter_get_basic(iter, &status);
        tapi_log_info("tapi_network_get_registration_info -> status %s\n", status);
        registration_info->reg_state = tapi_registration_status_from_string(status);
    } else if (strcmp(prop, "Mode") == 0) {
        char* mode;
        dbus_message_iter_get_basic(iter, &mode);
        tapi_log_info("tapi_network_get_registration_info -> mode %s\n", mode);
        registration_info->selection_mode = tapi_registration_mode_from_string(mode);
    } else if (strcmp(prop, "Technology") == 0) {
        char* technology;
        dbus_message_iter_get_basic(iter, &technology);
        tapi_log_info("tapi_network_get_registration_info -> technology %s\n", technology);
        memset(registration_info->technology, 0, sizeof(registration_info->technology));
        sprintf(registration_info->technology, "%s", technology);
    } else if (strcmp(prop, "Name") == 0) {
        char* display_name;
        dbus_message_iter_get_basic(iter, &display_name);
        tapi_log_info("tapi_network_get_registration_info -> display_name %s\n", display_name);
        memset(registration_info->operator_name, 0, sizeof(registration_info->operator_name));
        sprintf(registration_info->operator_name, "%s", display_name);
    } else if (strcmp(prop, "MobileCountryCode") == 0) {
        char* mcc;
        dbus_message_iter_get_basic(iter, &mcc);
        tapi_log_info("tapi_network_get_registration_info -> mcc %s\n", mcc);
        memset(registration_info->mcc, 0, sizeof(registration_info->mcc));
        sprintf(registration_info->mcc, "%s", mcc);
    } else if (strcmp(prop, "MobileNetworkCode") == 0) {
        char* mnc;
        dbus_message_iter_get_basic(iter, &mnc);
        tapi_log_info("tapi_network_get_registration_info -> mnc %s\n", mnc);
        memset(registration_info->mnc, 0, sizeof(registration_info->mnc));
        sprintf(registration_info->mnc, "%s", mnc);
    } else if (strcmp(prop, "BaseStation") == 0) {
        char* station;
        dbus_message_iter_get_basic(iter, &station);
        tapi_log_info("tapi_network_get_registration_info -> station %s\n", station);
        memset(registration_info->station, 0, sizeof(registration_info->station));
        sprintf(registration_info->station, "%s", station);
    } else if (strcmp(prop, "CellId") == 0) {
        u_int32_t cell_id;
        dbus_message_iter_get_basic(iter, &cell_id);
        tapi_log_info("tapi_network_get_registration_info -> cell_id %d\n", cell_id);
        registration_info->cell_id = cell_id;
    } else if (strcmp(prop, "LocationAreaCode") == 0) {
        u_int16_t lac;
        dbus_message_iter_get_basic(iter, &lac);
        tapi_log_info("tapi_network_get_registration_info -> lac %d\n", lac);
        registration_info->lac = lac;
    } else if (strcmp(prop, "NITZ") == 0) {
        char* str;
        tapi_network_time nitz_time;
        dbus_message_iter_get_basic(iter, &str);
        tapi_log_info("tapi_network_get_registration_info -> nitz_time %s\n", str);
        parse_nitz(str, &nitz_time);
        registration_info->nitz_time = nitz_time;
    }
}

static void fill_cell_identity(const char* prop, DBusMessageIter* iter, tapi_cell_identity* identity)
{
    if (strcmp(prop, "MobileCountryCode") == 0) {
        const char* value;
        dbus_message_iter_get_basic(iter, &value);
        memset(identity->mcc_str, 0, sizeof(identity->mcc_str));
        sprintf(identity->mcc_str, "%s", value);
    } else if (strcmp(prop, "MobileNetworkCode") == 0) {
        const char* value;
        dbus_message_iter_get_basic(iter, &value);
        memset(identity->mnc_str, 0, sizeof(identity->mnc_str));
        sprintf(identity->mnc_str, "%s", value);
    } else if (strcmp(prop, "LocationAreaCode") == 0) {
        int value;
        dbus_message_iter_get_basic(iter, &value);
        identity->lac = value;
    } else if (strcmp(prop, "CellId") == 0) {
        u_int32_t value;
        dbus_message_iter_get_basic(iter, &value);
        identity->ci = value;
    } else if (strcmp(prop, "EARFCN") == 0) {
        u_int16_t value;
        dbus_message_iter_get_basic(iter, &value);
        identity->earfcn = value;
    } else if (strcmp(prop, "PhysicalCellId") == 0) {
        u_int16_t value;
        dbus_message_iter_get_basic(iter, &value);
        identity->pci = value;
    } else if (strcmp(prop, "TrackingAreaCode") == 0) {
        u_int16_t value;
        dbus_message_iter_get_basic(iter, &value);
        identity->tac = value;
    }
}

static void fill_signal_strength(const char* prop, DBusMessageIter* iter, tapi_signal_strength* ss)
{
    int value;

    if (strcmp(prop, "Strength") == 0) {
        dbus_message_iter_get_basic(iter, &value);
        ss->rssi = value;
    } else if (strcmp(prop, "SingalToNoiseRatio") == 0) {
        dbus_message_iter_get_basic(iter, &value);
        ss->rssnr = value;
    } else if (strcmp(prop, "ReferenceSignalReceivedQuality") == 0) {
        dbus_message_iter_get_basic(iter, &value);
        ss->rsrq = value;
    } else if (strcmp(prop, "ReferenceSignalReceivedPower") == 0) {
        dbus_message_iter_get_basic(iter, &value);
        ss->rsrp = value;
    }
}

static void update_network_operator(const char* prop, DBusMessageIter* iter, tapi_operator_info* operator)
{
    char* value;

    if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_STRING)
        return;

    dbus_message_iter_get_basic(iter, &value);
    tapi_log_info("update_network_operator value : %s", value);

    if (strcmp(prop, "Name") == 0) {
        memset(operator->name, 0, sizeof(operator->name));
        sprintf(operator->name, "%s", value);
    } else if (strcmp(prop, "Status") == 0) {
        operator->status = tapi_operator_status_from_string(value);
    } else if (strcmp(prop, "MobileCountryCode") == 0) {
        memset(operator->mcc, 0, sizeof(operator->mcc));
        sprintf(operator->mcc, "%s", value);
    } else if (strcmp(prop, "MobileNetworkCode") == 0) {
        memset(operator->mnc, 0, sizeof(operator->mnc));
        sprintf(operator->mnc, "%s", value);
    } else if (strcmp(prop, "Technologies") == 0) {
        memset(operator->technology, 0, sizeof(operator->technology));
        sprintf(operator->technology, "%s", value);
    }
}

static void fill_neighbouring_cell_list(DBusMessageIter* iter, tapi_cell_identity* cell)
{
    while (dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry, value;
        const char* key;

        dbus_message_iter_recurse(iter, &entry);
        dbus_message_iter_get_basic(&entry, &key);

        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &value);

        fill_cell_identity(key, &value, cell);

        dbus_message_iter_next(iter);
    }
}

static void fill_cell_identity_list(DBusMessageIter* iter, tapi_cell_identity* cell)
{
    while (dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry, value;
        const char* key;

        dbus_message_iter_recurse(iter, &entry);
        dbus_message_iter_get_basic(&entry, &key);

        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &value);

        fill_cell_identity(key, &value, cell);

        dbus_message_iter_next(iter);
    }
}

static void fill_signal_strength_list(DBusMessageIter* iter, tapi_signal_strength* ss)
{
    while (dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry, value;
        const char* key;

        dbus_message_iter_recurse(iter, &entry);
        dbus_message_iter_get_basic(&entry, &key);

        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &value);

        fill_signal_strength(key, &value, ss);

        dbus_message_iter_next(iter);
    }
}

static void fill_operator_list(DBusMessageIter* iter, tapi_operator_info* operator)
{
    while (dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry, value;
        const char* key;

        dbus_message_iter_recurse(iter, &entry);
        dbus_message_iter_get_basic(&entry, &key);
        tapi_log_info("operator_scan_complete operator key : %s", key);

        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &value);

        update_network_operator(key, &value, operator);
        dbus_message_iter_next(iter);
    }
}

static int network_state_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    DBusMessageIter args, list;
    tapi_async_result* ar;
    tapi_async_function cb;
    tapi_registration_info* registration_info;
    const char* sender;

    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (!dbus_message_is_signal(message, OFONO_NETWORK_REGISTRATION_INTERFACE, "PropertyChanged")
        || ar->msg_id != MSG_NETWORK_STATE_CHANGE_IND) {
        return false;
    }

    sender = dbus_message_get_sender(message);
    if (sender == NULL)
        return false;

    if (!dbus_message_iter_init(message, &args))
        return false;

    if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_ARRAY)
        return false;

    dbus_message_iter_recurse(&args, &list);

    registration_info = malloc(sizeof(tapi_registration_info));

    if (registration_info == NULL)
        return false;

    while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry, value;
        const char* name;

        dbus_message_iter_recurse(&list, &entry);
        dbus_message_iter_get_basic(&entry, &name);
        dbus_message_iter_next(&entry);

        dbus_message_iter_recurse(&entry, &value);
        fill_registration_info(name, &value, registration_info);

        dbus_message_iter_next(&list);
    }

    ar->data = registration_info;
    cb(ar);

    if (registration_info != NULL)
        free(registration_info);

    return true;
}

static int cellinfo_list_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_cell_identity** cell_info_list;
    tapi_async_handler* handler = user_data;
    tapi_cell_identity* cell_identity;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter, list;
    const char* sender;
    int cell_index;

    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (!dbus_message_is_signal(message, OFONO_NETWORK_REGISTRATION_INTERFACE, "PropertyChanged")
        || ar->msg_id != MSG_CELLINFO_CHANGE_IND) {
        return false;
    }

    sender = dbus_message_get_sender(message);
    if (sender == NULL)
        return false;

    if (!dbus_message_iter_init(message, &iter))
        return false;

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY)
        return false;

    dbus_message_iter_recurse(&iter, &list);

    cell_info_list = ar->data;
    cell_index = 0;
    while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_STRUCT) {
        DBusMessageIter entry, dict;

        cell_identity = malloc(sizeof(tapi_cell_identity));
        if (cell_identity == NULL)
            break;

        dbus_message_iter_recurse(&list, &entry);
        dbus_message_iter_recurse(&entry, &dict);

        fill_cell_identity_list(&dict, cell_identity);
        fill_signal_strength_list(&dict, &(cell_identity->signal_strength));

        cell_info_list[cell_index++] = cell_identity;
        if (cell_index >= MAX_CELL_INFO_LIST_SIZE)
            break;

        dbus_message_iter_next(&list);
    }

    ar->arg2 = cell_index; // cell_info count;
    ar->data = cell_info_list;
    cb(ar);

    while (--cell_index >= 0) {
        free(cell_info_list[cell_index]);
    }

    return true;
}

static int signal_strength_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter;
    DBusMessageIter var;
    const char* property;
    const char* sender;
    int32_t strength;

    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (!dbus_message_is_signal(message, OFONO_NETWORK_REGISTRATION_INTERFACE, "PropertyChanged")
        || ar->msg_id != MSG_SIGNAL_STRENGTH_STATE_CHANGE_IND) {
        return false;
    }

    sender = dbus_message_get_sender(message);
    if (sender == NULL)
        return false;

    if (!dbus_message_iter_init(message, &iter))
        return false;

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING)
        return false;

    dbus_message_iter_get_basic(&iter, &property);
    dbus_message_iter_next(&iter);

    dbus_message_iter_recurse(&iter, &var);
    if (strcmp(property, "Strength") == 0) {
        if (dbus_message_iter_get_arg_type(&var) != DBUS_TYPE_BYTE)
            return false;

        dbus_message_iter_get_basic(&var, &strength);
    }

    ar->arg2 = strength;
    cb(ar);

    return true;
}

static int nitz_state_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter;
    const char* sender;
    char result[128];
    tapi_network_time* nitz_time;

    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (!dbus_message_is_signal(message, OFONO_NETWORK_REGISTRATION_INTERFACE, "PropertyChanged")
        || ar->msg_id != MSG_SIGNAL_STRENGTH_STATE_CHANGE_IND) {
        return false;
    }

    sender = dbus_message_get_sender(message);
    if (sender == NULL)
        return false;

    if (!dbus_message_iter_init(message, &iter))
        return false;

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING)
        return false;

    dbus_message_iter_get_basic(&iter, &result);

    nitz_time = malloc(sizeof(tapi_network_time));
    if (nitz_time == NULL)
        return false;

    parse_nitz(result, nitz_time);

    ar->data = nitz_time;
    cb(ar);

    if (nitz_time != NULL)
        free(nitz_time);

    return true;
}

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

static void network_register_cb(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusError err;

    if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_ERROR) {
        const char* dbus_error = dbus_message_get_error_name(message);
        tapi_log_error("network_register_cb failed: %s", dbus_error);
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
    }

    cb(ar);
}

static void cellinfo_request_complete(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter, list;
    DBusError err;

    if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_ERROR) {
        const char* dbus_error = dbus_message_get_error_name(message);
        tapi_log_error("cellinfo_request_complete failed: %s", dbus_error);
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
    if (dbus_message_has_signature(message, "a{sv}") == false) {
        ar->status = ERROR;
        goto done;
    }
    if (dbus_message_iter_init(message, &iter) == false) {
        ar->status = ERROR;
        goto done;
    }

    dbus_message_iter_recurse(&iter, &list);

    tapi_cell_identity* identity = malloc(sizeof(tapi_cell_identity));
    if (identity == NULL)
        return;

    while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry, value;
        const char* key;

        dbus_message_iter_recurse(&list, &entry);
        dbus_message_iter_get_basic(&entry, &key);

        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &value);

        fill_cell_identity(key, &value, identity);
        fill_signal_strength(key, &value, &(identity->signal_strength));
        dbus_message_iter_next(&list);
    }

    ar->data = identity;

done:
    cb(ar);

    if (identity != NULL)
        free(identity);
}

static void neighbouring_cellinfo_request_complete(DBusMessage* message, void* user_data)
{
    tapi_cell_identity* neighbouring_cell_list[MAX_CELL_INFO_LIST_SIZE];
    tapi_async_handler* handler = user_data;
    tapi_cell_identity* cell_identity;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter, list;
    DBusError err;
    int cell_index;

    if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_ERROR) {
        const char* dbus_error = dbus_message_get_error_name(message);
        tapi_log_error("neighbouring_cellinfo_request_complete failed: %s", dbus_error);
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

    if (dbus_message_has_signature(message, "a(a{sv})") == false) {
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        ar->status = ERROR;
        goto done;
    }

    dbus_message_iter_recurse(&iter, &list);

    cell_index = 0;
    while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_STRUCT) {
        DBusMessageIter entry, dict;

        cell_identity = malloc(sizeof(tapi_cell_identity));
        if (cell_identity == NULL)
            break;

        dbus_message_iter_recurse(&list, &entry);
        dbus_message_iter_recurse(&entry, &dict);

        fill_neighbouring_cell_list(&dict, cell_identity);

        neighbouring_cell_list[cell_index++] = cell_identity;
        if (cell_index >= MAX_CELL_INFO_LIST_SIZE)
            break;

        dbus_message_iter_next(&list);
    }

    ar->arg2 = cell_index; // neighbouring_cell count;
    ar->data = neighbouring_cell_list;

done:
    cb(ar);

    while (--cell_index >= 0) {
        free(neighbouring_cell_list[cell_index]);
    }
}

static void registration_info_query_done(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter args, list;
    DBusError err;
    tapi_registration_info* registration_info;

    if (handler == NULL)
        return;

    ar = handler->result;
    if (ar == NULL)
        return;

    cb = handler->cb_function;
    if (cb == NULL)
        return;

    registration_info = ar->data;
    ar->status = OK;

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == TRUE) {
        tapi_log_error("%s: %s\n", err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
    }

    if (dbus_message_has_signature(message, "a{sv}") == false)
        goto done;
    if (dbus_message_iter_init(message, &args) == false)
        goto done;
    dbus_message_iter_recurse(&args, &list);

    while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry, value;
        const char* name;

        dbus_message_iter_recurse(&list, &entry);
        dbus_message_iter_get_basic(&entry, &name);
        dbus_message_iter_next(&entry);

        dbus_message_iter_recurse(&entry, &value);
        fill_registration_info(name, &value, registration_info);

        dbus_message_iter_next(&list);
    }

    ar->data = registration_info;

done:
    cb(ar);
}

static void operator_scan_complete(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_operator_info* operator_list[MAX_OPERATOR_INFO_LIST_SIZE];
    tapi_operator_info* operator;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter, list;
    DBusError err;
    int operator_index;

    if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_ERROR) {
        const char* dbus_error = dbus_message_get_error_name(message);
        tapi_log_error("operator_scan_complete failed: %s", dbus_error);
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

    if (dbus_message_has_signature(message, "a(oa{sv})") == false) {
        ar->status = ERROR;
        goto done;
    }

    if (dbus_message_iter_init(message, &iter) == false) {
        ar->status = ERROR;
        goto done;
    }

    dbus_message_iter_recurse(&iter, &list);

    operator_index = 0;
    while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_STRUCT) {
        DBusMessageIter entry, dict;
        char* path;

        operator= malloc(sizeof(tapi_operator_info));
        if (operator== NULL)
            break;

        dbus_message_iter_recurse(&list, &entry);
        dbus_message_iter_get_basic(&entry, &path);
        memset(operator->id, 0, sizeof(operator->id));
        sprintf(operator->id, "%s", path);
        tapi_log_info("operator_scan_complete operator path : %s", operator->id);

        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &dict);

        fill_operator_list(&dict, operator);

        operator_list[operator_index++] = operator;
        if (operator_index >= MAX_OPERATOR_INFO_LIST_SIZE)
            break;

        dbus_message_iter_next(&list);
    }

    ar->arg2 = operator_index; // operator count;
    ar->data = operator_list;

done:
    cb(ar);

    while (--operator_index >= 0) {
        free(operator_list[operator_index]);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int tapi_network_select_auto(tapi_context context,
    int slot_id, int event_id, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_NETREG];
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

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    handler->result = ar;
    handler->cb_function = p_handle;

    return g_dbus_proxy_method_call(proxy,
        "Register", NULL, network_register_cb, handler, user_data_free);
}

int tapi_network_select_manual(tapi_context context,
    int slot_id, int event_id, tapi_operator_info operator, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    char* path;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    path = operator.extra;
    if (path == NULL)
        return -EINVAL;

    handler = malloc(sizeof(tapi_async_handler));
    if (handler == NULL)
        return -ENOMEM;

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(handler);
        return -ENOMEM;
    }

    ar->arg1 = slot_id;
    handler->result = ar;
    handler->cb_function = p_handle;

    proxy = g_dbus_proxy_new(
        ctx->client, tapi_utils_get_modem_path(slot_id), OFONO_NETWORK_OPERATOR_INTERFACE);
    g_dbus_proxy_method_call(proxy,
        "Register", NULL, network_register_cb, handler, user_data_free);
    g_dbus_proxy_unref(proxy);

    return 0;
}

int tapi_network_scan(tapi_context context,
    int slot_id, int event_id, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_NETREG];
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

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    handler->result = ar;
    handler->cb_function = p_handle;

    return g_dbus_proxy_method_call(proxy,
        "Scan", NULL, operator_scan_complete, handler, user_data_free);
}

int tapi_network_get_serving_cellinfo(tapi_context context,
    int slot_id, int event_id, tapi_cell_identity* cell, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_NETMON];
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

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->data = cell;
    handler->result = ar;
    handler->cb_function = p_handle;

    return g_dbus_proxy_method_call(proxy,
        "GetServingCellInformation", NULL, cellinfo_request_complete, handler, user_data_free);
}

int tapi_network_get_neighbouring_cellinfo(tapi_context context,
    int slot_id, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_NETMON];
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

    ar->arg1 = slot_id;
    handler->result = ar;
    handler->cb_function = p_handle;

    return g_dbus_proxy_method_call(proxy,
        "GetNeighbouringCellInformation", NULL, neighbouring_cellinfo_request_complete, handler, user_data_free);
}

int tapi_network_get_voice_network_type(tapi_context context, int slot_id, tapi_network_type* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    char* result;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_NETREG];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Technology", &iter)) {
        dbus_message_iter_get_basic(&iter, &result);
    }

    tapi_network_type_from_string(result, out);

    return 0;
}

bool tapi_network_is_voice_roaming(tapi_context context, int slot_id)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    char* modem_path;
    char* result;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_NETREG];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    modem_path = tapi_utils_get_modem_path(slot_id);
    if (modem_path == NULL) {
        tapi_log_error("no available modem ...\n");
        return false;
    }

    if (g_dbus_proxy_get_property(proxy, "Status", &iter)) {
        dbus_message_iter_get_basic(&iter, &result);
    }

    tapi_registration_state reg_state = tapi_registration_status_from_string(result);

    return (reg_state == NETWORK_REGISTRATION_STATUS_ROAMING);
}

int tapi_network_get_display_name(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_NETREG];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Name", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
    }

    return 0;
}

int tapi_network_get_signalstrength(tapi_context context, int slot_id, tapi_signal_strength* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    int32_t value;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_NETREG];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Strength", &iter)) {
        dbus_message_iter_get_basic(&iter, &value);
    }

    out->rsrp = value;

    return 0;
}

int tapi_network_get_registration_info(tapi_context context,
    int slot_id, tapi_registration_info* out, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_NETREG];
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

    ar->data = out;
    handler->result = ar;
    handler->cb_function = p_handle;

    return g_dbus_proxy_method_call(proxy,
        "GetProperties", NULL, registration_info_query_done, handler, user_data_free);
}

int tapi_network_register(tapi_context context,
    int slot_id, tapi_indication_msg msg, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    GDBusSignalFunction callback;
    char* modem_path;
    int watch_id;

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

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(handler);
        return -ENOMEM;
    }

    ar->msg_id = msg;
    ar->arg1 = slot_id;
    handler->result = ar;
    handler->cb_function = p_handle;

    switch (msg) {
    case MSG_NETWORK_STATE_CHANGE_IND:
        callback = network_state_changed;
        break;
    case MSG_CELLINFO_CHANGE_IND:
        callback = cellinfo_list_changed;
        break;
    case MSG_SIGNAL_STRENGTH_STATE_CHANGE_IND:
        callback = signal_strength_changed;
        break;
    case MSG_NITZ_STATE_CHANGE_IND:
        callback = nitz_state_changed;
        break;
    default:
        break;
    }

    if (callback == NULL)
        return -EINVAL;

    watch_id = g_dbus_add_signal_watch(ctx->connection,
        OFONO_SERVICE, modem_path, OFONO_NETWORK_REGISTRATION_INTERFACE,
        "PropertyChanged", callback, handler, user_data_free);

    return watch_id;
}

int tapi_network_unregister(tapi_context context, int watch_id)
{
    dbus_context* ctx = context;
    if (ctx == NULL || watch_id <= 0)
        return -EINVAL;

    return g_dbus_remove_watch(ctx->connection, watch_id);
}
