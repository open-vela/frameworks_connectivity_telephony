/*
 * Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dlfcn.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "tapi.h"

/**
 * tapi_info:
 * @format: format string
 * @Varargs: list of arguments
 *
 * Output general information
 */
void tapi_log_info(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vsyslog(LOG_INFO, format, ap);
    va_end(ap);
}

/**
 * tapi_warn:
 * @format: format string
 * @Varargs: list of arguments
 *
 * Output warning messages
 */
void tapi_log_warn(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vsyslog(LOG_WARNING, format, ap);
    va_end(ap);
}

/**
 * tapi_error:
 * @format: format string
 * @varargs: list of arguments
 *
 * Output error messages
 */
void tapi_log_error(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vsyslog(LOG_ERR, format, ap);
    va_end(ap);
}

/**
 * tapi_debug:
 * @format: format string
 * @varargs: list of arguments
 *
 * Output debug message
 *
 * The actual output of the debug message is controlled via a command line
 * switch. If not enabled, these messages will be ignored.
 */
void tapi_log_debug(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vsyslog(LOG_DEBUG, format, ap);
    va_end(ap);
}
