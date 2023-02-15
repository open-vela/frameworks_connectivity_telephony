/****************************************************************************
 * frameworks/telephony/include/tapi_stk.h
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

#ifndef __TELEPHONY_STK_H
#define __TELEPHONY_STK_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "tapi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_STK_MENU_TEXT_LEN 20
#define MAX_STK_MAIN_MENU_LENGTH 20
#define STK_AGENT_DEFAULT_ID "/stkagent0"

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct {
    char text[MAX_STK_MENU_TEXT_LEN + 1];
    unsigned char icon_id;
} tapi_stk_menu_item;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Registers a agent to be used for SIM initiated actions
 * such as Display Text, Get Inkey or Get Input. If no agent
 * is registered then unsolicited commands from the SIM
 * are rejected.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] agent_id       Agent ID.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_agent_register(tapi_context context, int slot_id,
    int event_id, char* agent_id, tapi_async_function p_handle);

/**
 * Unregisters the agent.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] agent_id       Agent ID.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_agent_unregister(tapi_context context, int slot_id,
    int event_id, char* agent_id, tapi_async_function p_handle);

/**
 * Registers a default agent using a fixed default agent id.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_default_agent_register(tapi_context context, int slot_id,
    int event_id, tapi_async_function p_handle);

/**
 * Unregisters the default agent.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_default_agent_unregister(tapi_context context, int slot_id,
    int event_id, tapi_async_function p_handle);
/**
 * Selects an item from the main menu, thus triggering
 * a new user session.  The agent parameter specifies
 * a new agent to be used for the duration of the
 * user session.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] event_id       Async event identifier.
 * @param[in] item           Item selected from the main menu.
 * @param[in] agent_id       Agent ID.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_select_item(tapi_context context, int slot_id,
    int event_id, unsigned char item, char* agent_id, tapi_async_function p_handle);

/**
 * Contains the text to be used when the home screen is
 * idle.  This text is set by the SIM and can change
 * at any time.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] text          Text content.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_get_idle_mode_text(tapi_context context, int slot_id, char** text);

/**
 * Contains the identifier of the icon accompanying
 * the idle mode text.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] icon          Identifier of the icon.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_get_idle_mode_icon(tapi_context context, int slot_id, char** icon);

/**
 * Contains the items that make up the main menu.  This
 * is populated by the SIM when it sends the Setup Menu
 * Proactive Command.  The main menu is always available,
 * but its contents can be changed at any time.  Each
 * item contains the item label and icon identifier.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] length         Length of out array.
 * @param[out] out           Items that make up the main menu.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_get_main_menu(tapi_context context, int slot_id,
    int length, tapi_stk_menu_item out[]);

/**
 * Contains the title of the main menu.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] title         Title of main menu.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_get_main_menu_title(tapi_context context, int slot_id, char** title);

/**
 * Contains the identifier of the icon for the main menu.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] icon          Icon id of main menu.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_get_main_menu_icon(tapi_context context, int slot_id, char** icon);

#ifdef __cplusplus
}
#endif

#endif /* __TELEPHONY_STK_H */
