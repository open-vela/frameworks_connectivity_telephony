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

#define EVENT_MODEM_LIST_QUERY_DONE 0x01
#define EVENT_RADIO_STATE_SET_DONE 0x02
#define EVENT_RAT_MODE_SET_DONE 0x03

#define EVENT_APN_LOADED_DONE 0x04
#define EVENT_APN_SAVE_DONE 0x05
#define EVENT_APN_RESTORE_DONE 0x06
#define EVENT_IP_SETTINGS_QUERY_DONE 0x07

#define EVENT_CHANGE_SIM_PIN_DONE 0x08
#define EVENT_ENTER_SIM_PIN_DONE 0x09
#define EVENT_RESET_SIM_PIN_DONE 0x0A
#define EVENT_LOCK_SIM_PIN_DONE 0x0B
#define EVENT_UNLOCK_SIM_PIN_DONE 0x0C

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
static int telephonytool_cmd_modem_unregister(tapi_context context, char* pargs);
static int telephonytool_cmd_is_feature_supported(tapi_context context, char* pargs);
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

/** Data interface*/
static int telephonytool_cmd_data_register(tapi_context context, char* pargs);
static int telephonytool_cmd_load_apns(tapi_context context, char* pargs);
static int telephonytool_cmd_save_apn(tapi_context context, char* pargs);
static int telephonytool_cmd_reset_apn(tapi_context context, char* pargs);
static int telephonytool_cmd_request_network(tapi_context context, char* pargs);
static int telephonytool_cmd_release_network(tapi_context context, char* pargs);
static int telephonytool_cmd_set_data_roaming(tapi_context context, char* pargs);
static int telephonytool_cmd_get_data_roaming(tapi_context context, char* pargs);
static int telephonytool_cmd_set_data_enabled(tapi_context context, char* pargs);
static int telephonytool_cmd_get_data_enabled(tapi_context context, char* pargs);
static int telephonytool_cmd_get_ip_settings(tapi_context context, char* pargs);

