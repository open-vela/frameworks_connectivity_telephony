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
 * Private Functions
 ****************************************************************************/

static void parse_nitz(const char* str, tapi_network_time* info)
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

static void register_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_operator_info* network_info;
    char *mcc, *mnc, *tech;

    if (handler == NULL || handler->result == NULL) {
        tapi_log_error("invalid network register argument !!");
        return;
    }

    ar = handler->result;
    network_info = ar->data;

    mcc = strdup(network_info->mcc);
    mnc = strdup(network_info->mnc);
    tech = strdup(network_info->technology);

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &mcc);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &mnc);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &tech);

    free(mcc);
    free(mnc);
    free(tech);
}

static void fill_registration_info(const char* prop, DBusMessageIter* iter,
    tapi_registration_info* registration_info)
{
    const char* value_str;
    int value_int;

    if (strcmp(prop, "Status") == 0) {
        dbus_message_iter_get_basic(iter, &value_str);
        registration_info->reg_state = tapi_utils_registration_status_from_string(value_str);
    } else if (strcmp(prop, "Mode") == 0) {
        dbus_message_iter_get_basic(iter, &value_str);
        registration_info->selection_mode = tapi_utils_registration_mode_from_string(value_str);
    } else if (strcmp(prop, "Technology") == 0) {
        dbus_message_iter_get_basic(iter, &value_str);

        if (strlen(value_str) <= MAX_NETWORK_INFO_LENGTH)
            strcpy(registration_info->technology, value_str);
    } else if (strcmp(prop, "Name") == 0) {
        dbus_message_iter_get_basic(iter, &value_str);

        if (strlen(value_str) <= MAX_OPERATOR_NAME_LENGTH)
            strcpy(registration_info->operator_name, value_str);
    } else if (strcmp(prop, "MobileCountryCode") == 0) {
        dbus_message_iter_get_basic(iter, &value_str);

        if (strlen(value_str) <= MAX_MCC_LENGTH)
            strcpy(registration_info->mcc, value_str);
    } else if (strcmp(prop, "MobileNetworkCode") == 0) {
        dbus_message_iter_get_basic(iter, &value_str);

        if (strlen(value_str) <= MAX_MNC_LENGTH)
            strcpy(registration_info->mnc, value_str);
    } else if (strcmp(prop, "BaseStation") == 0) {
        dbus_message_iter_get_basic(iter, &value_str);

        if (strlen(value_str) <= MAX_NETWORK_INFO_LENGTH)
            strcpy(registration_info->station, value_str);
    } else if (strcmp(prop, "CellId") == 0) {
        dbus_message_iter_get_basic(iter, &value_int);
        registration_info->cell_id = value_int;
    } else if (strcmp(prop, "LocationAreaCode") == 0) {
        dbus_message_iter_get_basic(iter, &value_int);
        registration_info->lac = value_int;
    } else if (strcmp(prop, "NITZ") == 0) {
        tapi_network_time nitz_time;
        dbus_message_iter_get_basic(iter, &value_str);
        parse_nitz(value_str, &nitz_time);
        registration_info->nitz_time = nitz_time;
    } else if (strcmp(prop, "DenialReason") == 0) {
        dbus_message_iter_get_basic(iter, &value_int);
        registration_info->denial_reason = value_int;
    }
}

