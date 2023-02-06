/****************************************************************************
 * framework/telephony/telephony_tool.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ctype.h>
#include <errno.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <system/readline.h>
#include <tapi.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define EVENT_MODEM_LIST_QUERY_DONE 100
#define EVENT_RADIO_STATE_SET_DONE 101
#define EVENT_RAT_MODE_SET_DONE 102

/****************************************************************************
 * Public Type Declarations
 ****************************************************************************/

typedef int (*telephonytool_cmd_func)(tapi_context context, char* pargs);
struct telephonytool_cmd_s {
    const char* cmd; /* The command text */
    telephonytool_cmd_func pfunc; /* Pointer to command handler */
    const char* help; /* The help text */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int telephonytool_cmd_help(tapi_context context, char* pargs);

/** Radio interface*/
static int telephonytool_cmd_query_modem_list(tapi_context context, char* pargs);
static int telephonytool_cmd_modem_register(tapi_context context, char* pargs);
static int telephonytool_cmd_set_radio_power(tapi_context context, char* pargs);
static int telephonytool_cmd_get_radio_power(tapi_context context, char* pargs);
static int telephonytool_cmd_set_rat_mode(tapi_context context, char* pargs);
static int telephonytool_cmd_get_rat_mode(tapi_context context, char* pargs);
static int telephonytool_cmd_get_imei(tapi_context context, char* pargs);
static int telephonytool_cmd_get_imeisv(tapi_context context, char* pargs);
static int telephonytool_cmd_get_phone_state(tapi_context context, char* pargs);
static int telephonytool_cmd_reboot_modem(tapi_context context, char* pargs);
static int telephonytool_cmd_get_radio_state(tapi_context context, char* pargs);
static int telephonytool_cmd_get_modem_manufacturer(tapi_context context, char* pargs);
static int telephonytool_cmd_get_modem_model(tapi_context context, char* pargs);
static int telephonytool_cmd_get_modem_revision(tapi_context context, char* pargs);
static int telephonytool_cmd_get_line_number(tapi_context context, char* pargs);

/** Call interface*/
static int telephonytool_cmd_dial(tapi_context context, char* pargs);
static int telephonytool_cmd_hangup_all(tapi_context context, char* pargs);
static int telephonytool_cmd_hangup_call(tapi_context context, char* pargs);
static int telephonytool_cmd_answer_call(tapi_context context, char* pargs);
static int telephonytool_cmd_swap_call(tapi_context context, char* pargs);
static int telephonytool_cmd_listen_call_property_change(tapi_context context, char* pargs);
static int telephonytool_cmd_get_call(tapi_context context, char* pargs);
static int telephonytool_cmd_transfer_call(tapi_context context, char* pargs);
static int telephonytool_cmd_merge_call(tapi_context context, char* pargs);
static int telephonytool_cmd_separate_call(tapi_context context, char* pargs);
static int telephonytool_cmd_get_ecc_list(tapi_context context, char* pargs);
static int telephonytool_cmd_is_emergency_number(tapi_context context, char* pargs);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct telephonytool_cmd_s g_telephonytool_cmds[] = {
    { "list-modem", telephonytool_cmd_query_modem_list,
        "list available modem list (enter example : list-modem" },
    { "listen", telephonytool_cmd_modem_register,
        "modem event callback (enter example : listen 0 0 \
        [slot_id][event_id, 0:radio_state_change 1:call_added 2:call_removed])" },
    { "radio-set", telephonytool_cmd_set_radio_power,
        "set radio power (enter example : radio-set 0 1 [slot_id][state, 0:radio off 1:radio on])" },
    { "radio-get", telephonytool_cmd_get_radio_power,
        "get radio power (enter example : radio-get 0 [slot_id])" },
    { "dial", telephonytool_cmd_dial,
        "Dial (enter example : dial 0 10086 0 [slot_id][number][hide_call_id, 0:show 1:hide])" },
    { "answer", telephonytool_cmd_answer_call,
        "Answer (enter example : answer 0 0 [slot_id][action:0-answer 1-realse&answer])" },
    { "swap", telephonytool_cmd_swap_call,
        "callSwap (enter example : swap 0 1 [slot_id][action:1-hold 0-unhold])" },
    { "call_listen", telephonytool_cmd_listen_call_property_change,
        "Call_listen (enter example : call_listen 0 [call_event]" },
    { "hangup_all", telephonytool_cmd_hangup_all,
        "hangup_all (enter example : hangup_all 0 [slot_id])" },
    { "hangup", telephonytool_cmd_hangup_call,
        "hangup (enter example : hangup 0 [call_id] /phonesim/voicecall01)" },
    { "get_call", telephonytool_cmd_get_call,
        "get_call (enter example : get_call 0 \
        [slot_id][state], 0-active 1-held 2-dialing 3-alerting 4-incoming 5-waiting)" },
    { "transfer", telephonytool_cmd_transfer_call,
        "call transfer  (enter example : transfer 0 [slot_id])" },
    { "merge", telephonytool_cmd_merge_call,
        "call merge  (enter example : merge 0 [slot_id])" },
    { "separate", telephonytool_cmd_separate_call,
        "call separate  (enter example : separate 0 [slot_id][call_id: /phonesim/voicecall01])" },
    { "get_ecc_list", telephonytool_cmd_get_ecc_list,
        "get_ecc_list  (enter example : get_ecc_list 0 [slot_id])" },
    { "is_ecc_num", telephonytool_cmd_is_emergency_number,
        "is emergency number  (enter example : is_ecc_num 110 [number])" },
    { "rat-set", telephonytool_cmd_set_rat_mode,
        "set rat mode (enter example : rat-set 0 9 \
        [slot_id][mode: 0-any 1-gsm_only 2-wcdma_only 3-umts 9-lte_gsm_wcdma 12-lte_wcdma 14-lte_only])" },
    { "rat-get", telephonytool_cmd_get_rat_mode,
        "get rat mode (enter example : rat-get 0 [slot_id])" },
    { "imei-get", telephonytool_cmd_get_imei,
        "get imei (enter example : imei-get 0 [slot_id])" },
    { "imeisv-get", telephonytool_cmd_get_imeisv,
        "get imeisv (enter example : imeisv-get 0 [slot_id])" },
    { "phone-state", telephonytool_cmd_get_phone_state,
        "get phone state (enter example : phone-state 0 [slot_id])" },
    { "modem-reboot", telephonytool_cmd_reboot_modem,
        "reboot modem (enter example : modem-reboot 0 [slot_id])" },
    { "radio-state", telephonytool_cmd_get_radio_state,
        "get radio state (enter example : radio-state 0 [slot_id])" },
    { "modem_manufacturer", telephonytool_cmd_get_modem_manufacturer,
        "get modem manufacturer (enter example : modem_manufacturer 0 [slot_id])" },
    { "modem_model", telephonytool_cmd_get_modem_model,
        "get modem_model (enter example : modem_model 0 [slot_id])" },
    { "modem_revision", telephonytool_cmd_get_modem_revision,
        "get modem_revision (enter example : modem_revision 0 [slot_id])" },
    { "line-number", telephonytool_cmd_get_line_number,
        "query line number (enter example : line-number 0 [slot_id])" },
    { "q", NULL, "Quit (pls enter : q)" },
    { "help", telephonytool_cmd_help,
        "Show this message (pls enter : help)" },
    { 0 },
};

/****************************************************************************
 * Private Function
 ****************************************************************************/

static int split(char dst[][CONFIG_NSH_LINELEN], char* str, const char* spl)
{
    int n = 0;
    char* result;

    result = strtok(str, spl);
    while (result != NULL) {
        strcpy(dst[n++], result);
        result = strtok(NULL, spl);
    }

    return n;
}

static void tele_call_async_fun(tapi_async_result* result)
{
    printf("tele_call_async_fun : \n");
    printf("result->msg_id : %d\n", result->msg_id);
    printf("result->status : %d\n", result->status);
    printf("result->arg1 : %d\n", result->arg1);
    printf("result->arg2 : %d\n", result->arg2);
}

static void modem_list_query_complete(tapi_async_result* result)
{
    char* modem_path;
    int modem_count;
    char** list = result->data;

    printf("%s : \n", __func__);
    if (list == NULL)
        return;

    printf("result->status : %d\n", result->status);
    modem_count = result->arg1;
    for (int i = 0; i < modem_count; i++) {
        modem_path = list[i];
        if (modem_path != NULL)
            printf("modem found with path -> %s \n", modem_path);
    }
}

static void call_list_query_complete(tapi_async_result* result)
{
    tapi_call_list* pHead = result->data;

    printf("%s : \n", __func__);
    if (pHead == NULL)
        return;

    for (tapi_call_list* p = pHead; p != NULL; p = p->next) {
        printf("call path : %s\n", p->data->call_path);
    }

    printf("result->status : %d \n", result->status);
    tapi_call_tapi_free_call_list(pHead);
}

static int telephonytool_cmd_dial(tapi_context context, char* pargs)
{
    char* slot_id;
    char* number;
    char* temp;
    char* hide_callerid;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &temp);
    if (slot_id == NULL)
        return -EINVAL;

