#ifndef TELEPHONY_SMS_TEST_H_
#define TELEPHONY_SMS_TEST_H_

#include "telephony_test.h"

int sms_send_message_test(tapi_context context, int slot_id, char* number, char* text);

int sms_set_service_center_number_test(int slot_id);
int sms_check_service_center_number_test(int slot_id);
int sms_send_data_message_test(int slot_id, char* to, int port, char* text);
int sms_send_message_in_dialing(int slot_id, char* to, char* text);
int sms_send_data_message_in_dialing(int slot_id, char* to, char* text, int port);

#endif /* TELEPHONY_SMS_TEST_H_ */
