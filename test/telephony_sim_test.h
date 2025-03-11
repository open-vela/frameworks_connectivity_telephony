#ifndef TELEPHONY_SIM_TEST_H_
#define TELEPHONY_SIM_TEST_H_

#include "telephony_test.h"
#define TEST_COUNT 10

int setup_sim(void** state);
int teardown_sim(void** state);
int teardown_sim_channel(void** state);
int phonebook_load_adn_entries_test(int slot_id);
int phonebook_load_fdn_entries_test(int slot_id);
int phonebook_insert_fdn_entry_test(int slot_id);
int phonebook_update_fdn_entry_test(int slot_id);
int phonebook_delete_fdn_entry_test(int slot_id);
int remote_sim_absent_operation_test(int slot_id);
int remote_sim_insert_operation_test(int slot_id);
int remote_sim_absent_insert_operation_test(int slot_id);
int sim_change_and_restore_pin_test(int slot_id);
int sim_change_pin_test(int slot_id, char* old_pin, char* new_pin);
int sim_close_logical_channel_test(int slot_id);
int sim_enter_pin_test(int slot_id);
int sim_get_ef_msisdn_test(int slot_id, const char* expect_res);
int sim_get_sim_iccid_test(int slot_id, const char* expect_res);
int sim_get_sim_operator_name_numerous(int slot_id, const char* expect_res);
int sim_get_sim_operator_name_test(int slot_id, const char* expect_res);
int sim_get_sim_operator_test(int slot_id, char* expect_res);
int sim_get_sim_subscriber_id_test(int slot_id, const char* expect_res);
int sim_get_state_test(int slot_id);
int sim_get_uicc_enablement_test(int slot_id);
int sim_has_icc_card_test(int slot_id);
int sim_multi_has_icc_card_test(int slot_id);
int sim_multi_get_sim_subscriber_id_test(int slot_id, const char* expect_res);
int sim_multi_get_sim_iccid_test(int slot_id, const char* expect_res);
int sim_multi_get_ef_msisdn_test(int slot_id, const char* expect_res);
int sim_listen_sim_test(int slot_id);
int sim_lock_pin_test(int slot_id);
int sim_open_close_logical_channel_numerous(int slot_id);
int sim_open_close_logical_channel_with_error_code_numerous(int slot_id);
int sim_open_logical_channel_test(int slot_id);
int sim_open_logical_channel_with_error_code_test(int slot_id, int error_code);
int sim_set_operator_test(int slot_id, const char* expect_res);
int sim_set_uicc_enablement_test(int slot_id);
int sim_transmit_apdu_by_logical_channel(int slot_id);
int sim_transmit_apdu_by_logical_channel_with_error_code(int slot_id, int error_code);
int sim_transmit_apdu_basic_channel_test(int slot_id);
int sim_transmit_apdu_basic_channel_with_error_code_test(int slot_id, int error_code);
int sim_transmit_apdu_logical_channel_test(int slot_id);
int sim_unlisten_sim_test(void);
int sim_unlock_pin_test(int slot_id);
int sim_set_and_check_sim_invalid(int slot_id);
#endif
