/****************************************************************************
 * frameworks/telephony/tapi_tool.h
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

#ifndef __TELEPHONY_TOOL_H
#define __TELEPHONY_TOOL_H

#include "tapi.h"
#include <sys/queue.h>
#include <syslog.h>
#include <uv.h>

#define TAPI_DBUS_NAME_MAX_LEN 256
#define TAPI_DBUS_NAME_DEFAULT "vela.telephony.tool"
#define MAX_INPUT_ARGS_LEN 128

enum cmd_type {
    RADIO_CMD = 1,
    CALL_CMD,
    DATA_CMD,
    SIM_CMD,
    SMS_AND_CBS_CMD,
    NETWORK_CMD,
    SS_CMD,
    IMS_CMD,
    PHONEBOOK_CMD,
    STK_CMD,
    TAPI_CMD,
    PHONE_SERVICE_CMD,
    QUIT_CMD,
    HELP_CMD
};

typedef int (*telephonytool_cmd_func)(tapi_context context, char* pargs);
struct telephonytool_cmd_s {
    const char* cmd; /* The command text */
    enum cmd_type type; /* The command type */
    telephonytool_cmd_func pfunc; /* Pointer to command handler */
    const char* help; /* The help text */
};

typedef int (*commontool_cmd_func)(char* pargs);
struct commontool_cmd_s {
    const char* cmd; /* The command text */
    enum cmd_type type; /* The command type */
    commontool_cmd_func pfunc; /* Pointer to command handler */
    const char* help; /* The help text */
};

typedef int (*phoneservicetool_cmd_func)(char* pargs);
struct phoneservicetool_cmd_s {
    const char* cmd; /* The command text */
    enum cmd_type type; /* The command type */
    phoneservicetool_cmd_func pfunc; /* Pointer to command handler */
    const char* help; /* The help text */
};

typedef struct {
    uv_async_t async;
    char cmd[CONFIG_NSH_LINELEN];
    char param[CONFIG_NSH_LINELEN];
    void* data;
} async_message_t;

#ifdef CONFIG_PHONE_SERVICE
typedef struct MListNode {
    int id;
    void* data;
    SIMPLEQ_ENTRY(MListNode)
    entries;
} MListNode;

typedef struct {
    SIMPLEQ_HEAD(, MListNode)
    head;
    int next_id;
} MLinkedList;

void show_tapi_phoneservice_cmd(void);
bool execute_phone_service_cmd(char* cmd, char* arg);
int phone_client_init(void);
void phone_client_clean(void);
#endif

#ifdef CONFIG_TELEPHONY
void show_tapi_telephony_cmd(int num);
bool execute_telephony_cmd(char* cmd, char* arg);
int tapi_init(char* dbus_name);
void tapi_clean(void);
void update_uv_exit_flag(void);
bool g_context_is_null(void);
#endif

int split_input(char dst[][MAX_INPUT_ARGS_LEN], int size, char* str, const char* spl);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif
