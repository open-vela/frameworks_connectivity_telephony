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

#define MAX_CALL_PROPERTY_NAME_LENGTH 20
#define MAX_CALL_PATH_LENGTH 100
#define MAX_CONFERENCE_PARTICIPANT_COUNT 6
#define MAX_ECC_LIST_SIZE 20

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum tapi_call_status {
    CALL_STATUS_UNKNOW = -1,
    CALL_STATUS_ACTIVE = 0,
    CALL_STATUS_HELD = 1,
    CALL_STATUS_DIALING = 2,
    CALL_STATUS_ALERTING = 3,
    CALL_STATUS_INCOMING = 4,
    CALL_STATUS_WAITING = 5,
    CALL_STATUS_DISCONNECTED
};

typedef struct {
    unsigned int id;
    int type;
    int direction;
    int status;
    tapi_phone_number phone_number;
    tapi_phone_number called_number;
    char name[MAX_CALLER_NAME_LENGTH + 1];
    int clip_validity;
    int cnap_validity;
} tapi_call;

typedef struct {
    char call_path[MAX_CALL_PATH_LENGTH + 1];
    int state;
    char lineIdentification[MAX_CALLER_NAME_LENGTH + 1];
    char incoming_line[MAX_CALLER_NAME_LENGTH + 1];
    char name[MAX_CALLER_NAME_LENGTH + 1];
    char start_time[MAX_CALLER_NAME_LENGTH + 1];
    bool multiparty;
    bool remote_held;
    bool remote_multiparty;
    char info[MAX_CALLER_NAME_LENGTH + 1];
    unsigned char icon;
    bool is_emergency_number;
} tapi_call_info;

typedef struct call_list {
    tapi_call_info* data;
    struct call_list* next;
} tapi_call_list;

typedef struct {
    char call_path[MAX_CALL_PATH_LENGTH + 1];
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
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_dial(tapi_context context, int slot_id, char* number, int hide_callerid);

/**
 * Hangup one active call.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] call_id        Call Id.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_hangup_call(tapi_context context, int slot_id, char* call_id);

/**
 * Hangup all calls.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_hangup_all_calls(tapi_context context, int slot_id);

/**
 * Answer one incoming call.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] call_list      Ongoing call list.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_answer_call(tapi_context context, int slot_id, tapi_call_list* call_list);

/**
 * Hangup one active call and answer another waiting call.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_release_and_answer(tapi_context context, int slot_id);

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
 * Deflect one incoming/waiting call to one specified number.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] call_id        Call id to be deflected.
 * @param[in] number         Number to receive one deflected call.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_deflect_call(tapi_context context, int slot_id, char* call_id, char* number);

/**
 * Merge multiple calls into one conference call.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           Call path list in conference session.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_merge_call(tapi_context context, int slot_id, char** out, tapi_async_function p_handle);

/**
 * Separate one call from one conference session.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           Call path list in conference session.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_separate_call(tapi_context context, int slot_id, char* call_id, char** out, tapi_async_function p_handle);

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
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_get_all_calls(tapi_context context, int slot_id,
    tapi_call_list* list, tapi_async_function p_handle);

/**
 * Get call by given call state.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] state          Call state.
 * @param[in] call_list      Ongoing call list.
 * @param[out] info          Pointer or NULL if no any call is existing.
 */
void tapi_call_tapi_get_call_by_state(tapi_context context, int slot_id,
    int state, tapi_call_list* call_list, tapi_call_info* info);

/**
 * Register call state change event
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] msg            Call state change event.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_register_managercall_change(tapi_context context, int slot_id,
    tapi_indication_msg msg,
    tapi_async_function p_handle);

/**
 * Register call info change event
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] msg            Call info change event.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_call_register_call_info_change(tapi_context context, int slot_id, char* call_path,
    tapi_indication_msg msg, tapi_async_function p_handle);

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
 * @param[in] p_handle       Event callback.
 * @return Positive value as watch_id on success; a negated errno value on failure.
 */
int tapi_call_register_emergencylist_change(tapi_context context, int slot_id, tapi_async_function p_handle);

/**
 * Recycle call list.
 * @param[in] head        Call list pointer.
 */
void tapi_call_free_call_list(tapi_call_list* head);

#ifdef __cplusplus
}
#endif

#endif /* __TELEPHONY_CALL_H */
