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
