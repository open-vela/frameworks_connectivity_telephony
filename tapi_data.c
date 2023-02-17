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

#include "tapi_data.h"
#include "tapi_internal.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int data_property_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter iter, var;
    const char* property;
    const char* value_str;
    bool isvalid = false;

    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (dbus_message_iter_init(message, &iter) == false)
        goto done;

    dbus_message_iter_get_basic(&iter, &property);
    dbus_message_iter_next(&iter);
    dbus_message_iter_recurse(&iter, &var);

    if (strcmp(property, "DataOn") == 0 || strcmp(property, "DataSlot") == 0) {
        ar->status = OK;

        dbus_message_iter_get_basic(&var, &ar->arg2);
        isvalid = true;
    } else if (strcmp(property, "Bearer") == 0) {
        tapi_network_type type;
        ar->status = OK;

        dbus_message_iter_get_basic(&var, &value_str);
        if (tapi_network_type_from_string(value_str, &type)) {
            ar->arg2 = type;
        }
        isvalid = true;
    }

done:
    if (isvalid)
        cb(ar);

    return true;
}

static int data_connection_state_changed(DBusConnection* connection,
    DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter args;

    if (handler == NULL)
        return false;

    ar = handler->result;
    if (ar == NULL)
        return false;

    cb = handler->cb_function;
    if (cb == NULL)
        return false;

    if (dbus_message_iter_init(message, &args) == false)
        goto done;

    ar->status = OK;
    dbus_message_iter_get_basic(&args, &ar->arg2); // APN type

    dbus_message_iter_next(&args);
    dbus_message_iter_get_basic(&args, &ar->status); // Data state

done:
    cb(ar);
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

static void update_apn_context(const char* prop, DBusMessageIter* iter, tapi_apn_context* apn)
{
    const char* value;

    if (strcmp(prop, "Name") == 0) {
        dbus_message_iter_get_basic(iter, &value);

        if (strlen(value) <= MAX_APN_DOMAIN_LENGTH)
            strcpy(apn->name, value);
    } else if (strcmp(prop, "active") == 0) {
        dbus_message_iter_get_basic(iter, &apn->active);
    } else if (strcmp(prop, "Type") == 0) {
        dbus_message_iter_get_basic(iter, &value);
        apn->type = tapi_apn_context_type_from_string(value);
    } else if (strcmp(prop, "Protocol") == 0) {
        dbus_message_iter_get_basic(iter, &value);
        tapi_apn_proto_from_string(value, &apn->protocol);
    } else if (strcmp(prop, "AccessPointName") == 0) {
        dbus_message_iter_get_basic(iter, &value);

        if (strlen(value) <= MAX_APN_DOMAIN_LENGTH)
            strcpy(apn->accesspointname, value);
    } else if (strcmp(prop, "Username") == 0) {
        dbus_message_iter_get_basic(iter, &value);

        if (strlen(value) <= MAX_APN_DOMAIN_LENGTH)
            strcpy(apn->username, value);
    } else if (strcmp(prop, "Password") == 0) {
        dbus_message_iter_get_basic(iter, &value);

        if (strlen(value) <= MAX_APN_DOMAIN_LENGTH)
            strcpy(apn->password, value);
    } else if (strcmp(prop, "AuthenticationMethod") == 0) {
        dbus_message_iter_get_basic(iter, &value);
        apn->auth_method = tapi_apn_auth_method_from_string(value);
    } else if (strcmp(prop, "MessageProxy") == 0) {
        dbus_message_iter_get_basic(iter, &value);

        if (strlen(value) <= MAX_APN_DOMAIN_LENGTH)
            strcpy(apn->messageproxy, value);
    } else if (strcmp(prop, "MessageCenter") == 0) {
        dbus_message_iter_get_basic(iter, &value);
        if (strlen(value) <= MAX_APN_DOMAIN_LENGTH)
            strcpy(apn->messagecenter, value);
    }
}

static void update_apn_contexts(DBusMessageIter* iter, tapi_apn_context* apn)
{
    while (dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry, value;
        const char* key;

        dbus_message_iter_recurse(iter, &entry);
        dbus_message_iter_get_basic(&entry, &key);

        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &value);
        update_apn_context(key, &value, apn);

        dbus_message_iter_next(iter);
    }
}

