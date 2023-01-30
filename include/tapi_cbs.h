/****************************************************************************
 * frameworks/telephony/include/tapi_cbs.h
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

#ifndef __TELEPHONY_CBS_H
#define __TELEPHONY_CBS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "tapi.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct {
    char* text;
    unsigned short channel;
} tapi_cbs_message;

typedef struct {
    char* text;
    char* emergency_type;
    bool emergency_alert;
    bool popup;
} tapi_cbs_emergency_message;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * set cbs power on.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] state          set power state.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sms_set_cell_broadcast_power_on(tapi_context context, int slot_id, bool state);

/**
 * get scbs power state.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] state          set power state.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sms_get_cell_broadcast_power_on(tapi_context context, int slot_id, bool* state);

/**
 * set cbs topics type
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] topics         CBS topics.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sms_set_cell_broadcast_topics(tapi_context context, int slot_id, char* topics);

/**
 * get cbs topics type
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_sms_get_cell_broadcast_topics(tapi_context context, int slot_id, char** topics);

/**
 * get service center number.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] msg            indicate unsol message type.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_cbs_register(tapi_context context, int slot_id, tapi_indication_msg msg, tapi_async_function p_handle);

#ifdef __cplusplus
}
#endif

#endif /* __TELEPHONY_CBS_H */