    while (*temp == ' ')
        temp++;
    number = strtok_r(temp, " ", &hide_callerid);
    if (number == NULL)
        return -EINVAL;

    while (*hide_callerid == ' ')
        hide_callerid++;
    if (hide_callerid == NULL)
        return -EINVAL;

    printf("%s, slot_id: %s number: %s  hide_callerid: %s \n", __func__, slot_id, number, hide_callerid);
    tapi_call_dial(context, atoi(slot_id), number, atoi(hide_callerid));
    return 0;
}

static int telephonytool_cmd_answer_call(tapi_context context, char* pargs)
{
    char dst[2][CONFIG_NSH_LINELEN];
    char* slot_id;
    int type;
    int cnt;

    if (strlen(pargs) == 0)
        return -EINVAL;

    cnt = split(dst, pargs, " ");
    if (cnt != 2)
        return -EINVAL;

    slot_id = dst[0];
    type = atoi(dst[1]);

    printf("%s, slotId : %s\n", __func__, dst[0]);
    if (type == 0) {
        tapi_call_answer_call(context, atoi(slot_id), NULL);
    } else if (type == 1) {
        tapi_release_and_answer(context, atoi(slot_id));
    }

    return 0;
}

static int telephonytool_cmd_hangup_all(tapi_context context, char* pargs)
{
    char* slot_id;
    char* temp;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &temp);
    if (slot_id == NULL)
        return -EINVAL;

    printf("%s, slotId : %s\n", __func__, slot_id);
    tapi_call_hangup_all_calls(context, atoi(slot_id));
    return 0;
}

