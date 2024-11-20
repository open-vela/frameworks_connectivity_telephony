#ifndef TELEPHONY_SS_TEST_H_
#define TELEPHONY_SS_TEST_H_

#include "telephony_test.h"

int tapi_listen_ss_test(int slot_id);
int tapi_unlisten_ss_test(void);
int tapi_ss_request_call_barring_test(int slot_id);
int tapi_ss_set_call_barring_option_test(int slot_id, char* facility, char* pin2);
int tapi_ss_get_call_barring_option_test(int slot_id, char* key, char* expect);
int tapi_ss_change_call_barring_password_test(int slot_id, char* old_passwd, char* new_passwd);
int tapi_ss_disable_all_incoming_test(int slot_id, char* passwd);
int tapi_ss_disable_all_outgoing_test(int slot_id, char* passwd);
int tapi_ss_disable_all_call_barrings_test(int slot_id, char* passwd);
int tapi_ss_set_call_forwarding_option_test(int slot_id, int cf_type, char* number);
int tapi_ss_get_call_forwarding_option_test(int slot_id, int cf_type);
int tapi_ss_set_call_waiting_test(int slot_id, bool enable);
int tapi_ss_get_call_waiting_test(int slot_id, bool expect);
int tapi_ss_enable_fdn_test(int slot_id, bool enable, char* passwd);
int tapi_ss_query_fdn_test(int slot_id, bool expect);

#endif /* TELEPHONY_SS_TEST_H_ */