/** Sim interface*/
static int telephonytool_cmd_has_icc_card(tapi_context context, char* pargs);
static int telephonytool_cmd_get_sim_iccid(tapi_context context, char* pargs);
static int telephonytool_cmd_get_sim_operator(tapi_context context, char* pargs);
static int telephonytool_cmd_get_sim_operator_name(tapi_context context, char* pargs);
static int telephonytool_cmd_change_sim_pin(tapi_context context, char* pargs);
static int telephonytool_cmd_enter_sim_pin(tapi_context context, char* pargs);
static int telephonytool_cmd_reset_sim_pin(tapi_context context, char* pargs);
static int telephonytool_cmd_lock_sim_pin(tapi_context context, char* pargs);
static int telephonytool_cmd_unlock_sim_pin(tapi_context context, char* pargs);
static int telephonytool_cmd_listen_sim_state_change(tapi_context context, char* pargs);
static int telephonytool_cmd_unlisten_sim_state_change(tapi_context context, char* pargs);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct telephonytool_cmd_s g_telephonytool_cmds[] = {
    { "list-modem", telephonytool_cmd_query_modem_list,
        "list available modem list (enter example : list-modem" },
    { "listen-modem", telephonytool_cmd_modem_register,
        "modem event callback (enter example : listen-modem 0 0 \
        [slot_id][event_id, 0:radio_state_change 1:call_added 2:call_removed])" },
    { "unlisten-modem", telephonytool_cmd_modem_unregister,
        "modem event callback (enter example : unlisten-modem 0 \
        [watch_id, one uint value returned from \"listen\"])" },
    { "radio-capability", telephonytool_cmd_is_feature_supported,
        "modem event callback (enter example : radio-capability 0 \
        [feature_type, 0: voice 1: data 2: sms 3: ims])" },
    { "radio-set", telephonytool_cmd_set_radio_power,
        "set radio power (enter example : radio-set 0 1 [slot_id][state, 0:radio off 1:radio on])" },
    { "radio-get", telephonytool_cmd_get_radio_power,
        "get radio power (enter example : radio-get 0 [slot_id])" },
    { "dial", telephonytool_cmd_dial,
        "Dial (enter example : dial 0 10086 0 [slot_id][number][hide_call_id, 0:show 1:hide])" },
    { "answer", telephonytool_cmd_answer_call,
        "Answer (enter example : answer 0 0 [slot_id][action:0-answer 1-realse&answer])" },
    { "swap", telephonytool_cmd_swap_call,
        "call Swap (enter example : swap 0 1 [slot_id][action:1-hold 0-unhold])" },
    { "call-listen", telephonytool_cmd_listen_call_property_change,
        "call event callback (enter example : call-listen 0 [call_event]" },
    { "hangup-all", telephonytool_cmd_hangup_all,
        "hangup all call (enter example : hangup-all 0 [slot_id])" },
    { "hangup", telephonytool_cmd_hangup_call,
        "hangup (enter example : hangup 0 [call_id] /phonesim/voicecall01)" },
    { "get-call", telephonytool_cmd_get_call,
        "get call list (enter example : get-call 0 \
        [slot_id][state], 0-active 1-held 2-dialing 3-alerting 4-incoming 5-waiting)" },
    { "transfer", telephonytool_cmd_transfer_call,
        "call transfer  (enter example : transfer 0 [slot_id])" },
    { "merge", telephonytool_cmd_merge_call,
        "call merge  (enter example : merge 0 [slot_id])" },
    { "separate", telephonytool_cmd_separate_call,
        "call separate  (enter example : separate 0 [slot_id][call_id: /phonesim/voicecall01])" },
    { "get-ecclist", telephonytool_cmd_get_ecc_list,
        "get ecc list  (enter example : get-ecclist 0 [slot_id])" },
    { "is-ecc", telephonytool_cmd_is_emergency_number,
        "is emergency number  (enter example : is-ecc 110 [number])" },
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
    { "load-apns", telephonytool_cmd_load_apns,
        "load apn settings (enter example : load-apns 0 [slot_id])" },
    { "save-apn", telephonytool_cmd_save_apn,
        "save apn (enter example : save-apn 0 1 [slot_id][apn_type])" },
    { "reset-apn", telephonytool_cmd_reset_apn,
        "reset apn (enter example : reset-apn 0 [slot_id])" },
    { "request-network", telephonytool_cmd_request_network,
        "request network (enter example : request-network 0 internet [slot_id][apn_type_string])" },
    { "release-network", telephonytool_cmd_release_network,
        "release network (enter example : release-network 0 internet [slot_id][apn_type_string])" },
    { "data-roaming-set", telephonytool_cmd_set_data_roaming,
        "set data roaming (enter example : data-roaming-set 0 1 [slot_id][state])" },
    { "data-roaming-get", telephonytool_cmd_get_data_roaming,
        "get data roaming (enter example : data-roaming-get 0 [slot_id])" },
    { "data-set", telephonytool_cmd_set_data_enabled,
        "set data enabled (enter example : data-set 0 1 [slot_id][state])" },
    { "data-get", telephonytool_cmd_get_data_enabled,
        "get data enabled (enter example : data-get 0 [slot_id])" },
    { "ip-settings", telephonytool_cmd_get_ip_settings,
        "get ip-settings (enter example : ip-settings 0 1 [slot_id][apn_type])" },
    { "listen-data", telephonytool_cmd_data_register,
        "listen data event (enter example : listen-data 0 1 [slot_id][event_id])" },
    { "has-icc", telephonytool_cmd_has_icc_card,
        "has icc card (enter example : has-icc 0 [slot_id])" },
    { "iccid", telephonytool_cmd_get_sim_iccid,
        "get sim iccid (enter example : iccid 0 [slot_id])" },
    { "operator", telephonytool_cmd_get_sim_operator,
        "get sim operator (mcc+mnc) : operator 0 [slot_id])" },
    { "operator-name", telephonytool_cmd_get_sim_operator_name,
        "get sim operator name (enter example : operator-name 0 [slot_id])" },
    { "change-pin", telephonytool_cmd_change_sim_pin,
        "change old pin to new pin (enter example : \
        change-pin 0 pin 1234 2345 [slot_id][pin_type, pin or pin2][old_pin][new_pin])" },
    { "enter-pin", telephonytool_cmd_enter_sim_pin,
        "enter pin to verify (enter example : enter-pin 0 pin 1234 [slot_id][pin_type, pin or pin2][pin])" },
    { "reset-pin", telephonytool_cmd_reset_sim_pin,
        "using puk reset pin (enter example : \
        reset-pin 0 puk 12345678 2345 [slot_id][puk_type, puk or puk2][puk][new_pin])" },
    { "lock-pin", telephonytool_cmd_lock_sim_pin,
        "active sim lock (enter example : lock-pin 0 pin 1234 [slot_id][pin_type, pin or pin2][pin])" },
    { "unlock-pin", telephonytool_cmd_unlock_sim_pin,
        "deactive sim lock (enter example : unlock-pin 0 pin 1234 [slot_id][pin_type, pin or pin2][pin])" },
    { "listen-sim", telephonytool_cmd_listen_sim_state_change,
        "register sim state change (enter example : listen-sim 0 [slot_id])" },
    { "unlisten-sim", telephonytool_cmd_unlisten_sim_state_change,
        "unregister sim state change (enter example : unlisten-sim 0 [slot_id])" },
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

static void tele_call_ecc_list_async_fun(tapi_async_result* result)
{
    int status = result->status;
    int list_length = result->arg1;
    char** ret = result->data;

    printf("tele_call_ecc_list_async_fun : \n");
    printf("msg_id : %d\n", result->msg_id);
    printf("status : %d\n", status);
    printf("list length: %d\n", list_length);

    if (result->status == 0) {
        for (int i = 0; i < list_length; i++) {
            printf("ecc number : %s \n", ret[i]);
        }
    }
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
    tapi_call_free_call_list(pHead);
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
        tapi_call_release_and_answer(context, atoi(slot_id));
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
    tapi_call_register_managercall_change(context, 0, MSG_CALL_ADD_MESSAGE_IND,
        tele_call_async_fun);
    tapi_call_register_managercall_change(context, 0, MSG_CALL_REMOVE_MESSAGE_IND,
        tele_call_async_fun);
    tapi_call_register_emergencylist_change(context, 0, tele_call_ecc_list_async_fun);

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
    char* out[20];
    char* slot_id;
    int size = 0;
    int cnt = split(dst, pargs, " ");
    int i;

    if (cnt != 1)
        return -EINVAL;

    slot_id = dst[0];
    printf("%s, slotId : %s \n", __func__, slot_id);

    size = tapi_call_get_ecc_list(context, atoi(slot_id), out);
    for (i = 0; i < size; i++) {
        printf("ecc number : %s \n", out[i]);
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

    ret = tapi_call_is_emergency_number(context, dst[0]);
    printf("%s, ret : %d\n", __func__, ret);

    return 0;
}

static int telephonytool_cmd_query_modem_list(tapi_context context, char* pargs)
{
    char* modem_list[MAX_MODEM_COUNT];

    if (strlen(pargs) > 0)
        return -EINVAL;

    tapi_query_modem_list(context,
        EVENT_MODEM_LIST_QUERY_DONE, modem_list, MAX_MODEM_COUNT, modem_list_query_complete);
    return 0;
}

static int telephonytool_cmd_modem_register(tapi_context context, char* pargs)
{
    char* slot_id;
    char* target_state;
    int watch_id;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &target_state);
    if (slot_id == NULL)
        return -EINVAL;

    while (*target_state == ' ')
        target_state++;

    if (target_state == NULL)
        return -EINVAL;

    watch_id = tapi_register(context, atoi(slot_id), atoi(target_state), tele_call_async_fun);
    printf("start to watch radio event : %d , return watch_id : %d \n",
        atoi(target_state), watch_id);

    return watch_id;
}

static int telephonytool_cmd_modem_unregister(tapi_context context, char* pargs)
{
    char* watch_id;
    int ret;

    if (strlen(pargs) == 0)
        return -EINVAL;

    watch_id = strtok_r(pargs, " ", NULL);
    if (watch_id == NULL)
        return -EINVAL;

    ret = tapi_unregister(context, atoi(watch_id));
    printf("stop to watch radio event : %s , with watch_id : %s with return value : %d \n",
        watch_id, watch_id, ret);

    return ret;
}

static int telephonytool_cmd_is_feature_supported(tapi_context context, char* pargs)
{
    char* arg;
    bool ret;

    if (strlen(pargs) == 0)
        return -EINVAL;

    arg = strtok_r(pargs, " ", NULL);
    if (arg == NULL)
        return -EINVAL;

    ret = tapi_is_feature_supported(atoi(arg));
    printf("radio feature type : %d is supported ? %d \n", atoi(arg), ret);

    return ret;
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

static int telephonytool_cmd_load_apns(tapi_context context, char* pargs)
{
    tapi_apn_context* apns[MAX_APN_LIST_CAPACITY];
    char* slot_id;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    return tapi_data_load_apn_contexts(context,
        atoi(slot_id), EVENT_APN_LOADED_DONE, apns, MAX_APN_LIST_CAPACITY, tele_call_async_fun);
}

static int telephonytool_cmd_save_apn(tapi_context context, char* pargs)
{
    char* slot_id;
    tapi_apn_context* apn;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    apn = malloc(sizeof(tapi_apn_context));
    if (apn == NULL)
        return -EINVAL;

    apn->type = APN_CONTEXT_TYPE_INTERNET;
    tapi_data_save_apn_context(context, atoi(slot_id), EVENT_APN_SAVE_DONE, apn, tele_call_async_fun);
    free(apn);

    return 0;
}

static int telephonytool_cmd_reset_apn(tapi_context context, char* pargs)
{
    char* slot_id;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    return tapi_data_reset_apn_contexts(context, atoi(slot_id), EVENT_APN_RESTORE_DONE, NULL);
}

static int telephonytool_cmd_request_network(tapi_context context, char* pargs)
{
    char* slot_id;
    char* target_state;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &target_state);
    if (slot_id == NULL)
        return -EINVAL;

    while (*target_state == ' ')
        target_state++;

    if (target_state == NULL)
        return -EINVAL;

    return tapi_data_request_network(context, atoi(slot_id), target_state);
}

static int telephonytool_cmd_release_network(tapi_context context, char* pargs)
{
    char* slot_id;
    char* target_state;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &target_state);
    if (slot_id == NULL)
        return -EINVAL;

    while (*target_state == ' ')
        target_state++;

    if (target_state == NULL)
        return -EINVAL;

    return tapi_data_release_network(context, atoi(slot_id), target_state);
}

