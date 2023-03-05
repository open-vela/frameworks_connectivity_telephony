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
#include <syslog.h>
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
#define EVENT_APN_REMOVAL_DONE 0x06
#define EVENT_APN_RESTORE_DONE 0x07
#define EVENT_DATA_ALLOWED_DONE 0x08

#define EVENT_CHANGE_SIM_PIN_DONE 0x09
#define EVENT_ENTER_SIM_PIN_DONE 0x0A
#define EVENT_RESET_SIM_PIN_DONE 0x0B
#define EVENT_LOCK_SIM_PIN_DONE 0x0C
#define EVENT_UNLOCK_SIM_PIN_DONE 0x0D

#define EVENT_NETWORK_SCAN_DONE 0x0E
#define EVENT_REGISTER_AUTO_DONE 0x0F
#define EVENT_REGISTER_MANUAL_DONE 0x10
#define EVENT_QUERY_REGISTRATION_INFO_DONE 0x11
#define EVENT_QUERY_SERVING_CELL_DONE 0x12
#define EVENT_QUERY_NEIGHBOURING_CELL_DONE 0x13

#define EVENT_REQUEST_CALL_BARRING_DONE 0x14
#define EVENT_CALL_BARRING_PASSWD_CHANGE_DONE 0x15
#define EVENT_DISABLE_ALL_CALL_BARRINGS_DONE 0x16
#define EVENT_DISABLE_ALL_INCOMING_DONE 0x17
#define EVENT_DISABLE_ALL_OUTGOING_DONE 0x18
#define EVENT_REQUEST_CALL_FORWARDING_DONE 0x19
#define EVENT_DISABLE_CALL_FORWARDING_DONE 0x1A
#define EVENT_CANCEL_USSD_DONE 0x1B
#define EVENT_REQUEST_CALL_WAITING_DONE 0x1C
#define EVENT_SEND_USSD_DONE 0x1D
#define EVENT_INITIATE_SERVICE_DONE 0x1E
#define EVENT_ENABLE_FDN_DONE 0x1F
#define EVENT_QUERY_FDN_DONE 0x20

#define EVENT_LOAD_ADN_ENTRIES_DONE 0x21

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
 * Private Declarations
 ****************************************************************************/

static struct telephonytool_cmd_s g_telephonytool_cmds[];
static int telephonytool_cmd_help(tapi_context context, char* pargs);
static void telephonytool_execute(tapi_context context, char* cmd, char* arg);
static void* read_stdin(pthread_addr_t pvarg);

/****************************************************************************
 * Private Function
 ****************************************************************************/

static int split_input(char dst[][CONFIG_NSH_LINELEN], int size, char* str, const char* spl)
{
    char* result;
    char* p_save;
    int n = 0;

    result = strtok_r(str, spl, &p_save);
    while (result != NULL) {
        if (n < size)
            strcpy(dst[n], result);

        n++;
        result = strtok_r(NULL, spl, &p_save);
    }

    return n;
}

static void tele_call_async_fun(tapi_async_result* result)
{
    syslog(LOG_DEBUG, "%s : \n", __func__);
    syslog(LOG_DEBUG, "result->msg_id : %d\n", result->msg_id);
    syslog(LOG_DEBUG, "result->status : %d\n", result->status);
    syslog(LOG_DEBUG, "result->arg1 : %d\n", result->arg1);
    syslog(LOG_DEBUG, "result->arg2 : %d\n", result->arg2);

    if (result->msg_id == MSG_CALL_MERGE_IND || result->msg_id == MSG_CALL_SEPERATE_IND) {
        char** ret = result->data;

        syslog(LOG_DEBUG, "conference size : %d\n", result->arg2);

        for (int i = 0; i < result->arg2; i++) {
            syslog(LOG_DEBUG, "conference call id : %s\n", ret[i]);
        }
    } else if (result->msg_id == EVENT_LOAD_ADN_ENTRIES_DONE) {
        syslog(LOG_DEBUG, "adn entries : %s\n", (char*)result->data);
    }
}

static void tele_call_manager_call_async_fun(tapi_async_result* result)
{
    tapi_call_info* call_info;

    syslog(LOG_DEBUG, "%s : %d\n", __func__, result->status);

    if (result->msg_id == MSG_CALL_ADD_MESSAGE_IND) {
        call_info = (tapi_call_info*)result->data;

        syslog(LOG_DEBUG, "call added call_id : %s\n", call_info->call_id);
        syslog(LOG_DEBUG, "call state: %d \n", call_info->state);
        syslog(LOG_DEBUG, "call LineIdentification: %s \n", call_info->lineIdentification);
        syslog(LOG_DEBUG, "call IncomingLine: %s \n", call_info->incoming_line);
        syslog(LOG_DEBUG, "call Name: %s \n", call_info->name);
        syslog(LOG_DEBUG, "call StartTime: %s \n", call_info->start_time);
        syslog(LOG_DEBUG, "call Multiparty: %d \n", call_info->multiparty);
        syslog(LOG_DEBUG, "call RemoteHeld: %d \n", call_info->remote_held);
        syslog(LOG_DEBUG, "call RemoteMultiparty: %d \n", call_info->remote_multiparty);
        syslog(LOG_DEBUG, "call Information: %s \n", call_info->info);
        syslog(LOG_DEBUG, "call Icon: %d \n", call_info->icon);
        syslog(LOG_DEBUG, "call Emergency: %d \n\n", call_info->is_emergency_number);

    } else if (result->msg_id == MSG_CALL_REMOVE_MESSAGE_IND) {
        syslog(LOG_DEBUG, "call removed call_id : %s\n", (char*)result->data);
    } else if (result->msg_id == MSG_CALL_RING_BACK_TONE_IND) {
        syslog(LOG_DEBUG, "ring back tone status : %d\n", result->arg2);
    }
}

static void tele_call_info_call_async_fun(tapi_async_result* result)
{
    syslog(LOG_DEBUG, "tele_call_info_call_async_fun msg id: %d\n", result->msg_id);

    if (result->msg_id == MSG_CALL_PROPERTY_CHANGED_MESSAGE_IND) {
        tapi_call_property* call = result->data;

        if (strcmp(call->key, "State") == 0 || strcmp(call->key, "LineIdentification") == 0
            || strcmp(call->key, "IncomingLine") == 0 || strcmp(call->key, "Name") == 0
            || strcmp(call->key, "StartTime") == 0 || strcmp(call->key, "Information") == 0) {
            syslog(LOG_DEBUG, "call property call_id : %s  [%s,%s]\n\n", call->call_id, call->key,
                (char*)call->value);
        } else {
            syslog(LOG_DEBUG, "call property call_id : %s  [%s,%d]\n\n", call->call_id, call->key,
                (int)call->value);
        }
    } else if (result->msg_id == MSG_CALL_DISCONNECTED_REASON_MESSAGE_IND) {
        syslog(LOG_DEBUG, "call disconnected reason call_id : %s  reason:%d\n\n",
            (char*)result->data, result->arg2);
    }
}

static void tele_call_ecc_list_async_fun(tapi_async_result* result)
{
    int status = result->status;
    int list_length = result->arg2;
    char** ret = result->data;

    syslog(LOG_DEBUG, "%s : \n", __func__);
    syslog(LOG_DEBUG, "msg_id : %d\n", result->msg_id);
    syslog(LOG_DEBUG, "status : %d\n", status);
    syslog(LOG_DEBUG, "list length: %d\n", list_length);

    if (result->status == 0) {
        for (int i = 0; i < list_length; i++) {
            syslog(LOG_DEBUG, "ecc number : %s \n", ret[i]);
        }
    }
}

static void tele_ims_async_fun(tapi_async_result* result)
{
    int status = result->status;
    tapi_ims_registration_info* info;

    syslog(LOG_DEBUG, "%s : \n", __func__);
    syslog(LOG_DEBUG, "msg_id : %d\n", result->msg_id);

    if (status == OK) {
        info = result->data;
        syslog(LOG_DEBUG, "%s: ret_info: %d, ext_info: %d\n", __func__,
            info->reg_info, info->ext_info);
    }
}

static void tele_sms_async_fun(tapi_async_result* result)
{
    tapi_message_info* message_info;

    syslog(LOG_DEBUG, "%s : %d\n", __func__, result->status);

    if (result->msg_id == MSG_INCOMING_MESSAGE_IND) {
        message_info = (tapi_message_info*)result->data;
        syslog(LOG_DEBUG, "receive incoming message tele_sms_async_fun msg: %s\n",
            message_info->text);
        syslog(LOG_DEBUG, "receive incoming message tele_sms_async_fun to: %s\n",
            message_info->sender);
    } else if (result->msg_id == MSG_IMMEDIATE_MESSAGE_IND) {
        message_info = (tapi_message_info*)result->data;
        syslog(LOG_DEBUG, "receive immediate message tele_sms_async_fun msg: %s\n",
            message_info->text);
        syslog(LOG_DEBUG, "receive immediate message tele_sms_async_fun to: %s\n",
            message_info->sender);
    }
}

static void tele_cbs_async_fun(tapi_async_result* result)
{
    syslog(LOG_DEBUG, "%s : %d\n", __func__, result->status);

    if (result->msg_id == MSG_INCOMING_CBS_IND) {
        tapi_cbs_message* cbs_info = (tapi_cbs_message*)result->data;
        syslog(LOG_DEBUG, "receive incoming cbs tele_cbs_async_fun msg: %s\n",
            cbs_info->text);
    } else if (result->msg_id == MSG_EMERGENCY_CBS_IND) {
        tapi_cbs_emergency_message* cbs_emergency_message
            = (tapi_cbs_emergency_message*)result->data;
        syslog(LOG_DEBUG, "receive immediate cbs tele_cbs_async_fun msg: %s\n",
            cbs_emergency_message->text);
    }
}

static void modem_list_query_complete(tapi_async_result* result)
{
    char* modem_path;
    int modem_count;
    char** list = result->data;

    syslog(LOG_DEBUG, "%s : \n", __func__);
    if (list == NULL)
        return;

    syslog(LOG_DEBUG, "result->status : %d\n", result->status);
    modem_count = result->arg1;
    for (int i = 0; i < modem_count; i++) {
        modem_path = list[i];
        if (modem_path != NULL)
            syslog(LOG_DEBUG, "modem found with path -> %s \n", modem_path);
    }
}

