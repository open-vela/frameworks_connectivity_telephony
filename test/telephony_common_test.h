#ifndef TELEPHONY_COMMON_TEST_H_
#define TELEPHONY_COMMON_TEST_H_
#define MAX_INPUT_ARGS_LEN 128

#include "telephony_test.h"

int setup_modem(void** state);
int teardown_modem(void** state);
int enable_modem_test(int slot_id, int target_state);
int get_imei_test(int slot_id);
int get_modem_activity_info_test(int slot_id);
int get_modem_revision_test(int slot_id);
int get_modem_status_test(int slot_id, int* state);
int get_phone_state_test(int slot_id, tapi_phone_state target);
int get_pref_net_mode_test(int slot_id, tapi_pref_net_mode* value);
int get_radio_power_test(int slot_id, bool* value);
int modem_disable_power_off_pending_test(int slot_id);
int modem_enable_disable_pending_test(int slot_id);
int modem_enable_status_test(int slot_id);
int modem_disable_status_test(int slot_id);
int modem_invoke_oem_ril_request_raw_test(int slot_id, char* oem_req, int length);
int modem_invoke_oem_ril_request_strings_test(int slot_id, char* req_data, int length);
int modem_register_test(int slot_id);
int modem_reset_test(int slot_id);
int modem_unregister_test(void);
int radio_power_on_modem_disable_pending_test(int slot_id);
int radio_power_on_off_pending_test(int slot_id);
int set_pref_net_mode_test(int slot_id, tapi_pref_net_mode target_state);
int set_radio_power_test(int slot_id, bool target_state);
#endif
