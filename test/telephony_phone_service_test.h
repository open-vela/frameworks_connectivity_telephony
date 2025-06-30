#include "telephony_test.h"
#include "uv.h"
#include <tapi_phone.h>

typedef struct {
    uv_async_t async;
    int param;
    int param1;
    char str1[50];
    char cmd[50];
} async_message_t;

typedef struct {
    char* cmd;
    char* str1;
    int param;
    int param1;
} exec_data_t;

typedef struct {
    const char* cmd;
    void (*func_cb)(exec_data_t parms);
} exec_cmds_t;

#ifdef CONFIG_PHONE_SERVICE
void uv_async_callback(uv_async_t* handle);
#ifdef CONFIG_PHONE_SERVICE_WTP
int phone_service_register_wtp_cb_test(async_message_t* msg);
void phone_service_register_wtp_cb_exec(exec_data_t parms);
int phone_service_unregister_wtp_cb_test(async_message_t* msg;
void phone_service_unregister_wtp_cb_exec(exec_data_t parms);
int phone_service_update_local_info_test(async_message_t* msg);
void phone_service_update_local_info_exec(exec_data_t parms);
int phone_service_set_discovery_test(async_message_t* msg, int value);
void phone_service_set_discovery_exec(exec_data_t parms);
int phone_service_set_visibility_test(async_message_t* msg, int value);
void phone_service_set_visibility_exec(exec_data_t parms);
int phone_service_set_audio_test(async_message_t* msg, int value);
void phone_service_set_audio_exec(exec_data_t parms);
int phone_service_dial_wtp_test(async_message_t* msg);
void phone_service_dial_wtp_exec(exec_data_t parms);
int phone_service_hangup_wtp_test(async_message_t* msg);
void phone_service_hangup_wtp_exec(exec_data_t parms);
int phone_service_answer_wtp_test(async_message_t* msg);
void phone_service_answer_wtp_exec(exec_data_t parms);
int phone_service_reject_wtp_test(async_message_t* msg);
void phone_service_reject_wtp_exec(exec_data_t parms);
#endif
int phone_service_dial_and_hangup_esim_test(async_message_t* msg, char* number);
void phone_service_dial_esim_exec(exec_data_t parms);
void phone_service_hangup_esim_exec(exec_data_t parms);
int phone_service_incoming_answer_and_hangup_esim_test(async_message_t* msg);
void phone_service_incoming_esim_exec(exec_data_t parms);
void phone_service_answer_esim_exec(exec_data_t parms);
int phone_service_incoming_and_reject_esim_test(async_message_t* msg);
void phone_service_reject_esim_exec(exec_data_t parms);
int phone_service_release_and_answer_esim_test(async_message_t* msg);
void phone_service_hold_and_answer_esim_exec(exec_data_t parms);
void phone_service_release_and_answer_esim_exec(exec_data_t parms);
int phone_service_hold_and_unhold_esim_test(async_message_t* msg);
void phone_service_hold_and_unhold_esim_exec(exec_data_t parms);
int phone_service_merge_esim_test(async_message_t* msg);
void phone_service_merge_esim_exec(exec_data_t parms);
int phone_service_send_tones_esim_test(async_message_t* msg);
void phone_service_send_tones_esim_exec(exec_data_t parms);
int set_phone_radio_power_test(async_message_t* msg, int target_state);
void phone_service_set_radiopower_exec(exec_data_t parms);
int phone_service_register_esim_cb_test(async_message_t* msg);
void phone_service_register_esim_cb_exec(exec_data_t parms);
int phone_service_unregister_esim_cb_test(async_message_t* msg);
void phone_service_unregister_esim_cb_exec(exec_data_t parms);
#endif