static void call_list_query_complete(tapi_async_result* result)
{
    tapi_call_info* call_info;

    syslog(LOG_DEBUG, "%s : \n", __func__);
    if (result->status != OK)
        return;

    syslog(LOG_DEBUG, "call count: %d \n\n", result->arg2);

    call_info = result->data;

    for (int i = 0; i < result->arg2; i++) {

        syslog(LOG_DEBUG, "call id: %s \n", call_info[i].call_id);
        syslog(LOG_DEBUG, "call state: %d \n", call_info[i].state);
        syslog(LOG_DEBUG, "call LineIdentification: %s \n", call_info[i].lineIdentification);
        syslog(LOG_DEBUG, "call IncomingLine: %s \n", call_info[i].incoming_line);
        syslog(LOG_DEBUG, "call Name: %s \n", call_info[i].name);
        syslog(LOG_DEBUG, "call StartTime: %s \n", call_info[i].start_time);
        syslog(LOG_DEBUG, "call Multiparty: %d \n", call_info[i].multiparty);
        syslog(LOG_DEBUG, "call RemoteHeld: %d \n", call_info[i].remote_held);
        syslog(LOG_DEBUG, "call RemoteMultiparty: %d \n", call_info[i].remote_multiparty);
        syslog(LOG_DEBUG, "call Information: %s \n", call_info[i].info);
        syslog(LOG_DEBUG, "call Icon: %d \n", call_info[i].icon);
        syslog(LOG_DEBUG, "call Emergency: %d \n\n", call_info[i].is_emergency_number);
    }
}

static void radio_signal_change(tapi_async_result* result)
{
    int signal = result->msg_id;
    int slot_id = result->arg1;
    int param = result->arg2;

    switch (signal) {
    case MSG_RADIO_STATE_CHANGE_IND:
        syslog(LOG_DEBUG, "radio state changed to %d in slot[%d] \n", param, slot_id);
        break;
    case MSG_PHONE_STATE_CHANGE_IND:
        syslog(LOG_DEBUG, "phone state changed to %d in slot[%d] \n", param, slot_id);
        break;
    default:
        break;
    }
}

static void network_event_callback(tapi_async_result* result)
{
    tapi_operator_info** operator_list;
    tapi_operator_info* operator_info;
    tapi_registration_info* info;
    tapi_cell_identity** cell_list;
    tapi_cell_identity* cell;
    int msg, index;

    msg = result->msg_id;

    switch (msg) {
    case EVENT_NETWORK_SCAN_DONE:
        index = result->arg2;
        operator_list = result->data;

        while (--index >= 0) {
            operator_info = *operator_list++;
            syslog(LOG_DEBUG, "id : %s, name : %s, status : %d, mcc : %s, mnc : %s \n",
                operator_info->id, operator_info->name, operator_info->status,
                operator_info->mcc, operator_info->mnc);
        }
        break;
    case EVENT_REGISTER_AUTO_DONE:
        syslog(LOG_DEBUG, "auto register status : %d \n", result->status);
        break;
    case EVENT_REGISTER_MANUAL_DONE:
        syslog(LOG_DEBUG, "manual register status : %d \n", result->status);
        break;
    case EVENT_QUERY_REGISTRATION_INFO_DONE:
        info = result->data;
        syslog(LOG_DEBUG, "%s : \n", __func__);
        if (info == NULL)
            return;

        syslog(LOG_DEBUG, "reg_state = %d operator_name = %s mcc = %s mnc = %s \n",
            info->reg_state, info->operator_name, info->mcc, info->mnc);
        break;
    case EVENT_QUERY_SERVING_CELL_DONE:
    case EVENT_QUERY_NEIGHBOURING_CELL_DONE:
        index = result->arg2;
        cell_list = result->data;

        while (--index >= 0) {
            cell = *cell_list++;
            syslog(LOG_DEBUG, "ci : %d, mcc : %s, mnc : %s, registered : %d, type : %d, \n",
                cell->ci, cell->mcc_str, cell->mnc_str, cell->registered, cell->type);
        }
        break;
    default:
        break;
    }
}

static void network_signal_change(tapi_async_result* result)
{
    tapi_cell_identity** cell_list;
    tapi_cell_identity* cell;
    tapi_network_time* nitz;
    int signal = result->msg_id;
    int slot_id = result->arg1;
    int param = result->arg2;

    switch (signal) {
    case MSG_NETWORK_STATE_CHANGE_IND:
        syslog(LOG_DEBUG, "MSG_NETWORK_STATE_CHANGE_IND \n");
        break;
    case MSG_CELLINFO_CHANGE_IND:
        cell_list = result->data;
        if (cell_list != NULL) {
            while (--param >= 0) {
                cell = *cell_list++;
                syslog(LOG_DEBUG, "ci : %d, pci : %d, tac : %d, earfcn : %d, bandwidth : %d, \
                    type : %d mcc : %s, mnc : %s, alpha_long : %s, alpha_short : %s \n",
                    cell->ci, cell->pci, cell->tac, cell->earfcn, cell->bandwidth, cell->type,
                    cell->mcc_str, cell->mnc_str, cell->alpha_long, cell->alpha_short);
            }
            syslog(LOG_DEBUG, "phone state changed to %d in slot[%d] \n", param, slot_id);
        }
        break;
    case MSG_SIGNAL_STRENGTH_STATE_CHANGE_IND:
        syslog(LOG_DEBUG, "signal strength changed to %d in slot[%d] \n", param, slot_id);
        break;
    case MSG_NITZ_STATE_CHANGE_IND:
        nitz = result->data;
        if (nitz != NULL) {
            syslog(LOG_DEBUG, "sec = %d min = %d hour = %d \
                mday = %d mon = %d year = %d dst = %d utcoff = %d \n",
                nitz->sec, nitz->min, nitz->hour, nitz->mday, nitz->mon,
                nitz->year, nitz->dst, nitz->utcoff);
        }
        break;
    default:
        break;
    }
}

static void data_event_response(tapi_async_result* result)
{
    int event = result->msg_id;
    int apn_capacity;
    tapi_apn_context** apn_list;
    tapi_apn_context* apn_item;

    switch (event) {
    case EVENT_APN_LOADED_DONE:
        apn_capacity = result->arg2;
        apn_list = result->data;

        while (--apn_capacity >= 0) {
            apn_item = apn_list[apn_capacity];
            syslog(LOG_DEBUG, "id : %s, name : %s, type : %d, apn : %s \n",
                apn_item->id, apn_item->name, apn_item->type, apn_item->accesspointname);
        }
        break;
    case EVENT_APN_SAVE_DONE:
        syslog(LOG_DEBUG, "apn saved with error code : %d \n", result->status);
        break;
    case EVENT_APN_REMOVAL_DONE:
        syslog(LOG_DEBUG, "apn removed with error code : %d \n", result->status);
        break;
    case EVENT_APN_RESTORE_DONE:
        syslog(LOG_DEBUG, "apn restored with error code : %d \n", result->status);
        break;
    case EVENT_DATA_ALLOWED_DONE:
        syslog(LOG_DEBUG, "data allowed with error code : %d \n", result->status);
        break;
    default:
        break;
    }
}

static void data_signal_change(tapi_async_result* result)
{
    tapi_apn_context* dc;
    tapi_ipv4_settings* ipv4;
    tapi_ipv6_settings* ipv6;
    int signal = result->msg_id;
    int slot_id = result->arg1;
    int param = result->arg2;

    switch (signal) {
    case MSG_DATA_ENABLED_CHANGE_IND:
        syslog(LOG_DEBUG, "data switch changed to %d in slot[%d] \n", param, slot_id);
        break;
    case MSG_DATA_REGISTRATION_STATE_CHANGE_IND:
        syslog(LOG_DEBUG, "data registration state changed to %d in slot[%d] \n", param, slot_id);
        break;
    case MSG_DATA_NETWORK_TYPE_CHANGE_IND:
        syslog(LOG_DEBUG, "data network type changed to %d in slot[%d] \n", param, slot_id);
        break;
    case MSG_DATA_CONNECTION_STATE_CHANGE_IND:
        dc = result->data;
        if (dc != NULL) {
            syslog(LOG_DEBUG, "id : %s, name : %s, type : %d, apn : %s, active : %d \n",
                dc->id, dc->name, dc->type, dc->accesspointname, dc->active);

            if (dc->ip_settings != NULL) {
                ipv4 = dc->ip_settings->ipv4;
                if (ipv4 != NULL) {
                    syslog(LOG_DEBUG, "ipv4-interface : %s, ip : %s, gateway : %s, dns : %s \n",
                        ipv4->interface, ipv4->ip, ipv4->gateway, ipv4->dns[0]);
                }

                ipv6 = dc->ip_settings->ipv6;
                if (ipv6 != NULL) {
                    syslog(LOG_DEBUG, "ipv6-interface : %s, ip : %s, gateway : %s, dns : %s \n",
                        ipv6->interface, ipv6->ip, ipv6->gateway, ipv6->dns[0]);
                }
            }
        }
        break;
    case MSG_DEFAULT_DATA_SLOT_CHANGE_IND:
        syslog(LOG_DEBUG, "data slot changed to slot[%d] \n", param);
        break;
    default:
        break;
    }
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

    syslog(LOG_DEBUG, "%s, slot_id: %s number: %s  hide_callerid: %s \n", __func__,
        slot_id, number, hide_callerid);
    return tapi_call_dial(context, atoi(slot_id), number, atoi(hide_callerid));
}

static int telephonytool_cmd_answer_call(tapi_context context, char* pargs)
{
    char dst[3][CONFIG_NSH_LINELEN];
    int slot_id, type;
    char* call_id;
    int cnt, ret;

    if (strlen(pargs) == 0)
        return -EINVAL;

    cnt = split_input(dst, 3, pargs, " ");
    if (cnt != 2 && cnt != 3)
        return -EINVAL;

    slot_id = atoi(dst[0]);
    type = atoi(dst[1]);

    if (type == 0 && cnt == 3) {
        call_id = dst[2];
        syslog(LOG_DEBUG, "%s, slotId : %d callId : %s\n", __func__, slot_id, call_id);
        ret = tapi_call_answer_call(context, slot_id, call_id, 1);
    } else if (type == 1) {
        syslog(LOG_DEBUG, "%s, slotId : %d\n", __func__, slot_id);
        ret = tapi_call_release_and_answer(context, slot_id);
    } else {
        ret = -EINVAL;
    }

    return ret;
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

    syslog(LOG_DEBUG, "%s, slotId : %s\n", __func__, slot_id);
    return tapi_call_hangup_all_calls(context, atoi(slot_id));
}

static int telephonytool_cmd_hangup_call(tapi_context context, char* pargs)
{
    char dst[2][CONFIG_NSH_LINELEN];
    int cnt = split_input(dst, 2, pargs, " ");
    char* slot_id;

    if (cnt != 2)
        return -EINVAL;

    slot_id = dst[0];
    syslog(LOG_DEBUG, "%s, slotId : %s\n", __func__, dst[0]);

    return tapi_call_hangup_call(context, atoi(slot_id), (char*)dst[1]);
}

