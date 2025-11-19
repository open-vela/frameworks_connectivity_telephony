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

#include "tapi.h"
#include "tapi_internal.h"

/* Radio technologies */
#define RADIO_TECH_UNKNOWN 0
#define RADIO_TECH_GPRS 1
#define RADIO_TECH_EDGE 2
#define RADIO_TECH_UMTS 3
#define RADIO_TECH_IS95A 4
#define RADIO_TECH_IS95B 5
#define RADIO_TECH_1xRTT 6
#define RADIO_TECH_EVDO_0 7
#define RADIO_TECH_EVDO_A 8
#define RADIO_TECH_HSDPA 9
#define RADIO_TECH_HSUPA 10
#define RADIO_TECH_HSPA 11
#define RADIO_TECH_EVDO_B 12
#define RADIO_TECH_EHRPD 13
#define RADIO_TECH_LTE 14
#define RADIO_TECH_HSPAP 15
#define RADIO_TECH_GSM 16
#define RADIO_TECH_TD_SCDMA 17
#define RADIO_TECH_IWLAN 18
#define RADIO_TECH_LTE_CA 19
#define RADIO_TECH_NR 20

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void no_operate_callback(DBusMessage* message, void* user_data)
{
    DBusError err;

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("error from message in %s, %s: %s", __func__, err.name, err.message);
        dbus_error_free(&err);
    }
}

void method_call_complete(DBusMessage* message, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_function cb;
    tapi_async_result* ar;
    DBusError err;
    int status = OK;

    dbus_error_init(&err);
    if (dbus_set_error_from_message(&err, message) == true) {
        tapi_log_error("error from message in %s, %s: %s", __func__, err.name, err.message);
        status = ERROR;
        dbus_error_free(&err);
    }

    if (handler == NULL) {
        tapi_log_debug("handler in %s is null", __func__);
        return;
    }

    ar = handler->result;
    if (ar == NULL) {
        tapi_log_debug("async result in %s is null", __func__);
        return;
    }

    cb = handler->cb_function;
    if (cb == NULL) {
        tapi_log_debug("callback in %s is null", __func__);
        return;
    }

    ar->status = status;
    cb(ar);
}

bool is_interface_supported(const char* interface)
{
    bool is_support = false;
    static const char* const supported[] = {
        OFONO_MODEM_INTERFACE,
        OFONO_RADIO_SETTINGS_INTERFACE,
        OFONO_SIM_MANAGER_INTERFACE,
        OFONO_CONNECTION_MANAGER_INTERFACE,
        OFONO_NETWORK_REGISTRATION_INTERFACE,
        OFONO_NETMON_INTERFACE,
        OFONO_NETWORK_OPERATOR_INTERFACE,
#ifdef CONFIG_OFONO_VOICE_CALL_MANAGER
        OFONO_VOICECALL_MANAGER_INTERFACE,
#endif
#ifdef CONFIG_OFONO_STK
        OFONO_STK_INTERFACE,
#endif
#ifdef CONFIG_OFONO_SMS_MANAGER
        OFONO_MESSAGE_MANAGER_INTERFACE,
#endif
#ifdef CONFIG_OFONO_CELL_BROADCAST_SERVICE
        OFONO_CELL_BROADCAST_INTERFACE,
#endif
#ifdef CONFIG_OFONO_CALL_BARRING
        OFONO_CALL_BARRING_INTERFACE,
#endif
#ifdef CONFIG_OFONO_CALL_FORWARDING
        OFONO_CALL_FORWARDING_INTERFACE,
#endif
#ifdef CONFIG_OFONO_SUPPLEMENTARY_SERVICES
        OFONO_SUPPLEMENTARY_SERVICES_INTERFACE,
#endif
#ifdef CONFIG_OFONO_CALL_SETTING
        OFONO_CALL_SETTINGS_INTERFACE,
#endif
#ifdef CONFIG_OFONO_IMS
        OFONO_IMS_INTERFACE,
#endif
#ifdef CONFIG_OFONO_PHONEBOOK
        OFONO_PHONEBOOK_INTERFACE,
#endif
        NULL
    };

    for (int i = 0; supported[i] != NULL; i++) {
        if (strcmp(supported[i], interface) == 0) {
            is_support = true;
            break;
        }
    }
    tapi_log_info("%s:%s,%d", __func__, interface, is_support);
    return is_support;
}

