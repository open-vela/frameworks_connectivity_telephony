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

int remote_command_response_fail(int slot_id, int command_fail)
{
    memset(remote_command_buf, 0, sizeof(remote_command_buf));
    sprintf(remote_command_buf, "AT+REMOTECMDERROR=%d", command_fail);
    return modem_invoke_oem_ril_request_strings_test(slot_id, remote_command_buf, 1);
}

int remote_call_clcc_with_data(int slot_id, int clcc_with_data)
{
    memset(remote_command_buf, 0, sizeof(remote_command_buf));
    sprintf(remote_command_buf, "AT+REMOTECLCCWITHDATA=%d", clcc_with_data);
    return modem_invoke_oem_ril_request_strings_test(slot_id, remote_command_buf, 1);
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

int remote_sim_set_channel_error_code(int slot_id, int error_code)
{
    memset(remote_command_buf, 0, sizeof(remote_command_buf));
    sprintf(remote_command_buf, "AT+REMOTELOGICALCHANNELERR=%d", error_code);
    return modem_invoke_oem_ril_request_strings_test(slot_id, remote_command_buf, 1);
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
    char req_data[75] = "AT+REMOTESMS=0891683110305005F0200BA15157069026F90000528002517054230133";
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

int remote_sim_invalid_operation(int slot_id)
{
    char req_data[30] = { 0 };
    char* oem_req[1];
    oem_req[0] = req_data;

    sprintf(req_data, "AT+REMOTENETSIMVALID");
    syslog(LOG_DEBUG, "%s, req_data: %s\n", __func__, req_data);
    return tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), 0,
        EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, 1, NULL);
}

#ifndef CONFIG_TELEPHONY_DFX
int remote_abnormal_event_report(int type_id)
{
    char req_data[30] = { 0 };
    char* oem_req[1];
    oem_req[0] = req_data;

    sprintf(req_data, "AT+ABNORMAL=%d 1", type_id);
    syslog(LOG_DEBUG, "%s, req_data: %s\n", __func__, req_data);
    return tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), 0,
        EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, 1, NULL);
}

int remote_unexpected_abnormal_event_report(void)
{
    char* oem_req[1];
    oem_req[0] = "AT+ABNORMALWRONGDATA? 1";
    return tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), 0,
        EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, 1, NULL);
}

int remote_data_block_operation(bool enable)
{
    char req_data[30] = { 0 };
    char* oem_req[1];
    oem_req[0] = req_data;

    sprintf(req_data, "AT+REMOTEBLOCKDATA=%d 1", enable ? 1 : 0);
    syslog(LOG_DEBUG, "%s, req_data: %s\n", __func__, req_data);
    return tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), 0,
        EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, 1, NULL);
}

int remote_trigger_oos(int type)
{
    char* oem_req[1];
    if (type == 0) {
        oem_req[0] = "AT+REMOTEDATAREG=0 1";
    } else {
        oem_req[0] = "AT+REMOTEVOICEREG=0 1";
    }
    return tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), 0,
        EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, 1, NULL);
}
#endif

int remote_modem_upgrade_state_report(int slot_id, int report_state)
{
    char req_data[30] = { 0 };
    char* oem_req[1];
    oem_req[0] = req_data;

    sprintf(req_data, "AT+MDUPGRADESTATECHANGE=%d 1", report_state);
    syslog(LOG_DEBUG, "%s, req_data: %s\n", __func__, req_data);
    return tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), 0,
        EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, 1, NULL);
}

int remote_incoming_call_state_change(int slot_id, const char* phone_number, int target_status)
{
    char* oem_req[1];
    oem_req[0] = remote_command_buf;
    memset(remote_command_buf, 0, sizeof(remote_command_buf));
    sprintf(remote_command_buf, "AT+REMOTEERRORSTATEREPORT=%d,%s", target_status, phone_number);
    return tapi_invoke_oem_ril_request_strings(get_tapi_ctx(), slot_id,
        EVENT_OEM_RIL_REQUEST_STRINGS_DONE, oem_req, 1, NULL);
}