static int telephonytool_cmd_release_and_swap(tapi_context context, char* pargs)
{
    char dst[1][CONFIG_NSH_LINELEN];
    int cnt = split_input(dst, 1, pargs, " ");
    char* slot_id;

    if (cnt != 1)
        return -EINVAL;

    slot_id = dst[0];
    syslog(LOG_DEBUG, "%s, slotId : %s\n", __func__, dst[0]);
    return tapi_call_release_and_swap(context, atoi(slot_id));
}

static int telephonytool_cmd_swap_call(tapi_context context, char* pargs)
{
    char dst[2][CONFIG_NSH_LINELEN];
    int cnt = split_input(dst, 2, pargs, " ");
    int ret = -EINVAL;
    char* slot_id;

    if (cnt != 2)
        return -EINVAL;

    slot_id = dst[0];
    syslog(LOG_DEBUG, "%s, slotId : %s\n", __func__, dst[0]);

    if (atoi(dst[1]) == 1) {
        ret = tapi_call_hold_call(context, atoi(slot_id));
    } else {
        ret = tapi_call_unhold_call(context, atoi(slot_id));
    }

    return ret;
}

static int telephonytool_cmd_get_call(tapi_context context, char* pargs)
{
    char dst[2][CONFIG_NSH_LINELEN];
    int cnt = split_input(dst, 2, pargs, " ");
    tapi_call_info call_info;
    char* call_id;
    int ret = -EINVAL;
    int slot_id;

    printf("%s\n", __func__);

    if (cnt == 1) {
        slot_id = atoi(dst[0]);
        ret = tapi_call_get_all_calls(context, slot_id, call_list_query_complete);
    } else if (cnt == 2) {
        slot_id = atoi(dst[0]);
        call_id = dst[1];

        memset(&call_info, 0, sizeof(tapi_call_info));

        ret = tapi_call_get_call_info(context, slot_id, call_id, &call_info);
        if (ret == OK) {
            syslog(LOG_DEBUG, "call id: %s \n", call_info.call_id);
            syslog(LOG_DEBUG, "call state: %d \n", call_info.state);
            syslog(LOG_DEBUG, "call LineIdentification: %s \n", call_info.lineIdentification);
            syslog(LOG_DEBUG, "call IncomingLine: %s \n", call_info.incoming_line);
            syslog(LOG_DEBUG, "call Name: %s \n", call_info.name);
            syslog(LOG_DEBUG, "call StartTime: %s \n", call_info.start_time);
            syslog(LOG_DEBUG, "call Multiparty: %d \n", call_info.multiparty);
            syslog(LOG_DEBUG, "call RemoteHeld: %d \n", call_info.remote_held);
            syslog(LOG_DEBUG, "call RemoteMultiparty: %d \n", call_info.remote_multiparty);
            syslog(LOG_DEBUG, "call Information: %s \n", call_info.info);
            syslog(LOG_DEBUG, "call Icon: %d \n", call_info.icon);
            syslog(LOG_DEBUG, "call Emergency: %d \n", call_info.is_emergency_number);
        }
        return ret;
    } else {
        return -EINVAL;
    }

    return ret;
}

static int telephonytool_cmd_listen_call_manager_change(tapi_context context, char* pargs)
{
    char dst[1][CONFIG_NSH_LINELEN];
    int cnt = split_input(dst, 1, pargs, " ");
    int watch_id, slot_id;

    if (cnt != 1)
        return -EINVAL;

    slot_id = atoi(dst[0]);

    syslog(LOG_DEBUG, "%s, cnt : %d\n", __func__, cnt);
    watch_id = tapi_call_register_managercall_change(context, slot_id, MSG_CALL_ADD_MESSAGE_IND,
        tele_call_manager_call_async_fun);
    syslog(LOG_DEBUG, "retister call add change, return watch id: %d\n", watch_id);
    if (watch_id < 0)
        return watch_id;

    watch_id = tapi_call_register_managercall_change(context, slot_id, MSG_CALL_REMOVE_MESSAGE_IND,
        tele_call_manager_call_async_fun);
    syslog(LOG_DEBUG, "reitster call remove change, return watch id: %d\n", watch_id);
    if (watch_id < 0)
        return watch_id;

    watch_id = tapi_call_register_emergencylist_change(context, slot_id,
        tele_call_ecc_list_async_fun);
    syslog(LOG_DEBUG, "retister ecc list change, return watch id: %d\n", watch_id);
    if (watch_id < 0)
        return watch_id;

    watch_id = tapi_call_register_ring_back_tone_change(context, slot_id,
        tele_call_manager_call_async_fun);
    syslog(LOG_DEBUG, "retister ring back tone change, return watch id: %d\n", watch_id);

    return watch_id;
}

static int telephonytool_cmd_unlisten_call_singal(tapi_context context, char* pargs)
{
    char dst[1][CONFIG_NSH_LINELEN];
    int cnt = split_input(dst, 1, pargs, " ");

    if (cnt != 1)
        return -EINVAL;

    syslog(LOG_DEBUG, "%s, cnt : %d\n", __func__, cnt);
    return tapi_unregister(context, atoi(dst[0]));
}

static int telephonytool_cmd_call_proxy(tapi_context context, char* pargs)
{
    char dst[3][CONFIG_NSH_LINELEN];
    int cnt = split_input(dst, 3, pargs, " ");
    int slot_id, action;
    int ret = -EINVAL;
    char* call_id;

    if (cnt != 3)
        return -EINVAL;

    slot_id = atoi(dst[0]);
    action = atoi(dst[1]);
    call_id = dst[2];

    if (action == 0) {
        syslog(LOG_DEBUG, "%s, new call proxy\n", __func__);

        ret = tapi_call_new_voice_call_proxy(context, slot_id, call_id);
    } else if (action == 1) {
        syslog(LOG_DEBUG, "%s, release call proxy\n", __func__);

        ret = tapi_call_release_voice_call_proxy(context, slot_id, call_id);
    }
    return ret;
}

static int telephonytool_cmd_listen_call_info_change(tapi_context context, char* pargs)
{
    char dst[2][CONFIG_NSH_LINELEN];
    int cnt = split_input(dst, 2, pargs, " ");
    int slot_id, watch_id;
    char* call_id;

    if (cnt != 2)
        return -EINVAL;

    slot_id = atoi(dst[0]);
    call_id = dst[1];

    syslog(LOG_DEBUG, "%s, cnt : %d\n", __func__, cnt);

    watch_id = tapi_call_register_call_info_change(context, slot_id, call_id,
        MSG_CALL_PROPERTY_CHANGED_MESSAGE_IND, tele_call_info_call_async_fun);
    syslog(LOG_DEBUG, "register call info change, return watch id: %d\n", watch_id);
    if (watch_id < 0)
        return watch_id;

    watch_id = tapi_call_register_call_info_change(context, slot_id, call_id,
        MSG_CALL_DISCONNECTED_REASON_MESSAGE_IND, tele_call_info_call_async_fun);
    syslog(LOG_DEBUG, "register call disconnected reason change, return watch id: %d\n", watch_id);
    return watch_id;
}

static int telephonytool_cmd_transfer_call(tapi_context context, char* pargs)
{
    char dst[1][CONFIG_NSH_LINELEN];
    int cnt = split_input(dst, 1, pargs, " ");
    char* slot_id;

    if (cnt != 1)
        return -EINVAL;

    slot_id = dst[0];
    syslog(LOG_DEBUG, "%s, slotId : %s\n", __func__, slot_id);

    return tapi_call_transfer(context, atoi(slot_id));
}

static int telephonytool_cmd_merge_call(tapi_context context, char* pargs)
{
    char dst[1][CONFIG_NSH_LINELEN];
    char* slot_id;
    int cnt = split_input(dst, 1, pargs, " ");

    if (cnt != 1)
        return -EINVAL;

    slot_id = dst[0];
    syslog(LOG_DEBUG, "%s, slotId : %s\n", __func__, slot_id);

    return tapi_call_merge_call(context, atoi(slot_id), tele_call_async_fun);
}

static int telephonytool_cmd_separate_call(tapi_context context, char* pargs)
{
    char dst[2][CONFIG_NSH_LINELEN];
    int cnt = split_input(dst, 2, pargs, " ");
    char* slot_id;

    if (cnt != 2)
        return -EINVAL;

    slot_id = dst[0];
    syslog(LOG_DEBUG, "%s, slotId : %s\n", __func__, slot_id);

    return tapi_call_separate_call(context, atoi(slot_id), dst[1], tele_call_async_fun);
}

static int telephonytool_cmd_get_ecc_list(tapi_context context, char* pargs)
{
    char dst[1][CONFIG_NSH_LINELEN];
    int cnt = split_input(dst, 1, pargs, " ");
    char* out[20];
    char* slot_id;
    int size = 0;
    int i;

    if (cnt != 1)
        return -EINVAL;

    slot_id = dst[0];
    syslog(LOG_DEBUG, "%s, slotId : %s \n", __func__, slot_id);

    size = tapi_call_get_ecc_list(context, atoi(slot_id), out);
    for (i = 0; i < size; i++) {
        syslog(LOG_DEBUG, "ecc number : %s \n", out[i]);
    }

    return size;
}

static int telephonytool_cmd_is_emergency_number(tapi_context context, char* pargs)
{
    char dst[1][CONFIG_NSH_LINELEN];
    int cnt = split_input(dst, 1, pargs, " ");
    int ret;

    if (cnt != 1)
        return -EINVAL;

    ret = tapi_call_is_emergency_number(context, dst[0]);
    syslog(LOG_DEBUG, "%s, ret : %d\n", __func__, ret);

    return 0;
}