static void fill_cell_identity(const char* prop, DBusMessageIter* iter,
    tapi_cell_identity* identity)
{
    const char* value_str;
    int value_int;

    if (strcmp(prop, "MobileCountryCode") == 0) {
        dbus_message_iter_get_basic(iter, &value_str);

        if (strlen(value_str) <= MAX_MCC_LENGTH)
            strcpy(identity->mcc_str, value_str);
    } else if (strcmp(prop, "MobileNetworkCode") == 0) {
        dbus_message_iter_get_basic(iter, &value_str);

        if (strlen(value_str) <= MAX_MNC_LENGTH)
            strcpy(identity->mnc_str, value_str);
    } else if (strcmp(prop, "LocationAreaCode") == 0) {
        dbus_message_iter_get_basic(iter, &value_int);
        identity->lac = value_int;
    } else if (strcmp(prop, "CellId") == 0) {
        dbus_message_iter_get_basic(iter, &value_int);
        identity->ci = value_int;
    } else if (strcmp(prop, "EARFCN") == 0) {
        dbus_message_iter_get_basic(iter, &value_int);
        identity->earfcn = value_int;
    } else if (strcmp(prop, "PhysicalCellId") == 0) {
        dbus_message_iter_get_basic(iter, &value_int);
        identity->pci = value_int;
    } else if (strcmp(prop, "TrackingAreaCode") == 0) {
        dbus_message_iter_get_basic(iter, &value_int);
        identity->tac = value_int;
    } else if (strcmp(prop, "Registered") == 0) {
        dbus_message_iter_get_basic(iter, &value_int);
        identity->registered = value_int;
    } else if (strcmp(prop, "Technology") == 0) {
        dbus_message_iter_get_basic(iter, &value_str);
        identity->type = tapi_utils_cell_type_from_string(value_str);
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

    if (strcmp(prop, "Name") == 0) {
        if (strlen(value) <= MAX_NETWORK_INFO_LENGTH)
            strcpy(operator->name, value);
    } else if (strcmp(prop, "Status") == 0) {
        operator->status = tapi_utils_operator_status_from_string(value);
    } else if (strcmp(prop, "MobileCountryCode") == 0) {
        if (strlen(value) <= MAX_MCC_LENGTH)
            strcpy(operator->mcc, value);
    } else if (strcmp(prop, "MobileNetworkCode") == 0) {
        if (strlen(value) <= MAX_MNC_LENGTH)
            strcpy(operator->mnc, value);
    } else if (strcmp(prop, "Technologies") == 0) {
        if (strlen(value) <= MAX_NETWORK_INFO_LENGTH)
            strcpy(operator->technology, value);
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
    DBusMessageIter iter;
    tapi_async_result* ar;
    tapi_async_function cb;
    const char* property;

    if (handler == NULL)
        return 0;

    ar = handler->result;
    if (ar == NULL)
        return 0;

    cb = handler->cb_function;
    if (cb == NULL)
        return 0;

    if (ar->msg_id != MSG_NETWORK_STATE_CHANGE_IND) {
        return 0;
    }

    if (dbus_message_iter_init(message, &iter) == false)
        return 0;

    dbus_message_iter_get_basic(&iter, &property);
    if (strcmp(property, "Mode") == 0 || strcmp(property, "Name") == 0
        || strcmp(property, "Status") == 0 || strcmp(property, "LocationAreaCode") == 0
        || strcmp(property, "CellId") == 0 || strcmp(property, "DenialReason") == 0
        || strcmp(property, "Technology") == 0 || strcmp(property, "BaseStation") == 0
        || strcmp(property, "MobileCountryCode") == 0
        || strcmp(property, "MobileNetworkCode") == 0) {
        ar->status = OK;
        cb(ar);
    }

    return 1;
}

static int cellinfo_list_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_cell_identity* cell_info_list[MAX_CELL_INFO_LIST_SIZE];
    tapi_async_handler* handler = user_data;
    tapi_cell_identity* cell_identity;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter, list;
    const char* property;
    int cell_index;

    if (handler == NULL)
        return 0;

    ar = handler->result;
    if (ar == NULL)
        return 0;

    cb = handler->cb_function;
    if (cb == NULL)
        return 0;

    if (ar->msg_id != MSG_CELLINFO_CHANGE_IND) {
        return 0;
    }

    if (dbus_message_iter_init(message, &iter) == false)
        return 0;

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY)
        return 0;

    dbus_message_iter_get_basic(&iter, &property);
    if (strcmp(property, "CellList") != 0) {
        return 0;
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

    return 1;
}

static int signal_strength_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter, var;
    const char* property;
    int strength;

    if (handler == NULL)
        return 0;

    ar = handler->result;
    if (ar == NULL)
        return 0;

    cb = handler->cb_function;
    if (cb == NULL)
        return 0;

    if (ar->msg_id != MSG_SIGNAL_STRENGTH_CHANGE_IND) {
        return 0;
    }

    if (dbus_message_iter_init(message, &iter) == false)
        return 0;

    dbus_message_iter_get_basic(&iter, &property);
    dbus_message_iter_next(&iter);

    dbus_message_iter_recurse(&iter, &var);
    if (strcmp(property, "Strength") == 0) {
        dbus_message_iter_get_basic(&var, &strength);
        ar->status = OK;
        ar->arg2 = strength;
        cb(ar);
    }

    return 1;
}

