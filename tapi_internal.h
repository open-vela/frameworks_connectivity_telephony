/****************************************************************************
 * frameworks/telephony/tapi_internal.h
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

#ifndef __TELEPHONY_INTERNAL_H
#define __TELEPHONY_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <gdbus.h>
#include <ofono/dbus.h>
#include <stdbool.h>
#include <syslog.h>

#include "tapi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define tapi_log_info(format, ...) syslog(LOG_INFO, format, ##__VA_ARGS__)
#define tapi_log_warn(format, ...) syslog(LOG_WARN, format, ##__VA_ARGS__)
#define tapi_log_error(format, ...) syslog(LOG_ERR, format, ##__VA_ARGS__)
#define tapi_log_debug(format, ...) syslog(LOG_DEBUG, format, ##__VA_ARGS__)

#define MAX_CONTEXT_NAME_LENGTH 256

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum dbus_proxy_type {
    DBUS_PROXY_MODEM = 0,
    DBUS_PROXY_RADIO,
    DBUS_PROXY_CALL,
    DBUS_PROXY_SIM,
    DBUS_PROXY_MAX_COUNT,
};

typedef struct {
    char name[MAX_CONTEXT_NAME_LENGTH];
    DBusConnection* connection;
    GDBusClient* client;
    GDBusProxy* dbus_proxy_manager;
    GDBusProxy* dbus_proxy[CONFIG_ACTIVE_MODEM_COUNT][DBUS_PROXY_MAX_COUNT];
} dbus_context;

typedef struct {
    tapi_async_result* result;
    tapi_async_function cb_function;
} tapi_async_handler;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

static inline bool tapi_is_valid_slotid(int slot_id)
{
    return (slot_id >= 0 && slot_id < CONFIG_ACTIVE_MODEM_COUNT);
}

const char* tapi_pref_network_mode_to_string(tapi_pref_net_mode mode);
bool tapi_pref_network_mode_from_string(const char* str, tapi_pref_net_mode* mode);
bool tapi_network_type_from_string(const char* str, tapi_network_type* type);
const char* tapi_registration_status_to_string(int status);
int tapi_registration_status_from_string(char* status);
int tapi_registration_mode_from_string(char* mode);
int tapi_operator_status_from_string(char* mode);
char* tapi_utils_get_modem_path(int slot_id);
enum tapi_call_status tapi_call_string_to_status(char* str_status);
bool tapi_is_call_signal_message(DBusMessage* message, DBusMessageIter* iter, int msg_type);
tapi_call_list* tapi_add_call_to_list(tapi_call_list* head, tapi_call_info* new_val);
void tapi_free_call_list(tapi_call_list* head);
unsigned int tapi_get_call_count(tapi_call_list* First);
int tapi_get_call_by_state(tapi_call_list* pfirst, int state, tapi_call_info* val);
const char* tapi_get_call_signal_member(tapi_indication_msg msg);

#ifdef __cplusplus
}
#endif

#endif /* __TELEPHONY_INTERNAL_H */
