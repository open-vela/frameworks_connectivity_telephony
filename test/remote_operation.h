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
int remote_sim_absent_operation(int slot_id);
int remote_sim_insert_operation(int slot_id);
int remote_sim_set_sim_operator(int slot_id, const char* expect_mccmnc);
int remote_ss_operation_delay(int slot_id, int delay_sec);
int remote_sms_delay(int slot_id, int delay_sec);
int remote_radio_on_off_delay(int delay_sec);
