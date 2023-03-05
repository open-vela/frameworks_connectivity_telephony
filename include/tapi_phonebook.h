/****************************************************************************
 * frameworks/telephony/include/tapi_phonebook.h
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

#ifndef __TELEPHONY_PHONEBOOK_H
#define __TELEPHONY_PHONEBOOK_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "tapi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_FDN_RECORD_LENGTH 10

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct {
    int fdn_idx; // record numer in EFFDN
    char* tag;
    char* number;
} fdn_entry;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Load ADN entries from SIM.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Positive value as watch_id on success; a negated errno value on failure.
 */
int tapi_phonebook_load_adn_entries(tapi_context context,
    int slot_id, int event_id, tapi_async_function p_handle);

/**
 * Query the FDN records in the SIM phonebook.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Positive value as watch_id on success; a negated errno value on failure.
 */
int tapi_phonebook_load_fdn_entries(tapi_context context,
    int slot_id, int event_id, tapi_async_function p_handle);

/**
 * Insert the FDN record into the SIM phonebook.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] name           FDN entry tag.
 * @param[in] number         FDN entry number.
 * @param[in] pin2           PIN2 code.
 * @param[in] p_handle       Event callback.
 * @return Positive value as watch_id on success; a negated errno value on failure.
 */
int tapi_phonebook_insert_fdn_entry(tapi_context context,
    int slot_id, int event_id, char* name, char* number,
    char* pin2, tapi_async_function p_handle);

/**
 * Delete the FDN record in the SIM phonebook.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] fdn_idx        Record number on EFFDN.
 * @param[in] pin2           PIN2 code.
 * @param[in] p_handle       Event callback.
 * @return Positive value as watch_id on success; a negated errno value on failure.
 */
int tapi_phonebook_delete_fdn_entry(tapi_context context, int slot_id,
    int event_id, int fdn_idx, char* pin2, tapi_async_function p_handle);

/**
 * Update the FDN record to the SIM phonebook.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] fdn_idx        Record number on EFFDN.
 * @param[in] new_name       New Entry name.
 * @param[in] new_number     New Entry number.
 * @param[in] pin2           PIN2 code.
 * @param[in] p_handle       Event callback.
 * @return Positive value as watch_id on success; a negated errno value on failure.
 */
int tapi_phonebook_update_fdn_entry(tapi_context context,
    int slot_id, int event_id, int fdn_idx, char* new_name,
    char* new_number, char* pin2, tapi_async_function p_handle);

#ifdef __cplusplus
}
#endif

#endif /* __TELEPHONY_PHONEBOOK_H */
