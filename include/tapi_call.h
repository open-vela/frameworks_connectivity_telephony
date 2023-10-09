/****************************************************************************
 * frameworks/telephony/include/tapi_call.h
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

#ifndef __TELEPHONY_CALL_H
#define __TELEPHONY_CALL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tapi.h>
#include <time.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_CALL_PATH_LENGTH 100
#define MAX_CONFERENCE_PARTICIPANT_COUNT 6
#define MAX_ECC_LIST_SIZE 30
#define MAX_CALL_PROPERTY_NAME_LENGTH 40
#define MAX_CALL_ID_LENGTH 100
#define MAX_CALL_NAME_LENGTH 100
#define MAX_CALL_LINE_ID_LENGTH 100
#define MAX_CALL_INCOMING_LINE_LENGTH 100
#define MAX_CALL_START_TIME_LENGTH 100
#define MAX_CALL_INFO_LENGTH 100
#define MAX_CALL_LIST_COUNT 10
#define MAX_IMS_CONFERENCE_CALLS 5

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef enum {
    CALL_STATUS_UNKNOW = -1,
    CALL_STATUS_ACTIVE = 0,
    CALL_STATUS_HELD = 1,
    CALL_STATUS_DIALING = 2,
    CALL_STATUS_ALERTING = 3,
    CALL_STATUS_INCOMING = 4,
    CALL_STATUS_WAITING = 5,
    CALL_STATUS_DISCONNECTED
} tapi_call_status;

typedef enum {
    CALL_DISCONNECT_REASON_UNKNOWN = 0,
    CALL_DISCONNECT_REASON_LOCAL_HANGUP,
    CALL_DISCONNECT_REASON_REMOTE_HANGUP,
    CALL_DISCONNECT_REASON_NETWORK_HANGUP,
    CALL_DISCONNECT_REASON_ERROR,
} tapi_call_disconnect_reason;

typedef struct {
    char call_id[MAX_CALL_ID_LENGTH + 1];
    tapi_call_status state;
    char lineIdentification[MAX_CALL_LINE_ID_LENGTH + 1];
    char incoming_line[MAX_CALL_INCOMING_LINE_LENGTH + 1];
    char name[MAX_CALL_NAME_LENGTH + 1];
    char start_time[MAX_CALL_START_TIME_LENGTH + 1];
    bool multiparty;
    bool remote_held;
    bool remote_multiparty;
    char info[MAX_CALL_INFO_LENGTH + 1];
    unsigned char icon;
    bool is_emergency_number;
    tapi_call_disconnect_reason disconnect_reason;
} tapi_call_info;

typedef struct {
    char call_id[MAX_CALL_ID_LENGTH + 1];
    char key[MAX_CALL_PROPERTY_NAME_LENGTH + 1];
    void* value;
} tapi_call_property;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Make one outgoing call.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] number         Dialing number.
 * @param[in] hide_callerid  Flag about whether to hide calling number.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_dial(tapi_context context, int slot_id, char* number, int hide_callerid,
    int event_id, tapi_async_function p_handle);

/**
 * Hangup all calls.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_hangup_all_calls(tapi_context context, int slot_id);

/**
 * Hangup one active call and answer another waiting call.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_release_and_answer(tapi_context context, int slot_id);

/**
 * Hold one active call and answer another waiting call.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_hold_and_answer(tapi_context context, int slot_id);

/**
 * Hangup one active call and active hold call.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_release_and_swap(tapi_context context, int slot_id);

/**
 * Hold one active call as background call.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_hold_call(tapi_context context, int slot_id);

/**
 * Resume one background call to foreground.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_unhold_call(tapi_context context, int slot_id);

/**
 * Transfer one active call.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_transfer(tapi_context context, int slot_id);

/**
 * Merge multiple calls into one conference call.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_merge_call(tapi_context context, int slot_id, int event_id, tapi_async_function p_handle);

/**
 * Separate one call from one conference session.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_separate_call(tapi_context context, int slot_id, int event_id, char* call_id,
    tapi_async_function p_handle);

/**
 * Hangup one conference session.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_hangup_multiparty(tapi_context context, int slot_id);

/**
 * Send DTMF tone playing request.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] tones          DTMF tone character.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_send_tones(void* context, int slot_id, char* tones);

/**
 * Get all calls.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_get_all_calls(tapi_context context, int slot_id, int event_id,
    tapi_async_function p_handle);

/**
 * Get all calls.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] call_id        Call id of current call.
 * @param[out] info          Call info of queryed.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_get_call_info(tapi_context context, int slot_id,
    char* call_id, tapi_call_info* info);

/**
 * Get call by given call state.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] state          Call state.
 * @param[in] call_list      All calls of current sim.
 * @param[in] size           Count of call list.
 * @param[out] out_list      Call list of matched call state.
 * @return Positive value as matched size on success; a negated errno value on failure.
 */
