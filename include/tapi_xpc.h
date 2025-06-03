/****************************************************************************
 * frameworks/telephony/include/tapi_xpc.h
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

#ifndef __TAPI_XPC_H
#define __TAPI_XPC_H

#ifdef CONFIG_PHONE_SERVICE_WTP
#include "bt_wtp.h"
#endif
#include "tapi.h"
#include "tapi_phone.h"

// module id
enum phonesrv_module_type {
    PHONE_SERVICE_ESIM = 1,
    PHONE_SERVICE_WTP,
    PHONE_SERVICE_HFP_HF,
};

// message id
enum phonesrv_msg_type {
    PHONE_SERVICE_WTP_REGISTER_CALLBACK = 1,
    PHONE_SERVICE_WTP_UNREGISTER_CALLBACK,
    PHONE_SERVICE_WTP_CONN_STATE_CHANGED,
    PHONE_SERVICE_WTP_DISCOVERY_STATE_CHANGED,
    PHONE_SERVICE_WTP_VISIBILITY_STATE_CHANGED,
    PHONE_SERVICE_WTP_TRANSPORT_REQUESTED,
    PHONE_SERVICE_WTP_DEVICE_FOUND,
    PHONE_SERVICE_WTP_REMOTE_INFO_CHANGED,
    PHONE_SERVICE_WTP_SET_LOCAL_INFO,
    PHONE_SERVICE_WTP_SET_AUDIO_TYPE,
    PHONE_SERVICE_WTP_MODIFY_DISCOVERY,
    PHONE_SERVICE_WTP_MODIFY_VISIBILITY,
    PHONE_SERVICE_WTP_DIAL,
    PHONE_SERVICE_WTP_HANGUP,
    PHONE_SERVICE_WTP_ANSWER,
    PHONE_SERVICE_WTP_REJECT,
    PHONE_SERVICE_WTP_MAX,

    PHONE_SERVICE_ESIM_REGISTER_CALLBACK = 50,
    PHONE_SERVICE_ESIM_UNREGISTER_CALLBACK,
    PHONE_SERVICE_ESIM_MODIFY_RADIO_POWER,
    PHONE_SERVICE_ESIM_DIAL,
    PHONE_SERVICE_ESIM_ANSWER,
    PHONE_SERVICE_ESIM_REJECT,
    PHONE_SERVICE_ESIM_HANGUP,
    PHONE_SERVICE_ESIM_RELEASE_AND_ANSWER,
    PHONE_SERVICE_ESIM_HOLD_AND_ANSWER,
    PHONE_SERVICE_ESIM_HOLD_CALL,
    PHONE_SERVICE_ESIM_MERGE_CALL,
    PHONE_SERVICE_ESIM_SEND_TONES,
    PHONE_SERVICE_ESIM_NETWORK_OPERATOR_STATUS_CHANGED,
    PHONE_SERVICE_ESIM_NETWORK_OPERATOR_NAME_CHANGED,
    PHONE_SERVICE_ESIM_NETWORK_REG_STATUS_CHANGED,
    PHONE_SERVICE_ESIM_NETWORK_STRENGTH_CHANGED,
    PHONE_SERVICE_ESIM_RADIO_POWER_CHANGED,
    PHONE_SERVICE_ESIM_MODEM_STATUS_CHANGED,
    PHONE_SERVICE_ESIM_RADIO_STATE_CHANGED,
    PHONE_SERVICE_ESIM_CALL_STATE_CHANGED,

    PHONE_SERVICE_MAX,
};

#ifdef CONFIG_PHONE_SERVICE_WTP
typedef struct {
    uint8_t addr_type;
    uint8_t signal;
    uint16_t rfu;
    bt_address_t addr;
    char name[WTP_NAME_LEN_MAX + 1];
    char number1[WTP_PHONE_NUMBER_LEN_MAX + 1];
    char number2[WTP_PHONE_NUMBER_LEN_MAX + 1];
    uint8_t position[sizeof(wtp_data_t) + WTP_PRIV_DATA_LEN_MAX];
} wtp_xpc_device_t; // ref wtp_remote_t

typedef struct {
    wtp_xpc_device_t device_info;
    wtp_param_t param;
    void* user_data;
    uint16_t other_info_len;
    uint8_t value[0];
} wtp_xpc_data_t;

typedef struct {
    char name[WTP_NAME_LEN_MAX + 1];
    char number1[WTP_PHONE_NUMBER_LEN_MAX + 1];
    char number2[WTP_PHONE_NUMBER_LEN_MAX + 1];
    uint8_t position[sizeof(wtp_data_t) + WTP_PRIV_DATA_LEN_MAX];
    void* user_data;
} wtp_xpc_device_local_t; // ref wtp_local_t

typedef struct {
    wtp_callbacks_t wtp_callback;
    void* user_data;
} wtp_xpc_callbacks_t;

typedef struct {
    void* user_data;
} wtp_xpc_unregister_callback_t;

typedef struct {
    int type;
    void* user_data;
} wtp_audio_type_t;

typedef struct {
    bool enable;
    void* user_data;
} wtp_visibility_t;

typedef struct {
    bool enable;
    void* user_data;
} wtp_discovery_t;

typedef struct {
    int state;
    int reason;
    wtp_connection_state_changed_callback func_cb;
    wtp_xpc_device_t device_info;
    wtp_param_t param;
} wtp_conn_state_xpc_data_t;

typedef struct {
    int started;
    int reason;
    wtp_discovery_state_changed_callback func_cb;
} wtp_discovery_state_xpc_data_t;

typedef struct {
    int visible;
    int reason;
    wtp_visibility_changed_callback func_cb;
} wtp_visibility_state_xpc_data_t;

typedef struct {
    wtp_xpc_device_t device_info;
    wtp_transport_requested_callback func_cb;
    wtp_param_t param;
} wtp_requested_xpc_data_t;

typedef struct {
    wtp_xpc_device_t device_info;
    wtp_device_found_callback func_cb;
    wtp_param_t param;
} wtp_found_xpc_data_t;

typedef struct {
    wtp_xpc_device_t device_info;
    wtp_remote_info_changed_callback func_cb;
} wtp_remote_info_xpc_data_t;
#endif

typedef struct {
    int ret;
    void* aync_handler;
} common_resp_t;

typedef struct {
    tele_callbacks_t tele_callback;
    void* user_data;
} xpc_tele_reg_callbacks_t;

typedef struct {
    void* user_data;
} xpc_tele_unreg_callbacks_t;

typedef struct {
    bool enable;
    void* user_data;
} xpc_tele_radio_power_t;

typedef struct {
    int slot_id;
    char number[81];
    int hide_callerid;
    void* user_data;
} xpc_tele_dial_t;

typedef struct {
    int slot_id;
    char call_id[MAX_CALL_ID_LENGTH + 1];
    void* user_data;
} xpc_tele_answer_t;

typedef struct {
    int slot_id;
    int call_id_exist; // 0 means reject all,1 means reject by call id
    char call_id[MAX_CALL_ID_LENGTH + 1];
    void* user_data;
} xpc_tele_reject_t;

typedef struct {
    int slot_id;
    int call_id_exist; // 0 means hangup all,1 means hangup by call id
    char call_id[MAX_CALL_ID_LENGTH + 1];
    void* user_data;
} xpc_tele_hangup_t;

typedef struct {
    int slot_id;
    void* user_data;
} xpc_tele_common_req_t;

typedef struct {
    int slot_id;
    bool hold;
    void* user_data;
} xpc_tele_hold_unhold_req_t;

typedef struct {
    int slot_id;
    char tone[MAX_TONE_LEN];
    void* user_data;
} xpc_tele_tones_t;

typedef struct {
    int radio_state;
    radio_state_change_callback_t radio_state_change_cb;
} xpc_tele_radio_state_change_cb_t;

typedef struct {
    int status;
    network_operator_status_changed_callback_t operator_status_changed_cb;
} xpc_tele_operator_status_change_cb_t;

typedef struct {
    char operator_name[MAX_OPERATOR_NAME_LENGTH + 1];
    network_operator_name_changed_callback_t operator_name_changed_cb;
} xpc_tele_operator_name_change_cb_t;

typedef struct {
    int status;
    network_reg_state_changed_callback_t network_reg_state_changed_cb;
} xpc_tele_reg_state_change_cb_t;

typedef struct {
    int strength;
    network_strength_changed_callback_t strength_changed_cb;
} xpc_tele_network_strength_change_cb_t;

typedef struct {
    int status;
    modem_status_changed_callback_t modem_status_changed_cb;
} xpc_tele_modem_status_change_cb_t;

typedef struct {
    bool state;
    radio_power_changed_callback_t radio_power_changed_cb;
} xpc_tele_radio_power_change_cb_t;

typedef struct {
    tapi_call_info call_info;
    call_state_changed_callback_t call_state_changed_cb;
} xpc_tele_call_state_change_cb_t;

#endif
