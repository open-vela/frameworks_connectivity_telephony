/****************************************************************************
 * frameworks/telephony/include/tapi_network.h
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

#ifndef __TELE_NETWORK_H
#define __TELE_NETWORK_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tapi.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_OPERATOR_INFO_LIST_SIZE 30
#define MAX_CELL_INFO_LIST_SIZE 30
#define MAX_NETWORK_INFO_LENGTH 100

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef enum {
    TYPE_GSM = 0,
    TYPE_CDMA,
    TYPE_LTE,
    TYPE_WCDMA,
    TYPE_TDSCDMA,
    TYPE_NR,
} tapi_cell_type;

typedef struct {
    int sec;
    int min;
    int hour;
    int mday;
    int mon;
    int year;
    int dst;
    int utcoff;
} tapi_network_time;

typedef struct {
    tapi_registration_state reg_state;
    char technology[MAX_NETWORK_INFO_LENGTH + 1];
    char operator_name[MAX_OPERATOR_NAME_LENGTH + 1];
    char mcc[MAX_MCC_LENGTH + 1];
    char mnc[MAX_MCC_LENGTH + 1];
    char station[MAX_NETWORK_INFO_LENGTH + 1];
    tapi_network_time nitz_time;
    tapi_selection_mode selection_mode;
    bool emergency_only;
    tapi_roaming_type roaming_type;
    u_int32_t cell_id;
    u_int16_t lac;
    u_int16_t denial_reason;
} tapi_registration_info;

typedef struct {
    char id[MAX_NETWORK_INFO_LENGTH + 1];
    char name[MAX_OPERATOR_NAME_LENGTH + 1];
    char mcc[MAX_MCC_LENGTH + 1];
    char mnc[MAX_MNC_LENGTH + 1];
    int status;
    char technology[MAX_NETWORK_INFO_LENGTH + 1];
    void* extra;
} tapi_operator_info;

typedef struct {
    int rssi;
    int rsrp;
    int rsrq;
    int rssnr;
    int level;
} tapi_signal_strength;

typedef struct {
    int ci;
    int pci;
    int tac;
    int lac;
    int earfcn;
    int bandwidth;
    int bands[MAX_NETWORK_INFO_LENGTH + 1];
    tapi_cell_type type;
    char mcc_str[MAX_MCC_LENGTH + 1];
    char mnc_str[MAX_MCC_LENGTH + 1];
    char alpha_long[MAX_NETWORK_INFO_LENGTH + 1];
    char alpha_short[MAX_NETWORK_INFO_LENGTH + 1];
    tapi_signal_strength signal_strength;
} tapi_cell_identity;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Automatic network selection.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_network_select_auto(tapi_context context,
    int slot_id, int event_id, tapi_async_function p_handle);

/**
 * Manual network selection.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] operator       Operator Information returned from modem
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_network_select_manual(tapi_context context,
    int slot_id, int event_id, tapi_operator_info operator, tapi_async_function p_handle);

/**
 * Manual network scan.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_network_scan(tapi_context context,
    int slot_id, int event_id, tapi_async_function p_handle);

/**
 * Get service cell information.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_network_get_serving_cellinfo(tapi_context context,
    int slot_id, int event_id, tapi_async_function p_handle);

/**
 * Get Neighborhood Information.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_network_get_neighbouring_cellinfos(tapi_context context,
    int slot_id, int event_id, tapi_async_function p_handle);

/**
 * Get CS domain network type.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           Network type returned from modem.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_network_get_voice_network_type(tapi_context context, int slot_id, tapi_network_type* out);

/**
 * Determine whether the CS domain is roaming.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @return Zero on success; a negated errno value on failure.
 */
bool tapi_network_is_voice_roaming(tapi_context context, int slot_id);

/**
 * Get the display name of current registered operator.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           display name returned from modem.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_network_get_display_name(tapi_context context, int slot_id, char** out);

/**
 * Get the current signal strength.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           signal strength returned from modem.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_network_get_signalstrength(tapi_context context, int slot_id, tapi_signal_strength* out);

/**
 * Get current registration status infomation.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_network_get_registration_info(tapi_context context,
    int slot_id, int event_id, tapi_async_function p_handle);

/**
 * Register network event callback.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] msg            Network event ID.
 * @param[in] p_handle       Event callback.
 * @return Positive value as watch_id on success; a negated errno value on failure.
 */
int tapi_network_register(tapi_context context,
    int slot_id, tapi_indication_msg msg, tapi_async_function p_handle);

/**
 * Unregister network event callback.
 * @param[in] context        Telephony api context.
 * @param[in] watch_id       Watch id.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_network_unregister(tapi_context context, int watch_id);

#ifdef __cplusplus
}
#endif

#endif /* __TELE_NETWORK_H */