static int telephonytool_cmd_set_data_roaming(tapi_context context, char* pargs)
{
    char* status;

    if (!strlen(pargs))
        return -EINVAL;

    status = strtok_r(pargs, " ", NULL);
    if (status == NULL)
        return -EINVAL;

    return tapi_data_enable_roaming(context, atoi(status));
}

static int telephonytool_cmd_get_data_roaming(tapi_context context, char* pargs)
{
    bool result;

    tapi_data_get_roaming_enabled(context, &result);
    printf("%s : %d \n", __func__, result);

    return 0;
}

static int telephonytool_cmd_set_data_enabled(tapi_context context, char* pargs)
{
    char* value;

    if (!strlen(pargs))
        return -EINVAL;

    value = strtok_r(pargs, " ", NULL);
    if (value == NULL)
        return -EINVAL;

    return tapi_data_enable(context, atoi(value));
}

static int telephonytool_cmd_get_data_enabled(tapi_context context, char* pargs)
{
    bool result;

    tapi_data_get_enabled(context, &result);
    printf("%s : %d \n", __func__, result);

    return 0;
}

static int telephonytool_cmd_get_ip_settings(tapi_context context, char* pargs)
{
    char* slot_id;
    char* target_state;
    tapi_ip_settings ip_settings;
    int ret;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &target_state);
    if (slot_id == NULL)
        return -EINVAL;

    while (*target_state == ' ')
        target_state++;

    if (target_state == NULL)
        return -EINVAL;

    ip_settings.ipv4 = malloc(sizeof(tapi_ipv4_settings));
    if (ip_settings.ipv4 == NULL) {
        return -ENOMEM;
    }

    ip_settings.ipv6 = malloc(sizeof(tapi_ipv6_settings));
    if (ip_settings.ipv6 == NULL) {
        free(ip_settings.ipv4);
        return -ENOMEM;
    }

    ret = tapi_data_get_ip_settings(context, atoi(slot_id),
        EVENT_IP_SETTINGS_QUERY_DONE, atoi(target_state), &ip_settings, tele_call_async_fun);
    printf("ipv4 settings : %s -- %s \n", ip_settings.ipv4->interface, ip_settings.ipv4->ip);
    printf("ipv6_settings : %s -- %s \n", ip_settings.ipv6->interface, ip_settings.ipv6->ip);

    free(ip_settings.ipv4);
    free(ip_settings.ipv6);
    return ret;
}