const char* tapi_utils_network_mode_to_string(tapi_pref_net_mode mode)
{
    switch (mode) {
    case NETWORK_PREF_NET_TYPE_ANY:
        return "any";
    case NETWORK_PREF_NET_TYPE_GSM_ONLY:
        return "gsm";
    case NETWORK_PREF_NET_TYPE_WCDMA_ONLY:
        return "umts";
    case NETWORK_PREF_NET_TYPE_LTE_ONLY:
        return "lte";
    case NETWORK_PREF_NET_TYPE_UMTS:
        return "umts,gsm";
    case NETWORK_PREF_NET_TYPE_LTE_WCDMA:
        return "lte,umts";
    case NETWORK_PREF_NET_TYPE_LTE_GSM_WCDMA:
        return "lte,umts,gsm";
    }

    return "";
}

tapi_pref_net_mode tapi_utils_network_mode_from_string(const char* mode)
{
    if (mode == NULL) {
        return NETWORK_PREF_NET_TYPE_ANY;
    }

    if (strcmp(mode, "any") == 0) {
        return NETWORK_PREF_NET_TYPE_ANY;
    } else if (strcmp(mode, "gsm") == 0) {
        return NETWORK_PREF_NET_TYPE_GSM_ONLY;
    } else if (strcmp(mode, "umts") == 0) {
        return NETWORK_PREF_NET_TYPE_WCDMA_ONLY;
    } else if (strcmp(mode, "lte") == 0) {
        return NETWORK_PREF_NET_TYPE_LTE_ONLY;
    } else if (strcmp(mode, "umts,gsm") == 0) {
        return NETWORK_PREF_NET_TYPE_UMTS;
    } else if (strcmp(mode, "lte,umts") == 0) {
        return NETWORK_PREF_NET_TYPE_LTE_WCDMA;
    } else if (strcmp(mode, "lte,umts,gsm") == 0) {
        return NETWORK_PREF_NET_TYPE_LTE_GSM_WCDMA;
    }

    return NETWORK_PREF_NET_TYPE_ANY;
}

tapi_network_type tapi_utils_network_type_from_ril_tech(int type)
{
    if (type == RADIO_TECH_EDGE) {
        return NETWORK_TYPE_EDGE;
    } else if (type == RADIO_TECH_UMTS) {
        return NETWORK_TYPE_UMTS;
    } else if (type == RADIO_TECH_LTE) {
        return NETWORK_TYPE_LTE;
    } else if (type == RADIO_TECH_HSDPA) {
        return NETWORK_TYPE_HSDPA;
    } else if (type == RADIO_TECH_HSPA) {
        return NETWORK_TYPE_HSPA;
    } else if (type == RADIO_TECH_HSUPA) {
        return NETWORK_TYPE_HSUPA;
    } else if (type == RADIO_TECH_LTE_CA) {
        return NETWORK_TYPE_LTE_CA;
    }

    return NETWORK_TYPE_UNKNOWN;
}

