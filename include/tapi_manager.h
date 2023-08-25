/****************************************************************************
 * frameworks/telephony/include/tapi_manager.h
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

#ifndef __TELEPHONY_MANAGER_H
#define __TELEPHONY_MANAGER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "tapi.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct {
    int sleep_time;
    int idle_time;
    int tx_time[MAX_TX_TIME_ARRAY_LEN];
    int rx_time;
} modem_activity_info;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Init telephony library.
 * @param[in] client_name        Calling context name.
 * Must contain one dot character at least. For example, miot.app
 * @param[in] user_data          user data pointer
 * @param[in] callback           callback function one tapi is ready.
 * @return Pointer to created context or NULL on failure.
 */
tapi_context tapi_open(const char* client_name,
    tapi_client_ready_function callback, void* user_data);

/**
 * Close telephony library.
 * @param[in] context        Telephony api context.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_close(tapi_context context);

/**
 * Check whether the specified feature is supported or not.
 * @param[in] feature   Telephony feature type.
 * @return
 *         - true: the feature is supported.
 *         - false: the feature is not supported.
 */
bool tapi_is_feature_supported(tapi_feature_type feature);

/**
 * Set radio power state.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] state          State passed to modem.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_set_radio_power(tapi_context context,
    int slot_id, int event_id, bool state, tapi_async_function p_handle);

/**
 * Get radio power state.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           State returned from modem.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_get_radio_power(tapi_context context, int slot_id, bool* out);

/**
 * Set preferred network mode.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] mode           Network mode passed to modem.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_set_pref_net_mode(tapi_context context,
    int slot_id, int event_id, tapi_pref_net_mode mode, tapi_async_function p_handle);

/**
 * Get preferred network mode.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           Network mode returned from modem.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_get_pref_net_mode(tapi_context context, int slot_id, tapi_pref_net_mode* out);

/**
 * Get radio state.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           Radio state returned from modem.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_get_radio_state(tapi_context context, int slot_id, tapi_radio_state* out);

/**
 * Get device imei.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           Imei returned from modem.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_get_imei(tapi_context context, int slot_id, char** out);

/**
 * Get device imeisv.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           Imeisv returned from modem.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_get_imeisv(tapi_context context, int slot_id, char** out);

/**
 * Get modem revision.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           Revision returned from modem.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_get_modem_revision(tapi_context context, int slot_id, char** out);

/**
 * Get phone state.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] state         Phone state returned from modem.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_get_phone_state(tapi_context context, int slot_id, tapi_phone_state* state);

/**
 * Get msisdn number.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           Msisdn number returned from modem.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_get_msisdn_number(tapi_context context, int slot_id, char** out);

/**
 * get modem activity info.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_get_modem_activity_info(tapi_context context, int slot_id,
    int event_id, tapi_async_function p_handle);

/**
 * Returns RIL responses to raw oem request.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] oem_req        Oem RIL request data.
 * @param[in] length         The length of oem RIL request data.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_invoke_oem_ril_request_raw(tapi_context context, int slot_id, int event_id,
    unsigned char oem_req[], int length, tapi_async_function p_handle);

/**
 * Returns RIL responses to strings oem request.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] oem_req        Oem RIL request data.
 * @param[in] length         The length of oem RIL request data.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_invoke_oem_ril_request_strings(tapi_context context, int slot_id, int event_id,
    char* oem_req[], int length, tapi_async_function p_handle);

/**
 * enable or disable modem.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] enable         Indicate whether to enable modem.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_enable_modem(tapi_context context, int slot_id,
    int event_id, bool enable, tapi_async_function p_handle);

/**
 * get modem status asynchronously.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_get_modem_status(tapi_context context, int slot_id,
    int event_id, tapi_async_function p_handle);

/**
 * get modem status synchronously.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           Modem state returned core stack.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_get_modem_status_sync(tapi_context context, int slot_id, tapi_modem_state* out);

/**
 * Set fast dormancy to modem.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] state          fast dormancy state to modem.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_set_fast_dormancy(tapi_context context,
    int slot_id, int event_id, bool state, tapi_async_function p_handle);

/**
 * Register radio event callback.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] msg            Radio event ID.
 * @param[in] user_obj       User data.
 * @param[in] p_handle       Event callback.
 * @return Positive value as watch_id on success; a negated errno value on failure.
 */
int tapi_register(tapi_context context,
    int slot_id,
    tapi_indication_msg msg,
    void* user_obj,
    tapi_async_function p_handle);

/**
 * Unregister radio event callback.
 * @param[in] context        Telephony api context.
 * @param[in] watch_id       Watch id.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_unregister(tapi_context context, int watch_id);

#ifdef __cplusplus
}
#endif

#endif /* __TELEPHONY_MANAGER_H */