static int telephonytool_cmd_query_modem_list(tapi_context context, char* pargs)
{
    if (strlen(pargs) > 0)
        return -EINVAL;

    tapi_query_modem_list(context,
        EVENT_MODEM_LIST_QUERY_DONE, modem_list_query_complete);
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

    watch_id = tapi_register(context, atoi(slot_id), atoi(target_state), radio_signal_change);
    syslog(LOG_DEBUG, "start to watch radio event : %d , return watch_id : %d \n",
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
    syslog(LOG_DEBUG, "stop to watch radio event : %s , with watch_id : \
        %s with return value : %d \n",
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
    syslog(LOG_DEBUG, "radio feature type : %d is supported ? %d \n", atoi(arg), ret);

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

    syslog(LOG_DEBUG, "%s, slotId : %s target_state: %s \n", __func__, slot_id, target_state);
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
    syslog(LOG_DEBUG, "%s, slotId : %s value : %d \n", __func__, slot_id, value);

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

    syslog(LOG_DEBUG, "%s, slotId : %s target_state: %s \n", __func__, slot_id, target_state);
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
    syslog(LOG_DEBUG, "%s, slotId : %s value :%d \n", __func__, slot_id, value);

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
    syslog(LOG_DEBUG, "%s, slotId : %s imei : %s \n", __func__, slot_id, imei);

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
    syslog(LOG_DEBUG, "%s, slotId : %s imeisv : %s \n", __func__, slot_id, imeisv);

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
    syslog(LOG_DEBUG, "%s, slotId : %s value : %s \n", __func__, slot_id, value);

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
    syslog(LOG_DEBUG, "%s, slotId : %s value : %s \n", __func__, slot_id, value);

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
    syslog(LOG_DEBUG, "%s, slotId : %s value : %s \n", __func__, slot_id, value);

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
    syslog(LOG_DEBUG, "%s, slotId : %s state : %d \n", __func__, slot_id, state);

    return 0;
}

static int telephonytool_cmd_send_modem_power(tapi_context context, char* pargs)
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

    syslog(LOG_DEBUG, "%s, slotId : %s target_state: %s \n", __func__, slot_id, target_state);
    return tapi_send_modem_power(context, atoi(slot_id), atoi(target_state));
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
    syslog(LOG_DEBUG, "%s, slotId : %s state : %d \n", __func__, slot_id, state);

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
    syslog(LOG_DEBUG, "%s, slotId : %s  number : %s \n", __func__, slot_id, number);

    return 0;
}

static int telephonytool_cmd_load_apns(tapi_context context, char* pargs)
{
    char* slot_id;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    return tapi_data_load_apn_contexts(context,
        atoi(slot_id), EVENT_APN_LOADED_DONE, data_event_response);
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
    tapi_data_save_apn_context(context,
        atoi(slot_id), EVENT_APN_SAVE_DONE, apn, data_event_response);
    free(apn);

    return 0;
}

static int telephonytool_cmd_remove_apn(tapi_context context, char* pargs)
{
    char* slot_id;
    char* target_state;
    tapi_apn_context* apn;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &target_state);
    if (slot_id == NULL)
        return -EINVAL;

    while (*target_state == ' ')
        target_state++;

    if (target_state == NULL)
        return -EINVAL;

    apn = malloc(sizeof(tapi_apn_context));
    if (apn == NULL)
        return -EINVAL;
    apn->id = target_state;

    tapi_data_remove_apn_context(context,
        atoi(slot_id), EVENT_APN_REMOVAL_DONE, apn, data_event_response);
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

    return tapi_data_reset_apn_contexts(context,
        atoi(slot_id), EVENT_APN_RESTORE_DONE, data_event_response);
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
    syslog(LOG_DEBUG, "%s : %d \n", __func__, result);

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
    syslog(LOG_DEBUG, "%s : %d \n", __func__, result);

    return 0;
}

static int telephonytool_cmd_get_ps_attached(tapi_context context, char* pargs)
{
    char* slot_id;
    bool result;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_data_is_ps_attached(context, atoi(slot_id), &result);
    syslog(LOG_DEBUG, "%s, slotId : %s result : %d \n", __func__, slot_id, result);

    return 0;
}

static int telephonytool_cmd_get_ps_network_type(tapi_context context, char* pargs)
{
    char* slot_id;
    tapi_network_type result;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_data_get_network_type(context, atoi(slot_id), &result);
    syslog(LOG_DEBUG, "%s, slotId : %s result : %d \n", __func__, slot_id, result);

    return 0;
}

static int telephonytool_cmd_set_pref_apn(tapi_context context, char* pargs)
{
    char* slot_id;
    char* target_state;
    tapi_apn_context* apn;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &target_state);
    if (slot_id == NULL)
        return -EINVAL;

    while (*target_state == ' ')
        target_state++;

    if (target_state == NULL)
        return -EINVAL;

    apn = malloc(sizeof(tapi_apn_context));
    if (apn == NULL)
        return -EINVAL;
    apn->id = target_state;

    tapi_data_set_preferred_apn(context, atoi(slot_id), apn);
    syslog(LOG_DEBUG, "%s, slotId : %s apn : %s \n", __func__, slot_id, target_state);
    free(apn);

    return 0;
}

static int telephonytool_cmd_get_pref_apn(tapi_context context, char* pargs)
{
    char* slot_id;
    char* apn;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_data_get_preferred_apn(context, atoi(slot_id), &apn);
    syslog(LOG_DEBUG, "%s, slotId : %s apn : %s \n", __func__, slot_id, apn);

    return 0;
}

static int telephonytool_cmd_set_default_data_slot(tapi_context context, char* pargs)
{
    char* value;

    if (!strlen(pargs))
        return -EINVAL;

    value = strtok_r(pargs, " ", NULL);
    if (value == NULL)
        return -EINVAL;

    return tapi_data_set_default_data_slot(context, atoi(value));
}

static int telephonytool_cmd_get_default_data_slot(tapi_context context, char* pargs)
{
    int result;

    tapi_data_get_default_data_slot(context, &result);
    syslog(LOG_DEBUG, "%s : %d \n", __func__, result);

    return 0;
}

static int telephonytool_cmd_set_data_allow(tapi_context context, char* pargs)
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

    return tapi_data_set_data_allow(context, atoi(slot_id),
        EVENT_DATA_ALLOWED_DONE, atoi(target_state), data_event_response);
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

    watch_id = tapi_data_register(context, atoi(slot_id), atoi(target_state), data_signal_change);
    syslog(LOG_DEBUG, "start to watch data event : %d , return watch_id : %d \n",
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

    syslog(LOG_DEBUG, "%s, slotId : %s value : %d \n", __func__, slot_id, value);

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

    syslog(LOG_DEBUG, "%s, slotId : %s iccid : %s \n", __func__, slot_id, iccid);

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

    syslog(LOG_DEBUG, "%s, slotId : %s operator : %s \n", __func__, slot_id, operator);

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

    syslog(LOG_DEBUG, "%s, slotId : %s spn : %s \n", __func__, slot_id, spn);

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

    syslog(LOG_DEBUG, "%s, slot_id: %s pin_type: %s old_pin: %s new_pin: %s \n", __func__, slot_id, pin_type, old_pin, new_pin);

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

    syslog(LOG_DEBUG, "%s, slot_id: %s pin_type: %s pin: %s \n", __func__,
        slot_id, pin_type, pin);

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

    syslog(LOG_DEBUG, "%s, slot_id: %s puk_type: %s puk: %s new_pin: %s \n", __func__,
        slot_id, puk_type, puk, new_pin);

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

    syslog(LOG_DEBUG, "%s, slot_id: %s pin_type: %s pin: %s \n", __func__, slot_id, pin_type, pin);

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

    syslog(LOG_DEBUG, "%s, slot_id: %s pin_type: %s pin: %s \n", __func__, slot_id, pin_type, pin);

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

    syslog(LOG_DEBUG, "%s, slotId : %s \n", __func__, slot_id);

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

    syslog(LOG_DEBUG, "%s, watch_id : %s \n", __func__, watch_id);

    tapi_sim_unregister_sim_state_change(context, atoi(watch_id));

    return 0;
}

static int telephonytool_tapi_sms_send_message(tapi_context context, char* pargs)
{
    char* slot_id;
    char* to;
    char* temp;
    char* text;
    int ret;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &temp);
    if (slot_id == NULL)
        return -EINVAL;

    while (*temp == ' ')
        temp++;

    to = strtok_r(temp, " ", &text);
    if (to == NULL)
        return -EINVAL;

    while (*text == ' ')
        text++;

    if (text == NULL)
        return -EINVAL;

    syslog(LOG_DEBUG, "%s, slotId : %s  number : %s text: %s \n", __func__, slot_id, to, text);
    ret = tapi_sms_send_message(context, atoi(slot_id), to, text);
    return ret;
}

static int telephonytool_tapi_sms_send_data_message(tapi_context context, char* pargs)
{
    char* slot_id;
    char* to;
    char* temp;
    char* text;
    int ret;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &temp);
    if (slot_id == NULL)
        return -EINVAL;

    while (*temp == ' ')
        temp++;

    to = strtok_r(temp, " ", &text);
    if (to == NULL)
        return -EINVAL;

    while (*text == ' ')
        text++;

    if (text == NULL)
        return -EINVAL;

    syslog(LOG_DEBUG, "%s, slotId : %s  number : %s text: %s port 0 \n",
        __func__, slot_id, to, text);
    ret = tapi_sms_send_data_message(context, atoi(slot_id), to, 0, text);
    return ret;
}

static int telephonytool_tapi_sms_get_service_center_number(tapi_context context, char* pargs)
{
    char* slot_id;
    char* smsc_addr;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = pargs;
    if (slot_id == NULL)
        return -EINVAL;

    tapi_sms_get_service_center_address(context, atoi(slot_id), &smsc_addr);
    syslog(LOG_DEBUG, "%s, slotId : %s  smsc_addr: %s \n", __func__, slot_id, smsc_addr);

    return 0;
}

static int telephonytool_tapi_sms_set_service_center_number(tapi_context context, char* pargs)
{
    char* slot_id;
    char* smsc_addr;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &smsc_addr);
    if (slot_id == NULL)
        return -EINVAL;

    while (*smsc_addr == ' ')
        smsc_addr++;

    if (smsc_addr == NULL)
        return -EINVAL;

    syslog(LOG_DEBUG, "%s, slotId : %s smsc_addr: %s \n", __func__, slot_id, smsc_addr);
    tapi_sms_set_service_center_address(context, atoi(slot_id), smsc_addr);

    return 0;
}

static int telephonytool_tapi_sms_register(tapi_context context, char* pargs)
{
    char* slot_id;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = pargs;
    syslog(LOG_DEBUG, "telephonytool_tapi_sms_register slotId : %s\n", slot_id);
    if (slot_id == NULL)
        return -EINVAL;

    syslog(LOG_DEBUG, "%s, slotId : %s \n", __func__, slot_id);
    tapi_sms_register(context, atoi(slot_id), MSG_INCOMING_MESSAGE_IND, tele_sms_async_fun);
    tapi_sms_register(context, atoi(slot_id), MSG_IMMEDIATE_MESSAGE_IND, tele_sms_async_fun);

    return 0;
}

static int telephonytool_tapi_sms_get_cell_broadcast_power(tapi_context context, char* pargs)
{
    char* slot_id;
    bool state;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = pargs;
    if (slot_id == NULL)
        return -EINVAL;

    tapi_sms_get_cell_broadcast_power_on(context, atoi(slot_id), &state);
    syslog(LOG_DEBUG, "%s, slotId : %s state: %d \n", __func__, slot_id, state);

    return 0;
}

