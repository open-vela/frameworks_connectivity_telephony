/****************************************************************************
 * frameworks/telephony/include/tapi.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __TELEPHONY_APIS_H
#define __TELEPHONY_APIS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_PHONE_NUMBER_LENGTH 80
#define MAX_CALLER_NAME_LENGTH 80
#define MAX_IMEI_STRING_LENGTH 20
#define MAX_OPERATOR_NAME_LENGTH 63
#define MAX_CALL_ID_LENGTH 100
#define MAX_TX_TIME_ARRAY_LEN 5
#define MAX_OEM_RIL_RESP_STRINGS_LENTH 20
#define MAX_MODEM_COUNT 10

/* MCC is always three digits. MNC is either two or three digits */
#define MAX_MCC_LENGTH 3
#define MAX_MNC_LENGTH 3

#define DEFAULT_SLOT_ID 0

#define KEY_CARRIER_CONFIG_SPN_STRING "Spn"

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef enum {
    SLOT_ID_1 = 0,
    SLOT_ID_2,
    SLOT_ID_MAX
} tapi_slot_id;

typedef enum {
    FEATURE_VOICE = 0,
    FEATURE_DATA,
    FEATURE_SMS,
    FEATURE_IMS
} tapi_feature_type;

typedef enum {
    PHONE_IDLE = 0,
    PHONE_RINGING,
    PHONE_OFFHOOK,
} tapi_phone_state;

typedef enum {
    STATE_IN_SERVICE = 0,
    STATE_OUT_OF_SERVICE,
    STATE_EMERGENCY_ONLY,
    STATE_POWER_OFF
} tapi_service_state;

typedef enum {
    UNKNOWN = -1,
    AVAILABLE = 0,
    CURRENTL,
    FORBIDDEN,
} tapi_operator_status;

typedef enum {
    MODEM_STATE_POWER_OFF,
    MODEM_STATE_AWARE,
    MODEM_STATE_ALIVE,
} tapi_modem_state;

/*
 * registration state as follows:
 * 0 - Not registered, MT is not currently searching a new operator to register.
 * 1 - Registered, home network.
 * 2 - Not registered, but MT is currently searching a new operator to register.
 * 3 - Registration denied.
 * 4 - Unknown.
 * 5 - Registered, roaming.
 * 10 - Same as 0, but indicates that emergency calls are enabled.
 * 12 - Same as 2, but indicates that emergency calls are enabled.
 * 13 - Same as 3, but indicates that emergency calls are enabled.
 * 14 - Same as 4, but indicates that emergency calls are enabled.
 */
typedef enum {
    NETWORK_REGISTRATION_STATUS_NOT_REGISTERED = 0,
    NETWORK_REGISTRATION_STATUS_REGISTERED = 1,
    NETWORK_REGISTRATION_STATUS_SEARCHING = 2,
    NETWORK_REGISTRATION_STATUS_DENIED = 3,
    NETWORK_REGISTRATION_STATUS_UNKNOWN = 4,
    NETWORK_REGISTRATION_STATUS_ROAMING = 5,
    NETWORK_REGISTRATION_STATUS_REGISTERED_SMS_EUTRAN = 6,
    NETWORK_REGISTRATION_STATUS_ROAMING_SMS_EUTRAN = 7,
    NETWORK_REGISTRATION_STATUS_REGISTED_EM = 8,
    NETWORK_REGISTRATION_STATUS_NOT_REGISTERED_EM = 10,
    NETWORK_REGISTRATION_STATUS_SEARCHING_EM = 12,
    NETWORK_REGISTRATION_STATUS_DENIED_EM = 13,
    NETWORK_REGISTRATION_STATUS_UNKNOWN_EM = 14,
} tapi_registration_state;

typedef enum {
    NETWORK_ROAMING_UNKNOWN = 0,
    NETWORK_ROAMING_DOMESTIC,
    NETWORK_ROAMING_INTERNATIONAL,
} tapi_roaming_type;

typedef enum {
    NETWORK_SELECTION_UNKNOWN = -1,
    NETWORK_SELECTION_AUTO = 0,
    NETWORK_SELECTION_MANUAL,
} tapi_selection_mode;

typedef enum {
    NETWORK_PREF_NET_TYPE_ANY = -1,
    NETWORK_PREF_NET_TYPE_UMTS = 0,
    NETWORK_PREF_NET_TYPE_GSM_ONLY = 1,
    NETWORK_PREF_NET_TYPE_WCDMA_ONLY = 2,
    NETWORK_PREF_NET_TYPE_LTE_GSM_WCDMA = 9,
    NETWORK_PREF_NET_TYPE_LTE_ONLY = 11,
    NETWORK_PREF_NET_TYPE_LTE_WCDMA = 12,
} tapi_pref_net_mode;

