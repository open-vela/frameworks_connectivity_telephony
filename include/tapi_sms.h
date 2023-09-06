/****************************************************************************
 * frameworks/telephony/include/tapi_sms.h
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

#ifndef __TELEPHONY_SMS_H
#define __TELEPHONY_SMS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "tapi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_TIME_LENGTH 32
#define MAX_TEXT_LENGTH 480
#define MAX_MESSAGE_LIST_COUNT 200

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct {
    char* text;
    char* sender;
    char* sent_time;
    char* local_sent_time;
    tapi_indication_msg sms_type;
} tapi_message_info;

typedef struct message_list {
    tapi_message_info* data;
    struct message_list* next;
} tapi_message_list;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * send a message
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] number         sms send number.
 * @param[in] text           send text.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sms_send_message(tapi_context context, int slot_id, char* number, char* text,
    int event_id, tapi_async_function p_handle);

/**
 * send a message
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] dest_addr      sms send number.
 * @param[in] port           data address port.
 * @param[in] text           send data text.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sms_send_data_message(tapi_context context, int slot_id, char* dest_addr,
    unsigned int port, char* text, int event_id, tapi_async_function p_handle);

/**
 * set service center number.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] number         service center number.
 * @return Zero on success; a negated errno value on failure.
 */
bool tapi_sms_set_service_center_address(tapi_context context, int slot_id, char* number);

/**
 * get service center number.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] number        service center number.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sms_get_service_center_address(tapi_context context, int slot_id, char** number);

/**
 * get all sim messages
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] list          sim message list.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sms_get_all_messages_from_sim(tapi_context context, int slot_id,
    tapi_message_list* list, tapi_async_function p_handle);

/**
 * copy message to sim card
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] number         sms send number.
 * @param[in] text           send text.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sms_copy_message_to_sim(tapi_context context, int slot_id,
    char* number, char* text, char* send_time, int type);

/**
 * get all sim messages
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] index          message index.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sms_delete_message_from_sim(tapi_context context, int slot_id, int index);

/**
 * register incoming message event.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] msg            message type.
 * @param[in] p_handle       Event callback.
 * @param[in] user_obj       User data.
 * @return Positive value as watch_id on success; a negated errno value on failure.
 */
int tapi_sms_register(tapi_context context, int slot_id,
    tapi_indication_msg msg, void* user_obj, tapi_async_function p_handle);

/**
 * Unregister telephony sms event callback.
 * @param[in] context        Telephony api context.
 * @param[in] watch_id       Watch id.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sms_unregister(tapi_context context, int watch_id);

/**
 * Set default sms slot id.
 * @param[in] context       Telephony api context.
 * @param[in] slot_id       The default slot id to set (not set: -1, slot_id: 0/1).
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sms_set_default_slot(tapi_context context, int slot_id);

/**
 * Get default sms slot id.
 * @param[in] context       Telephony api context.
 * @param[out] out          Default sms slot id (not set: -1, slot_id: 0/1).
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sms_get_default_slot(tapi_context context, int* out);

#ifdef __cplusplus
}
#endif

#endif /* __TELEPHONY_SMS_H */
