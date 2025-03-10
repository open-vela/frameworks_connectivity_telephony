#ifndef TELEPHONY_IMS_TEST_H_
#define TELEPHONY_IMS_TEST_H_

#include "telephony_test.h"

int setup_ims(void** state);
int teardown_ims(void** state);
int teardown_imsAndRadio(void** state);
int tapi_ims_listen_ims_test(int slot_id);
int tapi_ims_unlisten_ims_test(void);
int tapi_ims_turn_on_test(int slot_id);
int tapi_ims_turn_off_test(int slot_id);
int tapi_ims_get_registration_test(int slot_id);
int tapi_ims_get_enabled_test(int slot_id);
int tapi_ims_set_service_status_test(int slot_id, int status);
int ims_turn_on_test(int slot_id);
int ims_turn_off_test(int slot_id);
int ims_is_reg_after_radio_off_on_test(int slot_id, bool expect_reg_status);
int ims_is_volte_available_after_radio_off_on_test(int slot_id, bool expect_volte_avail);
#endif
