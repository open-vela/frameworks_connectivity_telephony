#ifndef TELEPHONY_IMS_TEST_H_
#define TELEPHONY_IMS_TEST_H_

#include "telephony_test.h"

int tapi_ims_listen_ims_test(int slot_id);
int tapi_ims_turn_on_test(int slot_id);
int tapi_ims_turn_off_test(int slot_id);
int tapi_ims_get_registration_test(int slot_id);
int tapi_ims_get_enabled_test(int slot_id);
int tapi_ims_set_service_status_test(int slot_id, int status);

#endif