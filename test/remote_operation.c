#include <stdio.h>
#include <string.h>

#include "remote_operation.h"
#include "telephony_common_test.h"

char remote_command_buf[512];

void remote_call_operation(int slot_id, const char* phone_number, enum REMOTE_CALL_COMMAND_TYPE op)
{
    char* oem_req[1];
    oem_req[0] = remote_command_buf;
    memset(remote_command_buf, 0, sizeof(remote_command_buf));
    sprintf(remote_command_buf, "AT+REMOTECALL=%d,0,0,%s,129", (int)op, phone_number);
    tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), slot_id,
        EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, 1, NULL);
}

void remote_call_hangup_with_disconnect_reason(int slot_id, const char* phone_number, int disconnect_reason)
{
    char* oem_req[1];
    oem_req[0] = remote_command_buf;
    memset(remote_command_buf, 0, sizeof(remote_command_buf));
    sprintf(remote_command_buf, "AT+REMOTECALL=6,0,0,%s,129,%d", phone_number, disconnect_reason);
    tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), slot_id,
        EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, 1, NULL);
}