static int telephonytool_tapi_sms_set_cell_broadcast_power(tapi_context context, char* pargs)
{
    char* slot_id;
    char* state;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &state);
    if (slot_id == NULL)
        return -EINVAL;

    while (*state == ' ')
        state++;

    if (state == NULL)
        return -EINVAL;

    syslog(LOG_DEBUG, "%s, slotId : %s state: %s \n", __func__, slot_id, state);
    tapi_sms_set_cell_broadcast_power_on(context, atoi(slot_id), atoi(state));

    return 0;
}

static int telephonytool_tapi_sms_get_cell_broadcast_topics(tapi_context context, char* pargs)
{
    char* slot_id;
    char* cbs_topics;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = pargs;
    if (slot_id == NULL)
        return -EINVAL;

    tapi_sms_get_cell_broadcast_topics(context, atoi(slot_id), &cbs_topics);
    syslog(LOG_DEBUG, "%s, slotId : %s  cbs_topics: %s \n", __func__, slot_id, cbs_topics);

    return 0;
}

static int telephonytool_tapi_sms_set_cell_broadcast_topics(tapi_context context, char* pargs)
{
    char* slot_id;
    char* cbs_topics;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &cbs_topics);
    if (slot_id == NULL)
        return -EINVAL;

    while (*cbs_topics == ' ')
        cbs_topics++;

    if (cbs_topics == NULL)
        return -EINVAL;

    syslog(LOG_DEBUG, "%s, slotId : %s cbs_topics: %s \n", __func__, slot_id, cbs_topics);
    tapi_sms_set_cell_broadcast_topics(context, atoi(slot_id), cbs_topics);

    return 0;
}

static int telephonytool_tapi_cbs_register(tapi_context context, char* pargs)
{
    char* slot_id;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = pargs;
    if (slot_id == NULL)
        return -EINVAL;

    syslog(LOG_DEBUG, "%s, slotId : %s \n", __func__, slot_id);
    tapi_cbs_register(context, atoi(slot_id), MSG_INCOMING_CBS_IND, tele_cbs_async_fun);
    tapi_cbs_register(context, atoi(slot_id), MSG_EMERGENCY_CBS_IND, tele_cbs_async_fun);

    return 0;
}

static int telephonytool_tapi_sms_copy_message_to_sim(tapi_context context, char* pargs)
{
    char* slot_id;
    char* to;
    char* temp;
    char* text;
    int ret;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &temp);
    if (slot_id == NULL)
        return -EINVAL;

    while (*temp == ' ')
        temp++;

    to = strtok_r(temp, " ", &text);
    if (to == NULL)
        return -EINVAL;

    while (*text == ' ')
        text++;

    if (text == NULL)
        return -EINVAL;

    syslog(LOG_DEBUG, "%s, slotId : %s  number : %s text: %s port 0 \n", __func__,
        slot_id, to, text);
    ret = tapi_sms_copy_message_to_sim(context, atoi(slot_id), to, text, "221020081012", 0);
    return ret;
}

static int telephonytool_tapi_sms_delete_message_from_sim(tapi_context context, char* pargs)
{
    char* slot_id;
    char* index;

    if (!strlen(pargs))
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &index);
    if (slot_id == NULL)
        return -EINVAL;

    while (*index == ' ')
        index++;

    if (index == NULL)
        return -EINVAL;

    tapi_sms_delete_message_from_sim(context, atoi(slot_id), atoi(index));
    syslog(LOG_DEBUG, "%s, slotId : %s index: %s \n", __func__, slot_id, index);

    return 0;
}

static int telephonytool_cmd_network_listen(tapi_context context, char* pargs)
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

    watch_id = tapi_network_register(context,
        atoi(slot_id), atoi(target_state), network_signal_change);
    syslog(LOG_DEBUG, "start to watch network event : %d , return watch_id : %d \n",
        atoi(target_state), watch_id);

    return watch_id;
}

static int telephonytool_cmd_network_unlisten(tapi_context context, char* pargs)
{
    char* watch_id;
    int ret;

    if (strlen(pargs) == 0)
        return -EINVAL;

    watch_id = strtok_r(pargs, " ", NULL);
    if (watch_id == NULL)
        return -EINVAL;

    ret = tapi_network_unregister(context, atoi(watch_id));
    syslog(LOG_DEBUG, "stop to watch network event : %s , with watch_id : \
        %s with return value : %d \n",
        watch_id, watch_id, ret);

    return ret;
}

static int telephonytool_cmd_network_select_auto(tapi_context context, char* pargs)
{
    char* slot_id;
    int ret = -1;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    ret = tapi_network_select_auto(context,
        atoi(slot_id), EVENT_REGISTER_AUTO_DONE, network_event_callback);
    syslog(LOG_DEBUG, "%s, slotId : %s value :%d \n", __func__, slot_id, ret);

    return 0;
}

static int telephonytool_cmd_network_select_manual(tapi_context context, char* pargs)
{
    char* slot_id;
    char* mcc;
    char* mnc;
    char* tech;
    char *temp, *temp2;
    tapi_operator_info* network_info;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &temp);
    if (slot_id == NULL)
        return -EINVAL;

    while (*temp == ' ')
        temp++;

    mcc = strtok_r(temp, " ", &temp2);
    if (mcc == NULL)
        return -EINVAL;

    while (*temp2 == ' ')
        temp2++;

    mnc = strtok_r(temp2, " ", &tech);
    if (mnc == NULL)
        return -EINVAL;

    while (*tech == ' ')
        tech++;

    if (tech == NULL)
        return -EINVAL;

    network_info = malloc(sizeof(tapi_operator_info));
    if (network_info == NULL) {
        return -ENOMEM;
    }

    snprintf(network_info->mcc, sizeof(network_info->mcc), "%s", mcc);
    snprintf(network_info->mnc, sizeof(network_info->mnc), "%s", mnc);
    snprintf(network_info->technology, sizeof(network_info->technology), "%s", tech);

    tapi_network_select_manual(context,
        atoi(slot_id), EVENT_REGISTER_MANUAL_DONE, network_info, network_event_callback);

    free(network_info);

    return 0;
}

static int telephonytool_cmd_query_signalstrength(tapi_context context, char* pargs)
{
    char* slot_id;
    tapi_signal_strength ss;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_network_get_signalstrength(context, atoi(slot_id), &ss);
    syslog(LOG_DEBUG, "%s, slotId : %s RSRP value :%d \n", __func__, slot_id, ss.rsrp);

    return 0;
}

static int telephonytool_cmd_get_operator_name(tapi_context context, char* pargs)
{
    char* slot_id;
    char* operator;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_network_get_display_name(context, atoi(slot_id), &operator);
    syslog(LOG_DEBUG, "%s, slotId : %s value :%s \n", __func__, slot_id, operator);

    return 0;
}

static int telephonytool_cmd_get_net_registration_info(tapi_context context, char* pargs)
{
    char* slot_id;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_network_get_registration_info(context, atoi(slot_id),
        EVENT_QUERY_REGISTRATION_INFO_DONE, network_event_callback);

    return 0;
}

static int telephonytool_cmd_get_voice_networktype(tapi_context context, char* pargs)
{
    char* slot_id;
    tapi_network_type type;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_network_get_voice_network_type(context, atoi(slot_id), &type);
    syslog(LOG_DEBUG, "%s, slotId : %s value :%d \n", __func__, slot_id, type);

    return 0;
}

static int telephonytool_cmd_is_voice_roaming(tapi_context context, char* pargs)
{
    bool value;
    char* slot_id;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_network_is_voice_roaming(context, atoi(slot_id), &value);
    syslog(LOG_DEBUG, "%s, slotId : %s value :%d \n", __func__, slot_id, value);

    return 0;
}

static int telephonytool_cmd_network_scan(tapi_context context, char* pargs)
{
    char* slot_id;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    return tapi_network_scan(context,
        atoi(slot_id), EVENT_NETWORK_SCAN_DONE, network_event_callback);
}

static int telephonytool_cmd_get_serving_cellinfos(tapi_context context, char* pargs)
{
    char* slot_id;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    return tapi_network_get_serving_cellinfos(context,
        atoi(slot_id), EVENT_QUERY_SERVING_CELL_DONE, network_event_callback);
}

static int telephonytool_cmd_get_neighbouring_cellInfos(tapi_context context, char* pargs)
{
    char* slot_id;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    return tapi_network_get_neighbouring_cellinfos(context,
        atoi(slot_id), EVENT_QUERY_NEIGHBOURING_CELL_DONE, network_event_callback);
}

static int telephonytool_cmd_set_call_barring(tapi_context context, char* pargs)
{
    char* slot_id;
    char* fac;
    char* temp;
    char* pin2;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &temp);
    if (slot_id == NULL)
        return -EINVAL;

    while (*temp == ' ')
        temp++;

    fac = strtok_r(temp, " ", &pin2);
    if (fac == NULL)
        return -EINVAL;

    while (*pin2 == ' ')
        pin2++;

    if (pin2 == NULL)
        return -EINVAL;

    tapi_ss_request_call_barring(context, atoi(slot_id), EVENT_REQUEST_CALL_BARRING_DONE, fac, pin2, tele_call_async_fun);
    syslog(LOG_DEBUG, "%s, slot_id : %s fc : %s  pin2 : %s \n", __func__, slot_id, fac, pin2);

    return 0;
}

static int telephonytool_cmd_get_call_barring(tapi_context context, char* pargs)
{
    char* slot_id;
    char* key;
    char* cb_info;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &key);
    if (slot_id == NULL)
        return -EINVAL;

    while (*key == ' ')
        key++;

    if (key == NULL)
        return -EINVAL;

    tapi_ss_query_call_barring_info(context, atoi(slot_id), key, &cb_info);
    syslog(LOG_DEBUG, "%s, slotId : %s key : %s cb_info : %s \n", __func__, slot_id, key, cb_info);

    return 0;
}

static int telephonytool_cmd_change_call_barring_passwd(tapi_context context, char* pargs)
{
    char* slot_id;
    char* temp;
    char* old_passwd;
    char* new_passwd;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &temp);
    if (slot_id == NULL)
        return -EINVAL;

    while (*temp == ' ')
        temp++;

    old_passwd = strtok_r(temp, " ", &new_passwd);
    if (old_passwd == NULL)
        return -EINVAL;

    while (*new_passwd == ' ')
        new_passwd++;

    if (new_passwd == NULL)
        return -EINVAL;

    tapi_ss_change_call_barring_password(context, atoi(slot_id),
        EVENT_CALL_BARRING_PASSWD_CHANGE_DONE, old_passwd, old_passwd, tele_call_async_fun);
    syslog(LOG_DEBUG, "%s, slotId : %s old_passwd : %s new_passwd : %s \n", __func__,
        slot_id, old_passwd, new_passwd);

    return 0;
}

