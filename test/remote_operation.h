enum REMOTE_CALL_COMMAND_TYPE {
    ACTIVE_CALL = 0,
    HOLD_CALL = 1,
    INCOMING_CALL = 4,
    REJECT_CALL = 6,
};

void remote_call_operation(int slot_id, const char* phone_number, enum REMOTE_CALL_COMMAND_TYPE op);