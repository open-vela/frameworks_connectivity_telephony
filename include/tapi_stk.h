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
#define STK_AGENT_DEFAULT_ID_0 "/stk_agent_default_0"
#define STK_AGENT_DEFAULT_ID_1 "/stk_agent_default_1"

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct {
    char text[MAX_STK_MENU_TEXT_LEN + 1];
    unsigned char icon_id;
} tapi_stk_menu_item;

typedef struct {
    char* text;
    unsigned char icon_id;
    int urgent;
} tapi_stk_display_text_params;

typedef struct {
    char* alpha;
    unsigned char icon_id;
} tapi_stk_request_key_params;

typedef struct {
    char* alpha;
    char* def_input;
    char icon_id;
    char min_chars;
    char max_chars;
    int hide_typing;
} tapi_stk_request_input_params;

typedef struct {
    char* tone;
    char* text;
    unsigned char icon_id;
} tapi_stk_play_tone_params;

typedef struct {
    char* alpha;
    unsigned char icon_id;
    tapi_stk_menu_item items[MAX_STK_MAIN_MENU_LENGTH + 1];
    short default_item;
} tapi_stk_request_selection_params;

typedef struct {
    char* information;
    unsigned char icon_id;
    char* url;
} tapi_stk_confirm_launch_browser_params;

typedef enum {
    OP_CODE_CONFIRM_POSITIVE = 0,
    OP_CODE_CONFIRM_NEGATIVE = 1,
    OP_CODE_CONFIRM_BACKWARD = 2,
} tapi_stk_agent_operator_code;

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
 * Registers the interfaces for a agent.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] agent_id       Agent ID.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_agent_interface_register(tapi_context context, int slot_id, char* agent_id,
    tapi_async_function p_handle);

/**
 * Unregisters the interfaces for a agent.
 * @param[in] context        Telephony api context.
 * @param[in] agent_id       Agent ID.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_agent_interface_unregister(tapi_context context, char* agent_id);

/**
 * Registers the interfaces for the default agent.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_default_agent_interface_register(tapi_context context, int slot_id,
    tapi_async_function p_handle);

/**
 * Unregisters the interfaces for the default agent.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_default_agent_interface_unregister(tapi_context context, int slot_id);

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

/**
 * Handle a request to select an item from the menu.
 * @param[in] context        Telephony api context.
 * @param[in] op             Operation code.
 * @param[in] selection      The index of the item.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_handle_agent_request_selection(tapi_context context,
    tapi_stk_agent_operator_code op, unsigned char selection);

/**
 * Handles display text requests from the SIM.
 * @param[in] context        Telephony api context.
 * @param[in] op             Operation code.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_handle_agent_display_text(tapi_context context,
    tapi_stk_agent_operator_code op);

/**
 * Handles input string requests from the SIM.
 * @param[in] context        Telephony api context.
 * @param[in] op             Operation code.
 * @param[in] input          The input string.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_handle_agent_request_input(tapi_context context,
    tapi_stk_agent_operator_code op, char* input);

/**
 * Handles the request to enter a string of digits from the SIM.
 * @param[in] context        Telephony api context.
 * @param[in] op             Operation code.
 * @param[in] digits         The input string of digits.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_handle_agent_request_digits(tapi_context context,
    tapi_stk_agent_operator_code op, char* digits);

/**
 * Handles a request to enter a single input key from the SIM.
 * @param[in] context        Telephony api context.
 * @param[in] op             Operation code.
 * @param[in] key            A single input key.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_handle_agent_request_key(tapi_context context,
    tapi_stk_agent_operator_code op, char* key);

/**
 * Handles a request to enter a single input digit from the SIM.
 * @param[in] context        Telephony api context.
 * @param[in] op             Operation code.
 * @param[in] digit          A single input digit.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_handle_agent_request_digit(tapi_context context,
    tapi_stk_agent_operator_code op, char* digit);

/**
 * Handles a request to enter a single input digit from the SIM.
 * The entered digit shall not be displayed and the response
 * shall be sent immediately after the key press.
 * @param[in] context        Telephony api context.
 * @param[in] op             Operation code.
 * @param[in] digit          A single input digit.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_handle_agent_request_quick_digit(tapi_context context,
    tapi_stk_agent_operator_code op, char* digit);

/**
 * Handles confirmation requests from SIM cards.
 * @param[in] context        Telephony api context.
 * @param[in] op             Operation code.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_handle_agent_request_confirmation(tapi_context context,
    tapi_stk_agent_operator_code op);

/**
 * Handles confirmation of an outgoing call settings from the SIM.
 * @param[in] context        Telephony api context.
 * @param[in] op             Operation code.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_handle_agent_confirm_call_setup(tapi_context context,
    tapi_stk_agent_operator_code op);

/**
 * Handles a request to play audio once from the SIM.
 * @param[in] context        Telephony api context.
 * @param[in] op             Operation code.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_handle_agent_play_tone(tapi_context context,
    tapi_stk_agent_operator_code op);

/**
 * Handles a request to loop audio from the SIM.
 * @param[in] context        Telephony api context.
 * @param[in] op             Operation code.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_handle_agent_loop_tone(tapi_context context,
    tapi_stk_agent_operator_code op);

/**
 * Handles a request to display action infomation from the SIM.
 * @param[in] context        Telephony api context.
 * @param[in] op             Operation code.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_handle_agent_display_action_information(tapi_context context,
    tapi_stk_agent_operator_code op);

/**
 * Handles confirmation to launch browser from the SIM.
 * @param[in] context        Telephony api context.
 * @param[in] op             Operation code.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_handle_agent_confirm_launch_browser(tapi_context context,
    tapi_stk_agent_operator_code op);

/**
 * Handles a request to display action from the SIM.
 * @param[in] context        Telephony api context.
 * @param[in] op             Operation code.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_handle_agent_display_action(tapi_context context,
    tapi_stk_agent_operator_code op);

/**
 * Handles the channel set-up confirmation from the SIM.
 * @param[in] context        Telephony api context.
 * @param[in] op             Operation code.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_stk_handle_agent_confirm_open_channel(tapi_context context,
    tapi_stk_agent_operator_code op);

#ifdef __cplusplus
}
#endif

#endif /* __TELEPHONY_STK_H */
