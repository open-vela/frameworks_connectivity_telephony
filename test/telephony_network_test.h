#ifndef TELEPHONY_NETWORK_TEST_H_
#define TELEPHONY_NETWORK_TEST_H_

#include "telephony_test.h"

int net_get_neighbouring_cellInfos_test(int slot_id);
int net_get_operator_name_test(int slot_id);
int net_get_serving_cellinfos_test(int slot_id);
int net_get_voice_networktype_test(int slot_id, tapi_network_type* type);
int net_get_voice_registered_test(int slot_id);
int net_query_signalstrength_test(int slot_id);
int net_registration_info_test(int slot_id);
int net_scan_test(int slot_id);
int net_select_auto_test(int slot_id);
int net_select_manual_test(int slot_id, char* mcc, char* mnc, char* tech);

#endif /* TELEPHONY_NETWORK_TEST_H_ */
