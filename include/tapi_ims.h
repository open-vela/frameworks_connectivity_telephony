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

#ifdef __cplusplus
}
#endif

#endif /* __TELEPHONY_IMS_H */