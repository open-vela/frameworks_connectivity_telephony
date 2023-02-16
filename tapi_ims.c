/****************************************************************************
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "tapi_ims.h"
#include "tapi_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IMS_REGISTER_ENABLE 1
#define IMS_REGISTER_DISABLE 0

/****************************************************************************
 * Private Function
 ****************************************************************************/

static int tapi_ims_enable(tapi_context context, int slot_id, int state)
{
    dbus_context* ctx = context;
    GDBusProxy* proxy;
    char* member;

    if (ctx == NULL || !tapi_is_valid_slotid(slot_id)) {
        return -EINVAL;
    }

    proxy = ctx->dbus_proxy[slot_id][DBUS_PROXY_IMS];
    if (proxy == NULL) {
        tapi_log_error("no available proxy ...\n");
        return -EIO;
    }

    if (state == IMS_REGISTER_ENABLE)
        member = "Register";
    else if (state == IMS_REGISTER_DISABLE)
        member = "Unregister";
    else
        return -EINVAL;

    if (!g_dbus_proxy_method_call(proxy, member, NULL, NULL, NULL, NULL))
        return -EIO;

    return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int tapi_ims_turn_on(tapi_context context, int slot_id)
{
    return tapi_ims_enable(context, slot_id, IMS_REGISTER_ENABLE);
}

int tapi_ims_turn_off(tapi_context context, int slot_id)
{
    return tapi_ims_enable(context, slot_id, IMS_REGISTER_DISABLE);
}