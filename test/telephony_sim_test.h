#ifndef TELEPHONY_SIM_TEST_H_
#define TELEPHONY_SIM_TEST_H_

#include "telephony_test.h"
#define TEST_COUNT 10

int tapi_sim_has_icc_card_test(int slot_id);
int tapi_sim_multi_has_icc_card_test(int slot_id);
int tapi_sim_get_sim_operator_test(int slot_id, const char* expect_res);
int tapi_sim_multi_get_sim_operator(int slot_id, const char* expect_res);
int tapi_sim_get_sim_operator_name_test(int slot_id, const char* expect_res);
int tapi_sim_get_sim_operator_name_numerous(int slot_id, const char* expect_res);
int tapi_sim_get_sim_subscriber_id_test(int slot_id, const char* expect_res);
int tapi_sim_multi_get_sim_subscriber_id_test(int slot_id, const char* expect_res);
int tapi_sim_get_sim_iccid_test(int slot_id, const char* expect_res);
int tapi_sim_multi_get_sim_iccid_test(int slot_id, const char* expect_res);
int tapi_sim_get_ef_msisdn_test(int slot_id, const char* expect_res);
int tapi_sim_multi_get_ef_msisdn_test(int slot_id, const char* expect_res);
int tapi_sim_listen_sim_test(int slot_id, int event_id);
int tapi_sim_unlisten_sim_test(int slot_id, int watch_id);
int tapi_open_logical_channel_test(int slot_id);
int tapi_close_logical_channel_test(int slot_id);
int tapi_transmit_apdu_basic_channel_test(int slot_id);
int tapi_sim_get_state_test(int slot_id);
int tapi_sim_set_uicc_enablement_test(int slot_id);
int tapi_sim_get_uicc_enablement_test(int slot_id);
int tapi_sim_enter_pin_test(int slot_id);
int tapi_sim_change_pin_test(int slot_id, char* old_pin, char* new_pin);
int tapi_sim_lock_pin_test(int slot_id);
int tapi_sim_unlock_pin_test(int slot_id);
int tapi_phonebook_load_adn_entries_test(int slot_id);
int tapi_phonebook_load_fdn_entries_test(int slot_id);
int tapi_phonebook_insert_fdn_entry_test(int slot_id);
int tapi_phonebook_update_fdn_entry_test(int slot_id);
int tapi_phonebook_delete_fdn_entry_test(int slot_id);
int sim_open_close_logical_channel_numerous(int slot_id);
int tapi_transmit_apdu_logical_channel_test(int slot_id);
int sim_transmit_apdu_by_logical_channel(int slot_id);
int sim_change_pin_test(int slot_id);

#endif