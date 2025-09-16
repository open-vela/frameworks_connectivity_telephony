/****************************************************************************
 * frameworks/telephony/tapi_common.h
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

#ifndef __TELEPHONY_COMMON_H
#define __TELEPHONY_COMMON_H

#include "tapi.h"
#include <syslog.h>
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define TAPI_TAG "[tapi]"

#define tapi_log_info(format, ...) syslog(LOG_INFO, TAPI_TAG format, ##__VA_ARGS__)
#define tapi_log_warn(format, ...) syslog(LOG_WARN, TAPI_TAG format, ##__VA_ARGS__)
#define tapi_log_error(format, ...) syslog(LOG_ERR, TAPI_TAG format, ##__VA_ARGS__)
#define tapi_log_debug(format, ...) syslog(LOG_DEBUG, TAPI_TAG format, ##__VA_ARGS__)

#define MAX_CONTEXT_NAME_LENGTH 256
#define MAX_VOICE_CALL_PROXY_COUNT 99
#define SLOT_NOT_SET "SLOT_NOT_SET"

typedef struct {
    tapi_async_result* result;
    tapi_async_function cb_function;
} tapi_async_handler;

#endif
