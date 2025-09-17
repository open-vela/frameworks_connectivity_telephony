#ifndef TELEPHONY_CALL_TEST_H_
#define TELEPHONY_CALL_TEST_H_

#include "telephony_test.h"
#include <uv.h>

#define CALL_LOCAL_HANGUP 0x01
#define CALL_STATE_CHANGE_TO_ACTIVE 0x02
#define CALL_REMOTE_HANGUP 0x03
#define NEW_CALL_INCOMING 0x04
#define INCOMING_CALL_WITH_NETWORK_NAME 0x05
#define HANGUP_DUE_TO_NETWORK_EXCEPTION 0x06
#define CALL_STATE_CHANGE_TO_HOLD 0x07
#define GET_ALL_CALLS 0x08
#define GET_TWO_CALL_STATES 0x09
#define GET_HOLD_CALL_ID 0x10
#define GET_CURRENT_CALL_STATE 0x11
#define NEW_CALL_WAITING 0x12
#define NEW_CALL_DIALING 0x13
#define NEW_CALL_ALERTING 0x14
#define NEW_CONFERENCE_CALL 0x15
#define EVENT_REQUEST_START_DTMF_DONE 0x30
#define EVENT_REQUEST_STOP_DTMF_DONE 0x31
#define EVENT_REQUEST_CALL_MERGE_DONE 0x32
#define EVENT_REQUEST_CALL_SEPARATE_DONE 0x33
#define EVENT_GENERIC_CALLBACK_STATUS 0x34
#define CALL_NETWORK_HANGUP 0x35
#define CALL_REMOTE_HOLD 0x36
#define EVENT_DIAL_CALL_DFX_DONE 0x37
#define EVENT_INCOMING_CALL_DFX_DONE 0x38
#define EVENT_ANSWER_CALL_DFX_DONE 0x39
#define EVENT_DIAL_ECC_CALL_DFX_DONE 0x40
#define EVENT_HANGUP_CALL_DFX_DONE 0x41

