#ifndef TELEPHONY_SMS_TEST_H_
#define TELEPHONY_SMS_TEST_H_

#include "telephony_test.h"

int setup_sms(void** state);
int setup_sms_and_call(void** state);
int teardown_sms(void** state);
int teardown_sms_and_call(void** state);
int sms_listen_sms_test(int slot_id);
int sms_unlisten_sms_test(int slot_id);
int sms_receive_chinese_long_message_test(int slot_id);
int sms_receive_english_long_message_test(int slot_id);
int sms_receive_message_test(int slot_id);
int sms_receive_report_test(int slot_id, char* to, int port, char* text);
int sms_send_data_message_test(int slot_id, char* to, int port, char* text);
int sms_send_message_test(int slot_id, char* number, char* text, int* result);
int sms_send_data_message_in_dialing(int slot_id, char* to, char* text, int port);
int sms_send_data_message_in_special_ims_cap(int slot_id, char* to, int port, char* text, int ims_cap);
int sms_send_message_fail_in_airplane_test(int slot_id, char* to, char* text);
int sms_send_message_in_dialing(int slot_id, char* to, char* text);
int sms_send_message_in_special_ims_cap(int slot_id, char* to, char* text, int ims_cap);
int sms_send_short_data_sms_continuous(int slot_id, char* to);
int sms_send_short_mix_sms_continuous(int slot_id, char* to);
int sms_send_short_sms_continuous(int slot_id, char* to);
int sms_set_and_get_cell_broadcast_power(int slot_id, bool enable);
int sms_set_and_get_cell_broadcast_topics(int slot_id, char* topics);
int sms_set_and_get_service_center_number_test(int slot_id);

#endif /* TELEPHONY_SMS_TEST_H_ */
