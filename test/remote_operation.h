#include <stdio.h>

enum REMOTE_CALL_COMMAND_TYPE {
    ACTIVE_CALL = 0,
    HOLD_CALL = 1,
    INCOMING_CALL = 4,
    REJECT_CALL = 6,
};

#define DISCONNECT_REASON_REMOTE_HANGUP 16
#define DISCONNECT_REASON_NETWORK_HANGUP 34

void remote_call_operation(int slot_id, const char* phone_number, enum REMOTE_CALL_COMMAND_TYPE op);
void remote_call_hangup_with_disconnect_reason(int slot_id, const char* phone_number, int disconnect_reason);
int remote_call_clcc_with_data(int slot_id, int clcc_with_data);
int remote_command_response_fail(int slot_id, int command_fail);
int remote_sim_absent_operation(int slot_id);
int remote_sim_insert_operation(int slot_id);
int remote_sim_set_sim_operator(int slot_id, const char* expect_mccmnc);
int remote_sim_set_channel_error_code(int slot_id, int error_code);
int remote_ss_operation_delay(int slot_id, int delay_sec);
int remote_sms_delay(int slot_id, int delay_sec);
int remote_sms_send_chinese_long_message(int slot_id);
int remote_sms_send_english_long_message(int slot_id);
int remote_sms_send_message(int slot_id);
int remote_radio_on_off_delay(int delay_sec);
int remote_sim_invalid_operation(int slot_id);
#ifndef CONFIG_TELEPHONY_DFX
int remote_abnormal_event_report(int type_id);
int remote_unexpected_abnormal_event_report(void);
int remote_data_block_operation(bool enable);
int remote_trigger_oos(int type);
#endif
int remote_modem_upgrade_state_report(int slot_id, int report_state);
int remote_incoming_call_state_change(int slot_id, const char* phone_number, int target_status);