static int telephonytool_cmd_disable_all_call_barrings(tapi_context context, char* pargs)
{
    char* slot_id;
    char* passwd;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &passwd);
    if (slot_id == NULL)
        return -EINVAL;

    while (*passwd == ' ')
        passwd++;

    if (passwd == NULL)
        return -EINVAL;

    tapi_ss_disable_all_call_barrings(context, atoi(slot_id),
        EVENT_DISABLE_ALL_CALL_BARRINGS_DONE, passwd, tele_call_async_fun);
    syslog(LOG_DEBUG, "%s, slotId : %s passwd : %s \n", __func__, slot_id, passwd);

    return 0;
}

static int telephonytool_cmd_disable_all_incoming(tapi_context context, char* pargs)
{
    char* slot_id;
    char* passwd;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &passwd);
    if (slot_id == NULL)
        return -EINVAL;

    while (*passwd == ' ')
        passwd++;

    if (passwd == NULL)
        return -EINVAL;

    tapi_ss_disable_all_incoming(context, atoi(slot_id),
        EVENT_DISABLE_ALL_INCOMING_DONE, passwd, tele_call_async_fun);
    syslog(LOG_DEBUG, "%s, slotId : %s passwd : %s \n", __func__, slot_id, passwd);

    return 0;
}

static int telephonytool_cmd_disable_all_outgoing(tapi_context context, char* pargs)
{
    char* slot_id;
    char* passwd;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &passwd);
    if (slot_id == NULL)
        return -EINVAL;

    while (*passwd == ' ')
        passwd++;

    if (passwd == NULL)
        return -EINVAL;

    tapi_ss_disable_all_outgoing(context, atoi(slot_id),
        EVENT_DISABLE_ALL_OUTGOING_DONE, passwd, tele_call_async_fun);
    syslog(LOG_DEBUG, "%s, slotId : %s passwd : %s \n", __func__, slot_id, passwd);

    return 0;
}

static int telephonytool_cmd_set_call_forwarding(tapi_context context, char* pargs)
{
    char* slot_id;
    char* temp;
    char* cf_type;
    char* value;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &temp);
    if (slot_id == NULL)
        return -EINVAL;

    while (*temp == ' ')
        temp++;

    cf_type = strtok_r(temp, " ", &value);
    if (cf_type == NULL)
        return -EINVAL;

    while (*value == ' ')
        value++;

    if (value == NULL)
        return -EINVAL;

    tapi_ss_request_call_forwarding(context, atoi(slot_id),
        EVENT_REQUEST_CALL_FORWARDING_DONE, cf_type, value, tele_call_async_fun);
    syslog(LOG_DEBUG, "%s, slot_id : %s cf_type : %s  value : %s \n", __func__,
        slot_id, cf_type, value);

    return 0;
}

static int telephonytool_cmd_get_call_forwarding(tapi_context context, char* pargs)
{
    char* slot_id;
    char* key;
    char* cf_info;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &key);
    if (slot_id == NULL)
        return -EINVAL;

    while (*key == ' ')
        key++;

    if (key == NULL)
        return -EINVAL;

    tapi_ss_query_call_forwarding_info(context, atoi(slot_id), key, &cf_info);
    syslog(LOG_DEBUG, "%s, slotId : %s key : %s cf_info : %s \n", __func__, slot_id, key, cf_info);

    return 0;
}

static int telephonytool_cmd_disable_call_forwarding(tapi_context context, char* pargs)
{
    char* slot_id;
    char* type;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &type);
    if (slot_id == NULL)
        return -EINVAL;

    while (*type == ' ')
        type++;

    if (type == NULL)
        return -EINVAL;

    tapi_ss_disable_call_forwarding(context, atoi(slot_id),
        EVENT_DISABLE_CALL_FORWARDING_DONE, type, tele_call_async_fun);
    syslog(LOG_DEBUG, "%s, slotId : %s type : %s \n", __func__, slot_id, type);

    return 0;
}

static int telephonytool_cmd_initiate_ss_service(tapi_context context, char* pargs)
{
    char* slot_id;
    char* ss_control_string;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &ss_control_string);
    if (slot_id == NULL)
        return -EINVAL;

    while (*ss_control_string == ' ')
        ss_control_string++;

    if (ss_control_string == NULL)
        return -EINVAL;

    tapi_ss_initiate_service(context, atoi(slot_id),
        EVENT_INITIATE_SERVICE_DONE, ss_control_string, tele_call_async_fun);
    syslog(LOG_DEBUG, "%s, slotId : %s ss_control_string : %s \n", __func__, slot_id, ss_control_string);

    return 0;
}

static int telephonytool_cmd_get_ussd_state(tapi_context context, char* pargs)
{
    char* slot_id;
    tapi_ussd_state ussd_state;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_get_ussd_state(context, atoi(slot_id), &ussd_state);
    syslog(LOG_DEBUG, "%s, slotId : %s ussd_state : %d \n", __func__, slot_id, ussd_state);

    return 0;
}

static int telephonytool_cmd_send_ussd(tapi_context context, char* pargs)
{
    char* slot_id;
    char* response_msg;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &response_msg);
    if (slot_id == NULL)
        return -EINVAL;

    while (*response_msg == ' ')
        response_msg++;

    if (response_msg == NULL)
        return -EINVAL;

    tapi_ss_send_ussd(context, atoi(slot_id),
        EVENT_SEND_USSD_DONE, response_msg, tele_call_async_fun);
    syslog(LOG_DEBUG, "%s, slotId : %s response_msg : %s \n", __func__, slot_id, response_msg);

    return 0;
}

static int telephonytool_cmd_cancel_ussd(tapi_context context, char* pargs)
{
    char* slot_id;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    return tapi_ss_cancel_ussd(context, atoi(slot_id), EVENT_CANCEL_USSD_DONE, tele_call_async_fun);
}

static int telephonytool_cmd_set_call_waiting(tapi_context context, char* pargs)
{
    char* slot_id;
    char* value;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &value);
    if (slot_id == NULL)
        return -EINVAL;

    while (*value == ' ')
        value++;

    if (value == NULL)
        return -EINVAL;

    tapi_ss_request_call_wating(context, atoi(slot_id),
        EVENT_REQUEST_CALL_WAITING_DONE, value, tele_call_async_fun);
    syslog(LOG_DEBUG, "%s, slot_id : %s value : %s \n", __func__, slot_id, value);

    return 0;
}

static int telephonytool_cmd_get_call_waiting(tapi_context context, char* pargs)
{
    char* slot_id;
    char* cw_info;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_ss_query_call_wating(context, atoi(slot_id), &cw_info);
    syslog(LOG_DEBUG, "%s, slotId : %s cw_info : %s \n", __func__, slot_id, cw_info);

    return 0;
}

static int telephonytool_cmd_get_clip(tapi_context context, char* pargs)
{
    char* slot_id;
    tapi_clip_status clip_status;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_ss_query_calling_line_presentation_info(context, atoi(slot_id), &clip_status);
    syslog(LOG_DEBUG, "%s, slotId : %s clip_status : %d \n", __func__, slot_id, clip_status);

    return 0;
}

static int telephonytool_cmd_get_clir(tapi_context context, char* pargs)
{
    char* slot_id;
    tapi_clir_status clir_status;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    tapi_ss_query_calling_line_restriction_info(context, atoi(slot_id), &clir_status);
    syslog(LOG_DEBUG, "%s, slotId : %s clir_status : %d \n", __func__, slot_id, clir_status);

    return 0;
}

static int telephonytool_cmd_enable_fdn(tapi_context context, char* pargs)
{
    char* slot_id;
    char* enable_fdn;
    char* passwd;
    char* temp;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", &temp);
    if (slot_id == NULL)
        return -EINVAL;

    while (*temp == ' ')
        temp++;

    enable_fdn = strtok_r(temp, " ", &passwd);
    if (enable_fdn == NULL)
        return -EINVAL;

    while (*passwd == ' ')
        passwd++;

    if (passwd == NULL)
        return -EINVAL;

    tapi_ss_enable_fdn(context, atoi(slot_id), EVENT_ENABLE_FDN_DONE,
        (bool)atoi(enable_fdn), passwd, tele_call_async_fun);
    syslog(LOG_DEBUG, "%s, slotId : %s enable_fdn : %s passwd : %s \n",
        __func__, slot_id, enable_fdn, passwd);

    return 0;
}

static int telephonytool_cmd_query_fdn(tapi_context context, char* pargs)
{
    char* slot_id;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    return tapi_ss_query_fdn(context, atoi(slot_id), EVENT_QUERY_FDN_DONE, tele_call_async_fun);
}

static int telephonytool_cmd_ss_register(tapi_context context, char* pargs)
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

    return tapi_ss_register(context, atoi(slot_id), atoi(target_state), tele_call_async_fun);
}

static int telephonytool_cmd_ims_enable(tapi_context context, char* pargs)
{
    char dst[2][CONFIG_NSH_LINELEN];
    int cnt = split_input(dst, 2, pargs, " ");
    int slot_id, action_type;
    int ret = -EINVAL;

    if (cnt != 2)
        return -EINVAL;

    slot_id = atoi(dst[0]);
    action_type = atoi(dst[1]);

    syslog(LOG_DEBUG, "%s: slot_id: %d, action: %d\n", __func__, slot_id, action_type);

    if (action_type == 0) {
        ret = tapi_ims_turn_off(context, slot_id);
    } else if (action_type == 1) {
        ret = tapi_ims_turn_on(context, slot_id);
    }

    return ret;
}

static int telephonytool_cmd_set_ims_service(tapi_context context, char* pargs)
{
    char dst[2][CONFIG_NSH_LINELEN];
    int cnt = split_input(dst, 2, pargs, " ");
    int slot_id, service_type;

    if (cnt != 2)
        return -EINVAL;

    slot_id = atoi(dst[0]);
    service_type = atoi(dst[1]);

    syslog(LOG_DEBUG, "%s: slot_id: %d, action: %d\n", __func__, slot_id, service_type);

    return tapi_ims_set_service_status(context, slot_id, service_type);
}