const char* tapi_utils_get_registration_status_string(int status)
{
    if (status == NETWORK_REGISTRATION_STATUS_NOT_REGISTERED) {
        return "unregistered";
    } else if (status == NETWORK_REGISTRATION_STATUS_REGISTERED) {
        return "registered";
    } else if (status == NETWORK_REGISTRATION_STATUS_SEARCHING) {
        return "searching";
    } else if (status == NETWORK_REGISTRATION_STATUS_DENIED) {
        return "denied";
    } else if (status == NETWORK_REGISTRATION_STATUS_UNKNOWN) {
        return "unknown";
    } else if (status == NETWORK_REGISTRATION_STATUS_ROAMING) {
        return "roaming";
    } else if (status == NETWORK_REGISTRATION_STATUS_REGISTED_EM) {
        return "registered_em";
    } else if (status == NETWORK_REGISTRATION_STATUS_NOT_REGISTERED_EM) {
        return "unregistered_em";
    } else if (status == NETWORK_REGISTRATION_STATUS_SEARCHING_EM) {
        return "searching_em";
    } else if (status == NETWORK_REGISTRATION_STATUS_DENIED_EM) {
        return "denied_em";
    } else if (status == NETWORK_REGISTRATION_STATUS_UNKNOWN_EM) {
        return "unknown_em";
    }

    return "";
}

tapi_network_operator_status tapi_utils_network_operator_status_from_string(const char* status)
{
    if (status == NULL)
        return OPERATOR_STATUS_UNKNOWN;

    if (strcmp(status, "current") == 0) {
        return OPERATOR_STATUS_CURRENT;
    } else if (strcmp(status, "available") == 0) {
        return OPERATOR_STATUS_AVAILABLE;
    } else if (strcmp(status, "forbidden") == 0) {
        return OPERATOR_STATUS_FORBIDDEN;
    }

    return OPERATOR_STATUS_UNKNOWN;
}

tapi_registration_state tapi_utils_registration_status_from_string(const char* status)
{
    if (status == NULL)
        return NETWORK_REGISTRATION_STATUS_UNKNOWN;

    if (strcmp(status, "unregistered") == 0) {
        return NETWORK_REGISTRATION_STATUS_NOT_REGISTERED;
    } else if (strcmp(status, "registered") == 0) {
        return NETWORK_REGISTRATION_STATUS_REGISTERED;
    } else if (strcmp(status, "searching") == 0) {
        return NETWORK_REGISTRATION_STATUS_SEARCHING;
    } else if (strcmp(status, "denied") == 0) {
        return NETWORK_REGISTRATION_STATUS_DENIED;
    } else if (strcmp(status, "unknown") == 0) {
        return NETWORK_REGISTRATION_STATUS_UNKNOWN;
    } else if (strcmp(status, "roaming") == 0) {
        return NETWORK_REGISTRATION_STATUS_ROAMING;
    } else if (strcmp(status, "registered_em") == 0) {
        return NETWORK_REGISTRATION_STATUS_REGISTED_EM;
    } else if (strcmp(status, "unregistered_em") == 0) {
        return NETWORK_REGISTRATION_STATUS_NOT_REGISTERED_EM;
    } else if (strcmp(status, "searching_em") == 0) {
        return NETWORK_REGISTRATION_STATUS_SEARCHING_EM;
    } else if (strcmp(status, "denied_em") == 0) {
        return NETWORK_REGISTRATION_STATUS_DENIED_EM;
    } else if (strcmp(status, "unknown_em") == 0) {
        return NETWORK_REGISTRATION_STATUS_UNKNOWN_EM;
    }

    return NETWORK_REGISTRATION_STATUS_UNKNOWN;
}

tapi_selection_mode tapi_utils_registration_mode_from_string(const char* mode)
{
    if (mode == NULL)
        return NETWORK_SELECTION_UNKNOWN;

    if (strcmp(mode, "auto") == 0) {
        return NETWORK_SELECTION_AUTO;
    } else if (strcmp(mode, "manual") == 0) {
        return NETWORK_SELECTION_MANUAL;
    }

    return NETWORK_SELECTION_UNKNOWN;
}

tapi_operator_status tapi_utils_operator_status_from_string(const char* mode)
{
    if (mode == NULL)
        return UNKNOWN;

    if (strcmp(mode, "current") == 0) {
        return CURRENTL;
    } else if (strcmp(mode, "available") == 0) {
        return AVAILABLE;
    } else if (strcmp(mode, "forbidden") == 0) {
        return FORBIDDEN;
    }

    return UNKNOWN;
}