static void update_ip_settings(DBusMessageIter* iter, const char* type, tapi_ip_settings* ip)
{
    DBusMessageIter entry, value;
    DBusMessageIter ip_entry, ip_value, dns_iter;
    const char* key;
    int is_active;
    char* iter_type;
    const char* ip_key;
    const char* value_str;
    int dns_count;

    while (dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_DICT_ENTRY) {
        dbus_message_iter_recurse(iter, &entry);
        dbus_message_iter_get_basic(&entry, &key);
        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &value);

        if (strcmp(key, "Active") == 0) {
            dbus_message_iter_get_basic(&value, &is_active);
            if (!is_active)
                break;
        }

        if (strcmp(key, "Type") == 0) {
            dbus_message_iter_get_basic(&value, &iter_type);
            if (strcmp(type, iter_type) != 0)
                break;
        }

        if (strcmp(key, "Settings") == 0) {
            while (dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_DICT_ENTRY) {
                dbus_message_iter_recurse(iter, &ip_entry);
                dbus_message_iter_get_basic(&ip_entry, &ip_key);

                dbus_message_iter_next(&ip_entry);
                dbus_message_iter_recurse(&ip_entry, &ip_value);

                if (strcmp(ip_key, "Interface") == 0) {
                    dbus_message_iter_get_basic(&ip_value, &value_str);

                    if (strlen(value_str) <= MAX_IP_INTERFACE_NAME_LENGTH)
                        strcpy(ip->ipv4->interface, value_str);
                } else if (strcmp(ip_key, "Address") == 0) {
                    dbus_message_iter_get_basic(&ip_value, &value_str);

                    if (strlen(value_str) <= MAX_IP_STRING_LENGTH)
                        strcpy(ip->ipv4->ip, value_str);
                } else if (strcmp(ip_key, "Gateway") == 0) {
                    dbus_message_iter_get_basic(&ip_value, &value_str);

                    if (strlen(value_str) <= MAX_IP_STRING_LENGTH)
                        strcpy(ip->ipv4->gateway, value_str);
                } else if (strcmp(ip_key, "DomainNameServers") == 0) {
                    dbus_message_iter_next(&ip_value);
                    dbus_message_iter_recurse(&ip_value, &dns_iter);

                    dns_count = dbus_message_iter_get_element_count(&dns_iter);
                    if (dns_count <= MAX_DATA_DNS_COUNT)
                        dbus_message_iter_get_fixed_array(&dns_iter, ip->ipv4->dns, &dns_count);
                } else if (strcmp(ip_key, "Proxy") == 0) {
                    dbus_message_iter_get_basic(&ip_value, &value_str);

                    if (strlen(value_str) <= MAX_IP_STRING_LENGTH)
                        strcpy(ip->ipv4->proxy, value_str);
                }

                dbus_message_iter_next(iter);
            }
        }

        if (strcmp(key, "IPv6.Settings") == 0) {
            while (dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_DICT_ENTRY) {
                dbus_message_iter_recurse(iter, &ip_entry);
                dbus_message_iter_get_basic(&ip_entry, &ip_key);

                dbus_message_iter_next(&ip_entry);
                dbus_message_iter_recurse(&ip_entry, &ip_value);

                if (strcmp(ip_key, "Interface") == 0) {
                    dbus_message_iter_get_basic(&ip_value, &value_str);

                    if (strlen(value_str) <= MAX_IP_INTERFACE_NAME_LENGTH)
                        strcpy(ip->ipv6->interface, value_str);
                } else if (strcmp(ip_key, "Address") == 0) {
                    dbus_message_iter_get_basic(&ip_value, &value_str);

                    if (strlen(value_str) <= MAX_IP_STRING_LENGTH)
                        strcpy(ip->ipv6->ip, value_str);
                } else if (strcmp(ip_key, "Gateway") == 0) {
                    dbus_message_iter_get_basic(&ip_value, &value_str);

                    if (strlen(value_str) <= MAX_IP_STRING_LENGTH)
                        strcpy(ip->ipv6->gateway, value_str);
                } else if (strcmp(ip_key, "DomainNameServers") == 0) {
                    dbus_message_iter_next(&ip_value);
                    dbus_message_iter_recurse(&ip_value, &dns_iter);

                    dns_count = dbus_message_iter_get_element_count(&dns_iter);
                    if (dns_count <= MAX_DATA_DNS_COUNT)
                        dbus_message_iter_get_fixed_array(&dns_iter, ip->ipv6->dns, &dns_count);
                }

                dbus_message_iter_next(iter);
            }
        }
        dbus_message_iter_next(iter);
    }
}

