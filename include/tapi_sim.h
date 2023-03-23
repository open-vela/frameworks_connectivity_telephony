/****************************************************************************
 * frameworks/telephony/include/tapi_sim.h
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

#ifndef __TELEPHONY_SIM_H
#define __TELEPHONY_SIM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "tapi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_SIM_PWD_TYPE 20

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct {
    char* name;
    int value;
    void* data;
} sim_state_result;

typedef struct {
    char* sim_pwd_type[MAX_SIM_PWD_TYPE];
    char retry_count[MAX_SIM_PWD_TYPE];
} sim_lock_state;

typedef enum {
    SIM_STATE_NOT_PRESENT = 0,
    SIM_STATE_INSERTED = 1,
    SIM_STATE_LOCKED_OUT = 2,
    SIM_STATE_READY = 3,
    SIM_STATE_RESETTING = 4,
} tapi_sim_state;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Whether an ICC card is present for a subscription.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           State of Icc card.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sim_has_icc_card(tapi_context context, int slot_id, bool* out);

/**
 * Returns a constant indicating the state of the current SIM card.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           State of Icc card. The possible values are:
				0 - SIM_STATE_NOT_PRESENT
				1 - SIM_STATE_INSERTED
				2 - SIM_STATE_LOCKED_OUT
				3 - STATE_READY
				4 - SIM_STATE_RESETTING
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sim_get_sim_state(tapi_context context, int slot_id, int* out);

/**
 * Get the MCC+MNC (mobile country code + mobile network code) of the
 * provider of the SIM. 5 or 6 decimal digits.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] length         Length of out param.
 * @param[out] out           Operator returned from ICC.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sim_get_sim_operator(tapi_context context, int slot_id, int length, char* out);

/**
 * Get the Service Provider Name (SPN).
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           SimOperatorName returned from ICC.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sim_get_sim_operator_name(tapi_context context, int slot_id, char** out);

/**
 * Get the sim serial number for the given subscription, if applicable.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           Sim serial number returned from ICC.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sim_get_sim_iccid(tapi_context context, int slot_id, char** out);

/**
 * Get the unique subscriber ID, for example, the IMSI of current sim.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           Subcriber id returned from ICC.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sim_get_subscriber_id(tapi_context context, int slot_id, char** out);

/**
 * Register sim state changed event callback.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] msg            Sim Message ID.
 * @param[in] p_handle       Event callback.
 * @return Positive value as watch_id on success; a negated errno value on failure.
 */
int tapi_sim_register(tapi_context context,
    int slot_id, tapi_indication_msg msg, tapi_async_function p_handle);

/**
 * Unregister sim state changed event callback.
 * @param[in] context        Telephony api context.
 * @param[in] watch_id       Watch id.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sim_unregister(tapi_context context, int watch_id);

/**
 * Change the ICC password used in ICC pin lock or ICC fdn enable.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] pin_type       Supported by two types "pin" or "pin2".
 * @param[in] old_pin        Old pin value.
 * @param[in] new_pin        New pin value.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sim_change_pin(tapi_context context, int slot_id,
    int event_id, char* pin_type, char* old_pin, char* new_pin, tapi_async_function p_handle);

/**
 * Enters the currently pending pin.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] pin_type       Supported by two types "pin" or "pin2".
 * @param[in] pin            Pin value.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sim_enter_pin(tapi_context context, int slot_id,
    int event_id, char* pin_type, char* pin, tapi_async_function p_handle);

/**
 * Provides the unblock key to the modem and if correct
 * resets the pin to the new value of newpin.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] puk_type       Supported by two types "puk" or "puk2".
 * @param[in] puk            Puk value.
 * @param[in] new_pin        New pin value.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sim_reset_pin(tapi_context context, int slot_id,
    int event_id, char* puk_type, char* puk, char* new_pin, tapi_async_function p_handle);

/**
 * Activates the lock for the particular pin type. The
 * device will ask for a PIN automatically next time the
 * device is turned on or the SIM is removed and
 * re-inserted. The current PIN is required for the
 * operation to succeed.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] pin_type       Supported by "pin". "pin2" is not supported according to 27.007.
 * @param[in] pin            Pin value.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sim_lock_pin(tapi_context context, int slot_id,
    int event_id, char* pin_type, char* pin, tapi_async_function p_handle);

/**
 * Deactivates the lock for the particular pin type. The
 * current PIN is required for the operation to succeed.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] pin_type       Supported by "pin". "pin2" is not supported according to 27.007.
 * @param[in] pin            Pin value.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sim_unlock_pin(tapi_context context, int slot_id,
    int event_id, char* pin_type, char* pin, tapi_async_function p_handle);

/**
 * Open a new logical channel and select the given application.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] aid            Aid value.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sim_open_logical_channel(tapi_context context, int slot_id,
    int event_id, char* aid, tapi_async_function p_handle);

/**
 * Close a previously opened logical channel given by
 * the logical channel's session id.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] session_id     The logical channel's session id.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sim_close_logical_channel(tapi_context context, int slot_id,
    int event_id, int session_id, tapi_async_function p_handle);

/**
 * Transmit an APDU to the ICC card over a previously opened logical channel.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] sessionid      The logical channel's session id.
 * @param[in] pdu            Package Data Units.
 * @param[in] len            The length of pdu.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sim_transmit_apdu_logical_channel(tapi_context context, int slot_id,
    int event_id, int session_id, char* pdu, unsigned int len, tapi_async_function p_handle);

/**
 * Transmit an APDU to the ICC card over the basic channel.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] pdu            Package Data Units.
 * @param[in] len            The length of pdu.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sim_transmit_apdu_basic_channel(tapi_context context, int slot_id,
    int event_id, char* pdu, unsigned int len, tapi_async_function p_handle);

/**
 * Get uicc applications state.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           Uicc Applications state of current sim.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sim_get_uicc_enablement(tapi_context context, int slot_id, tapi_sim_uicc_app_state* out);

/**
 * Set uicc applications state.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] state          State uicc applications to sim.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sim_set_uicc_enablement(tapi_context context,
    int slot_id, int event_id, int state, tapi_async_function p_handle);

#ifdef __cplusplus
}
#endif

#endif /* __TELEPHONY_SIM_H */