const char* get_call_signal_member(tapi_indication_msg msg)
{
    int msg_id = msg;

    switch (msg_id) {
    case MSG_ECC_LIST_CHANGE_IND:
    case MSG_DEFAULT_VOICECALL_SLOT_CHANGE_IND:
        return "PropertyChanged";
    case MSG_CALL_RING_BACK_TONE_IND:
        return "RingBackTone";
    }

    return NULL;
}

const char* tapi_utils_get_modem_path(int slot_id)
{
    switch (slot_id) {
    case SLOT_ID_1:
        return CONFIG_MODEM_PATH;
    case SLOT_ID_2:
#ifdef CONFIG_MODEM_PATH_2
        return CONFIG_MODEM_PATH_2;
#endif
    case -1:
        return SLOT_NOT_SET;
    default:
        break;
    }

    return NULL;
}

int tapi_utils_get_slot_id(const char* modem_path)
{
    if (!strcmp(modem_path, CONFIG_MODEM_PATH))
        return 0;
#ifdef CONFIG_MODEM_PATH_2
    else if (!strcmp(modem_path, CONFIG_MODEM_PATH_2))
        return 1;
#endif
    return -1;
}

tapi_call_status tapi_utils_call_status_from_string(const char* status)
{
    tapi_call_status ret = CALL_STATUS_UNKNOW;

    if (status == NULL)
        return CALL_STATUS_UNKNOW;

    if (strcmp(status, "active") == 0) {
        ret = CALL_STATUS_ACTIVE;
    } else if (strcmp(status, "held") == 0) {
        ret = CALL_STATUS_HELD;
    } else if (strcmp(status, "dialing") == 0) {
        ret = CALL_STATUS_DIALING;
    } else if (strcmp(status, "alerting") == 0) {
        ret = CALL_STATUS_ALERTING;
    } else if (strcmp(status, "incoming") == 0) {
        ret = CALL_STATUS_INCOMING;
    } else if (strcmp(status, "waiting") == 0) {
        ret = CALL_STATUS_WAITING;
    } else if (strcmp(status, "disconnected") == 0) {
        ret = CALL_STATUS_DISCONNECTED;
    }

    return ret;
}

tapi_call_disconnect_reason tapi_utils_call_disconnected_reason(const char* str_status)
{
    tapi_call_disconnect_reason ret = CALL_DISCONNECT_REASON_UNKNOWN;

    if (str_status == NULL)
        return ret;

    if (strcmp(str_status, "local") == 0) {
        ret = CALL_DISCONNECT_REASON_LOCAL_HANGUP;
    } else if (strcmp(str_status, "remote") == 0) {
        ret = CALL_DISCONNECT_REASON_REMOTE_HANGUP;
    } else if (strcmp(str_status, "network") == 0) {
        ret = CALL_DISCONNECT_REASON_NETWORK_HANGUP;
    }

    return ret;
}

const char* tapi_utils_apn_type_to_string(tapi_data_context_type type)
{
    switch (type) {
    case DATA_CONTEXT_TYPE_ANY:
        return "";
    case DATA_CONTEXT_TYPE_INTERNET:
        return "internet";
    case DATA_CONTEXT_TYPE_HIPRI:
        return "hipri";
    case DATA_CONTEXT_TYPE_SUPL:
        return "supl";
    case DATA_CONTEXT_TYPE_MMS:
        return "mms";
    case DATA_CONTEXT_TYPE_WAP:
        return "wap";
    case DATA_CONTEXT_TYPE_IMS:
        return "ims";
    case DATA_CONTEXT_TYPE_EMERGENCY:
        return "emergency";
    }

    return NULL;
}