static int telephonytool_cmd_hangup_call(tapi_context context, char* pargs)
{
    char dst[2][CONFIG_NSH_LINELEN];
    char* slot_id;
    int cnt = split(dst, pargs, " ");

    slot_id = dst[0];
    printf("%s, slotId : %s\n", __func__, dst[0]);

    if (cnt == 1) {
        tapi_call_hangup_call(context, atoi(slot_id), NULL);
    } else if (cnt == 2) {
        tapi_call_hangup_call(context, atoi(slot_id), (char*)dst[1]);
    }

    return 0;
}

static int telephonytool_cmd_swap_call(tapi_context context, char* pargs)
{
    char dst[2][CONFIG_NSH_LINELEN];
    char* slot_id;
    int cnt = split(dst, pargs, " ");

    if (cnt != 2)
        return -EINVAL;

    slot_id = dst[0];
    printf("%s, slotId : %s\n", __func__, dst[0]);

    if (atoi(dst[1]) == 1) {
        tapi_call_hold_call(context, atoi(slot_id));
    } else {
        tapi_call_unhold_call(context, atoi(slot_id));
    }

    return 0;
}

static int telephonytool_cmd_get_call(tapi_context context, char* pargs)
{
    char dst[2][CONFIG_NSH_LINELEN];
    char* slot_id;
    int cnt = split(dst, pargs, " ");

    slot_id = dst[0];
    printf("%s, slotId : %s\n", __func__, slot_id);

    if (cnt == 1) {
        tapi_call_get_all_calls(context, atoi(slot_id), NULL, call_list_query_complete);
    } else if (cnt == 2) {
        tapi_call_info call;
        tapi_call_tapi_get_call_by_state(context, atoi(slot_id), atoi(dst[1]), NULL, &call);
    }

    return 0;
}

