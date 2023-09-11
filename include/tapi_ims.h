/****************************************************************************
 * frameworks/telephony/include/tapi_ims.h
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

#ifndef __TELEPHONY_IMS_H
#define __TELEPHONY_IMS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "tapi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VOICE_CAPABLE_FLAG 0x1
#define SMS_CAPABLE_FLAG 0x4

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef enum {
    IMS_REG = 0,
    VOICE_CAP,
    SMS_CAP,
} tapi_ims_prop_type;

typedef struct {
    int reg_info;
    int ext_info;
} tapi_ims_registration_info;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Attempts to register to IMS.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ims_turn_on(tapi_context context, int slot_id);

/**
 * Attempts to unregister to IMS.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ims_turn_off(tapi_context context, int slot_id);

/**
 * Attempts to set ims service capability
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] capability     ims service capability
 *                           ex:VOICE_CAPABLE_FLAG 0x1
 *                              SMS_CAPABLE_FLAG 0x4
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ims_set_service_status(tapi_context context, int slot_id, int capability);

/**
 * Query ims registration and ims service status
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] ims_reg       ims registration and service status
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ims_get_registration(tapi_context context, int slot_id,
    tapi_ims_registration_info* ims_reg);

/**
 * Register ims registration and ims service status callback.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] user_obj       User data.
 * @param[in] p_handle       Event callback.
 * @return Positive value as watch_id on success; a negated errno value on failure.
 */
int tapi_ims_register_registration_change(tapi_context context, int slot_id,
    void* user_obj, tapi_async_function p_handle);

/**
 * Query the ims registration status
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @return true if ims is registered
 */
int tapi_ims_is_registered(tapi_context context, int slot_id);

/**
 * Query ims voice capability status
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @return true if ims is registered and voice capability is enbaled
 */
int tapi_ims_is_volte_available(tapi_context context, int slot_id);

/**
 * Gets Subscriber Uri Number.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           Subscriber Uri Number returned.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_ims_get_subscriber_uri_number(tapi_context context, int slot_id, char** out);

/**
 * Gets ims switch status.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @return true if ims is enabled.
 */
int tapi_ims_get_enabled(tapi_context context, int slot_id, bool* out);

#ifdef __cplusplus
}
#endif

#endif /* __TELEPHONY_IMS_H */