tapi_data_context_type tapi_utils_apn_type_from_string(const char* type)
{
    if (type == NULL)
        return DATA_CONTEXT_TYPE_ANY;

    if (strcmp(type, "internet") == 0)
        return DATA_CONTEXT_TYPE_INTERNET;
    else if (strcmp(type, "hipri") == 0)
        return DATA_CONTEXT_TYPE_HIPRI;
    else if (strcmp(type, "supl") == 0)
        return DATA_CONTEXT_TYPE_SUPL;
    else if (strcmp(type, "mms") == 0)
        return DATA_CONTEXT_TYPE_MMS;
    else if (strcmp(type, "wap") == 0)
        return DATA_CONTEXT_TYPE_WAP;
    else if (strcmp(type, "ims") == 0)
        return DATA_CONTEXT_TYPE_IMS;
    else if (strcmp(type, "emergency") == 0)
        return DATA_CONTEXT_TYPE_EMERGENCY;

    return DATA_CONTEXT_TYPE_ANY;
}

const char* tapi_utils_apn_auth_to_string(tapi_data_auth_method auth)
{
    switch (auth) {
    case DATA_AUTH_METHOD_CHAP:
        return "chap";
    case DATA_AUTH_METHOD_PAP:
        return "pap";
    case DATA_AUTH_METHOD_NONE:
        return "none";
    };

    return NULL;
}

tapi_data_auth_method tapi_utils_apn_auth_from_string(const char* auth)
{
    if (auth == NULL)
        return DATA_AUTH_METHOD_NONE;

    if (strcmp(auth, "chap") == 0)
        return DATA_AUTH_METHOD_CHAP;
    else if (strcmp(auth, "pap") == 0)
        return DATA_AUTH_METHOD_PAP;
    else if (strcmp(auth, "none") != 0)
        return DATA_AUTH_METHOD_NONE;

    return DATA_AUTH_METHOD_NONE;
}

const char* tapi_utils_apn_proto_to_string(tapi_data_proto proto)
{
    switch (proto) {
    case DATA_PROTO_IP:
        return "IP";
    case DATA_PROTO_IPV6:
        return "IPV6";
    case DATA_PROTO_IPV4V6:
        return "IPV4V6";
    };

    return NULL;
}

tapi_data_proto tapi_utils_apn_proto_from_string(const char* proto)
{
    if (proto == NULL)
        return DATA_PROTO_IP;

    if (strcmp(proto, "IP") == 0) {
        return DATA_PROTO_IP;
    } else if (strcmp(proto, "IPV6") == 0) {
        return DATA_PROTO_IPV6;
    } else if (strcmp(proto, "IPV4V6") == 0) {
        return DATA_PROTO_IPV4V6;
    }

    return DATA_PROTO_IP;
}

const char* tapi_utils_cell_type_to_string(tapi_cell_type type)
{
    switch (type) {
    case TYPE_NONE:
        return NULL;
    case TYPE_GSM:
        return "gsm";
    case TYPE_UMTS:
        return "umts";
    case TYPE_LTE:
        return "lte";
    case TYPE_NR:
        return "nr";
    }

    return NULL;
}

tapi_cell_type tapi_utils_cell_type_from_string(const char* name)
{
    if (name == NULL)
        return TYPE_NONE;

    if (strcmp(name, "gsm") == 0) {
        return TYPE_GSM;
    } else if (strcmp(name, "umts") == 0) {
        return TYPE_UMTS;
    } else if (strcmp(name, "lte") == 0) {
        return TYPE_LTE;
    } else if (strcmp(name, "nr") == 0) {
        return TYPE_NR;
    }

    return TYPE_NONE;
}

bool is_call_signal_message(DBusMessage* message, DBusMessageIter* iter, int msg_type)
{
    bool ret = false;

    if (!dbus_message_iter_init(message, iter))
        tapi_log_error("manager_call_signal Message Has no Param");
    else if (dbus_message_iter_get_arg_type(iter) != msg_type)
        tapi_log_error("manager_call_signal Param is not object");
    else
        ret = true;

    return ret;
}