static int telephonytool_cmd_ims_register(tapi_context context, char* pargs)
{
    char dst[1][CONFIG_NSH_LINELEN];
    int cnt = split_input(dst, 1, pargs, " ");
    int watch_id, slot_id;

    if (cnt != 1)
        return -EINVAL;

    slot_id = atoi(dst[0]);

    watch_id = tapi_ims_register_registration_change(context, slot_id, tele_ims_async_fun);
    syslog(LOG_DEBUG, "%s: slot_id: %d, watch_id: %d\n", __func__, slot_id, watch_id);

    return watch_id;
}
static int telephonytool_cmd_ims_get_registration(tapi_context context, char* pargs)
{
    char dst[2][CONFIG_NSH_LINELEN];
    int cnt = split_input(dst, 2, pargs, " ");
    tapi_ims_registration_info info;
    int slot_id, ret, action_type;

    if (cnt != 2)
        return -EINVAL;

    slot_id = atoi(dst[0]);
    action_type = atoi(dst[1]);

    syslog(LOG_DEBUG, "%s: slot_id: %d, acton: %d\n", __func__, slot_id, action_type);

    if (action_type == 0) {
        ret = tapi_ims_get_registration(context, slot_id, &info);
        if (ret == OK) {
            syslog(LOG_DEBUG, "%s: ret_info: %d, ext_info: %d\n", __func__,
                info.reg_info, info.ext_info);
        }
    } else if (action_type == 1) {
        ret = tapi_ims_is_registered(context, slot_id);
        if (ret == OK) {
            syslog(LOG_DEBUG, "%s: ims_registered: %d\n", __func__, ret);
        }
    } else if (action_type == 2) {
        ret = tapi_ims_is_volte_available(context, slot_id);
        if (ret == OK) {
            syslog(LOG_DEBUG, "%s: ims_volte_enabled: %d\n", __func__, ret);
        }
    }

    return ret;
}

static int telephonytool_cmd_load_adn_entries(tapi_context context, char* pargs)
{
    char* slot_id;

    if (strlen(pargs) == 0)
        return -EINVAL;

    slot_id = strtok_r(pargs, " ", NULL);
    if (slot_id == NULL)
        return -EINVAL;

    return tapi_phonebook_load_adn_entries(context, atoi(slot_id),
        EVENT_LOAD_ADN_ENTRIES_DONE, tele_call_async_fun);
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
            if (g_telephonytool_cmds[i].pfunc(context, arg) < 0)
                printf("cmd:%s input parameter:%s invalid \n", cmd, arg);

            return;
        }
    }

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
 * Private Data
 ****************************************************************************/

static struct telephonytool_cmd_s g_telephonytool_cmds[] = {

    /* Radio Command */
    { "list-modem",
        telephonytool_cmd_query_modem_list,
        "list available modem list (enter example : list-modem" },
    { "listen-modem", telephonytool_cmd_modem_register,
        "modem event callback (enter example : listen-modem 0 0 \
        [slot_id][event_id, 0:radio_state_change 1:call_added 2:call_removed])" },
    { "unlisten-modem",
        telephonytool_cmd_modem_unregister,
        "modem event callback (enter example : unlisten-modem 0 \
        [watch_id, one uint value returned from \"listen\"])" },
    { "get-radio-cap",
        telephonytool_cmd_is_feature_supported,
        "query modem capability (enter example : get-radio-cap 0 \
        [feature_type, 0: voice 1: data 2: sms 3: ims])" },
    { "set-radio-power",
        telephonytool_cmd_set_radio_power,
        "set radio power (enter example : set-radio-power 0 1 \
        [slot_id][state, 0:radio off 1:radio on])" },
    { "get-radio-power",
        telephonytool_cmd_get_radio_power,
        "get radio power (enter example : get-radio-power 0 [slot_id])" },
    { "set-rat-mode",
        telephonytool_cmd_set_rat_mode,
        "set rat mode (enter example : set-rat-set 0 9 [slot_id] \
        [mode: 0-any 1-gsm_only 2-wcdma_only 3-umts 9-lte_gsm_wcdma 12-lte_wcdma 14-lte_only])" },
    { "get-rat-mode",
        telephonytool_cmd_get_rat_mode,
        "get rat mode (enter example : get-rat-mode 0 [slot_id])" },
    { "get-imei",
        telephonytool_cmd_get_imei,
        "get imei (enter example : get-imei 0 [slot_id])" },
    { "get-imeisv",
        telephonytool_cmd_get_imeisv,
        "get imeisv (enter example : get-imeisv 0 [slot_id])" },
    { "get-phone-state",
        telephonytool_cmd_get_phone_state,
        "get phone state (enter example : get-phone-state 0 [slot_id])" },
    { "send-modem-power",
        telephonytool_cmd_send_modem_power,
        "Power on or off modem (enter example : send-modem-power 0 1 [slot_id][on])" },
    { "get-radio-state",
        telephonytool_cmd_get_radio_state,
        "get radio state (enter example : get-radio-state 0 [slot_id])" },
    { "get-modem-manufacturer",
        telephonytool_cmd_get_modem_manufacturer,
        "get modem manufacturer (enter example : get-modem-manufacturer 0 [slot_id])" },
    { "get-modem-model",
        telephonytool_cmd_get_modem_model,
        "get modem_model (enter example : get-modem-model 0 [slot_id])" },
    { "get-modem-revision",
        telephonytool_cmd_get_modem_revision,
        "get modem_revision (enter example : get-modem-revision 0 [slot_id])" },
    { "get-msisdn",
        telephonytool_cmd_get_line_number,
        "query line number (enter example : get-msisdn 0 [slot_id])" },

    /* Call Command */
    { "dial",
        telephonytool_cmd_dial,
        "Dial (enter example : dial 0 10086 0 [slot_id][number][hide_call_id, 0:show 1:hide])" },
    { "answer",
        telephonytool_cmd_answer_call,
        "Answer (enter example : answer 0 0 [slot_id][action:0-answer 1-realse&answer][call_id])" },
    { "swap",
        telephonytool_cmd_swap_call,
        "call Swap (enter example : swap 0 1 [slot_id][action:1-hold 0-unhold])" },
    { "listen-call",
        telephonytool_cmd_listen_call_manager_change,
        "call manger event callback (enter example : listen-call 0" },
    { "unlisten-call",
        telephonytool_cmd_unlisten_call_singal,
        "call unlisten event callback (enter example : unlisten-call [watch_id]" },
    { "call-proxy",
        telephonytool_cmd_call_proxy,
        "new/release call proxy (enter example : \
        call-proxy [slot_id] [action:0-new 1-release] [call_id]" },
    { "listen-call-info",
        telephonytool_cmd_listen_call_info_change,
        "call info event callback (enter example : \
        listen-call-info [slot_id][call_id]" },
    { "hangup-all",
        telephonytool_cmd_hangup_all,
        "hangup all call (enter example : hangup-all 0 [slot_id])" },
    { "hangup",
        telephonytool_cmd_hangup_call,
        "hangup (enter example : hangup 0 [call_id] /phonesim/voicecall01)" },
    { "release-swap",
        telephonytool_cmd_release_and_swap,
        "release and swap (enter example : release-swap 0 [slot_id])" },
    { "get-call",
        telephonytool_cmd_get_call,
        "get call list/call info (enter example : get-call 0 \
        [slot_id][call_id])" },
    { "transfer",
        telephonytool_cmd_transfer_call,
        "call transfer  (enter example : transfer 0 [slot_id])" },
    { "merge",
        telephonytool_cmd_merge_call,
        "call merge  (enter example : merge 0 [slot_id])" },
    { "separate",
        telephonytool_cmd_separate_call,
        "call separate  (enter example : separate 0 [slot_id][call_id: /phonesim/voicecall01])" },
    { "get-ecclist",
        telephonytool_cmd_get_ecc_list,
        "get ecc list  (enter example : get-ecclist 0 [slot_id])" },
    { "is-ecc",
        telephonytool_cmd_is_emergency_number,
        "is emergency number  (enter example : is-ecc 110 [number])" },

    /* Data Command */
    { "load-apns",
        telephonytool_cmd_load_apns,
        "load apn settings (enter example : load-apns 0 [slot_id])" },
    { "save-apn",
        telephonytool_cmd_save_apn,
        "save apn (enter example : save-apn 0 1 [slot_id][apn_type])" },
    { "remove-apn",
        telephonytool_cmd_remove_apn,
        "remove apn (enter example : remove-apn 0 1 [slot_id][id])" },
    { "reset-apn",
        telephonytool_cmd_reset_apn,
        "reset apn (enter example : reset-apn 0 [slot_id])" },
    { "request-network",
        telephonytool_cmd_request_network,
        "request network (enter example : request-network 0 internet [slot_id][apn_type_string])" },
    { "release-network",
        telephonytool_cmd_release_network,
        "release network (enter example : release-network 0 internet [slot_id][apn_type_string])" },
    { "set-data-roaming",
        telephonytool_cmd_set_data_roaming,
        "set data roaming (enter example : set-data-roaming 0 1 [slot_id][state])" },
    { "get-data-roaming",
        telephonytool_cmd_get_data_roaming,
        "get data roaming (enter example : get-data-roaming 0 [slot_id])" },
    { "set-data-on",
        telephonytool_cmd_set_data_enabled,
        "set data enabled (enter example : set-data-on 0 1 [slot_id][state])" },
    { "is-data-on",
        telephonytool_cmd_get_data_enabled,
        "get data enabled (enter example : is-data-on 0 [slot_id])" },
    { "listen-data",
        telephonytool_cmd_data_register,
        "listen data event (enter example : listen-data 0 1 [slot_id][event_id])" },
    { "is-ps-attached",
        telephonytool_cmd_get_ps_attached,
        "checki if ps attached (enter example : is-ps-attached 0 [slot_id])" },
    { "get-ps-nwtype",
        telephonytool_cmd_get_ps_network_type,
        "get ps network type (enter example : get-ps-nwtype 0 [slot_id])" },
    { "set-pref-apn",
        telephonytool_cmd_set_pref_apn,
        "set preferred apn (enter example : set-pref-apn[slot_id][apn_id])" },
    { "get-pref-apn",
        telephonytool_cmd_get_pref_apn,
        "get preferred apn (enter example : get-pref-apn[slot_id][apn_id])" },
    { "set-data-slot",
        telephonytool_cmd_set_default_data_slot,
        "set default data slot (enter example : set-data-slot 0 [slot_id])" },
    { "get-data-slot",
        telephonytool_cmd_get_default_data_slot,
        "get default data slot (enter example : get-data-slot)" },
    { "set-data-allow",
        telephonytool_cmd_set_data_allow,
        "allow data in specific slot for multsim devices (enter example : set-data-allow 0 1)" },