static int nitz_state_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter, var;
    tapi_network_time* nitz_time;
    const char* property;
    char* result;

    if (handler == NULL)
        return 0;

    ar = handler->result;
    if (ar == NULL)
        return 0;

    cb = handler->cb_function;
    if (cb == NULL)
        return 0;

    if (ar->msg_id != MSG_NITZ_STATE_CHANGE_IND) {
        return 0;
    }

    if (dbus_message_iter_init(message, &iter) == false)
        return 0;

    dbus_message_iter_get_basic(&iter, &property);
    dbus_message_iter_next(&iter);

    dbus_message_iter_recurse(&iter, &var);
    if (strcmp(property, "NITZ") == 0) {
        dbus_message_iter_get_basic(&var, &result);
        nitz_time = malloc(sizeof(tapi_network_time));
        if (nitz_time == NULL)
            return 0;
        parse_nitz(result, nitz_time);

        ar->status = OK;
        ar->data = nitz_time;
        cb(ar);

        if (nitz_time != NULL)
            free(nitz_time);
    }

    return 1;
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

static void cell_list_request_complete(DBusMessage* message, void* user_data)
{
    tapi_cell_identity* cell_list[MAX_CELL_INFO_LIST_SIZE];
    tapi_async_handler* handler = user_data;
    tapi_cell_identity* cell_identity;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter, list;
    DBusError err;
    int cell_index;

    if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_ERROR) {
        const char* dbus_error = dbus_message_get_error_name(message);
        tapi_log_error("cell_list_request_complete failed: %s", dbus_error);
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

        fill_cell_identity_list(&dict, cell_identity);

        cell_list[cell_index++] = cell_identity;
        if (cell_index >= MAX_CELL_INFO_LIST_SIZE)
            break;

        dbus_message_iter_next(&list);
    }

    ar->arg2 = cell_index; // cell count;
    ar->data = cell_list;

done:
    cb(ar);

    while (--cell_index >= 0) {
        free(cell_list[cell_index]);
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
    tapi_context ctx;

    if (handler == NULL)
        return;

    ar = handler->result;
    if (ar == NULL)
        return;

    cb = handler->cb_function;
    if (cb == NULL)
        return;

    registration_info = malloc(sizeof(tapi_registration_info));
    if (registration_info == NULL)
        return;

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("%s: %s\n", err.name, err.message);
        dbus_error_free(&err);
        ar->status = ERROR;
        goto done;
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

    // set roaming type.
    registration_info->roaming_type = NETWORK_ROAMING_UNKNOWN;
    if (registration_info->reg_state == NETWORK_REGISTRATION_STATUS_ROAMING) {
        char sim_numeric[MAX_MCC_LENGTH + MAX_MNC_LENGTH + 1];
        char sim_mcc[MAX_MCC_LENGTH + 1];

        ctx = ar->data;
        if (ctx != NULL) {
            tapi_sim_get_sim_operator(ctx, ar->arg1,
                MAX_MCC_LENGTH + MAX_MNC_LENGTH + 1, sim_numeric);

            if (strlen(sim_numeric) > 0) {
                strncpy(sim_mcc, sim_numeric, MAX_MCC_LENGTH);
                sim_mcc[MAX_MCC_LENGTH] = '\0';

                if (strcmp(sim_mcc, registration_info->mcc) == 0) {
                    // same country
                    registration_info->roaming_type = NETWORK_ROAMING_DOMESTIC;
                } else {
                    registration_info->roaming_type = NETWORK_ROAMING_INTERNATIONAL;
                }
            }
        }
    }

    ar->status = OK;
    ar->data = registration_info;

done:
    cb(ar);

    if (registration_info != NULL)
        free(registration_info);
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

        if (strlen(path) <= MAX_NETWORK_INFO_LENGTH)
            strcpy(operator->id, path);

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
    ar->status = OK;

done:
    cb(ar);

    while (--operator_index >= 0) {
        free(operator_list[operator_index]);
    }
}