int setup_call(void** state);
int setup_callAndData(void** state);
int setup_callAndRadio(void** state);
int teardown_call(void** state);
int teardown_callAndData(void** state);
int teardown_callAndRadio(void** state);
int call_abnormal_answer_again_test(int slot_id);
int call_answer_error(int slot_id);
int call_answer_call_test(int slot_id, char* call_id);
int call_answer_call_aysnc_test(int slot_id, char* call_id);
int call_check_dialing_status_with_multi_call(int slot);
int call_check_status_in_call_active(int slot_id);
int call_clear_voicecall_slot(void);
int call_dial_active_hangup_due_to_caller_network_exception(int slot_id);
int call_dial_active_hangup_due_to_dialer_network_exception(int slot_id);
int call_dial_after_caller_reject(int slot_id);
int call_dtmf_after_dial_test(int slot_id);
int call_dial_and_remote_active(int slot_id, char* phone_number);
int call_dial_and_remote_hangup(int slot_id);
int call_dial_and_check_status_in_call_active(int slot_id);
int call_dial_and_keep_in_call_active(int slot_id, char* phone_number);
int call_dial_another_after_reject(int slot_id);
int call_dial_caller_reject_and_dial_another(int slot_id);
int call_dial_conference_test(int slot_id);
int call_dial_ecc_number_test(int slot_id);
int call_dial_ecc_number_without_sim_card(int slot_id);
int call_dial_in_two_calling(int slot_id);
int call_dial_long_phone_number_test(int slot_id);
int call_dial_number_test(int slot_id);
int call_dial_second_call_active_and_hangup_by_caller(int slot_id);
int call_dial_second_call_active_and_hangup_by_dialer(int slot_id);
int call_dial_second_call_and_reject_by_caller(int slot_id);
int call_dial_short_phone_number_test(int slot_id);
int call_dial_test(int slot_id, char* phone_number, int hide_caller_id);
int call_dial_to_phone_out_of_service(int slot_id);
int call_dial_with_area_code(int slot_id);
int call_dial_with_area_code_test(int slot_id);
int call_dial_with_disabled_hide_callerid_test(int slot_id);
int call_dial_with_default_hide_callerid_test(int slot_id);
int call_dial_with_enable_hide_callerid_test(int slot_id);
int call_dial_with_numerous_code_test(int slot_id);
int call_dial_with_pause_code_test(int slot_id);
int call_dial_with_wait_code_test(int slot_id);
int call_dial_without_sim_card(int slot_id);
int call_dialer_hold_recover_and_hold_by_caller(int slot_id);
int call_display_the_network_of_incoming_call(int slot_id, char* network_name);
int call_display_the_network_of_incoming_call_in_call_process(int slot_id, char* network_name);
int call_dial_third_call(int slot_id);
int call_get_call_count(int slot_id);
int call_get_default_voicecall_slot_test(void);
int call_hangup_after_dialing(int slot_id);
int call_hangup_all_call_in_two_calling(int slot_id);
int call_hangup_all_test(int slot_id);
int call_hangup_current_call_and_resume_call(int slot_id);
int call_hangup_current_call_test(int slot_id);
int call_hangup_hold_call_in_two_calls(int slot_id);
int call_hold_current_call_and_reject_new_incoming(int slot_id);
int call_hold_first_call_and_answer_second_call(int slot_id);
int call_hold_incoming_hangup_second_recover_first(int slot_id);
int call_incoming_and_check_number(int slot_id);
int call_incoming_and_check_number_in_call(int slot_id);
int call_incoming_and_hangup_by_dialer_before_answer_numerous(int slot_id);
int call_incoming_and_hangup_new_call(int slot_id);
int call_incoming_and_local_hangup(int slot_id);
int call_incoming_and_remote_hangup(int slot_id);
int call_incoming_and_remote_hangup_for_times(int slot_id);
int call_incoming_after_remote_hangup(int slot_id);
int call_incoming_answer_and_hangup(int slot_id);
int call_incoming_answer_and_remote_hangup(int slot_id);
int call_incoming_hangup_first_answer_call(int slot_id);
int call_incoming_hold_and_recover_by_dialer(int slot_id);
int call_incoming_hold_and_resume_by_caller(int slot_id);
int call_incoming_second_call_swap_answer_hangup_swap(int slot_id);
int call_incoming_third_call(int slot_id);
int call_listen_call_test(int slot_id);
int call_load_and_compare_ecclist_with_china_sim_card_test(int slot_id);
int call_load_and_compare_ecclist_without_sim_card_test(int slot_id);
int call_merge_by_user(int slot_id);
int call_outgoing_active_and_send_tones(int slot_id);
int call_outgoing_hold_and_unhold_by_caller(int slot_id);
int call_outgoing_remote_answer_and_hangup(int slot_id);
int call_outgoing_remote_answer_and_network_hangup(int slot_id);
int call_reject_second_call_in_call_active(int slot_id);
int call_release_and_answer(int slot_id);
int call_release_and_swap_other_call(int slot_id);
int call_remote_hold_after_local_hold_in_actve(int slot_id);
int call_remote_hold_after_local_unhold_in_actve(int slot_id);
int call_remote_hold_and_unhold_after_incoming_answer(int slot_id);
int call_separate_by_user(int slot_id);
int call_set_default_voicecall_slot_test(int slot_id);
int call_set_voicecall_slot(int slot_id);
int call_start_dtmf_test(int slot_id);
int call_stop_dtmf_test(int slot_id);
int call_swap_dial_reject_swap(int slot_id);
int call_swap_in_two_calling(int slot_id);
int call_swap_two_call_times_in_second_active(int slot_id);
int call_transfer_in_active_and_hold_call(int slot_id);
int call_unhold_first_incoming_call_after_hangup_second_call(int slot_id);
int call_unlisten_call_test(void);
int remote_operation_call_incoming_test(int slot_id, char* phone_number);
int remote_operation_call_active_test(int slot_id, char* phone_number);
int answer_incoming_call_test(int slot_id);
int get_current_call_state_test(int slot_id);
int call_dial_in_active_test(int slot_id);
int incoming_call_with_unexpected_status(int target_status);
#endif