    /* SIM Command */
    { "has-icc",
        telephonytool_cmd_has_icc_card,
        "has icc card (enter example : has-icc 0 [slot_id])" },
    { "get-iccid",
        telephonytool_cmd_get_sim_iccid,
        "get sim iccid (enter example : get-iccid 0 [slot_id])" },
    { "get-sim-operator",
        telephonytool_cmd_get_sim_operator,
        "get sim operator (mcc+mnc) : get-sim-operator 0 [slot_id])" },
    { "get-sim-operator-name",
        telephonytool_cmd_get_sim_operator_name,
        "get sim operator name (enter example : get-sim-operator-name 0 [slot_id])" },
    { "change-pin",
        telephonytool_cmd_change_sim_pin,
        "change old pin to new pin (enter example : \
        change-pin 0 pin 1234 2345 [slot_id][pin_type, pin or pin2][old_pin][new_pin])" },
    { "enter-pin",
        telephonytool_cmd_enter_sim_pin,
        "enter pin to verify (enter example : \
        enter-pin 0 pin 1234 [slot_id][pin_type, pin or pin2][pin])" },
    { "reset-pin",
        telephonytool_cmd_reset_sim_pin,
        "using puk reset pin (enter example : \
        reset-pin 0 puk 12345678 2345 [slot_id][puk_type, puk or puk2][puk][new_pin])" },
    { "lock-pin",
        telephonytool_cmd_lock_sim_pin,
        "active sim lock (enter example : \
        lock-pin 0 pin 1234 [slot_id][pin_type, pin or pin2][pin])" },
    { "unlock-pin",
        telephonytool_cmd_unlock_sim_pin,
        "deactive sim lock (enter example : \
        unlock-pin 0 pin 1234 [slot_id][pin_type, pin or pin2][pin])" },
    { "listen-sim",
        telephonytool_cmd_listen_sim_state_change,
        "register sim state change (enter example : listen-sim 0 [slot_id])" },
    { "unlisten-sim",
        telephonytool_cmd_unlisten_sim_state_change,
        "unregister sim state change (enter example : unlisten-sim 0 [slot_id])" },

    /* Sms & Cbs Command */
    { "send-sms",
        telephonytool_tapi_sms_send_message,
        "send message (enter example : send-sms 0 10086 hello)" },
    { "send-data-sms",
        telephonytool_tapi_sms_send_data_message,
        "send message (enter example : send-data-sms 0 10086 hello)" },
    { "get-service-center-number",
        telephonytool_tapi_sms_get_service_center_number,
        "get service center number ? (enter example : get-service-center-number 0)" },
    { "set-service-center-number",
        telephonytool_tapi_sms_set_service_center_number,
        "set service center number ? (enter example : set-service-center-number 0 10086)" },
    { "register-incoming-sms",
        telephonytool_tapi_sms_register,
        "get incoming sms ? (enter example : register-incoming-sms 0)" },
    { "get-cell-broadcast-power",
        telephonytool_tapi_sms_get_cell_broadcast_power,
        "get cell broadcast power ? (enter example : get-cell-broadcast-power 0)" },
    { "set-cell-broadcast-power",
        telephonytool_tapi_sms_set_cell_broadcast_power,
        "set service center number ? (enter example : set-cell-broadcast-power 0 1)" },
    { "get-cell-broadcast-topics",
        telephonytool_tapi_sms_get_cell_broadcast_topics,
        "get cell broadcast topics ? (enter example : get-cell-broadcast-topics 0)" },
    { "set-cell-broadcast-topics",
        telephonytool_tapi_sms_set_cell_broadcast_topics,
        "set cell broadcast topics ? (enter example : set-cell-broadcast-topics 0 1)" },
    { "register-incoming-cbs",
        telephonytool_tapi_cbs_register,
        "get incoming cbs ? (enter example : register-incoming-cbs 0)" },
    { "copy-sms-to-sim",
        telephonytool_tapi_sms_copy_message_to_sim,
        "send message (enter example : copy-sms-to-sim 0 10086 hello)" },
    { "delete-sms-from-sim",
        telephonytool_tapi_sms_delete_message_from_sim,
        "send message (enter example : delete-sms-from-sim 0 1)" },

    /* Network Command */
    { "listen-network",
        telephonytool_cmd_network_listen,
        "listen-network (enter example : listen-network 0 13 [slot_id][event_id])" },
    { "unlisten-network",
        telephonytool_cmd_network_unlisten,
        "unlisten-network (enter example : unlisten-network 0 [slot_id])" },
    { "register-auto",
        telephonytool_cmd_network_select_auto,
        "register auto (enter example : register-auto 0 [slot_id])" },
    { "register-manual",
        telephonytool_cmd_network_select_manual,
        "register manual (enter example : register-manual 0 [slot_id])" },
    { "get-signalstrength",
        telephonytool_cmd_query_signalstrength,
        "signalstrength (enter example : get-signalstrength 0 [slot_id])" },
    { "get-display-name",
        telephonytool_cmd_get_operator_name,
        "operator-name (enter example : get-display-name 0 [slot_id])" },
    { "get-registration-info",
        telephonytool_cmd_get_net_registration_info,
        "query registration-info (enter example : get-registration-info 0 [slot_id])" },
    { "get-voice-nwtype",
        telephonytool_cmd_get_voice_networktype,
        "query cs network type (enter example : get-voice-nwtype 0 [slot_id])" },
    { "get-voice-roaming",
        telephonytool_cmd_is_voice_roaming,
        "judge voice roaming  (enter example : get-voice-roaming 0 [slot_id])" },
    { "scan-network",
        telephonytool_cmd_network_scan,
        "network-scan  (enter example : scan-network 0 [slot_id])" },
    { "get-serving-cellinfo",
        telephonytool_cmd_get_serving_cellinfos,
        "get serving cellinfo  (enter example : get-serving-cellinfo 0)" },
    { "get-neighbouring-cellInfos",
        telephonytool_cmd_get_neighbouring_cellInfos,
        "get neighbouring cellInfos  (enter example : get-neighbouring-cellInfos 0)" },

    /* Ss Command */
    { "set-callbarring",
        telephonytool_cmd_set_call_barring,
        "set callbarring (enter example : set-callbarring 0 AI 1234 \
        [slot_id][facility][pin2])" },
    { "get-callbarring",
        telephonytool_cmd_get_call_barring,
        "get callbarring (enter example : get-callbarring 0 VoiceIncoming \
        [slot_id][call barring key])" },
    { "change-callbarring-passwd",
        telephonytool_cmd_change_call_barring_passwd,
        "change callbarring passwd (enter example : change-callbarring-passwd 0 1234 2345 \
        [slot_id][old passwd][new passwd])" },
    { "disable-all-callbarrings",
        telephonytool_cmd_disable_all_call_barrings,
        "disable all callbarrings (enter example : disable-all-callbarrings 0 2345 \
        [slot_id][passwd])" },
    { "disable-all-incoming",
        telephonytool_cmd_disable_all_incoming,
        "disable all incoming (enter example : disable-all-incoming 0 2345 \
        [slot_id][passwd])" },
    { "disable-all-outgoing",
        telephonytool_cmd_disable_all_outgoing,
        "disable all outgoing (enter example : disable-all-outgoing 0 2345 \
        [slot_id][passwd])" },
    { "set-callforwarding",
        telephonytool_cmd_set_call_forwarding,
        "set callforwarding (enter example : set-callforwarding 0 VoiceUnconditional 10086 \
        [slot_id][call forwarding type][value])" },
    { "get-callforwarding",
        telephonytool_cmd_get_call_forwarding,
        "get callforwarding (enter example : get-callforwarding 0 VoiceUnconditional \
        [slot_id][call forwarding type])" },
    { "disable-callforwarding",
        telephonytool_cmd_disable_call_forwarding,
        "disable callforwarding (enter example : disable-callforwarding 0 all \
        [slot_id][type])" },
    { "initiate-ss",
        telephonytool_cmd_initiate_ss_service,
        "initiate supplementary service (enter example : initiate-ss 0 *#06# \
        [slot_id][supplementary service code])" },
    { "get-ussd-state",
        telephonytool_cmd_get_ussd_state,
        "get ussd state (enter example : get-ussd-state 0 [slot_id])" },
    { "send-ussd",
        telephonytool_cmd_send_ussd,
        "send ussd (enter example : send-ussd 0 OK [slot_id][response message])" },
    { "cancel-ussd",
        telephonytool_cmd_cancel_ussd,
        "cancel ussd (enter example : cancel-ussd 0 [slot_id])" },
    { "set-callwaiting",
        telephonytool_cmd_set_call_waiting,
        "set callwaiting (enter example : set-callwaiting 0 enabled \
        [slot_id][call waiting value])" },
    { "get-callwaiting",
        telephonytool_cmd_get_call_waiting,
        "get callwaiting (enter example : get-callwaiting 0 [slot_id])" },
    { "get-clip",
        telephonytool_cmd_get_clip,
        "get clip (enter example : get-clip 0 [slot_id])" },
    { "get-clir",
        telephonytool_cmd_get_clir,
        "get clir (enter example : get-clir 0 [slot_id])" },
    { "enable-fdn",
        telephonytool_cmd_enable_fdn,
        "enable fdn (enter example : enable-fdn 0 1 123456 \
        [slot_id][enable 1 or disable 0][pin2])" },
    { "query-fdn",
        telephonytool_cmd_query_fdn,
        "query fdn (enter example : query-fdn 0 [slot_id])" },
    { "listen-ss",
        telephonytool_cmd_ss_register,
        "listen ss event (enter example : listen-ss 0 1 \
        [slot_id][event_id])" },

    /* IMS Command */
    { "enable-ims",
        telephonytool_cmd_ims_enable,
        "turn on/off ims (enter example : enable-ims 0 1 \
        [slot_id][action: 0-disable 1-enable])" },
    { "set-ims-cap",
        telephonytool_cmd_set_ims_service,
        "set ims service function (enter example : set-ims-cap 0 1 \
        [slot_id][cap-value: 1-voice 4-sms 5-voice&sms])" },
    { "listen-ims",
        telephonytool_cmd_ims_register,
        "listen ims registration(enter example : listen-ims 0 [slot_id]" },
    { "get-ims-registration",
        telephonytool_cmd_ims_get_registration,
        "get ims registration(enter example : get-ims-registration 0 0 \
        [slot_id][action:0-ims info 1-ims state 2-volte state]" },

    /* PhoneBook Command */
    { "load-adn",
        telephonytool_cmd_load_adn_entries,
        "load adn entries (enter example : load-adn 0 [slot_id])" },

    { "q", NULL, "Quit (pls enter : q)" },
    { "help", telephonytool_cmd_help,
        "Show this message (pls enter : help)" },
    { 0 },
};

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

    event_loop = g_main_loop_new(NULL, false);
    g_main_loop_run(event_loop);
    g_main_loop_unref(event_loop);

out:
    tapi_close(context);
    return ret;
}
