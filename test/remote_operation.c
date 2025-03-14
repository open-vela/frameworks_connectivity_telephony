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

int remote_sim_absent_operation(int slot_id)
{
    char* oem_req[1];
    oem_req[0] = "AT+REMOTESIMINSERT=0";
    return tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), slot_id,
        EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, 1, NULL);
}

int remote_sim_insert_operation(int slot_id)
{
    char* oem_req[1];
    oem_req[0] = "AT+REMOTESIMINSERT=1";
    return tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), slot_id,
        EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, 1, NULL);
}

int remote_sim_set_sim_operator(int slot_id, const char* expect_mccmnc)
{
    char req_data[30] = { 0 };
    char* oem_req[1];
    oem_req[0] = req_data;

    sprintf(req_data, "AT+REMOTEIMSI=%s", expect_mccmnc);
    syslog(LOG_DEBUG, "%s, req_data: %s\n", __func__, req_data);

    return tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), slot_id,
        EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, 1, NULL);
}

int remote_ss_operation_delay(int slot_id, int delay_sec)
{
    char req_data[30] = { 0 };
    char* oem_req[1];
    oem_req[0] = req_data;

    sprintf(req_data, "AT+REMOTESSDELAY=%d 1", delay_sec);
    syslog(LOG_DEBUG, "%s, req_data: %s\n", __func__, req_data);
    return tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), slot_id,
        EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, 1, NULL);
}

int remote_sms_send_message(int slot_id)
{
    char req_data[60] = "AT+REMOTESMS=00110005810180F60000A705E8329BFD06";
    char* oem_req[1];
    oem_req[0] = req_data;

    return tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), slot_id,
        EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, 1, NULL);
}

int remote_sms_send_english_long_message(int slot_id)
{
    char req_data[30] = "AT+REMOTELONGSMS=1";
    char* oem_req[1];
    oem_req[0] = req_data;

    return tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), slot_id,
        EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, 1, NULL);
}

int remote_sms_send_chinese_long_message(int slot_id)
{
    char req_data[30] = "AT+REMOTELONGSMS=0";
    char* oem_req[1];
    oem_req[0] = req_data;

    return tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), slot_id,
        EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, 1, NULL);
}

int remote_sms_delay(int slot_id, int delay_sec)
{
    char req_data[30] = { 0 };
    char* oem_req[1];
    oem_req[0] = req_data;

    sprintf(req_data, "AT+REMOTEDOS=%d 1", delay_sec);
    syslog(LOG_DEBUG, "%s, req_data: %s\n", __func__, req_data);
    return tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), slot_id,
        EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, 1, NULL);
}

int remote_radio_on_off_delay(int delay_sec)
{
    char req_data[30] = { 0 };
    char* oem_req[1];
    oem_req[0] = req_data;

    sprintf(req_data, "AT+REMOTENETDELAY=%d 1", delay_sec);
    syslog(LOG_DEBUG, "%s, req_data: %s\n", __func__, req_data);
    return tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), 0,
        EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, 1, NULL);
}