static void apn_list_loaded(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    DBusMessageIter args, list;
    DBusError err;
    tapi_apn_context* result[MAX_APN_LIST_CAPACITY];
    int index;

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
        goto done;
    }

    if (dbus_message_has_signature(message, "a(oa{sv})") == false)
        goto done;
    if (dbus_message_iter_init(message, &args) == false)
        goto done;
    dbus_message_iter_recurse(&args, &list);

    ar->status = OK;
    index = 0;
    while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_STRUCT) {
        DBusMessageIter entry, dict;
        char* path;

        tapi_apn_context* apn = malloc(sizeof(tapi_apn_context));
        if (apn == NULL)
            break;

        dbus_message_iter_recurse(&list, &entry);
        dbus_message_iter_get_basic(&entry, &path);
        strcpy(apn->id, path);

        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &dict);
        update_apn_contexts(&dict, apn);
        dbus_message_iter_next(&list);

        result[index++] = apn;
        if (index >= MAX_APN_LIST_CAPACITY)
            break;
    }
    ar->arg2 = index; // apn count;
    ar->data = result;

done:
    cb(ar);
    while (--index >= 0) {
        free(result[index]);
    }
}

static void apn_list_changd(DBusMessage* message, void* user_data)
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

static void ip_setting_query_completed(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;
    const char* type;
    DBusMessageIter args, list;
    DBusError err;
    tapi_ip_settings* ip_settings = NULL;

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
        goto done;
    }

    if (dbus_message_has_signature(message, "a(oa{sv})") == false)
        goto done;
    if (dbus_message_iter_init(message, &args) == false)
        goto done;

    type = tapi_apn_context_type_to_string(ar->arg2);

    ip_settings = malloc(sizeof(tapi_ip_settings));
    if (ip_settings == NULL) {
        goto done;
    }
    ip_settings->ipv4 = NULL;
    ip_settings->ipv6 = NULL;

    ip_settings->ipv4 = malloc(sizeof(tapi_ipv4_settings));
    if (ip_settings->ipv4 == NULL) {
        free(ip_settings);
        ip_settings = NULL;
        goto done;
    }

    ip_settings->ipv6 = malloc(sizeof(tapi_ipv6_settings));
    if (ip_settings->ipv6 == NULL) {
        free(ip_settings->ipv4);
        ip_settings->ipv4 = NULL;
        free(ip_settings);
        ip_settings = NULL;
        goto done;
    }

    dbus_message_iter_recurse(&args, &list);
    while (dbus_message_iter_get_arg_type(&list) == DBUS_TYPE_STRUCT) {
        DBusMessageIter entry, dict;
        const char* path;

        dbus_message_iter_recurse(&list, &entry);
        dbus_message_iter_get_basic(&entry, &path);

        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &dict);
        update_ip_settings(&dict, type, ip_settings);

        dbus_message_iter_next(&list);
    }

    ar->status = OK;
    ar->data = ip_settings;

done:
    cb(ar);

    if (ip_settings != NULL) {
        if (ip_settings->ipv4 != NULL)
            free(ip_settings->ipv4);
        if (ip_settings->ipv6 != NULL)
            free(ip_settings->ipv6);
        free(ip_settings);
    }
}

static void apn_context_append(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_apn_context* apn;
    const char* type;

    if (handler == NULL)
        return;

    ar = handler->result;
    if (ar == NULL)
        return;

    apn = ar->data;
    if (apn == NULL) {
        tapi_log_error("invalid apn settings ... \n");
        return;
    }

    type = tapi_apn_context_type_to_string(apn->type);

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &type);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &apn->name);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &apn->accesspointname);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &apn->username);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &apn->password);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &apn->protocol);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &apn->auth_method);
}