void property_set_done(const DBusError* error, void* user_data)
{
    tapi_async_handler* handler = user_data;
    tapi_async_result* ar;
    tapi_async_function cb;

    if (handler == NULL)
        return;

    ar = handler->result;
    if (ar == NULL)
        return;

    cb = handler->cb_function;
    if (cb == NULL)
        return;

    if (dbus_error_is_set(error)) {
        tapi_log_error("%s: %s\n", error->name, error->message);
        ar->status = ERROR;
    } else {
        ar->status = OK;
    }

    cb(ar);
}

const char* tapi_sim_state_to_string(tapi_sim_state sim_state)
{
    switch (sim_state) {
    case SIM_STATE_NOT_PRESENT:
        return "SIM_ABSENT";
    case SIM_STATE_INSERTED:
        return "SIM_PRESENT";
    case SIM_STATE_LOCKED_OUT:
        return "SIM_LOCKED";
    case SIM_STATE_READY:
        return "SIM_READY";
    case SIM_STATE_RESETTING:
        return "SIM_RESETTING";
    case SIM_STATE_ERROR:
        return "SIM_ERROR";
    default:
        return "SIM_UNKNOWN";
    }
}

void handler_free(void* obj)
{
    tapi_async_handler* handler = obj;
    tapi_async_result* ar;

    if (handler != NULL) {
        ar = handler->result;
        if (ar != NULL)
            free(ar);
        free(handler);
    }
}

int get_modem_id_by_proxy(dbus_context* context, GDBusProxy* proxy)
{
    if (proxy == NULL)
        return 0;

    for (int i = 0; i < CONFIG_MODEM_ACTIVE_COUNT; i++) {
        for (int j = 0; j < DBUS_PROXY_MAX_COUNT; j++) {
            if (context->dbus_proxy[i][j] == proxy)
                return i;
        }
    }

    return 0;
}

static void get_covered_plmn(const char* mcc, const char* mnc, char* covered_plmn)
{
    char* ptr = covered_plmn;

    for (int i = 0; mcc[i] != '\0'; i++) {
        *ptr++ = mcc[i] - '0' + 'a';
    }
    for (int i = 0; mnc[i] != '\0'; i++) {
        *ptr++ = mnc[i] - '0' + 'a';
    }
    *ptr = '\0';
}

void tapi_get_coverted_plmn(tapi_context context, int slot_id, char* covered_plmn)
{
    char mcc[MAX_MCC_LENGTH + 1] = { 0 };
    char mnc[MAX_MNC_LENGTH + 1] = { 0 };
    int result;

    result = tapi_network_get_mcc(context, slot_id, mcc, sizeof(mcc));
    if (result != OK) {
        strncpy(covered_plmn, "unknow", MAX_MCC_LENGTH + MAX_MNC_LENGTH + 1);
        return;
    }
    result = tapi_network_get_mnc(context, slot_id, mnc, sizeof(mnc));
    if (result != OK) {
        strncpy(covered_plmn, "unknow", MAX_MCC_LENGTH + MAX_MNC_LENGTH + 1);
        return;
    }

    get_covered_plmn(mcc, mnc, covered_plmn);
}

int get_op_code_base_mcc_mnc(const char* mcc, const char* mnc)
{
    tapi_plmn_op_code_info plmn_op_info_list[] = {
        { "460", "00", OP_CMCC },
        { "460", "02", OP_CMCC },
        { "460", "04", OP_CMCC },
        { "460", "07", OP_CMCC },
        { "460", "08", OP_CMCC },
        { "460", "03", OP_CT },
        { "460", "05", OP_CT },
        { "460", "11", OP_CT },
        { "460", "01", OP_CU },
        { "460", "06", OP_CU },
        { "460", "09", OP_CU },
        { "460", "15", OP_CBN },
    };
    int i = 0;
    int list_len = sizeof(plmn_op_info_list) / sizeof(tapi_plmn_op_code_info);

    for (i = 0; i < list_len; i++) {
        if (!strcmp(mcc, plmn_op_info_list[i].mcc) && !strcmp(mnc, plmn_op_info_list[i].mnc)) {
            return plmn_op_info_list[i].op_code;
        }
    }

    return OP_UNKNOW;
}

