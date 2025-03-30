#ifndef TELEPHONY_SS_TEST_H_
#define TELEPHONY_SS_TEST_H_

#include "telephony_test.h"

int setup_ss(void** state);
int setup_ssAndRadio(void** state);
int teardown_ss(void** state);
int teardown_ssAndRadio(void** state);
int ss_listen_ss_test(int slot_id);
int ss_unlisten_ss_test(void);
int ss_change_and_reset_call_barring_password_test(int slot_id, char* old_passwd, char* new_passwd);
int ss_call_forwarding_continuous_test(int slot_id, char* phone_num);
int ss_call_waiting_continuous_test(int slot_id);
int ss_disable_all_call_barrings_test(int slot_id, char* passwd);
int ss_disable_all_incoming_test(int slot_id, char* passwd);
int ss_disable_all_outgoing_test(int slot_id, char* passwd);
int ss_get_call_forwarding_option_test(int slot_id, int cf_type);
int ss_get_call_waiting_test(int slot_id, bool expect);
int ss_request_call_barring_test(int slot_id);
int ss_set_and_get_call_barring_option_test(int slot_id, char* facility, char* pin2);
int ss_set_and_get_call_forwarding_option_test(int slot_id, int cf_type, char* number);
int ss_set_call_forwarding_option_test(int slot_id, int cf_type, char* number);
int ss_clear_call_forwarding_option_test(int slot_id, int cf_type);
int ss_set_and_get_call_waiting_test(int slot_id, bool enable);
int ss_set_and_get_fdn_test(int slot_id, bool enable, char* passwd);
#endif /* TELEPHONY_SS_TEST_H_ */