static int telephonytool_cmd_listen_call_property_change(tapi_context context, char* pargs)
{
    char dst[2][CONFIG_NSH_LINELEN];
    int cnt = split(dst, pargs, " ");

    if (cnt != 1)
        return -EINVAL;

    printf("%s, cnt : %d\n", __func__, cnt);
    tapi_register_managercall_change(context, 0, MSG_CALL_ADD_MESSAGE_IND, tele_call_async_fun);
    tapi_register_managercall_change(context, 0, MSG_CALL_REMOVE_MESSAGE_IND, tele_call_async_fun);
    tapi_register_emergencylist_change(context, 0, tele_call_async_fun);

    return 0;
}

static int telephonytool_cmd_transfer_call(tapi_context context, char* pargs)
{
    char dst[5][CONFIG_NSH_LINELEN];
    char* slot_id;
    int cnt = split(dst, pargs, " ");

    if (cnt != 1)
        return -EINVAL;

    slot_id = dst[0];
    printf("%s, slotId : %s\n", __func__, slot_id);

    tapi_call_transfer(context, atoi(slot_id));
    return 0;
}

static int telephonytool_cmd_merge_call(tapi_context context, char* pargs)
{
    char dst[5][CONFIG_NSH_LINELEN];
    char* slot_id;
    char* out[5];
    int cnt = split(dst, pargs, " ");

    if (cnt != 1)
        return -EINVAL;

    slot_id = dst[0];
    printf("%s, slotId : %s\n", __func__, slot_id);

    tapi_call_merge_call(context, atoi(slot_id), out, tele_call_async_fun);
    return 0;
}

static int telephonytool_cmd_separate_call(tapi_context context, char* pargs)
{
    char dst[5][CONFIG_NSH_LINELEN];
    char* out[5];
    char* slot_id;
    int cnt = split(dst, pargs, " ");

    if (cnt != 2)
        return -EINVAL;

    slot_id = dst[0];
    printf("%s, slotId : %s\n", __func__, slot_id);

    tapi_call_separate_call(context, atoi(slot_id), dst[1], out, tele_call_async_fun);
    return 0;
}

static int telephonytool_cmd_get_ecc_list(tapi_context context, char* pargs)
{
    char dst[5][CONFIG_NSH_LINELEN];
    char* out[5];
    char* slot_id;
    int size = 0;
    int cnt = split(dst, pargs, " ");
    int i;

    if (cnt != 1)
        return -EINVAL;

    slot_id = dst[0];
    printf("%s, slotId : %s \n", __func__, slot_id);

    tapi_call_get_ecc_list(context, atoi(slot_id), out, &size);
    for (i = 0; i < size; i++) {
        printf("tapi_call_get_ecc_list : %p \n", out[i]);
        printf("tapi_call_get_ecc_list : %s \n", out[i]);
    }

    return 0;
}

static int telephonytool_cmd_is_emergency_number(tapi_context context, char* pargs)
{
    char dst[5][CONFIG_NSH_LINELEN];
    int cnt = split(dst, pargs, " ");
    int ret;

    if (cnt != 1)
        return -EINVAL;

    ret = tapi_is_emergency_number(context, dst[0]);
    printf("%s, ret : %d\n", __func__, ret);

    return 0;
}

static int telephonytool_cmd_query_modem_list(tapi_context context, char* pargs)
{
    char* modem_list[MAX_MODEM_COUNT];

    if (strlen(pargs) > 0)
        return -EINVAL;

    tapi_query_modem_list(context,
        EVENT_MODEM_LIST_QUERY_DONE, modem_list, modem_list_query_complete);
    return 0;
}

