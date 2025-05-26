/****************************************************************************
 * frameworks/telephony/include/tapi_phone.h
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

#ifndef __TAPI_PHONE_H
#define __TAPI_PHONE_H

#include "bt_wtp.h"
#include "tapi.h"

typedef struct {
    char* remote_bt_addr; // 60:136:70:219:143:100
    uint16_t other_info_len; // reserved field
    uint8_t value[0]; // reserved field
} tapi_wtp_call_data_t; // wtp call info

typedef struct {
    char* phone_number;
    char* call_id;
    uint16_t other_info_len; // reserved field
    uint8_t value[0]; // reserved field
} tapi_cell_call_data_t; // esim call info

typedef struct {
    tapi_async_result* phone_info; // esim call+hf call param
    tapi_wtp_call_data_t* wtp_info; // wtp call param
} tapi_call_data_t; // call data info

/**
 * @brief it will be called when phone service client is ready.client
 * should start other operations after client ready.
 *
 * @param[out] status - 0-client ready,1-client start fail
 * @return - 0-success,1-fail
 */
typedef int32_t (*phone_client_status_cb)(int status);

/**
 * @brief start a phone service client to connect phone service.
 * after start client,it can be used to communicate with phone service
 *
 * @param[in] loop - default uv_loop
 * @param[in] cb - Event callback.it will be called when client ready or fail
 * @param[in] user_data - user data
 * @return - 0-success,1-fail
 */
int tapi_start_phone_service_client(uv_loop_t* loop, phone_client_status_cb cb, void* user_data);

/**
 * @brief stop a phone service client.after stop client,client can't be used to
 * communicate with phone service
 */
void tapi_stop_phone_service_client(void);

/**
 * @brief register wtp callback functions.An application may register interested callbacks
 * on initialization. NULL should be used if no callbacks are needed.wtp_transport_requested_callback
 * and wtp_connection_state_changed_callback is mandatory.wtp_transport_requested_callback
 * is used to receive incoming call.wtp_connection_state_changed_callback is used to receive call state change.
 *
 * @param[in] cb_list - callback list,refer wtp_callbacks_t.
 * @param[in] async_cb - callback func when register done
 * @param[in] user_obj - user data info when callback
 * @return - 0-register cb success,other-fail
 */
int tapi_client_wtp_register_cb(const wtp_callbacks_t* cb_list, tapi_async_function async_cb, void* user_obj);

/**
 * @brief unregister wtp callback functions.An application may unregister interested
 * callbacks on destory function.
 *
 * @param[in] async_cb - callback func when unregister done
 * @param[in] user_obj - user data info when callback
 * @return - 0-register cb success,other-fail
 */
int tapi_client_wtp_unregister_cb(tapi_async_function async_cb, void* user_obj);

/**
 * @brief set audio type,headphones or speaker.speaker is used by default.
 * it can be used to set audio type anytime.And setting value can be used till changed.
 *
 * @param[in] type - 1-headphones,0-speaker
 * @param[in] async_cb - callback func when unregister done
 * @param[in] user_obj - user data info when callback
 * @return - 0-send success,other fail
 */
int tapi_client_set_audio_type(int type, tapi_async_function async_cb, void* user_obj);

/**
 * @brief Set the local WTP information. it is not mandatory.client can use bt_wtp_set_local_info directly
 * The local device information is to be displayed to remote users. This includes the local device
 * name, up to two phone numbers, and a position information.
 *
 * @param[in] param - the supported parameter sets of the media or data transport, see wtp_param_t for
 *                details.
 * @param[in] async_cb - callback func when unregister done
 * @param[in] user_obj - user data info when callback
 * @return - 0-set_local_info request send success,other fail
 */
int tapi_wtp_set_local_info(wtp_local_t* local, tapi_async_function async_cb, void* user_obj);

/**
 * @brief Start discovering the available peer devices or networks.it is not mandatory.
 * client can use bt_wtp_start_discovery or bt_wtp_stop_discovery directly
 *
 * This function is used by the application to find other interoperable devices.
 * @param[in] enable,true-request start discovery，false-request stop discovery
 * @param[in] async_cb - callback func when unregister done
 * @param[in] user_obj - user data info when callback
 * @return - 0-start_discovery request send success,other fail
 */
int tapi_wtp_modify_discovery(bool enable, tapi_async_function async_cb, void* user_obj);

/**
 * @brief Enable visibility of the local device.This function is used by the application
 * to enable the visibility of the local device. This allows the local device to be
 * discovered by other WTP devices.it is not mandatory.client can use bt_wtp_start_visibility
 * or bt_wtp_stop_visibility directly
 *
 * @param[in] enable,true-request start visibility，false-request stop visibilty
 * @param[in] async_cb - callback func when unregister done
 * @param[in] user_obj - user data info when callback
 * @return 0-start_visibility request send success,other fail
 */
int tapi_wtp_modify_visibility(bool enable, tapi_async_function async_cb, void* user_obj);

/**
 * @brief Initiate a call (wtp:WTP transport with the remote device or network).
 * This function is used by the application to initiate a call (wtp:WTP transport with the selected peer
 * device or network. At least one identification information (e.g., address or phone number) shall
 * be provided. Available parameter sets shall be provided to indicate the expected transport type.)
 *
 * @param[in] call_data- Information of call needed,wtp call wtp_remote_t* remote and wtp_param_t* param
 * @param[in] async_cb - Event callback.
 * @param[in] user_obj - user data
 * @return 0- call request send success,other fail
 */
int tapi_dial_call(tapi_call_data_t call_data, tapi_async_function async_cb, void* user_obj);

/**
 * @brief hangup a call.This function is used by the application to hangup a call
 *
 * @param[in] call_data- Information of call needed
 * @param[in] async_cb - Event callback.
 * @param[in] user_obj - user data
 * @return 0-call hangup send success,other fail
 */
int tapi_hangup_call(tapi_call_data_t call_data, tapi_async_function async_cb, void* user_obj);

/**
 * @brief answer a call.This function is used by the application to answer a call
 *
 * @param[in] call_data- Information of call needed,wtp call wtp_remote_t* remote and wtp_param_t* param
 * @param[in] async_cb - Event callback.
 * @param[in] user_obj - user data
 * @return 0-call answer request send success,other fail
 */
int tapi_answer_call(tapi_call_data_t call_data, tapi_async_function async_cb, void* user_obj);

/**
 * @brief reject a call.This function is used by the application to reject a call
 *
 * @param[in] call_data- Information of call needed,wtp call wtp_remote_t* remote and NULL
 * @param[in] async_cb - Event callback.
 * @param[in] user_obj - user data
 * @return 0-call rejct request send success,other fail
 */
int tapi_reject_call(tapi_call_data_t call_data, tapi_async_function async_cb, void* user_obj);

#endif