static void apn_context_remove(DBusMessageIter* iter, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_apn_context* apn;

    if (handler == NULL)
        return;

    ar = handler->result;
    if (ar == NULL)
        return;

    apn = ar->data;
    if (apn == NULL) {
        tapi_log_error("invalid apn settings ... \n");
        return;
    }

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &apn->id);
}

static void network_operation_append(DBusMessageIter* iter, void* user_data)
{
    const char* type = user_data;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &type);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int tapi_data_load_apn_contexts(tapi_context context,
    int slot_id, int event_id, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_DATA];
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
    handler->result = ar;
    handler->cb_function = p_handle;

    return g_dbus_proxy_method_call(proxy,
        "GetContexts", NULL, apn_list_loaded, handler, user_data_free);
}

int tapi_data_save_apn_context(tapi_context context,
    int slot_id, int event_id, tapi_apn_context* apn, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id) || apn == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_DATA];
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
    ar->data = apn;
    handler->result = ar;
    handler->cb_function = p_handle;

    return g_dbus_proxy_method_call(proxy,
        "AddContext", apn_context_append, apn_list_changd, handler, user_data_free);
}

int tapi_data_remove_apn_context(tapi_context context,
    int slot_id, int event_id, tapi_apn_context* apn, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id) || apn == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_DATA];
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
    ar->data = apn;
    handler->result = ar;
    handler->cb_function = p_handle;

    return g_dbus_proxy_method_call(proxy,
        "RemoveContext", apn_context_remove, apn_list_changd, handler, user_data_free);
}

int tapi_data_reset_apn_contexts(tapi_context context,
    int slot_id, int event_id, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_DATA];
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
    handler->result = ar;
    handler->cb_function = p_handle;

    return g_dbus_proxy_method_call(proxy,
        "ResetContexts", NULL, apn_list_changd, handler, user_data_free);
}

int tapi_data_is_ps_attached(tapi_context context, int slot_id, bool* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    int result;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_DATA];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Attached", &iter)) {
        dbus_message_iter_get_basic(&iter, &result);

        *out = result;
        return OK;
    }

    return ERROR;
}

int tapi_data_get_network_type(tapi_context context, int slot_id, tapi_network_type* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    char* result;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_DATA];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Bearer", &iter)) {
        dbus_message_iter_get_basic(&iter, &result);

        tapi_network_type_from_string(result, out);
        return OK;
    }

    return ERROR;
}

int tapi_data_is_roaming(tapi_context context, int slot_id, bool* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    int result;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_DATA];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "Status", &iter)) {
        dbus_message_iter_get_basic(&iter, &result);

        *out = (result == NETWORK_REGISTRATION_STATUS_ROAMING);
        return OK;
    }

    return ERROR;
}

int tapi_data_request_network(tapi_context context, int slot_id, const char* type)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id) || type == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_DATA];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    return g_dbus_proxy_method_call(proxy,
        "RequestNetwork", network_operation_append, NULL, (void*)type, NULL);
}

int tapi_data_release_network(tapi_context context, int slot_id, const char* type)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id) || type == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_DATA];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    return g_dbus_proxy_method_call(proxy,
        "ReleaseNetwork", network_operation_append, NULL, (void*)type, NULL);
}

int tapi_data_set_preferred_apn(tapi_context context,
    int slot_id, tapi_apn_context* apn)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    const char* path;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id) || apn == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_DATA];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    path = apn->id;
    return g_dbus_proxy_set_property_basic(proxy,
        "PreferredApn", DBUS_TYPE_STRING, &path, NULL, NULL, NULL);
}

int tapi_data_get_preferred_apn(tapi_context context, int slot_id, char** out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_DATA];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "PreferredApn", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    return ERROR;
}

int tapi_data_enable(tapi_context context, bool enabled)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    int value;
    int ret = ERROR;

    if (ctx == NULL) {
        return -EINVAL;
    }

    value = enabled;
    for (int i = 0; i < CONFIG_ACTIVE_MODEM_COUNT; i++) {
        proxy = ctx->dbus_proxy[i][DBUS_PROXY_DATA];
        if (proxy == NULL) {
            tapi_log_error("no available proxy ...\n");
            return -EIO;
        }

        ret = g_dbus_proxy_set_property_basic(proxy,
            "DataOn", DBUS_TYPE_BOOLEAN, &value, NULL, NULL, NULL);
    }

    return ret;
}