typedef enum {
    NETWORK_TYPE_UNKNOWN = 0,
    NETWORK_TYPE_GPRS = 1,
    NETWORK_TYPE_EDGE = 2,
    NETWORK_TYPE_UMTS = 3,
    NETWORK_TYPE_HSDPA = 8,
    NETWORK_TYPE_HSUPA = 9,
    NETWORK_TYPE_HSPA = 12,
    NETWORK_TYPE_LTE = 13,
    NETWORK_TYPE_LTE_CA = 19,
} tapi_network_type;

typedef enum {
    RADIO_STATE_UNKNOWN = -1,
    RADIO_STATE_UNAVAILABLE = 0,
    RADIO_STATE_ON,
    RADIO_STATE_OFF,
} tapi_radio_state;

typedef enum {
    SIM_UICC_APP_UNKNOWN = -1,
    SIM_UICC_APP_INACTIVE,
    SIM_UICC_APP_ACTIVE,
} tapi_sim_uicc_app_state;

typedef struct {
    char number[MAX_PHONE_NUMBER_LENGTH + 1];
    int type;
} tapi_phone_number;

typedef enum {
    RESPONSE = 0,
    INDICATION = 1,
} tapi_message_type;

typedef struct {
    int msg_id;
    tapi_message_type msg_type;
    int status;
    int arg1;
    int arg2;
    void* data;
    void* user_obj;
} tapi_async_result;

typedef void (*tapi_async_function)(tapi_async_result* result);
typedef void (*tapi_client_ready_function)(const char* client_name, void* user_data);

typedef enum {
    // Generic Indication Message
    MSG_RADIO_STATE_CHANGE_IND = 0,
    MSG_PHONE_STATE_CHANGE_IND,
    MSG_OEM_HOOK_RAW_IND,
    MSG_MODEM_RESTART_IND,
    MSG_DEVICE_INFO_CHANGE_IND,
    MSG_AIRPLANE_MODE_CHANGE_IND,

    // Below call signals were obsolete
    MSG_CALL_ADD_MESSAGE_IND,
    MSG_CALL_REMOVE_MESSAGE_IND,
    MSG_CALL_FORWARDED_MESSAGE_IND,
    MSG_CALL_BARRING_ACTIVE_MESSAGE_IND,
    MSG_CALL_PROPERTY_CHANGED_MESSAGE_IND,
    MSG_CALL_DISCONNECTED_REASON_MESSAGE_IND,
    MSG_CALL_MERGE_IND,
    MSG_CALL_SEPERATE_IND,

    // Call Indication Message
    MSG_CALL_STATE_CHANGE_IND,
    MSG_CALL_RING_BACK_TONE_IND,
    MSG_ECC_LIST_CHANGE_IND,
    MSG_DEFAULT_VOICECALL_SLOT_CHANGE_IND,

    // Network Indication Message
    MSG_NETWORK_STATE_CHANGE_IND,
    MSG_VOICE_REGISTRATION_STATE_CHANGE_IND,
    MSG_CELLINFO_CHANGE_IND,
    MSG_SIGNAL_STRENGTH_CHANGE_IND,
    MSG_NITZ_STATE_CHANGE_IND,

    // Data Indication Message
    MSG_DATA_ENABLED_CHANGE_IND,
    MSG_DATA_REGISTRATION_STATE_CHANGE_IND,
    MSG_DATA_NETWORK_TYPE_CHANGE_IND,
    MSG_DATA_CONNECTION_STATE_CHANGE_IND,
    MSG_DEFAULT_DATA_SLOT_CHANGE_IND,

    // SIM Indication Message
    MSG_SIM_STATE_CHANGE_IND,
    MSG_SIM_UICC_APP_ENABLED_CHANGE_IND,
    MSG_SIM_ICCID_CHANGE_IND,

    // STK Indication Message
    MSG_STK_AGENT_DISPLAY_TEXT_IND,
    MSG_STK_AGENT_REQUEST_DIGIT_IND,
    MSG_STK_AGENT_REQUEST_KEY_IND,
    MSG_STK_AGENT_REQUEST_CONFIRMATION_IND,
    MSG_STK_AGENT_REQUEST_INPUT_IND,
    MSG_STK_AGENT_REQUEST_DIGITS_IND,
    MSG_STK_AGENT_PLAY_TONE_IND,
    MSG_STK_AGENT_LOOP_TONE_IND,
    MSG_STK_AGENT_REQUEST_SELECTION_IND,
    MSG_STK_AGENT_REQUEST_QUICK_DIGIT_IND,
    MSG_STK_AGENT_CONFIRM_CALL_SETUP_IND,
    MSG_STK_AGENT_DISPLAY_ACTION_INFORMATION_IND,
    MSG_STK_AGENT_CONFIRM_LAUNCH_BROWSER_IND,
    MSG_STK_AGENT_DISPLAY_ACTION_IND,
    MSG_STK_AGENT_CONFIRM_OPEN_CHANNEL_IND,
    MSG_STK_AGENT_RELEASE_IND,
    MSG_STK_AGENT_CANCEL_IND,

    // SMS Indication Message
    MSG_INCOMING_MESSAGE_IND,
    MSG_IMMEDIATE_MESSAGE_IND,
    MSG_STATUS_REPORT_MESSAGE_IND,
    MSG_DEFAULT_SMS_SLOT_CHANGED_IND,

    // CBS Indication Message
    MSG_INCOMING_CBS_IND,
    MSG_EMERGENCY_CBS_IND,

    // SS Indication Message
    MSG_CALL_BARRING_PROPERTY_CHANGE_IND,
    MSG_USSD_NOTIFICATION_RECEIVED_IND,
    MSG_USSD_REQUEST_RECEIVED_IND,
    MSG_USSD_PROPERTY_CHANGE_IND,

    // IMS Indication Message
    MSG_IMS_REGISTRATION_MESSAGE_IND,

    // modem state change Message
    MSG_MODEM_STATE_CHANGE_IND,

    MSG_DATA_LOGING_IND,

    MSG_MODEM_ECC_LIST_CHANGE_IND = 61,

    // tapi indication msg value max.
    MSG_IND_MASK,

    // tapi indication msg user custom start.
    MSG_IND_USER_CUSTOM_FIRST,
} tapi_indication_msg;

