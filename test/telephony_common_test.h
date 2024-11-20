#ifndef TELEPHONY_COMMON_TEST_H_
#define TELEPHONY_COMMON_TEST_H_
#define MAX_INPUT_ARGS_LEN 128

#include "telephony_test.h"

int tapi_get_imei_test(int slot_id);
int tapi_get_modem_revision_test(int slot_id);
int tapi_get_modem_activity_info_test(int slot_id);
int tapi_get_pref_net_mode_test(int slot_id, tapi_pref_net_mode* value);
int tapi_set_radio_power_test(int slot_id, bool target_state);
int tapi_get_radio_power_test(int slot_id, bool* value);
int tapi_modem_register_test(int slot_id);
int tapi_modem_unregister_test(void);
int tapi_invoke_oem_ril_request_raw_test(int slot_id, char* oem_req, int length);
int tapi_invoke_oem_ril_request_strings_test(int slot_id, char* req_data, int length);
int tapi_enable_modem_test(int slot_id, int target_state);
int tapi_get_modem_status_test(int slot_id, int* state);
int tapi_set_pref_net_mode_test(int slot_id, tapi_pref_net_mode target_state);

#endif