int tapi_call_get_call_by_state(tapi_context context, int slot_id,
    int state, tapi_call_info* call_list, int size, tapi_call_info* out_list);

/**
 * Get all ecc list
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           Ecc list container.
 * @return size of ecc list on success; a negated errno value on failure.
 */
int tapi_call_get_ecc_list(tapi_context context, int slot_id, char** out);

/**
 * Check whether one give number is one emergency number or not
 * @param[in] context        Telephony api context.
 * @param[in] number         Phone number.
 * @return true: is emergency number; false: not emergency number.
 */
bool tapi_call_is_emergency_number(tapi_context context, char* number);

/**
 * Register ecc list change callback.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] user_obj       User data.
 * @param[in] p_handle       Event callback.
 * @return Positive value as watch_id on success; a negated errno value on failure.
 */
int tapi_call_register_emergency_list_change(tapi_context context, int slot_id,
    void* user_obj, tapi_async_function p_handle);

/**
 * Register ring back tone change callback.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] user_obj       User data.
 * @param[in] p_handle       Event callback.
 * @return Positive value as watch_id on success; a negated errno value on failure.
 */
int tapi_call_register_ringback_tone_change(tapi_context context, int slot_id,
    void* user_obj, tapi_async_function p_handle);

/**
 * Register default voicecall slot change callback.
 * @param[in] context       Telephony api context.
 * @param[in] user_obj      User data.
 * @return Positive value as watch_id on success; a negated errno value on failure.
 */
int tapi_call_register_default_voicecall_slot_change(tapi_context context, void* user_obj,
    tapi_async_function p_handle);

/**
 * Initiates an IMS conferenca call.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] participant    Participant list to initiate an IMS conference call.
 * @param[in] size           Size of participant list
 * @return Positive value as watch_id on success; a negated errno value on failure.
 */
int tapi_call_dial_conferece(tapi_context context, int slot_id, char* participants[], int size);

/**
 * Requests the conference server to invite an additional participants to the conference.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] participant    Participant list to initiate an IMS conference call.
 * @param[in] size           Size of participant list
 * @return Positive value as watch_id on success; a negated errno value on failure.
 */
int tapi_call_invite_participants(tapi_context context, int slot_id, char* participants[],
    int size);

/**
 * Register call event callback.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] user_obj       User data.
 * @param[in] p_handle       Event callback.
 * @return Positive value as watch_id on success; a negated errno value on failure.
 */
int tapi_call_register_call_state_change(tapi_context context, int slot_id,
    void* user_obj, tapi_async_function p_handle);

/**
 * Answer one call.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] call_id        Call id of current call.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_answer_by_id(tapi_context context, int slot_id, char* call_id);

/**
 * Hanup one call.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] call_id        Call id of current call.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_hangup_by_id(tapi_context context, int slot_id, char* call_id);

/**
 * Deflect one call.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] call_id        Call id of current call.
 * @param[in] number         Number to be deflected.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_deflect_by_id(tapi_context context, int slot_id, char* call_id, char* number);

/**
 * start DTMF request.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] digit          DTMF tone character.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_start_dtmf(tapi_context context, int slot_id, unsigned char digit,
    int event_id, tapi_async_function p_handle);

/**
 * stop DTMF request.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_stop_dtmf(tapi_context context, int slot_id, int event_id,
    tapi_async_function p_handle);

/**
 * Set default voicecall slot id.
 * @param[in] context       Telephony api context.
 * @param[in] slot_id       The default slot id to set (not set: -1, slot id: 0/1).
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_set_default_slot(tapi_context context, int slot_id);

/**
 * Get default voicecall slot id.
 * @param[in] context       Telephony api context.
 * @param[out] out          Default voicecall slot id (not set: -1, slot id: 0/1).
   @return Zero on success; a negated errno value on failure.
*/
int tapi_call_get_default_slot(tapi_context context, int* out);

#ifdef __cplusplus
}
#endif

#endif /* __TELEPHONY_CALL_H */