static int telephonytool_cmd_data_register(tapi_context context, char* pargs)
{
    char* slot_id;
    char* target_state;
    int watch_id;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &target_state);
    if (slot_id == NULL)
        return -EINVAL;

    while (*target_state == ' ')
        target_state++;

    if (target_state == NULL)
        return -EINVAL;

    watch_id = tapi_data_register(context, atoi(slot_id), atoi(target_state), tele_call_async_fun);
    printf("start to watch data event : %d , return watch_id : %d \n",
        atoi(target_state), watch_id);

    return watch_id;
}

static int telephonytool_cmd_has_icc_card(tapi_context context, char* pargs)
{
    char* slot_id;
    bool value;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_sim_has_icc_card(context, atoi(slot_id), &value);

    printf("%s, slotId : %s value : %d \n", __func__, slot_id, value);

    return 0;
}

static int telephonytool_cmd_get_sim_iccid(tapi_context context, char* pargs)
{
    char* slot_id;
    char* iccid;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = pargs;
    if (slot_id == NULL)
        return -EINVAL;

    tapi_sim_get_sim_iccid(context, atoi(slot_id), &iccid);

    printf("%s, slotId : %s iccid : %s \n", __func__, slot_id, iccid);

    return 0;
}

static int telephonytool_cmd_get_sim_operator(tapi_context context, char* pargs)
{
    char* slot_id;
    char operator[MAX_MCC_LENGTH + MAX_MNC_LENGTH + 1];

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = pargs;
    if (slot_id == NULL)
        return -EINVAL;

    tapi_sim_get_sim_operator(context,
        atoi(slot_id), (MAX_MCC_LENGTH + MAX_MNC_LENGTH + 1), operator);

    printf("%s, slotId : %s operator : %s \n", __func__, slot_id, operator);

    return 0;
}

