#ifndef TELEPHONY_DATA_TEST_H_
#define TELEPHONY_DATA_TEST_H_

#include "telephony_test.h"

int setup_data(void** state);
int setup_data_enable(void** state);
int teardown_data(void** state);
int teardown_data_enable(void** state);
bool data_get_roaming_enabled_test(bool* result);
int data_disable_and_get_roaming_test(void);
int data_disabled_test(bool enable);
int data_edit_apn_context_test(char* slot_id, char* id, char* type, char* name, char* apn, char* proto, char* auth);
int data_enable_and_get_roaming_test(void);
int data_enable_data_test(int state);
int data_enable_roaming_test(int);
int data_enabled_test(bool enable);
int data_get_call_list(int slot_id, int expect);
int data_get_data_call_list_test(int slot_id, int expect);
int data_enable_auto_when_airplane_mode_close_test(void);
int data_get_enabled_test(bool* result);
int data_get_network_type_test(int slot_id);
int data_is_ps_attached_test(int slot_id);
int data_listen_data_test(int slot_id);
int data_load_apn_contexts_test(int slot_id);
int data_release_network_test(int slot_id, char* target_state);
int data_release_internet_network_test(int slot_id);
int data_release_ims_network_test(int slot_id);
int data_remove_apn_context_test(char* slot_id, char* id);
int data_request_ims_network_test(int slot_id);
int data_request_internet_network_test(int slot_id);
int data_request_network_test(int slot_id, char* target_state);
int data_reset_apn_contexts_test(char* slot_id);
int data_save_apn_context_test(char* slot_id, char* type, char* name, char* apn, char* proto, char* auth);
int data_send_screen_stat_test(int slot_id);
int data_set_and_get_default_data_slot_test(int slot_id);
int data_set_and_get_preferred_apn_test(int slot_id, char* apn_id);
int data_network_type_changed_while_change_rat_test(int slot_id, int expect_rat, int expect_type);
int data_registration_changed_while_change_radio_power(int slot_id, int expect_power, int expect_registration_state);
int data_apn_after_flight_mode_test(int slot_id);
int data_load_carrier_apn_test(int slot_id, char* imsi);
int data_set_data_allow_test(int slot_id);
int data_unlisten_data_test(void);
#ifndef CONFIG_TELEPHONY_DFX
int data_enabled_fail_test(void);
#endif
#endif /* TELEPHONY_DATA_TEST_H_ */
