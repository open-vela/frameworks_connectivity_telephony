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
#include <nuttx/list.h>
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
#define MAX_VOICE_CALL_PROXY_COUNT 99

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum dbus_proxy_type {
    DBUS_PROXY_MODEM = 0,
    DBUS_PROXY_RADIO,
    DBUS_PROXY_CALL,
    DBUS_PROXY_SIM,
    DBUS_PROXY_STK,
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
    DBUS_PROXY_PHONEBOOK,
    DBUS_PROXY_MAX_COUNT,
};

typedef struct {
    char name[MAX_CONTEXT_NAME_LENGTH + 1];
    DBusConnection* connection;
    DBusMessage* pending;
    GDBusClient* client;
    GDBusProxy* dbus_proxy_manager;
    GDBusProxy* dbus_proxy[CONFIG_ACTIVE_MODEM_COUNT][DBUS_PROXY_MAX_COUNT];
    tapi_modem_state modem_state[CONFIG_ACTIVE_MODEM_COUNT];
    bool client_ready;
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
void no_operate_callback(DBusMessage* message, void* user_data);
bool is_call_signal_message(DBusMessage* message, DBusMessageIter* iter, int msg_type);
const char* get_call_signal_member(tapi_indication_msg msg);
void property_set_done(const DBusError* error, void* user_data);
void handler_free(void* obj);
const char* get_env_interface_support_string(const char* interface);
bool is_interface_supported(const char* interface);
int get_modem_id_by_proxy(dbus_context* context, GDBusProxy* proxy);

/**
 * Power on or off modem.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] state          Power state.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_send_modem_power(tapi_context context, int slot_id, bool state);

/**
 * @TestApi, only used for internal test.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[in] atom           Atom id.
 * @param[in] command        Ril request id.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_handle_command(tapi_context context, int slot_id, int atom, int command);

/**
 * Query availble modem list.
 * @param[in] context        Telephony api context.
 * @param[in] event_id       Async event identifier.
 * @param[in] p_handle       Event callback.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_query_modem_list(tapi_context context, int event_id, tapi_async_function p_handle);

/**
 * Get modem manufacturer.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           Manufacturer returned from modem.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_get_modem_manufacturer(tapi_context context, int slot_id, char** out);

/**
 * Get modem model.
 * @param[in] context        Telephony api context.
 * @param[in] slot_id        Slot id of current sim.
 * @param[out] out           Model returned from modem.
 * @return Zero on success; a negated errno value on failure.
 */
int tapi_get_modem_model(tapi_context context, int slot_id, char** out);

#ifdef __cplusplus
}
#endif

#endif /* __TELEPHONY_INTERNAL_H */
