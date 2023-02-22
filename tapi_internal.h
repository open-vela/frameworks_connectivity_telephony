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
#define MAX_VOICE_CALL_PROXY_COUNT 10

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum dbus_proxy_type {
    DBUS_PROXY_MODEM = 0,
    DBUS_PROXY_RADIO,
    DBUS_PROXY_CALL,
    DBUS_PROXY_SIM,
    DBUS_PROXY_DATA,
    DBUS_PROXY_SMS,
    DBUS_PROXY_CBS,
    DBUS_PROXY_NETREG,
    DBUS_PROXY_NETMON,
    DBUS_PROXY_CALL_BARRING,
    DBUS_PROXY_CALL_FORWARDING,
    DBUS_PROXY_SS,
    DBUS_PROXY_CALL_SETTING,
    DBUS_PROXY_IMS,
    DBUS_PROXY_MAX_COUNT,
};

typedef struct {
    char name[MAX_CONTEXT_NAME_LENGTH + 1];
    DBusConnection* connection;
    GDBusClient* client;
    GDBusProxy* dbus_proxy_manager;
    GDBusProxy* dbus_proxy[CONFIG_ACTIVE_MODEM_COUNT][DBUS_PROXY_MAX_COUNT];
    GDBusProxy* dbus_voice_call_proxy[CONFIG_ACTIVE_MODEM_COUNT][MAX_VOICE_CALL_PROXY_COUNT];
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
int tapi_registration_status_from_string(const char* status);
int tapi_registration_mode_from_string(const char* mode);
int tapi_operator_status_from_string(const char* mode);
const char* tapi_apn_context_type_to_string(tapi_apn_context_type type);
tapi_apn_context_type tapi_apn_context_type_from_string(const char* type);
const char* tapi_apn_auth_method_to_string(tapi_data_auth_method auth);
tapi_data_auth_method tapi_apn_auth_method_from_string(const char* auth);
const char* tapi_apn_proto_to_string(tapi_data_proto proto);
bool tapi_apn_proto_from_string(const char* str, tapi_data_proto* proto);
char* tapi_utils_get_modem_path(int slot_id);
enum tapi_call_status tapi_call_string_to_status(const char* str_status);
enum tapi_call_disconnect_reason tapi_call_disconnected_reason_from_string(const char* str_status);
bool tapi_is_call_signal_message(DBusMessage* message, DBusMessageIter* iter, int msg_type);
const char* tapi_get_call_signal_member(tapi_indication_msg msg);

#ifdef __cplusplus
}
#endif

#endif /* __TELEPHONY_INTERNAL_H */