static int telephonytool_cmd_modem_register(tapi_context context, char* pargs)
{
    char* slot_id;
    char* target_state;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &target_state);
    if (slot_id == NULL)
        return -EINVAL;

    while (*target_state == ' ')
        target_state++;
    if (target_state == NULL)
        return -EINVAL;

    printf("%s, slotId : %s target_state: %s \n", __func__, slot_id, target_state);
    tapi_register(context, atoi(slot_id), atoi(target_state), tele_call_async_fun);

    return 0;
}

static int telephonytool_cmd_set_radio_power(tapi_context context, char* pargs)
{
    char* slot_id;
    char* target_state;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &target_state);
    if (slot_id == NULL)
        return -EINVAL;

    while (*target_state == ' ')
        target_state++;
    if (target_state == NULL)
        return -EINVAL;

    printf("%s, slotId : %s target_state: %s \n", __func__, slot_id, target_state);
    tapi_set_radio_power(context, atoi(slot_id),
        EVENT_RADIO_STATE_SET_DONE, (bool)atoi(target_state), tele_call_async_fun);

    return 0;
}

static int telephonytool_cmd_get_radio_power(tapi_context context, char* pargs)
{
    char* slot_id;
    bool value;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_get_radio_power(context, atoi(slot_id), &value);
    printf("%s, slotId : %s value : %d \n", __func__, slot_id, value);

    return 0;
}

static int telephonytool_cmd_set_rat_mode(tapi_context context, char* pargs)
{
    char* slot_id;
    char* target_state;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &target_state);
    if (slot_id == NULL)
        return -EINVAL;

    while (*target_state == ' ')
        target_state++;
    if (target_state == NULL)
        return -EINVAL;

    printf("%s, slotId : %s target_state: %s \n", __func__, slot_id, target_state);
    tapi_set_pref_net_mode(context, atoi(slot_id),
        EVENT_RAT_MODE_SET_DONE, (tapi_pref_net_mode)atoi(target_state), tele_call_async_fun);

    return 0;
}

static int telephonytool_cmd_get_rat_mode(tapi_context context, char* pargs)
{
    char* slot_id;
    tapi_pref_net_mode value;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_get_pref_net_mode(context, atoi(slot_id), &value);
    printf("%s, slotId : %s value :%d \n", __func__, slot_id, value);

    return 0;
}

static int telephonytool_cmd_get_imei(tapi_context context, char* pargs)
{
    char* slot_id;
    char* imei;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_get_imei(context, atoi(slot_id), &imei);
    printf("%s, slotId : %s imei : %s \n", __func__, slot_id, imei);

    return 0;
}

static int telephonytool_cmd_get_imeisv(tapi_context context, char* pargs)
{
    char* slot_id;
    char* imeisv;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_get_imeisv(context, atoi(slot_id), &imeisv);
    printf("%s, slotId : %s imeisv : %s \n", __func__, slot_id, imeisv);

    return 0;
}

static int telephonytool_cmd_get_modem_revision(tapi_context context, char* pargs)
{
    char* slot_id;
    char* value;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_get_modem_revision(context, atoi(slot_id), &value);
    printf("%s, slotId : %s value : %s \n", __func__, slot_id, value);

    return 0;
}

static int telephonytool_cmd_get_modem_manufacturer(tapi_context context, char* pargs)
{
    char* slot_id;
    char* value;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_get_modem_manufacturer(context, atoi(slot_id), &value);
    printf("%s, slotId : %s value : %s \n", __func__, slot_id, value);

    return 0;
}

static int telephonytool_cmd_get_modem_model(tapi_context context, char* pargs)
{
    char* slot_id;
    char* value;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_get_modem_model(context, atoi(slot_id), &value);
    printf("%s, slotId : %s value : %s \n", __func__, slot_id, value);

    return 0;
}

static int telephonytool_cmd_get_phone_state(tapi_context context, char* pargs)
{
    char* slot_id;
    tapi_phone_state state;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_get_phone_state(context, atoi(slot_id), &state);
    printf("%s, slotId : %s state : %d \n", __func__, slot_id, state);

    return 0;
}