static int telephonytool_cmd_get_sim_operator_name(tapi_context context, char* pargs)
{
    char* slot_id;
    char* spn;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = pargs;
    if (slot_id == NULL)
        return -EINVAL;

    tapi_sim_get_sim_operator_name(context, atoi(slot_id), &spn);

    printf("%s, slotId : %s spn : %s \n", __func__, slot_id, spn);

    return 0;
}

static int telephonytool_cmd_change_sim_pin(tapi_context context, char* pargs)
{
    char* slot_id;
    char* pin_type;
    char* old_pin;
    char* new_pin;
    char *temp, *temp2;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &temp);
    if (slot_id == NULL)
        return -EINVAL;

    while (*temp == ' ')
        temp++;

    pin_type = strtok_r(temp, " ", &temp2);
    if (pin_type == NULL)
        return -EINVAL;

    while (*temp2 == ' ')
        temp2++;

    old_pin = strtok_r(temp2, " ", &new_pin);
    if (old_pin == NULL)
        return -EINVAL;

    while (*new_pin == ' ')
        new_pin++;

    if (new_pin == NULL)
        return -EINVAL;

    printf("%s, slot_id: %s pin_type: %s old_pin: %s new_pin: %s \n", __func__, slot_id, pin_type, old_pin, new_pin);

    tapi_sim_change_pin(context, atoi(slot_id),
        EVENT_CHANGE_SIM_PIN_DONE, pin_type, old_pin, new_pin, tele_call_async_fun);

    return 0;
}

