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
}

const char* get_env_interface_support_string(const char* interface)
{
    if (strcmp(interface, OFONO_MODEM_INTERFACE) == 0)
        return "OFONO_MODEM_INTERFACE_SUPPORT";
    else if (strcmp(interface, OFONO_RADIO_SETTINGS_INTERFACE) == 0)
        return "OFONO_RADIO_SETTINGS_INTERFACE_SUPPORT";
    else if (strcmp(interface, OFONO_VOICECALL_MANAGER_INTERFACE) == 0)
        return "OFONO_VOICECALL_MANAGER_INTERFACE_SUPPORT";
    else if (strcmp(interface, OFONO_SIM_MANAGER_INTERFACE) == 0)
        return "OFONO_SIM_MANAGER_INTERFACE_SUPPORT";
    else if (strcmp(interface, OFONO_STK_INTERFACE) == 0)
        return "OFONO_STK_INTERFACE_SUPPORT";
    else if (strcmp(interface, OFONO_CONNECTION_MANAGER_INTERFACE) == 0)
        return "OFONO_CONNECTION_MANAGER_INTERFACE_SUPPORT";
    else if (strcmp(interface, OFONO_MESSAGE_MANAGER_INTERFACE) == 0)
        return "OFONO_MESSAGE_MANAGER_INTERFACE_SUPPORT";
    else if (strcmp(interface, OFONO_CELL_BROADCAST_INTERFACE) == 0)
        return "OFONO_CELL_BROADCAST_INTERFACE_SUPPORT";
    else if (strcmp(interface, OFONO_NETWORK_REGISTRATION_INTERFACE) == 0)
        return "OFONO_NETWORK_REGISTRATION_INTERFACE_SUPPORT";
    else if (strcmp(interface, OFONO_NETMON_INTERFACE) == 0)
        return "OFONO_NETMON_INTERFACE_SUPPORT";
    else if (strcmp(interface, OFONO_CALL_BARRING_INTERFACE) == 0)
        return "OFONO_CALL_BARRING_INTERFACE_SUPPORT";
    else if (strcmp(interface, OFONO_CALL_FORWARDING_INTERFACE) == 0)
        return "OFONO_CALL_FORWARDING_INTERFACE_SUPPORT";
    else if (strcmp(interface, OFONO_SUPPLEMENTARY_SERVICES_INTERFACE) == 0)
        return "OFONO_SUPPLEMENTARY_SERVICES_INTERFACE_SUPPORT";
    else if (strcmp(interface, OFONO_CALL_SETTINGS_INTERFACE) == 0)
        return "OFONO_CALL_SETTINGS_INTERFACE_SUPPORT";
    else if (strcmp(interface, OFONO_IMS_INTERFACE) == 0)
        return "OFONO_IMS_INTERFACE_SUPPORT";
    else if (strcmp(interface, OFONO_PHONEBOOK_INTERFACE) == 0)
        return "OFONO_PHONEBOOK_INTERFACE_SUPPORT";

    return NULL;
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

const char* tapi_utils_registration_status_to_string(int status)
{
    switch (status) {
    case NETWORK_REGISTRATION_STATUS_NOT_REGISTERED:
        return "unregistered";
    case NETWORK_REGISTRATION_STATUS_REGISTERED:
        return "registered";
    case NETWORK_REGISTRATION_STATUS_SEARCHING:
        return "searching";
    case NETWORK_REGISTRATION_STATUS_DENIED:
        return "denied";
    case NETWORK_REGISTRATION_STATUS_UNKNOWN:
        return "unknown";
    case NETWORK_REGISTRATION_STATUS_ROAMING:
        return "roaming";
    case NETWORK_REGISTRATION_STATUS_NOT_REGISTERED_EM:
        return "unregistered_em";
    case NETWORK_REGISTRATION_STATUS_SEARCHING_EM:
        return "searching_em";
    case NETWORK_REGISTRATION_STATUS_DENIED_EM:
        return "denied_em";
    case NETWORK_REGISTRATION_STATUS_UNKNOWN_EM:
        return "unknown_em";
    }

    return "";
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

const char* tapi_utils_clir_status_to_string(tapi_clir_status status)
{
    switch (status) {
    case CLIR_DEFAULT:
        return "default";
    case CLIR_DISABLED:
        return "disabled";
    case CLIR_ENABLED:
        return "enabled";
    }

    return "default";
}

tapi_clir_status tapi_utils_clir_status_from_string(const char* status)
{
    if (status == NULL)
        return CLIR_DEFAULT;

    if (strcmp(status, "default") == 0) {
        return CLIR_DEFAULT;
    } else if (strcmp(status, "disabled") == 0) {
        return CLIR_DISABLED;
    } else if (strcmp(status, "enabled") == 0) {
        return CLIR_ENABLED;
    }

    return CLIR_DEFAULT;
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

bool is_interface_supported(const char* interface)
{
    const char* interface_support_str;

    interface_support_str = getenv(get_env_interface_support_string(interface));
    if (interface_support_str != NULL && *interface_support_str != '\0') {
        bool interface_cap;
        char* endp;

        interface_cap = (bool)strtoul(interface_support_str, &endp, 10);
        if (*endp == '\0')
            return interface_cap;
    }

    return true;
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