typedef enum {
    OP_CU = 1,
    OP_CMCC,
    OP_CT,
    OP_CBN,
    OP_UNKNOW,
} tapi_op_code;

typedef struct {
    char mcc[MAX_MCC_LENGTH + 1];
    char mnc[MAX_MNC_LENGTH + 1];
    int op_code;
} tapi_plmn_op_code_info;

typedef void* tapi_context;

#include <tapi_call.h>
#include <tapi_cbs.h>
#include <tapi_data.h>
#include <tapi_ims.h>
#include <tapi_manager.h>
#include <tapi_network.h>
#include <tapi_phonebook.h>
#include <tapi_sim.h>
#include <tapi_sms.h>
#include <tapi_ss.h>

const char* tapi_utils_network_mode_to_string(tapi_pref_net_mode mode);
tapi_pref_net_mode tapi_utils_network_mode_from_string(const char* mode);
tapi_network_type tapi_utils_network_type_from_ril_tech(int type);
const char* tapi_utils_get_registration_status_string(int status);
tapi_registration_state tapi_utils_registration_status_from_string(const char* status);
tapi_selection_mode tapi_utils_registration_mode_from_string(const char* mode);
tapi_operator_status tapi_utils_operator_status_from_string(const char* mode);
const char* tapi_utils_apn_type_to_string(tapi_data_context_type type);
tapi_data_context_type tapi_utils_apn_type_from_string(const char* type);
const char* tapi_utils_apn_auth_to_string(tapi_data_auth_method auth);
tapi_data_auth_method tapi_utils_apn_auth_from_string(const char* auth);
const char* tapi_utils_apn_proto_to_string(tapi_data_proto proto);
tapi_data_proto tapi_utils_apn_proto_from_string(const char* proto);
tapi_call_status tapi_utils_call_status_from_string(const char* status);
tapi_call_disconnect_reason tapi_utils_call_disconnected_reason(const char* str_status);
const char* tapi_utils_cell_type_to_string(tapi_cell_type type);
tapi_cell_type tapi_utils_cell_type_from_string(const char* name);
const char* tapi_utils_get_modem_path(int slot_id);
int tapi_utils_get_slot_id(const char* modem_path);
const char* tapi_sim_state_to_string(tapi_sim_state sim_state);
const char* tapi_utils_clir_status_to_string(tapi_clir_status status);
tapi_clir_status tapi_utils_clir_status_from_string(const char* status);

#endif /* __TELEPHONY_APIS_H */