static int telephonytool_cmd_enter_sim_pin(tapi_context context, char* pargs)
{
    char* slot_id;
    char* pin_type;
    char* pin;
    char* temp;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &temp);
    if (slot_id == NULL)
        return -EINVAL;

    while (*temp == ' ')
        temp++;

    pin_type = strtok_r(temp, " ", &pin);
    if (pin_type == NULL)
        return -EINVAL;

    while (*pin == ' ')
        pin++;

    if (pin == NULL)
        return -EINVAL;

    printf("%s, slot_id: %s pin_type: %s pin: %s \n", __func__, slot_id, pin_type, pin);

    tapi_sim_enter_pin(context, atoi(slot_id),
        EVENT_ENTER_SIM_PIN_DONE, pin_type, pin, tele_call_async_fun);

    return 0;
}

static int telephonytool_cmd_reset_sim_pin(tapi_context context, char* pargs)
{
    char* slot_id;
    char* puk_type;
    char* puk;
    char* new_pin;
    char *temp, *temp2;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &temp);
    if (slot_id == NULL)
        return -EINVAL;

    while (*temp == ' ')
        temp++;

    puk_type = strtok_r(temp, " ", &temp2);
    if (puk_type == NULL)
        return -EINVAL;

    while (*temp2 == ' ')
        temp2++;

    puk = strtok_r(temp2, " ", &new_pin);
    if (puk == NULL)
        return -EINVAL;

    while (*new_pin == ' ')
        new_pin++;

    if (new_pin == NULL)
        return -EINVAL;

    printf("%s, slot_id: %s puk_type: %s puk: %s new_pin: %s \n", __func__, slot_id, puk_type, puk, new_pin);

    tapi_sim_reset_pin(context, atoi(slot_id),
        EVENT_RESET_SIM_PIN_DONE, puk_type, puk, new_pin, tele_call_async_fun);

    return 0;
}

static int telephonytool_cmd_lock_sim_pin(tapi_context context, char* pargs)
{
    char* slot_id;
    char* pin_type;
    char* pin;
    char* temp;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &temp);
    if (slot_id == NULL)
        return -EINVAL;

    while (*temp == ' ')
        temp++;

    pin_type = strtok_r(temp, " ", &pin);
    if (pin_type == NULL)
        return -EINVAL;

    while (*pin == ' ')
        pin++;

    if (pin == NULL)
        return -EINVAL;

    printf("%s, slot_id: %s pin_type: %s pin: %s \n", __func__, slot_id, pin_type, pin);

    tapi_sim_lock_pin(context, atoi(slot_id),
        EVENT_LOCK_SIM_PIN_DONE, pin_type, pin, tele_call_async_fun);

    return 0;
}

static int telephonytool_cmd_unlock_sim_pin(tapi_context context, char* pargs)
{
    char* slot_id;
    char* pin_type;
    char* pin;
    char* temp;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &temp);
    if (slot_id == NULL)
        return -EINVAL;

    while (*temp == ' ')
        temp++;

    pin_type = strtok_r(temp, " ", &pin);
    if (pin_type == NULL)
        return -EINVAL;

    while (*pin == ' ')
        pin++;

    if (pin == NULL)
        return -EINVAL;

    printf("%s, slot_id: %s pin_type: %s pin: %s \n", __func__, slot_id, pin_type, pin);

    tapi_sim_unlock_pin(context, atoi(slot_id),
        EVENT_UNLOCK_SIM_PIN_DONE, pin_type, pin, tele_call_async_fun);

    return 0;
}

static int telephonytool_cmd_listen_sim_state_change(tapi_context context, char* pargs)
{
    char* slot_id;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    printf("%s, slotId : %s \n", __func__, slot_id);

    tapi_sim_register_sim_state_change(context, atoi(slot_id), tele_call_async_fun);

    return 0;
}

static int telephonytool_cmd_unlisten_sim_state_change(tapi_context context, char* pargs)
{
    char* watch_id;

    if (strlen(pargs) == 0)
        return -EINVAL;

    watch_id = strtok_r(pargs, " ", NULL);
    if (watch_id == NULL)
        return -EINVAL;

    printf("%s, watch_id : %s \n", __func__, watch_id);

    tapi_sim_unregister_sim_state_change(context, atoi(watch_id));

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