static void cell_info_list_rate_param_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* handler = user_data;
    uint32_t period;

    if (handler == NULL || handler->result == NULL) {
        tapi_log_error("invalid rate argument ... \n");
        return;
    }

    period = handler->result->arg2;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &period);
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

    if (!g_dbus_proxy_method_call(proxy,
            "Register", NULL, network_register_cb, handler, user_data_free)) {
        user_data_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_network_select_manual(tapi_context context,
    int slot_id, int event_id, tapi_operator_info* network, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id) || network == NULL) {
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

    ar->arg1 = slot_id;
    ar->data = network;
    handler->result = ar;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "RegisterManual",
            register_param_append, network_register_cb, handler, user_data_free)) {
        user_data_free(handler);
        return -EINVAL;
    }

    return -EINVAL;
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

    if (!g_dbus_proxy_method_call(proxy,
            "Scan", NULL, operator_scan_complete, handler, user_data_free)) {
        user_data_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_network_get_serving_cellinfos(tapi_context context,
    int slot_id, int event_id, tapi_async_function p_handle)
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
    handler->result = ar;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy, "GetServingCellInformation", NULL,
            cell_list_request_complete, handler, user_data_free)) {
        user_data_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_network_get_neighbouring_cellinfos(tapi_context context,
    int slot_id, int event_id, tapi_async_function p_handle)
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
    handler->result = ar;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy,
            "GetNeighbouringCellInformation", NULL, cell_list_request_complete,
            handler, user_data_free)) {
        user_data_free(handler);
        return -EINVAL;
    }

    return OK;
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

        *out = tapi_utils_network_type_from_string(result);
        return OK;
    }

    return -EINVAL;
}

int tapi_network_is_voice_roaming(tapi_context context, int slot_id, bool* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    char* result;
    tapi_registration_state reg_state;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_NETREG];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (!g_dbus_proxy_get_property(proxy, "Status", &iter)) {
        return -EINVAL;
    }

    dbus_message_iter_get_basic(&iter, &result);
    reg_state = tapi_utils_registration_status_from_string(result);
    *out = (reg_state == NETWORK_REGISTRATION_STATUS_ROAMING);

    return OK;
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
        return OK;
    }

    return -EINVAL;
}

int tapi_network_get_signalstrength(tapi_context context, int slot_id, tapi_signal_strength* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    int value;

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

        out->rsrp = value;
        return OK;
    }

    return -EINVAL;
}

int tapi_network_get_registration_info(tapi_context context,
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
    ar->data = context;
    handler->result = ar;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy,
            "GetProperties", NULL, registration_info_query_done, handler, user_data_free)) {
        user_data_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_network_set_cell_info_list_rate(tapi_context context, int slot_id,
    int event_id, u_int32_t period, tapi_async_function p_handle)
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
    if (handler == NULL) {
        return -ENOMEM;
    }

    ar = malloc(sizeof(tapi_async_result));
    if (ar == NULL) {
        free(handler);
        return -ENOMEM;
    }

    ar->msg_id = event_id;
    ar->arg1 = slot_id;
    ar->arg2 = period;
    handler->result = ar;
    handler->cb_function = p_handle;

    if (!g_dbus_proxy_method_call(proxy,
            "CellInfoUpdateRate", cell_info_list_rate_param_append,
            NULL, handler, user_data_free)) {
        user_data_free(handler);
        return -EINVAL;
    }

    return OK;
}

int tapi_network_register(tapi_context context,
    int slot_id, tapi_indication_msg msg, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    tapi_async_handler* handler;
    tapi_async_result* ar;
    const char* modem_path;
    int watch_id;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || msg < MSG_NETWORK_STATE_CHANGE_IND || msg > MSG_NITZ_STATE_CHANGE_IND) {
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
    case MSG_NETWORK_STATE_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_NETWORK_REGISTRATION_INTERFACE,
            "PropertyChanged", network_state_changed, handler, user_data_free);
        break;
    case MSG_CELLINFO_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_NETMON_INTERFACE,
            "PropertyChanged", cellinfo_list_changed, handler, user_data_free);
        break;
    case MSG_SIGNAL_STRENGTH_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_NETWORK_REGISTRATION_INTERFACE,
            "PropertyChanged", signal_strength_changed, handler, user_data_free);
        break;
    case MSG_NITZ_STATE_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_NETWORK_REGISTRATION_INTERFACE,
            "PropertyChanged", nitz_state_changed, handler, user_data_free);
        break;
    default:
        break;
    }

    if (watch_id == 0) {
        user_data_free(handler);
        return -EINVAL;
    }

    return watch_id;
}

int tapi_network_unregister(tapi_context context, int watch_id)
{
    dbus_context* ctx = context;
    if (ctx == NULL || watch_id <= 0)
        return -EINVAL;

    return g_dbus_remove_watch(ctx->connection, watch_id);
}
