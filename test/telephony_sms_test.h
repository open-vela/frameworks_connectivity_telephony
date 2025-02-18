#ifndef TELEPHONY_SMS_TEST_H_
#define TELEPHONY_SMS_TEST_H_

#include "telephony_test.h"

int tapi_sms_send_data_message_test(int slot_id, char* to, int port, char* text);
int tapi_sms_send_message_test(int slot_id, char* number, char* text);
int sms_send_data_message_in_dialing(int slot_id, char* to, char* text, int port);
int sms_send_data_message_in_special_ims_cap(int slot_id, char* to, int port, char* text, int ims_cap);
int sms_send_message_in_dialing(int slot_id, char* to, char* text);
int sms_send_message_in_special_ims_cap(int slot_id, char* to, char* text, int ims_cap);
int sms_send_short_data_sms_continuous(int slot_id, char* to);
int sms_send_short_mix_sms_continuous(int slot_id, char* to);
int sms_send_short_sms_continuous(int slot_id, char* to);
int sms_set_and_get_cell_broadcast_power(int slot_id, bool enable);
int sms_set_and_get_cell_broadcast_topics(int slot_id, char* topics);
int sms_set_and_get_service_center_number_test(int slot_id);

#endif /* TELEPHONY_SMS_TEST_H_ */