int tapi_data_get_enabled(tapi_context context, bool* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    int result;

    if (ctx == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[DEFAULT_SLOT_ID][DBUS_PROXY_DATA];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "DataOn", &iter)) {
        dbus_message_iter_get_basic(&iter, &result);

        *out = result;
        return OK;
    }

    return ERROR;
}

int tapi_data_enable_roaming(tapi_context context, bool enabled)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    int value;
    int ret = ERROR;

    if (ctx == NULL) {
        return -EINVAL;
    }

    value = enabled;
    for (int i = 0; i < CONFIG_ACTIVE_MODEM_COUNT; i++) {
        proxy = ctx->dbus_proxy[i][DBUS_PROXY_DATA];
        if (proxy == NULL) {
            tapi_log_error("no available proxy ...\n");
            return -EIO;
        }

        ret = g_dbus_proxy_set_property_basic(proxy,
            "DataRoamingOn", DBUS_TYPE_BOOLEAN, &value, NULL, NULL, NULL);
    }

    return ret;
}

int tapi_data_get_roaming_enabled(tapi_context context, bool* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;
    int result;

    if (ctx == NULL) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[DEFAULT_SLOT_ID][DBUS_PROXY_DATA];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "DataRoamingOn", &iter)) {
        dbus_message_iter_get_basic(&iter, &result);

        *out = result;
        return OK;
    }

    return ERROR;
}

int tapi_data_get_ip_settings(tapi_context context,
    int slot_id, int event_id, tapi_apn_context_type type, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    tapi_async_handler* handler;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id) || type <= 0) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_DATA];
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
    ar->arg2 = type;
    handler->result = ar;
    handler->cb_function = p_handle;

    return g_dbus_proxy_method_call(proxy,
        "GetContexts", NULL, ip_setting_query_completed, handler, user_data_free);
}

int tapi_data_set_default_data_slot(tapi_context context, int slot_id)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;

    proxy = ctx->dbus_proxy_manager;
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    return g_dbus_proxy_set_property_basic(proxy,
        "DataSlot", DBUS_TYPE_INT32, &slot_id, NULL, NULL, NULL);
}

int tapi_data_get_default_data_slot(tapi_context context, int* out)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    DBusMessageIter iter;

    proxy = ctx->dbus_proxy_manager;
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (g_dbus_proxy_get_property(proxy, "DataSlot", &iter)) {
        dbus_message_iter_get_basic(&iter, out);
        return OK;
    }

    return ERROR;
}

int tapi_data_register(tapi_context context,
    int slot_id, tapi_indication_msg msg, tapi_async_function p_handle)
{
    dbus_context* ctx = context;
    char* modem_path;
    int watch_id;
    tapi_async_handler* handler;
    tapi_async_result* ar;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)
        || msg < MSG_DATA_ENABLED_CHANGE_IND || msg > MSG_DEFAULT_DATA_SLOT_CHANGE_IND) {
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
    case MSG_DATA_ENABLED_CHANGE_IND:
    case MSG_DATA_REGISTRATION_STATE_CHANGE_IND:
    case MSG_DATA_NETWORK_TYPE_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_CONNECTION_MANAGER_INTERFACE,
            "PropertyChanged", data_property_changed, handler, user_data_free);
        break;
    case MSG_DEFAULT_DATA_SLOT_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, OFONO_MANAGER_PATH, OFONO_MANAGER_INTERFACE,
            "PropertyChanged", data_property_changed, handler, user_data_free);
        break;
    case MSG_DATA_CONNECTION_STATE_CHANGE_IND:
        watch_id = g_dbus_add_signal_watch(ctx->connection,
            OFONO_SERVICE, modem_path, OFONO_CONNECTION_MANAGER_INTERFACE,
            "DataConnectionChanged", data_connection_state_changed, handler, user_data_free);
        break;
    default:
        break;
    }

    if (watch_id <= 0)
        user_data_free(handler);

    return watch_id;
}

int tapi_data_unregister(tapi_context context, int watch_id)
{
    dbus_context* ctx = context;
    if (ctx == NULL || watch_id <= 0)
        return -EINVAL;

    return g_dbus_remove_watch(ctx->connection, watch_id);
}