tapi_service_module get_service_module_by_proxy_type(enum dbus_proxy_type type)
{
    unsigned int service_table[] = {
        TAPI_SERVICE_MODEM,
        TAPI_SERVICE_RADIO,
        TAPI_SERVICE_CALL,
        TAPI_SERVICE_SIM,
        TAPI_SERVICE_STK,
        TAPI_SERVICE_DATA,
        TAPI_SERVICE_SMS,
        TAPI_SERVICE_CBS,
        TAPI_SERVICE_NETREG,
        TAPI_SERVICE_NETMON,
        TAPI_SERVICE_CALL_BARRING,
        TAPI_SERVICE_CALL_FORWARDING,
        TAPI_SERVICE_SS,
        TAPI_SERVICE_CALL_SETTING,
        TAPI_SERVICE_IMS,
        TAPI_SERVICE_PHONEBOOK,
        TAPI_SERVICE_NETWORK_OPERATOR,
    };

    if (type >= DBUS_PROXY_MAX_COUNT)
        return TAPI_SERVICE_NONE;

    return service_table[type];
}

const char* get_dbus_proxy_type_interface(enum dbus_proxy_type type)
{
    const char* dbus_proxy_server[] = {
        OFONO_MODEM_INTERFACE,
        OFONO_RADIO_SETTINGS_INTERFACE,
        OFONO_VOICECALL_MANAGER_INTERFACE,
        OFONO_SIM_MANAGER_INTERFACE,
        OFONO_STK_INTERFACE,
        OFONO_CONNECTION_MANAGER_INTERFACE,
        OFONO_MESSAGE_MANAGER_INTERFACE,
        OFONO_CELL_BROADCAST_INTERFACE,
        OFONO_NETWORK_REGISTRATION_INTERFACE,
        OFONO_NETMON_INTERFACE,
        OFONO_CALL_BARRING_INTERFACE,
        OFONO_CALL_FORWARDING_INTERFACE,
        OFONO_SUPPLEMENTARY_SERVICES_INTERFACE,
        OFONO_CALL_SETTINGS_INTERFACE,
        OFONO_IMS_INTERFACE,
        OFONO_PHONEBOOK_INTERFACE,
        OFONO_NETWORK_OPERATOR_INTERFACE,
    };

    if (type >= DBUS_PROXY_MAX_COUNT)
        return NULL;

    return dbus_proxy_server[type];
}

bool tapi_support_proxy_type(enum dbus_proxy_type type)
{
    const char* interface = get_dbus_proxy_type_interface(type);

    return is_interface_supported(interface);
}

bool tapi_support_interface(const char* interface)
{
    if (strcmp(interface, OFONO_MANAGER_INTERFACE) == 0)
        return true;

    for (int i = DBUS_PROXY_MODEM; i < DBUS_PROXY_MAX_COUNT; i++) {
        if (strcmp(interface, get_dbus_proxy_type_interface(i)) == 0
            && is_interface_supported(interface))
            return true;
    }

    return false;
}

GDBusProxy* get_dbus_proxy_by_type(dbus_context* ctx, int slot_id, enum dbus_proxy_type type)
{
    if (ctx->dbus_proxy[slot_id][type] == NULL) {
        ctx->dbus_proxy[slot_id][type] = g_dbus_proxy_new(
            ctx->client, tapi_utils_get_modem_path(slot_id), get_dbus_proxy_type_interface(type));
    }
    return ctx->dbus_proxy[slot_id][type];
}
