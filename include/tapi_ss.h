/****************************************************************************
 * frameworks/telephony/include/tapi_ss.h
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

#ifndef __TAPI_SS_H
#define __TAPI_SS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "tapi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_SS_CALL_FORWARD_NUM_LEN 40

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct {
    char* old_passwd;
    char* new_passwd;
} cb_change_passwd_param;

typedef struct {
    char* name;
    char* value;
    char* fac;
} tapi_call_barring_lock;

typedef struct {
    char phone_number[MAX_SS_CALL_FORWARD_NUM_LEN + 1];
    char voice_busy[MAX_SS_CALL_FORWARD_NUM_LEN + 1];
    char voice_no_reply[MAX_SS_CALL_FORWARD_NUM_LEN + 1];
    char voice_not_reachable[MAX_SS_CALL_FORWARD_NUM_LEN + 1];
    int voice_no_reply_timeout;
    int forwarding_flag_on_sim;
} tapi_call_forwarding_info;

typedef struct {
    char* ss_service_type;
    char* ss_service_operation;
    char* service_operation_requested;
    char* call_setting_status;
    char* ussd_response;
    char* append_service;
    char* append_service_value;
} tapi_ss_initiate_info;

typedef enum {
    USSD_STATE_IDLE = 0,
    USSD_STATE_ACTIVE = 1,
    USSD_STATE_USER_ACTION = 2,
    USSD_STATE_RESPONSE_SENT = 3,
} tapi_ussd_state;

typedef enum {
    CLIR_STATUS_NOT_PROVISIONED = 0,
    CLIR_STATUS_PROVISIONED_PERMANENT = 1,
    CLIR_STATUS_UNKNOWN = 2,
    CLIR_STATUS_TEMPORARY_RESTRICTED = 3,
    CLIR_STATUS_TEMPORARY_ALLOWED = 4,
} tapi_clir_status;

typedef enum {
    CLIP_STATUS_NOT_PROVISIONE = 0,
    CLIP_STATUS_PROVISIONED = 1,
    CLIP_STATUS_UNKNOWN = 2,
} tapi_clip_status;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sets a Call Barring option.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] facility       Different Call Barring services.
 * @param[in] pin2           Pin2 type.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ss_request_call_barring(tapi_context context, int slot_id, int event_id,
    char* facility, char* pin2, tapi_async_function p_handle);

/**
 * Gets Call Barring info.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] service_type   Different Call Barring services.
 * @param[out] out           The Call Barring service value.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ss_query_call_barring_info(tapi_context context, int slot_id, const char* service_type, char** out);

/**
 * Registers new network password for the Call Barring services.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] old_passwd     The old password of the Call Barring services.
 * @param[in] new_passwd     The new password of the Call Barring services.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ss_change_call_barring_password(tapi_context context, int slot_id, int event_id,
    char* old_passwd, char* new_passwd, tapi_async_function p_handle);

/**
 * Disables all Call Barrings.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] passwd         The password of the Call Barring services.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ss_disable_all_call_barrings(tapi_context context, int slot_id, int event_id,
    char* passwd, tapi_async_function p_handle);

/**
 * Disables barrings for incoming calls.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] passwd         The password of the Call Barring services.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ss_disable_all_incoming(tapi_context context, int slot_id,
    int event_id, char* passwd, tapi_async_function p_handle);

/**
 * Disables barrings for outgoing calls.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] passwd         The password of the Call Barring services.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ss_disable_all_outgoing(tapi_context context, int slot_id,
    int event_id, char* passwd, tapi_async_function p_handle);

/**
 * Sets voice Call Forwarding behavior.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] cf_type        Differnt Call Forwarding services.
 * @param[in] value          The value of Call Forwarding service.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ss_request_call_forwarding(tapi_context context, int slot_id, int event_id,
    const char* cf_type, char* value, tapi_async_function p_handle);

/**
 * Gets the Call Forwarding info.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] service_type   Call Forwarding services.
 * @param[out] out           The Call Forwarding services value.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ss_query_call_forwarding_info(tapi_context context, int slot_id, const char* service_type, char** out);

/**
 * Disables all Call Forwarding rules for type.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] cf_type        Call Forwarding services type.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ss_disable_call_forwarding(tapi_context context, int slot_id, int event_id,
    char* cf_type, tapi_async_function p_handle);

/**
 * If the command is a recognized supplementary service control string,
 * the corresponding SS request is made and the result is returned.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] command        Control string.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ss_initiate_service(tapi_context context, int slot_id, int event_id,
    char* command, tapi_async_function p_handle);

/**
 * Gets USSD state.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           The USSD state.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_get_ussd_state(tapi_context context, int slot_id, tapi_ussd_state* out);

/**
 * Sends an USSD message.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] reply          The response to the network.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ss_send_ussd(tapi_context context, int slot_id, int event_id, char* reply,
    tapi_async_function p_handle);

/**
 * Cancels an ongoing USSD session.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ss_cancel_ussd(tapi_context context, int slot_id, int event_id,
    tapi_async_function p_handle);

/**
 * Sets Call Waiting option.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] state          Call Waiting property value.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ss_request_call_wating(tapi_context context, int slot_id, int event_id, char* state,
    tapi_async_function p_handle);

/**
 * Gets Call Waiting info.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           The value of Call Waiting service.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ss_query_call_wating(tapi_context context, int slot_id, char** out);

/**
 * Gets Calling Line Presentation info.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           The value of Calling Line Presentation service.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ss_query_calling_line_presentation_info(tapi_context context, int slot_id,
    tapi_clip_status* out);

/**
 * Gets Connected Line Restriction info.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           The value Connected Line Restriction service.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ss_query_calling_line_restriction_info(tapi_context context, int slot_id,
    tapi_clir_status* out);

/**
 * Registers SS event callback.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] msg            SS event ID.
 * @param[in] p_handle       Event callback.
 * @return Positive value as watch_id on success; a negated errno value on failure.
 */
int tapi_ss_register(tapi_context context,
    int slot_id, tapi_indication_msg msg, tapi_async_function p_handle);

/**
 * Unregisters SS event callback.
 * @param[in] context        Telephony api context.
 * @param[in] watch_id       Watch id.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ss_unregister(tapi_context context, int watch_id);

#ifdef __cplusplus
}
#endif

#endif /* __TAPI_SS_H */