static int telephonytool_cmd_reboot_modem(tapi_context context, char* pargs)
{
    char* slot_id;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    printf("%s, slotId : %s  \n", __func__, slot_id);
    tapi_reboot_modem(context, atoi(slot_id));

    return 0;
}

static int telephonytool_cmd_get_radio_state(tapi_context context, char* pargs)
{
    char* slot_id;
    tapi_radio_state state;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_get_radio_state(context, atoi(slot_id), &state);
    printf("%s, slotId : %s state : %d \n", __func__, slot_id, state);

    return 0;
}

static int telephonytool_cmd_get_line_number(tapi_context context, char* pargs)
{
    char* slot_id;
    char* number;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = pargs;
    if (slot_id == NULL)
        return -EINVAL;

    tapi_get_msisdn_number(context, atoi(slot_id), &number);
    printf("%s, slotId : %s  number : %s \n", __func__, slot_id, number);

    return 0;
}

static int telephonytool_cmd_help(tapi_context context, char* pargs)
{
    int i;

    for (i = 0; g_telephonytool_cmds[i].cmd; i++)
        printf("%-16s %s\n", g_telephonytool_cmds[i].cmd, g_telephonytool_cmds[i].help);

    return 0;
}

static void telephonytool_execute(tapi_context context, char* cmd, char* arg)
{
    int i;

    for (i = 0; g_telephonytool_cmds[i].cmd; i++) {
        if (strcmp(cmd, g_telephonytool_cmds[i].cmd) == 0) {
            int ret = g_telephonytool_cmds[i].pfunc(context, arg);
            if (ret < 0)
                printf("cmd:%s input parameter:%s invalid, res:%d\n", cmd, arg, ret);

            break;
        }
    }

    if (i == sizeof(g_telephonytool_cmds) / sizeof(g_telephonytool_cmds[0]) - 1)
        printf("Unknown cmd: \'%s\'. Type 'help' for more infomation.\n", cmd);
}

static void* read_stdin(pthread_addr_t pvarg)
{
    int arg_len, len;
    char *cmd, *arg, *buffer;
    tapi_context context = (void*)pvarg;

    buffer = malloc(CONFIG_NSH_LINELEN);
    if (buffer == NULL) {
        return NULL;
    }

    while (1) {
        printf("telephonytool> ");
        fflush(stdout);

        len = readline(buffer, CONFIG_NSH_LINELEN, stdin, stdout);
        buffer[len] = '\0';
        if (len < 0)
            continue;

        if (buffer[0] == '!') {
#ifdef CONFIG_SYSTEM_SYSTEM
            system(buffer + 1);
#endif
            continue;
        }

        if (buffer[len - 1] == '\n')
            buffer[len - 1] = '\0';
        cmd = strtok_r(buffer, " \n", &arg);

        if (cmd == NULL)
            continue;

        while (*arg == ' ')
            arg++;

        arg_len = strlen(arg);
        while (isspace(arg[arg_len - 1]))
            arg_len--;

        if (strcmp(cmd, "q") == 0)
            break;

        arg[arg_len] = '\0';
        telephonytool_execute(context, cmd, arg);
    }

    free(buffer);
    return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char* argv[])
{
    struct sched_param param;
    GMainLoop* event_loop;
    tapi_context context;
    pthread_attr_t attr;
    pthread_t thread;
    int ret;

    context = tapi_open("vale.telephony.tool");
    if (context == NULL) {
        return 0;
    }

    pthread_attr_init(&attr);
    param.sched_priority = CONFIG_TELEPHONY_TOOL_PRIORITY;
    pthread_attr_setschedparam(&attr, &param);
    pthread_attr_setstacksize(&attr, CONFIG_TELEPHONY_TOOL_STACKSIZE);

    ret = pthread_create(&thread, &attr, read_stdin, context);
    if (ret != 0) {
        goto out;
    }

    event_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(event_loop);
    g_main_loop_unref(event_loop);

out:
    tapi_close(context);
    return ret;
}
