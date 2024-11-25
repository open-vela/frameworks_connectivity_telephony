#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sched.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>

#include <cmocka.h>

#include "telephony_call_test.h"
#include "telephony_common_test.h"
#include "telephony_data_test.h"
#include "telephony_ims_test.h"
#include "telephony_network_test.h"
#include "telephony_sim_test.h"
#include "telephony_sms_test.h"
#include "telephony_ss_test.h"
#include "telephony_test.h"

#define REPEAT_TEST_MORE_FOR for (int _i = 0; _i < 10; _i++)
#define REPEAT_TEST_LESS_FOR for (int _i = 0; _i < 3; _i++)

char* phone_num = NULL;
static uv_async_t g_uv_exit;
static int ready_done;
tapi_context context = NULL;
static int count = 0;
typedef enum {
    CASE_NORMAL_MODE = 0,
    CASE_AIRPLANE_MODE = 1,
    CASE_CALL_DIALING = 2,
    CASE_MODEM_POWEROFF = 3,
} case_type;

struct judge_type judge_data;

char* short_english_text = "test";
char* long_english_text = "testtesttesttesttesttesttesttesttesttesttesttest"
                          "testtesttesttesttesttesttesttesttesttesttesttesttesttest"
                          "testtesttesttesttesttesttesttesttesttesttesttesttesttesttest"
                          "testtesttesttesttesttesttesttesttesttesttesttesttesttesttest"
                          "testtesttesttesttesttesttesttesttesttesttesttesttesttesttest"
                          "testtesttesttesttesttesttesttesttesttesttesttesttesttesttest"
                          "testtesttesttesttesttesttesttesttesttesttesttesttesttesttest"
                          "testtesttesttesttesttesttesttesttesttesttesttesttesttesttest"
                          "testtesttest";
char* short_chinese_text = "测试";
char* long_chinese_text = "测试测试测试测试测试测试测试测试测试测试"
                          "测试测试测试测试测试测试测试测试测试测试"
                          "测试测试测试测试测试测试测试测试测试测试"
                          "测试测试测试测试测试测试测试测试测试测试"
                          "测试测试测试测试测试测试测试测试测试测试"
                          "测试测试测试测试测试测试测试测试测试测试"
                          "测试测试测试测试测试测试测试测试测试测试"
                          "测试测试测试测试测试测试测试测试测试测试"
                          "测试测试测试测试测试测试测试测试测试测试";

static void exit_async_cleanup(uv_async_t* handle)
{
    tapi_close(context);
    context = NULL;
}

tapi_context get_tapi_ctx(void)
{
    return context;
}

int judge(void)
{
    int timeout = TIMEOUT;

    while (timeout-- > 0) {

        if (judge_data.flag == judge_data.expect) {
            if (judge_data.result != 0)
                syslog(LOG_ERR, "result error\n");
            else
                syslog(LOG_INFO, "result correct\n");

            return 0;
        }

        sleep(1);
        syslog(LOG_INFO, "There is %d second(s) remain.\n", timeout);
    }

    syslog(LOG_ERR, "judge timeout\n");
    assert(0);
    return -ETIME;
}

void judge_data_init(void)
{
    judge_data.flag = INVALID_VALUE;
    judge_data.expect = INVALID_VALUE;
    judge_data.result = INVALID_VALUE;
}

static void TestNuttxHasIccCard(void** state)
{
    (void)state;
    int ret = tapi_sim_has_icc_card_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxHasIccCardNumerousTimes(void** state)
{
    (void)state;
    int ret = tapi_sim_multi_has_icc_card_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxGetSimOperator(void** state)
{
    (void)state;
    int ret = tapi_sim_get_sim_operator_test(0, "310260");
    assert_int_equal(ret, OK);
}

static void TestNuttxGetSimOperatorName(void** state)
{
    (void)state;
    int ret = tapi_sim_get_sim_operator_name_test(0, "T-Mobile");
    assert_int_equal(ret, OK);
}

static void TestNuttxGetSimOperatorNameNumerousTimes(void** state)
{
    (void)state;
    int ret = tapi_sim_get_sim_operator_name_numerous(0, "T-Mobile");
    assert_int_equal(ret, OK);
}

static void TestNuttxGetSimOperatorNumerousTimes(void** state)
{
    (void)state;
    int ret = tapi_sim_multi_get_sim_operator(0, "310260");
    assert_int_equal(ret, OK);
}

static void TestNuttxGetSimSubscriberId(void** state)
{
    (void)state;
    int ret = tapi_sim_get_sim_subscriber_id_test(0, "310260000000000");
    assert_int_equal(ret, OK);
}

static void TestNuttxGetSimSubscriberIdNumerousTimes(void** state)
{
    (void)state;
    int ret = tapi_sim_multi_get_sim_subscriber_id_test(0, "310260000000000");
    assert_int_equal(ret, OK);
}

static void TestNuttxGetIccId(void** state)
{
    (void)state;
    int ret = tapi_sim_get_sim_iccid_test(0, "89860318640220133897");
    assert_int_equal(ret, OK);
}

static void TestNuttxGetIccIdNumerousTimes(void** state)
{
    (void)state;
    int ret = tapi_sim_multi_get_sim_iccid_test(0, "89860318640220133897");
    assert_int_equal(ret, OK);
}

static void TestNuttxGetMSISDN(void** state)
{
    (void)state;
    int ret = tapi_sim_get_ef_msisdn_test(0, "+15551234567");
    assert_int_equal(ret, OK);
}

static void TestNuttxGetMSISDNNumerousTimes(void** state)
{
    (void)state;
    int ret = tapi_sim_multi_get_ef_msisdn_test(0, "+15551234567");
    assert_int_equal(ret, OK);
}

static void TestNuttxTransmitAPDUInBasicChannel(void** state)
{
    (void)state;
    int ret = tapi_transmit_apdu_basic_channel_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxGetSimState(void** state)
{
    (void)state;
    int ret = tapi_sim_get_state_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxOpenLogicalChannel(void** state)
{
    (void)state;
    int ret = tapi_open_logical_channel_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxCloseLogicalChannel(void** state)
{
    (void)state;
    int ret = tapi_close_logical_channel_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxLogicalChannelOpenCloseNumerous(void** state)
{
    (void)state;
    int ret = sim_open_close_logical_channel_numerous(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxTransmitAPDUInLogicalChannel(void** state)
{
    int ret = sim_transmit_apdu_by_logical_channel(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxSetUiccEnablement(void** state)
{
    (void)state;
    int ret = tapi_sim_set_uicc_enablement_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxGetUiccEnablement(void** state)
{
    (void)state;
    int ret = tapi_sim_get_uicc_enablement_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxTransmitAPDUBasicChannel(void** state)
{
    (void)state;
    int ret = tapi_transmit_apdu_basic_channel_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxEnterPin(void** state)
{
    (void)state;
    int ret = tapi_sim_enter_pin_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxChangePin(void** state)
{
    (void)state;
    int ret = sim_change_pin_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxLockPin(void** state)
{
    (void)state;
    int ret = tapi_sim_lock_pin_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxUnlockPin(void** state)
{
    (void)state;
    int ret = tapi_sim_unlock_pin_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxLoadAdnEntries(void** state)
{
    (void)state;
    int ret = tapi_phonebook_load_adn_entries_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxLoadFdnEntries(void** state)
{
    (void)state;
    int ret = tapi_phonebook_load_fdn_entries_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxInsertFdnEntry(void** state)
{
    (void)state;
    int ret = tapi_phonebook_insert_fdn_entry_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxUpdateFdnEntry(void** state)
{
    (void)state;
    int ret = tapi_phonebook_update_fdn_entry_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDeleteFdnEntry(void** state)
{
    (void)state;
    int ret = tapi_phonebook_delete_fdn_entry_test(0);
    assert_int_equal(ret, OK);
}

// static void cmocka_tapi_sim_listen_sim_state_change(void **state)
// {
//     int ret = tapi_sim_listen_sim_test(0, 23);
//     assert_int_equal(ret, OK);
// }

// static void cmocka_tapi_sim_listen_sim_uicc_app_enabled_change(void **state)
// {
//     int ret = tapi_sim_listen_sim_test(0, 24);
//     assert_int_equal(ret, OK);
// }

// static void TestNuttxLoadEccList(void** state)
// {
//     (void)state;
//     int ret = tapi_call_load_ecc_list_test(0);
//     assert_int_equal(ret, 0);
// }

static void TestNuttxSetVoicecallSlot(void** state)
{
    (void)state;
    int ret = tapi_call_set_default_voicecall_slot_test(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxGetVoicecallSlot(void** state)
{
    (void)state;
    int ret = tapi_call_get_default_voicecall_slot_test(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxClearVoicecallSlotSet(void** state)
{
    (void)state;
    int ret = call_clear_voicecall_slot_set();
    assert_int_equal(ret, 0);
}

static void TestNuttxListenCall(void** state)
{
    (void)state;
    int ret = tapi_call_listen_call_test(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxUnlistenCall(void** state)
{
    (void)state;
    int ret = tapi_call_unlisten_call_test();
    assert_int_equal(ret, 0);
}

static void TestNuttxStartDtmf(void** state)
{
    (void)state;
    int ret = tapi_start_dtmf_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxStopDtmf(void** state)
{
    (void)state;
    int ret = tapi_stop_dtmf_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxHangupBetweenDialingAndAnswering(void** state)
{
    (void)state;
    sleep(2);
    int ret = call_hangup_between_dialing_and_answering(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxIncomingCallAnswerAndHangup(void** state)
{
    (void)state;
    int ret = incoming_call_answer_and_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxIncomingCallAnswerAndRemoteHangup(void** state)
{
    (void)state;
    int ret = incoming_call_answer_and_remote_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxCallHoldAndAnswerOtherCall(void** state)
{
    (void)state;
    int ret = call_hold_first_call_and_answer_second_call(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxCallMergeByUser(void** state)
{
    (void)state;
    int ret = call_merge_by_user(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxCallSeparateByUser(void** state)
{
    (void)state;
    int ret = call_separate_by_user(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxCallReleaseAndSwapOtherCall(void** state)
{
    (void)state;
    int ret = call_release_and_swap_other_call(0);
    assert_int_equal(ret, 0);
}

// static void TestNuttxDialAgainAfterRemoteHangup(void **state) {
//     int ret = call_dial_after_caller_reject(0);
//     assert_int_equal(ret, 0);
// }

// static void TestNuttxHangupCallAfterRemoteAnswer(void **state) {
//     int ret = call_hangup_after_caller_answer(0);
//     assert_int_equal(ret, 0);
// }

static void TestNuttxOutgoingCallRemoteAnswerAndHangup(void** state)
{
    int ret = outgoing_call_remote_answer_and_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxOutgoingCallHoldAndHangup(void** state)
{
    (void)state;
    int ret = outgoing_call_hold_and_unhold_by_caller(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxOutgoingCallActiveAndSendtones(void** state)
{
    (void)state;
    int ret = outgoing_call_active_and_send_tones(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxCallConnectAndLocalHangup(void** state)
{
    int ret = call_connect_and_local_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxDialAndRemoteHangup(void** state)
{
    int ret = dial_and_remote_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxIncomingCallandLocalHangup(void** state)
{
    (void)state;
    int ret = incoming_call_and_local_hangup(0);
    assert_int_equal(ret, 0);
}

// static void TestNuttxRemoteHangupThenIncomingNewCall(void **state) {
//     int ret = call_dial_caller_reject_and_incoming(0);
//     assert_int_equal(ret, 0);
// }

// static void TestNuttxRemoteHangupThenDialAnother(void **state) {
//     int ret = call_dial_caller_reject_and_dial_another(0);
//     assert_int_equal(ret, 0);
// }

static void TestNuttxHangupCurrentCall(void** state)
{
    (void)state;
    int ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxHangupAllCall(void** state)
{
    (void)state;
    int ret = tapi_call_hangup_all_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxGetCallCount(void** state)
{
    (void)state;
    int ret = tapi_get_call_count(0);
    assert_true(ret >= 0);
}

static void TestNuttxDialNumber(void** state)
{
    (void)state;
    int ret = tapi_dial_number(0);
    assert_int_equal(ret, OK);
    sleep(2);
}

static void TestNuttxDialEccNumber(void** state)
{
    (void)state;
    int ret = tapi_dial_ecc_number(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDialLongPhoneNumber(void** state)
{
    (void)state;
    int ret = tapi_dial_with_long_phone_number(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDialShotPhoneNumber(void** state)
{
    (void)state;
    int ret = tapi_dial_with_short_phone_number(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDialWithEnableHideCallId(void** state)
{
    (void)state;
    int ret = tapi_dial_with_enable_hide_callerid(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDialWithDisabledHideCallId(void** state)
{
    (void)state;
    int ret = tapi_dial_with_disabled_hide_callerid(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDialWithDefaultHideCallId(void** state)
{
    (void)state;
    int ret = tapi_dial_with_default_hide_callerid(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

// static void TestNuttxIncomingNewCallThenRemoteHangup(void **state) {
//     int ret = call_incoming_and_hangup_by_dialer_before_answer(0);
//     assert_int_equal(ret, 0);
// }

static void TestNuttxDialWithAreaCode(void** state)
{
    (void)state;
    int ret = tapi_call_dial_using_phone_number_with_area_code_test(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDialWithPauseCode(void** state)
{
    (void)state;
    int ret = tapi_call_dial_using_phone_number_with_pause_code_test(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDialWithWaitCode(void** state)
{
    (void)state;
    int ret = tapi_call_dial_using_phone_number_with_wait_code_test(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDialWithNumerousCode(void** state)
{
    (void)state;
    int ret = tapi_call_dial_using_phone_number_with_numerous_code_test(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDialConference(void** state)
{
    (void)state;
    int ret = tapi_call_dial_conference_test(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hangup_all_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttCheckAleringStatusAfterDial(void** state)
{
    (void)state;
    int ret = call_check_alerting_status_after_dial(0);
    assert_int_equal(ret, OK);
}

// static void TestNuttxCheckStatusInDialing(void **state)
// {
//     int ret = call_check_status_in_dialing(0);
//     assert_int_equal(ret, 0);
// }

// data testcases
static void TestNuttxDataLoadApnContexts(void** state)
{
    (void)state;
    int ret = tapi_data_load_apn_contexts_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDataSaveApnContextSupl(void** state)
{
    (void)state;
    int ret = tapi_data_save_apn_context_test("0", "3", "supl", "supl", "2", "2");
    assert_int_equal(ret, OK);
}

static void TestNuttxDataSaveApnContextEmergency(void** state)
{
    (void)state;
    int ret = tapi_data_save_apn_context_test("0", "7", "emergency",
        "emergency", "2", "2");
    assert_int_equal(ret, OK);
}

static void TestNuttxDataSaveLongApnContex(void** state)
{
    (void)state;
    int ret = tapi_data_save_apn_context_test("0", "1",
        "longname-----------------------------------------"
        "-----------------------------------------longname",
        "cmnet4", "2", "2");
    assert_int_equal(ret, OK);
}

static void TestNuttxDataSaveApnContext(void** state)
{
    (void)state;
    int ret = tapi_data_save_apn_context_test("0", "1", "cmcc1", "cmnet1", "2", "2");
    assert_int_equal(ret, 0);
}

static void TestNuttxDataRemoveApnContext(void** state)
{
    (void)state;
    int ret = tapi_data_remove_apn_context_test("0", "/ril_0/context3");
    assert_int_equal(ret, OK);
}

static void TestNuttxDataResetApnContexts(void** state)
{
    (void)state;
    int ret = tapi_data_reset_apn_contexts_test("0");
    assert_int_equal(ret, OK);
}

static void TestNuttxDataResetApnContextsRepeatedly(void** state)
{
    (void)state;
    REPEAT_TEST_MORE_FOR
    {
        int ret = tapi_data_reset_apn_contexts_test("0");
        assert_int_equal(ret, OK);
    }
}

static void TestNuttxDataEditApnName(void** state)
{
    (void)state;
    int ret;
    ret = tapi_data_save_apn_context_test("0", "1", "cmcc1", "cmnet1", "2", "2");
    assert_int_equal(ret, OK);
    ret = tapi_data_edit_apn_context_test("0", "/ril_0/context3", "1", "cmname", "cmname", "2", "2");
    assert_int_equal(ret, OK);
    ret = tapi_data_load_apn_contexts_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDataEditApnType(void** state)
{
    (void)state;
    int ret = tapi_data_edit_apn_context_test("0", "/ril_0/context3", "3", "cmname", "cmname", "2", "2");
    assert_int_equal(ret, OK);
}

static void TestNuttxDataEditApnProto(void** state)
{
    (void)state;
    int ret = tapi_data_edit_apn_context_test("0", "/ril_0/context3", "3", "cmname", "cmname", "0", "2");
    assert_int_equal(ret, OK);
}

static void TestNuttxDataEditApnAuth(void** state)
{
    (void)state;
    int ret = tapi_data_edit_apn_context_test("0", "/ril_0/context3", "3", "cmname", "cmname", "0", "0");
    assert_int_equal(ret, OK);
}

static void TestNuttxDataEditApnAll(void** state)
{
    (void)state;
    int ret;
    ret = tapi_data_edit_apn_context_test("0", "/ril_0/context3", "2", "cmnameall", "cmnameall", "1", "1");
    assert_int_equal(ret, OK);
    ret = tapi_data_load_apn_contexts_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDataEditApnAndRemove(void** state)
{
    (void)state;
    int ret;
    ret = tapi_data_edit_apn_context_test("0", "/ril_0/context3", "2", "cmnameall", "cmnameall", "2", "2");
    assert_int_equal(ret, OK);
    ret = tapi_data_remove_apn_context_test("0", "/ril_0/context3");
    assert_int_equal(ret, OK);
}

static void TestNuttxDataEditApnAndReset(void** state)
{
    (void)state;
    int ret;
    ret = tapi_data_save_apn_context_test("0", "1", "cmcc1", "cmnet1", "2", "2");
    assert_int_equal(ret, OK);
    ret = tapi_data_edit_apn_context_test("0", "/ril_0/context3", "1", "cmname", "cmname", "2", "2");
    assert_int_equal(ret, OK);
    ret = tapi_data_reset_apn_contexts_test("0");
    assert_int_equal(ret, OK);
}

static void TestNuttxDataEditApnRepeatedlyAndLoad(void** state)
{
    (void)state;
    int ret;
    ret = tapi_data_save_apn_context_test("0", "1", "cmcc1", "cmnet1", "2", "2");
    assert_int_equal(ret, OK);
    ret = tapi_data_edit_apn_context_test("0", "/ril_0/context1", "1", "cmname11", "cmname", "2", "2");
    assert_int_equal(ret, OK);
    ret = tapi_data_edit_apn_context_test("0", "/ril_0/context1", "1", "cmname22", "cmname", "2", "2");
    assert_int_equal(ret, OK);
    ret = tapi_data_edit_apn_context_test("0", "/ril_0/context1", "1", "cmname33", "cmname", "2", "2");
    assert_int_equal(ret, OK);
    ret = tapi_data_load_apn_contexts_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDataEnable(void** state)
{
    (void)state;
    int ret = data_enabled_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDataDisable(void** state)
{
    (void)state;
    int ret = data_disabled_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDataEnableRepeatedly(void** state)
{
    (void)state;
    REPEAT_TEST_MORE_FOR
    {
        TestNuttxDataEnable(state);
        TestNuttxDataDisable(state);
    }
}

static void TestNuttxDataIsEnable(void** state)
{
    (void)state;
    bool enable = false;
    int ret = tapi_data_get_enabled_test(&enable);
    assert_int_equal(ret, OK);
    assert_int_equal(enable, 1);
}

static void TestNuttxDataIsDisable(void** state)
{
    (void)state;
    bool enable = true;
    int ret = tapi_data_get_enabled_test(&enable);
    assert_int_equal(ret, OK);
    assert_int_equal(enable, 0);
}

static void TestNuttxDataRegister(void** state)
{
    (void)state;
    int ret = tapi_data_listen_data_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDataUnregister(void** state)
{
    (void)state;
    int ret = tapi_data_unlisten_data_test();
    assert_int_equal(ret, OK);
}

static void TestNuttxDataRequestNetworkInternet(void** state)
{
    (void)state;
    int ret = data_request_network_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDataReleaseNetworkInternet(void** state)
{
    (void)state;
    int ret = data_release_network_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDataReleaseNetworkInternetRepeatedly(void** state)
{
    (void)state;
    REPEAT_TEST_MORE_FOR
    {
        TestNuttxDataReleaseNetworkInternet(state);
        TestNuttxDataRequestNetworkInternet(state);
    }
}

static void TestNuttxDataRequestNetworkIms(void** state)
{
    (void)state;
    int ret = data_request_ims(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDataReleaseNetworkIms(void** state)
{
    (void)state;
    int ret = data_release_ims(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDataReleaseNetworkImsRepeatedly(void** state)
{
    (void)state;
    REPEAT_TEST_MORE_FOR
    {
        TestNuttxDataRequestNetworkIms(state);
        TestNuttxDataReleaseNetworkIms(state);
    }
}

static void TestNuttxDataSetPreferredApn(void** state)
{
    (void)state;
    int ret = tapi_data_set_preferred_apn_test(0, "/ril_0/context1");
    assert_int_equal(ret, OK);
}

static void TestNuttxDataGetPreferredApn(void** state)
{
    (void)state;
    int ret = tapi_data_get_preferred_apn_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDataSendScreenState(void** state)
{
    (void)state;
    int ret = tapi_data_send_screen_stat_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDataIsPsAttached(void** state)
{
    (void)state;
    int ret = tapi_data_is_ps_attached_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDataGetNetworkType(void** state)
{
    (void)state;
    int ret = tapi_data_get_network_type_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDataSetDefaultDataSlot(void** state)
{
    (void)state;
    int ret = tapi_data_set_default_data_slot_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDataGetDefaultDataSlot(void** state)
{
    (void)state;
    sleep(2);
    int ret = tapi_data_get_default_data_slot_test();
    assert_int_equal(ret, OK);
}

static void TestNuttxDataSetDataAllow(void** state)
{
    (void)state;
    int ret = tapi_data_set_data_allow_test(0);
    assert_true(ret == OK);
}

static void TestNuttxDataGetCallList(void** state)
{
    (void)state;
    int ret = data_get_call_list(0);
    assert_true(ret == OK);
}

static void TestNuttxEnableDataRoaming(void** state)
{
    (void)state;
    int ret = data_enable_roaming_test();
    assert_true(ret == OK);
}

static void TestNuttxDisableDataRoaming(void** state)
{
    (void)state;
    int ret = data_disable_roaming_test();
    assert_true(ret == OK);
}

static void TestNuttxSmsSetServiceCenterNum(void** state)
{
    (void)state;
    int ret = sms_set_service_center_number_test(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxSmsGetServiceCenterNum(void** state)
{
    (void)state;
    sleep(5);
    int ret = sms_check_service_center_number_test(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxSmsSendShortMessageInEnglish(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSmsSendShortMessageInChinese(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSmsSendShortDataMessageInEnglish(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSmsSendShortDataMessageInChinese(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSmsSendLongMessageInEnglish(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSmsSendLongMessageInChinese(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSmsSendLongDataMessageInEnglish(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSmsSendLongDataMessageInChinese(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSmsSendShortEnglishMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_message_in_dialing(0, phone_num, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSmsSendShortChineseMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_message_in_dialing(0, phone_num, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSmsSendLongEnglishMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_message_in_dialing(0, phone_num, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSmsSendLongChineseMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_message_in_dialing(0, phone_num, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSmsSendShortEnglishDataMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_dialing(0, phone_num, short_english_text, 0);
    assert_int_equal(ret, 0);
}

static void TestNuttxSmsSendShortChineseDataMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_dialing(0, phone_num, short_chinese_text, 0);
    assert_int_equal(ret, 0);
}

static void TestNuttxSmsSendLongEnglishDataMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_dialing(0, phone_num, long_english_text, 0);
    assert_int_equal(ret, 0);
}

static void TestNuttxSmsSendLongChineseDataMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_dialing(0, phone_num, long_chinese_text, 0);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendEnglishMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendChineseMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendLongEnglishMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendLongChineseMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendEnglishDataMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendChineseDataMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendLongEnglishDataMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendLongChineseDataMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendEnglishMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendChineseMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendLongEnglishMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendLongChineseMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendEnglishDataMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendChineseDataMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendLongEnglishDataMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendLongChineseDataMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendEnglishMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendChineseMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendLongEnglishMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendLongChineseMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendEnglishDataMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendChineseDataMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendLongEnglishDataMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSendLongChineseDataMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestNuttxSetSmsDefaultSlot(void** state)
{
    (void)state;
    int ret = tapi_sms_set_default_slot(get_tapi_ctx(), 0);
    sleep(5);
    assert_int_equal(ret, 0);
}

static void TestNuttxGetSmsDefaultSlot(void** state)
{
    (void)state;
    int result = -1;
    int ret = tapi_sms_get_default_slot(context, &result);
    syslog(LOG_INFO, "%s, ret: %d, result: %d", __func__, ret, result);
    assert_int_equal(ret, 0);
    assert_int_equal(result, 0);
}

static void TestNuttxSmsSetCellBroadcastPower(void** state)
{
    (void)state;
    int ret = tapi_sms_set_cell_broadcast_power_on(get_tapi_ctx(), 0, 1);
    sleep(5);
    assert_int_equal(ret, 0);
}

static void TestNuttxSmsGetCellBroadcastPower(void** state)
{
    (void)state;
    bool result = false;
    int ret = tapi_sms_get_cell_broadcast_power_on(get_tapi_ctx(), 0, &result);
    syslog(LOG_INFO, "%s, ret: %d, result: %d", __func__, 0, (int)result);
    assert_int_equal(ret, 0);
    assert_int_equal(result, 1);
}

static void TestNuttxSmsSetCellBroadcastTopics(void** state)
{
    (void)state;
    int ret = tapi_sms_set_cell_broadcast_topics(get_tapi_ctx(), 0, "1");
    sleep(5);
    assert_int_equal(ret, 0);
}

static void TestNuttxSmsGetCellBroadcastTopics(void** state)
{
    (void)state;
    char* result = NULL;
    int ret = tapi_sms_get_cell_broadcast_topics(get_tapi_ctx(), 0, &result);
    assert_int_equal(ret, 0);
    assert_int_equal(strcmp(result, "1"), 0);
}

static void TestNuttxNetSelectAuto(void** state)
{
    (void)state;
    int ret = tapi_net_select_auto_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxNetSelectManual(void** state)
{
    (void)state;
    sleep(4);
    int ret = tapi_net_select_manual_test(0, "310", "260", "lte");
    sleep(4);
    assert_int_equal(ret, OK);
}

static void TestNuttxNetScan(void** state)
{
    (void)state;
    int ret = tapi_net_scan_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxNetGetServingCellinfos(void** state)
{
    (void)state;
    int ret = tapi_net_get_serving_cellinfos_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxNetGetNeighbouringCellInfos(void** state)
{
    (void)state;
    int ret = tapi_net_get_neighbouring_cellInfos_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxNetRegistrationInfo(void** state)
{
    (void)state;
    int ret = tapi_net_registration_info_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxNetGetOperatorName(void** state)
{
    (void)state;
    int ret = tapi_net_get_operator_name_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxNetQuerySignalstrength(void** state)
{
    (void)state;
    int ret = tapi_net_query_signalstrength_test(0);
    assert_int_equal(ret, OK);
}

// static void TestNuttxNetSetCellInfoListRate(void **state)
// {
//     int ret = tapi_net_set_cell_info_list_rate_test(0, 10);
//     assert_int_equal(ret, OK);
// }

static void TestNuttxNetGetVoiceRegistered(void** state)
{
    (void)state;
    int ret = tapi_net_get_voice_registered_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxGetVoiceNwType(void** state)
{
    (void)state;
    tapi_network_type type = NETWORK_TYPE_UNKNOWN;
    int ret = tapi_network_get_voice_network_type(get_tapi_ctx(), 0, &type);
    syslog(LOG_INFO, "%s, ret: %d, type: %d", __func__, ret, (int)type);
    assert_int_equal(ret, OK);
    assert_int_equal((int)type, 13);
}

static void TestNuttxGetVoiceRoaming(void** state)
{
    (void)state;
    bool value = true;
    int ret = tapi_network_is_voice_roaming(get_tapi_ctx(), 0, &value);
    syslog(LOG_INFO, "%s, ret: %d, value: %d", __func__, ret, (int)value);
    assert_int_equal(ret, OK);
    assert_int_equal((int)value, 0);
}

// modem
static void TestNuttxModemGetImei(void** state)
{
    (void)state;
    int ret = tapi_get_imei_test(0);
    assert_int_equal(ret, OK);
}

// static void TestNuttxModemSetUmtsPrefNetMode(void** state)
// {
//     // defalut rat mode is NETWORK_PREF_NET_TYPE_LTE_GSM_WCDMA (9)
//     tapi_pref_net_mode set_value = NETWORK_PREF_NET_TYPE_UMTS;
//     int ret = tapi_set_pref_net_mode_test(0, set_value);
//     assert_int_equal(ret, OK);
// }

// static void TestNuttxModemSetGsmOnlyPrefNetMode(void** state)
// {
//     tapi_pref_net_mode set_value = NETWORK_PREF_NET_TYPE_GSM_ONLY;
//     int ret = tapi_set_pref_net_mode_test(0, set_value);
//     assert_int_equal(ret, OK);
// }

// static void TestNuttxModemSetWcdmaOnlyPrefNetMode(void** state)
// {
//     tapi_pref_net_mode set_value = NETWORK_PREF_NET_TYPE_WCDMA_ONLY;
//     int ret = tapi_set_pref_net_mode_test(0, set_value);
//     assert_int_equal(ret, OK);
// }

// static void TestNuttxModemSetLteOnlyPrefNetMode(void** state)
// {
//     tapi_pref_net_mode set_value = NETWORK_PREF_NET_TYPE_LTE_ONLY;
//     int ret = tapi_set_pref_net_mode_test(0, set_value);
//     assert_int_equal(ret, OK);
// }

// static void TestNuttxModemSetLteWcdmaPrefNetMode(void** state)
// {
//     tapi_pref_net_mode set_value = NETWORK_PREF_NET_TYPE_LTE_WCDMA;
//     int ret = tapi_set_pref_net_mode_test(0, set_value);
//     assert_int_equal(ret, OK);
// }

// static void TestNuttxModemSetLteGsmWcdmaPrefNetMode(void** state)
// {
//     tapi_pref_net_mode set_value = NETWORK_PREF_NET_TYPE_LTE_GSM_WCDMA;
//     int ret = tapi_set_pref_net_mode_test(0, set_value);
//     assert_int_equal(ret, OK);
// }

static void TestNuttxModemGetPrefNetMode(void** state)
{
    sleep(5);
    tapi_pref_net_mode get_value = NETWORK_PREF_NET_TYPE_ANY;
    int ret = tapi_get_pref_net_mode_test(0, &get_value);
    assert_int_equal(ret, OK);
}

static void TestNuttxModemSetRadioPowerOn(void** state)
{
    (void)state;
    int ret = tapi_set_radio_power_test(0, 1);
    assert_int_equal(ret, OK);
    sleep(10);
}

static void TestNuttxModemSetRadioPowerOff(void** state)
{
    (void)state;
    int ret = tapi_set_radio_power_test(0, 0);
    assert_int_equal(ret, OK);
    sleep(10);
}

static void TestNuttxModemSetRadioPowerOnOffRepeatedly(void** state)
{
    (void)state;
    REPEAT_TEST_LESS_FOR
    {
        TestNuttxModemSetRadioPowerOn(state);
        TestNuttxModemSetRadioPowerOff(state);
    }
}

static void TestNuttxGetModemEnableStatus(void** state)
{
    (void)state;
    int get_state = 1;
    int ret = tapi_get_modem_status_test(0, &get_state);
    assert_int_equal(ret, OK);
}

static void TestNuttxGetModemDsiableStatus(void** state)
{
    (void)state;
    int get_state = 0;
    int ret = tapi_get_modem_status_test(0, &get_state);
    assert_int_equal(ret, OK);
}

static void TestNuttxModemEnable(void** state)
{
    (void)state;
    int ret = tapi_enable_modem_test(0, 1);
    assert_int_equal(ret, OK);
    sleep(10);
}

static void TestNuttxModemDisable(void** state)
{
    (void)state;
    int ret = tapi_enable_modem_test(0, 0);
    assert_int_equal(ret, OK);
    sleep(10);
}

static void TestNuttxModemEnableDisableRepeatedly(void** state)
{
    (void)state;
    REPEAT_TEST_LESS_FOR
    {
        TestNuttxModemEnable(state);
        TestNuttxGetModemEnableStatus(state);
        TestNuttxModemDisable(state);
        TestNuttxGetModemDsiableStatus(state);
    }
}

static void TestNuttxModemRegister(void** state)
{
    (void)state;
    int ret = tapi_modem_register_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxModemUnregister(void** state)
{
    (void)state;
    int ret = tapi_modem_unregister_test();
    assert_true(ret == OK);
}

static void TestNuttxGetModemRevision(void** state)
{
    int ret;
    ret = tapi_get_modem_revision_test(0);
    assert_int_equal(ret, OK);
}

// static void TestNuttxImsServiceStatus(void** state)
// {
//     case_type* mode = *state;
//     // device default info.reg_info is 1, info.ext_info is 5
//     REPEAT_TEST_LESS_FOR
//     {
//         tapi_ims_registration_info info;
//         int ret = tapi_ims_set_service_status_test(0, 1);
//         if (*mode == CASE_NORMAL_MODE || *mode == CASE_AIRPLANE_MODE
//             || *mode == CASE_CALL_DIALING) {
//             assert_int_equal(ret, OK);
//             ret = tapi_ims_get_registration_test(0, &info);
//             assert_int_equal(ret, OK);
//             assert_int_equal(info.reg_info, 1);
//             assert_int_equal(info.ext_info, 1);
//         } else {
//             assert_int_equal(ret, -5);
//         }
//         sleep(1);

//         ret = tapi_ims_set_service_status_test(0, 5);
//         if (*mode == CASE_NORMAL_MODE || *mode == CASE_AIRPLANE_MODE
//             || *mode == CASE_CALL_DIALING) {
//             assert_int_equal(ret, OK);
//             ret = tapi_ims_get_registration_test(0, &info);
//             assert_int_equal(ret, OK);
//             assert_int_equal(info.reg_info, 1);
//             assert_int_equal(info.ext_info, 5);
//             sleep(1);
//         } else {
//             assert_int_equal(ret, -5);
//         }
//     }
// }

// static void TestNuttxModemDialCall(void** state)
// {
//     int slot_id = 0;
//     int ret1 = tapi_call_listen_call_test(slot_id);
//     int ret2 = tapi_call_dial_test(slot_id, phone_num, 0);
//     int ret = ret1 || ret2;
//     assert_int_equal(ret, OK);
//     sleep(2);
// }

// static void TestNuttxModemHangupCall(void** state)
// {
//     int slot_id = 0;
//     int ret1 = tapi_call_hanup_current_call_test(slot_id);
//     int ret2 = tapi_call_unlisten_call_test();
//     int ret = ret1 || ret2;
//     assert_int_equal(ret, OK);
// }

static void TestNuttxModemInvokeOemShotRilRequestRaw(void** state)
{
    int ret = tapi_invoke_oem_ril_request_raw_test(0, "01A0B023", 4);
    assert_int_equal(ret, OK);
}

static void TestNuttxModemInvokeOemLongRilRequestRaw(void** state)
{
    int ret = tapi_invoke_oem_ril_request_raw_test(0, "01A0B02301A0B02301A0B02301A0B02301A0B02301", 21);
    assert_int_equal(ret, OK);
}

static void TestNuttxModemInvokeOemSeperateRilRequestRaw(void** state)
{
    int ret = tapi_invoke_oem_ril_request_raw_test(0, "10|22", 2);
    assert_int_equal(ret, OK);
}

static void TestNuttxModemInvokeOemNormalRilRequestRaw(void** state)
{
    int ret = tapi_invoke_oem_ril_request_raw_test(0, "01A0B023", 2);
    assert_int_equal(ret, OK);
}

static void TestNuttxModemInvokeOemRilRequestATCmdStrings(void** state)
{
    char* req_data = "AT+CPIN?";
    int ret = tapi_invoke_oem_ril_request_strings_test(0, req_data, 1);
    assert_int_equal(ret, OK);
}

static void TestNuttxModemInvokeOemRilRequestNotATCmdStrings(void** state)
{
    // not AT cmd
    char* req_data = "10|22";
    int ret = tapi_invoke_oem_ril_request_strings_test(0, req_data, 2);
    assert_int_equal(ret, OK);
}

static void TestNuttxModemInvokeOemRilRequestHexStrings(void** state)
{
    char* req_data = "0x10|0x01";
    int ret = tapi_invoke_oem_ril_request_strings_test(0, req_data, 2);
    assert_int_equal(ret, OK);
}

// static void TestNuttxModemInvokeOemRilRequestLongStrings(void **state)
// {
//     char req_data[MAX_INPUT_ARGS_LEN];

//     // test error
//     // FIXME: tapi_invoke_oem_ril_request_strings_test interface buffer overflow
//     // when req_data len is 21, current max len is 20
//     strcpy(req_data,
//         "10|22|10|22|10|22|10|22|10|22|10|22|10|22|10|22|10|22|10|22");
//     int ret = tapi_invoke_oem_ril_request_strings_test(0, req_data, 20);
//     assert_int_equal(ret, -1);

//     // strcpy(req_data, "10|22");
//     // // FIXME: _dbus_check_is_valid_utf8 is called by dbus_message_iter_append_basic
//     // // cannot handle \0 in char * string;
//     // ret = tapi_invoke_oem_ril_request_strings_test(0, req_data, 20);
//     // assert_int_equal(ret, -1);
// }

// static void cmocka_set_radio_power_on_off_test(void **state)
// {
//     for(int i = 0; i < 3; i++)
//     {
//         tapi_set_radio_power_test(0, 0);
//         sleep(1);
//         tapi_set_radio_power_test(0, 1);
//         sleep(1);
//     }
//     int ret = tapi_get_radio_power_test(0);
//     assert_int_equal(ret, 1);
// }

static void TestNuttxImsListen(void** state)
{
    (void)state;
    int ret = tapi_ims_listen_ims_test(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxImsTurnOn(void** state)
{
    (void)state;
    int ret = tapi_ims_turn_on_test(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxImsGetRegistration(void** state)
{
    (void)state;
    int ret = tapi_ims_get_registration_test(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxImsGetEnabled(void** state)
{
    (void)state;
    int ret = tapi_ims_get_enabled_test(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxImsSetServiceStatus(void** state)
{
    (void)state;
    int ret = tapi_ims_set_service_status_test(0, 5);
    assert_int_equal(ret, 0);
    sleep(5);
}

static void TestNuttxImsSetSmsCap(void** state)
{
    (void)state;
    int ret = tapi_ims_set_service_status_test(0, 4);
    assert_int_equal(ret, 0);
}

static void TestNuttxImsSetSmsVoiceCap(void** state)
{
    (void)state;
    int ret = tapi_ims_set_service_status_test(0, 5);
    assert_int_equal(ret, 0);
}

static void TestNuttxImsResetImsCap(void** state)
{
    (void)state;
    int ret = tapi_ims_set_service_status_test(0, 1);
    assert_int_equal(ret, 0);
    sleep(10);
}

static void TestNuttxImsTurnOff(void** state)
{
    (void)state;
    int ret = tapi_ims_turn_off_test(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxImsTurnOnOff(void** state)
{
    (void)state;
    REPEAT_TEST_LESS_FOR
    {
        TestNuttxImsTurnOn(state);
        TestNuttxImsGetEnabled(state);
        TestNuttxImsGetRegistration(state);
        TestNuttxImsTurnOff(state);
    }
}

static void TestNuttxSSRegister(void** state)
{
    (void)state;
    int ret = tapi_listen_ss_test(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxSSUnRegister(void** state)
{
    (void)state;
    int ret = tapi_unlisten_ss_test();
    assert_int_equal(ret, 0);
}

static void TestNuttxRequestCallBarring(void** state)
{
    (void)state;
    int ret = tapi_ss_request_call_barring_test(0);
    assert_int_equal(ret, 0);
}

static void TestNuttxSetCallBarring(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_barring_option_test(0, "AI", "1234");
    assert_int_equal(ret, 0);
}

static void TestNuttxGetCallBarring(void** state)
{
    (void)state;
    sleep(5);
    int ret = tapi_ss_get_call_barring_option_test(0, "VoiceIncoming", "always");
    assert_int_equal(ret, 0);
}

static void TestNuttxChangeCallBarringPassword(void** state)
{
    (void)state;
    int ret = tapi_ss_change_call_barring_password_test(0, "1234", "2345");
    assert_int_equal(ret, 0);
}

static void TestNuttxResetCallBarringPassword(void** state)
{
    (void)state;
    int ret = tapi_ss_change_call_barring_password_test(0, "2345", "1234");
    assert_int_equal(ret, 0);
}

static void TestNuttxDisableAllIncoming(void** state)
{
    (void)state;
    int ret = tapi_ss_disable_all_incoming_test(0, "1234");
    assert_int_equal(ret, 0);
}

static void TestNuttxDisableAllOutgoing(void** state)
{
    (void)state;
    int ret = tapi_ss_disable_all_outgoing_test(0, "1234");
    assert_int_equal(ret, 0);
}

static void TestNuttxDisableAllCallBarrings(void** state)
{
    (void)state;
    int ret = tapi_ss_disable_all_call_barrings_test(0, "1234");
    assert_int_equal(ret, 0);
}

static void TestNuttxSetCallForwardingUnConditional(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 0, "10086");
    assert_int_equal(ret, 0);
}

static void TestNuttxGetCallForwardingUnConditional(void** state)
{
    (void)state;
    int ret = tapi_ss_get_call_forwarding_option_test(0, 0);
    assert_int_equal(ret, 0);
}

static void TestNuttxClearCallForwardingUnconditional(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 0, "\0");
    assert_int_equal(ret, 0);
}

static void TestNuttxSetCallForwardingBusy(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 1, "10086");
    assert_int_equal(ret, 0);
}

static void TestNuttxGetCallForwardingBusy(void** state)
{
    (void)state;
    int ret = tapi_ss_get_call_forwarding_option_test(0, 1);
    assert_int_equal(ret, 0);
}

static void TestNuttxClearCallForwardingBusy(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 1, "\0");
    assert_int_equal(ret, 0);
}

static void TestNuttxSetCallForwardingNoReply(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 2, "10086");
    assert_int_equal(ret, 0);
}

static void TestNuttxGetCallForwardingNoReply(void** state)
{
    (void)state;
    int ret = tapi_ss_get_call_forwarding_option_test(0, 2);
    assert_int_equal(ret, 0);
}

static void TestNuttxClearCallForwardingNoReply(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 2, "\0");
    assert_int_equal(ret, 0);
}

static void TestNuttxSetCallForwardingNotReachable(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 3, "10086");
    assert_int_equal(ret, 0);
}

static void TestNuttxGetCallForwardingNotReachable(void** state)
{
    (void)state;
    int ret = tapi_ss_get_call_forwarding_option_test(0, 3);
    assert_int_equal(ret, 0);
}

static void TestNuttxClearCallForwardingNotReachable(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 3, "\0");
    assert_int_equal(ret, 0);
}

static void TestNuttxEnableCallWaiting(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_waiting_test(0, true);
    assert_int_equal(ret, 0);
}

static void TestNuttxGetEnableCallWaiting(void** state)
{
    (void)state;
    int ret = tapi_ss_get_call_waiting_test(0, true);
    assert_int_equal(ret, 0);
}

static void TestNuttxDisableCallWaiting(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_waiting_test(0, false);
    assert_int_equal(ret, 0);
}

static void TestNuttxGetDisableCallWaiting(void** state)
{
    (void)state;
    int ret = tapi_ss_get_call_waiting_test(0, false);
    assert_int_equal(ret, 0);
}

static void TestNuttxEnableFdn(void** state)
{
    (void)state;
    int ret = tapi_ss_enable_fdn_test(0, true, "1234");
    assert_int_equal(ret, 0);
}

static void TestNuttxGetFdnEnabled(void** state)
{
    (void)state;
    int ret = tapi_ss_query_fdn_test(0, true);
    assert_int_equal(ret, 0);
}

static void TestNuttxDisableFdn(void** state)
{
    (void)state;
    int ret = tapi_ss_enable_fdn_test(0, false, "1234");
    assert_int_equal(ret, 0);
}

static void TestNuttxGetFdnDisabled(void** state)
{
    (void)state;
    int ret = tapi_ss_query_fdn_test(0, false);
    assert_int_equal(ret, 0);
}

static void tapi_cb(tapi_async_result* result)
{
    if (result->msg_id == EVENT_MODEM_ENABLE_DONE && result->status == OK) {
        ready_done = 1;
    } else if (result->msg_id == MSG_VOICE_REGISTRATION_STATE_CHANGE_IND) {
        ready_done = 1;
    } else {
        ready_done = -1;
    }
}

static int wait_for_async_result(const char* str)
{
    while (ready_done != 1) {
        if (ready_done == -1 || count >= 10) {
            syslog(LOG_ERR, "%s\n", str);
            return -1;
        } else {
            sleep(1);
            count++;
        }
    }

    return 0;
}

static void on_tapi_client_ready(const char* client_name, void* user_data)
{
    if (client_name != NULL)
        syslog(LOG_DEBUG, "tapi is ready for %s\n", client_name);

    ready_done = 1;

    /**
     * both args are NULL, that's tapi received disconnect message
     * so we here need to stop the default loop
     */
    if (client_name == NULL && user_data == NULL) {
        if (context != NULL) {
            syslog(LOG_ERR, "recieve dbus disconnected msg, free tapi context");
            tapi_close(context);
            context = NULL;
        }
        syslog(LOG_INFO, "tapi already closed, stop default loop");
        uv_stop(uv_default_loop());
    }
}

static void* run_test_loop(void* args)
{
    context = tapi_open("vela.telephony.test", on_tapi_client_ready, NULL);
    if (context == NULL) {
        return NULL;
    }

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());

    return NULL;
}

int main(int argc, char* argv[])
{
#ifndef CONFIG_TEST_PHONE_NUMBER
    printf("Please config phone number in Kconfig!\n");
    return 0;
#endif
    int ret;

    ready_done = 0;
    phone_num = CONFIG_TEST_PHONE_NUMBER;

    /* initialize async handler before the thread creation
     * in case we have some race issues
     */
    uv_async_init(uv_default_loop(), &g_uv_exit, exit_async_cleanup);

    pthread_t thread;
    pthread_attr_t attr;
    struct sched_param param;
    pthread_attr_init(&attr);
    /* tapi main thread priority should equal to ofono thread priority */
    param.sched_priority = 100;
    pthread_attr_setschedparam(&attr, &param);
    pthread_attr_setstacksize(&attr, 262144);
    ret = pthread_create(&thread, &attr, run_test_loop, NULL);
    if (ret != 0) {
        syslog(LOG_ERR, "pthread_create failed with %d\n", ret);
        return -1;
    }

    while (!ready_done)
        sleep(1);

    ready_done = 0;
    count = 0;
    tapi_enable_modem(get_tapi_ctx(), 0, EVENT_MODEM_ENABLE_DONE, 1, tapi_cb);
    ret = wait_for_async_result("modem failed to start and cannot be tested");
    if (ret == -1)
        goto do_exit;

    ready_done = 0;
    count = 0;
    tapi_network_register(get_tapi_ctx(), 0, MSG_VOICE_REGISTRATION_STATE_CHANGE_IND, NULL, tapi_cb);
    ret = wait_for_async_result("Network connection failure, unable to perform the test.");
    if (ret == -1)
        goto do_exit;

    const struct CMUnitTest SimTestSuites[] = {
        cmocka_unit_test(TestNuttxHasIccCard),
        cmocka_unit_test(TestNuttxHasIccCardNumerousTimes),
        cmocka_unit_test(TestNuttxGetSimOperatorName),
        cmocka_unit_test(TestNuttxGetSimOperatorNameNumerousTimes),
        cmocka_unit_test(TestNuttxGetSimOperator),
        cmocka_unit_test(TestNuttxGetSimOperatorNumerousTimes),
        cmocka_unit_test(TestNuttxGetSimSubscriberId),
        cmocka_unit_test(TestNuttxGetSimSubscriberIdNumerousTimes),
        cmocka_unit_test(TestNuttxGetIccId),
        cmocka_unit_test(TestNuttxGetIccIdNumerousTimes),
        cmocka_unit_test(TestNuttxGetMSISDN),
        cmocka_unit_test(TestNuttxGetMSISDNNumerousTimes),
        cmocka_unit_test(TestNuttxTransmitAPDUInBasicChannel),
        cmocka_unit_test(TestNuttxOpenLogicalChannel),
        cmocka_unit_test(TestNuttxCloseLogicalChannel),
        cmocka_unit_test(TestNuttxLogicalChannelOpenCloseNumerous),
        cmocka_unit_test(TestNuttxTransmitAPDUInLogicalChannel),
        cmocka_unit_test(TestNuttxSetUiccEnablement),
        cmocka_unit_test(TestNuttxGetUiccEnablement),
        cmocka_unit_test(TestNuttxTransmitAPDUBasicChannel),
        cmocka_unit_test(TestNuttxGetSimState),
        cmocka_unit_test(TestNuttxEnterPin),
        cmocka_unit_test(TestNuttxChangePin),
        cmocka_unit_test(TestNuttxLockPin),
        cmocka_unit_test(TestNuttxUnlockPin),
        cmocka_unit_test(TestNuttxLoadAdnEntries),
        cmocka_unit_test(TestNuttxLoadFdnEntries),
        cmocka_unit_test(TestNuttxInsertFdnEntry),
        cmocka_unit_test(TestNuttxUpdateFdnEntry),
        cmocka_unit_test(TestNuttxDeleteFdnEntry),
    };

    const struct CMUnitTest CallTestSuites[] = {
        cmocka_unit_test(TestNuttxListenCall),
        cmocka_unit_test(TestNuttxDialNumber),
        cmocka_unit_test(TestNuttxHangupCurrentCall),
        cmocka_unit_test(TestNuttxDialEccNumber),
        cmocka_unit_test(TestNuttxDialLongPhoneNumber),
        cmocka_unit_test(TestNuttxDialShotPhoneNumber),
        cmocka_unit_test(TestNuttxDialWithEnableHideCallId),
        cmocka_unit_test(TestNuttxDialWithDisabledHideCallId),
        cmocka_unit_test(TestNuttxDialWithDefaultHideCallId),
        cmocka_unit_test(TestNuttxDialWithAreaCode),
        cmocka_unit_test(TestNuttxDialWithPauseCode),
        cmocka_unit_test(TestNuttxDialWithWaitCode),
        cmocka_unit_test(TestNuttxDialWithNumerousCode),
        cmocka_unit_test(TestNuttxDialConference),
        cmocka_unit_test(TestNuttCheckAleringStatusAfterDial),
        cmocka_unit_test(TestNuttxStartDtmf),
        cmocka_unit_test(TestNuttxStopDtmf),
        cmocka_unit_test(TestNuttxGetCallCount),
        cmocka_unit_test(TestNuttxHangupAllCall),
        // cmocka_unit_test(TestNuttxLoadEccList),
        cmocka_unit_test(TestNuttxSetVoicecallSlot),
        cmocka_unit_test(TestNuttxGetVoicecallSlot),
        cmocka_unit_test(TestNuttxClearVoicecallSlotSet),
        // hangup between dialing and answering
        cmocka_unit_test(TestNuttxHangupBetweenDialingAndAnswering),
        cmocka_unit_test(TestNuttxIncomingCallAnswerAndHangup),
        cmocka_unit_test(TestNuttxIncomingCallAnswerAndRemoteHangup),
        cmocka_unit_test(TestNuttxEnableCallWaiting),
        cmocka_unit_test(TestNuttxCallHoldAndAnswerOtherCall),
        cmocka_unit_test(TestNuttxCallMergeByUser),
        cmocka_unit_test(TestNuttxCallSeparateByUser),
        cmocka_unit_test(TestNuttxCallReleaseAndSwapOtherCall),
        cmocka_unit_test(TestNuttxDisableCallWaiting),
        cmocka_unit_test(TestNuttxOutgoingCallRemoteAnswerAndHangup),
        cmocka_unit_test(TestNuttxOutgoingCallHoldAndHangup),
        cmocka_unit_test(TestNuttxOutgoingCallActiveAndSendtones),
        cmocka_unit_test(TestNuttxCallConnectAndLocalHangup),
        cmocka_unit_test(TestNuttxDialAndRemoteHangup),
        cmocka_unit_test(TestNuttxIncomingCallandLocalHangup),
        cmocka_unit_test(TestNuttxUnlistenCall),
#if 0

        // answer the incoming call then hangup it
        cmocka_unit_test(TestNuttxAnswerAndHangupTheIncomingCall),

        // dial again after remote hangup
        // cmocka_unit_test(TestNuttxDialAgainAfterRemoteHangup),

        // hangup after remote answer
        cmocka_unit_test(TestNuttxHangupCallAfterRemoteAnswer),

        // dial then remote answer and hangup
        // cmocka_unit_test(TestNuttxDialThenRemoteAnswerAndHangup),

        // remote hangup then incoming new call
        // cmocka_unit_test(TestNuttxRemoteHangupThenIncomingNewCall),

        // remote hangup then dial another
        // cmocka_unit_test(TestNuttxRemoteHangupThenDialAnother),

        // dial with numerous hide call id
        cmocka_unit_test(TestNuttxDialWithNumerousHideCallId),

        // incoming new call then remote hangup
        // cmocka_unit_test(TestNuttxIncomingNewCallThenRemoteHangup),

        // dial using a phone number with area code
        cmocka_unit_test(TestNuttxDialWithAreaCode),
#endif
    };

    const struct CMUnitTest DataTestSuites[] = {
        cmocka_unit_test(TestNuttxDataRegister),
        cmocka_unit_test(TestNuttxDataUnregister),
        cmocka_unit_test(TestNuttxDataRegister),
        cmocka_unit_test(TestNuttxDataLoadApnContexts),
        cmocka_unit_test(TestNuttxDataSaveApnContext),
        cmocka_unit_test(TestNuttxDataRemoveApnContext),
        cmocka_unit_test(TestNuttxDataResetApnContexts),
        cmocka_unit_test(TestNuttxDataResetApnContextsRepeatedly),
        cmocka_unit_test(TestNuttxDataEditApnName),
        cmocka_unit_test(TestNuttxDataEditApnType),
        cmocka_unit_test(TestNuttxDataEditApnProto),
        cmocka_unit_test(TestNuttxDataEditApnAuth),
        cmocka_unit_test(TestNuttxDataEditApnAll),
        cmocka_unit_test(TestNuttxDataEditApnAndRemove),
        cmocka_unit_test(TestNuttxDataEditApnAndReset),
        cmocka_unit_test(TestNuttxDataEditApnRepeatedlyAndLoad),
        cmocka_unit_test(TestNuttxDataResetApnContexts),
        cmocka_unit_test(TestNuttxDataEnable),
        cmocka_unit_test(TestNuttxDataIsEnable),
        cmocka_unit_test(TestNuttxDataDisable),
        cmocka_unit_test(TestNuttxDataIsDisable),
        cmocka_unit_test(TestNuttxDataEnableRepeatedly),
        cmocka_unit_test(TestNuttxDataEnable),
        cmocka_unit_test(TestNuttxDataReleaseNetworkInternet),
        cmocka_unit_test(TestNuttxDataRequestNetworkInternet),
        cmocka_unit_test(TestNuttxDataReleaseNetworkInternetRepeatedly),
        cmocka_unit_test(TestNuttxDataDisable),
        cmocka_unit_test(TestNuttxDataRequestNetworkIms),
        cmocka_unit_test(TestNuttxDataReleaseNetworkIms),
        cmocka_unit_test(TestNuttxDataReleaseNetworkImsRepeatedly),
        cmocka_unit_test(TestNuttxDataSaveApnContextSupl),
        cmocka_unit_test(TestNuttxDataSaveApnContextEmergency),
        cmocka_unit_test(TestNuttxDataResetApnContexts),
        cmocka_unit_test(TestNuttxDataSetPreferredApn),
        cmocka_unit_test(TestNuttxDataGetPreferredApn),
        cmocka_unit_test(TestNuttxDataSendScreenState),
        cmocka_unit_test(TestNuttxDataGetNetworkType),
        cmocka_unit_test(TestNuttxDataIsPsAttached),
        cmocka_unit_test(TestNuttxDataSetDefaultDataSlot),
        cmocka_unit_test(TestNuttxDataGetDefaultDataSlot),
        cmocka_unit_test(TestNuttxDataSetDataAllow),
        cmocka_unit_test(TestNuttxDataGetCallList),
        cmocka_unit_test(TestNuttxDataSaveLongApnContex),
        cmocka_unit_test(TestNuttxDataResetApnContexts),
        cmocka_unit_test(TestNuttxEnableDataRoaming),
        cmocka_unit_test(TestNuttxDisableDataRoaming),
        cmocka_unit_test(TestNuttxDataUnregister),
    };

    const struct CMUnitTest SmsTestSuites[] = {
        cmocka_unit_test(TestNuttxListenCall),
        cmocka_unit_test(TestNuttxImsListen),
        cmocka_unit_test(TestNuttxImsTurnOn),
        cmocka_unit_test(TestNuttxSmsSetServiceCenterNum),
        cmocka_unit_test(TestNuttxSmsGetServiceCenterNum),
        cmocka_unit_test(TestNuttxSmsSendShortMessageInEnglish),
        cmocka_unit_test(TestNuttxSmsSendShortMessageInChinese),
        cmocka_unit_test(TestNuttxSmsSendShortDataMessageInEnglish),
        cmocka_unit_test(TestNuttxSmsSendShortDataMessageInChinese),
        cmocka_unit_test(TestNuttxSmsSendLongMessageInEnglish),
        cmocka_unit_test(TestNuttxSmsSendLongMessageInChinese),
        cmocka_unit_test(TestNuttxSmsSendLongDataMessageInEnglish),
        cmocka_unit_test(TestNuttxSmsSendLongDataMessageInChinese),
        cmocka_unit_test(TestNuttxSmsSendShortEnglishMessageInDialing),
        cmocka_unit_test(TestNuttxSmsSendShortChineseMessageInDialing),
        cmocka_unit_test(TestNuttxSmsSendLongEnglishMessageInDialing),
        cmocka_unit_test(TestNuttxSmsSendLongChineseMessageInDialing),
        cmocka_unit_test(TestNuttxSmsSendShortEnglishDataMessageInDialing),
        cmocka_unit_test(TestNuttxSmsSendShortChineseDataMessageInDialing),
        cmocka_unit_test(TestNuttxSmsSendLongEnglishDataMessageInDialing),
        cmocka_unit_test(TestNuttxSmsSendLongChineseDataMessageInDialing),
        cmocka_unit_test(TestNuttxSendEnglishMessageInVoiceImsCap),
        cmocka_unit_test(TestNuttxSendChineseMessageInVoiceImsCap),
        cmocka_unit_test(TestNuttxSendLongEnglishMessageInVoiceImsCap),
        cmocka_unit_test(TestNuttxSendLongChineseMessageInVoiceImsCap),
        cmocka_unit_test(TestNuttxSendEnglishDataMessageInVoiceImsCap),
        cmocka_unit_test(TestNuttxSendChineseDataMessageInVoiceImsCap),
        cmocka_unit_test(TestNuttxSendLongEnglishDataMessageInVoiceImsCap),
        cmocka_unit_test(TestNuttxSendLongChineseDataMessageInVoiceImsCap),
        cmocka_unit_test(TestNuttxImsSetSmsCap),
        cmocka_unit_test(TestNuttxSendEnglishMessageInSmsImsCap),
        cmocka_unit_test(TestNuttxSendChineseMessageInSmsImsCap),
        cmocka_unit_test(TestNuttxSendLongEnglishMessageInSmsImsCap),
        cmocka_unit_test(TestNuttxSendLongChineseMessageInSmsImsCap),
        cmocka_unit_test(TestNuttxSendEnglishDataMessageInSmsImsCap),
        cmocka_unit_test(TestNuttxSendChineseDataMessageInSmsImsCap),
        cmocka_unit_test(TestNuttxSendLongEnglishDataMessageInSmsImsCap),
        cmocka_unit_test(TestNuttxSendLongChineseDataMessageInSmsImsCap),
        cmocka_unit_test(TestNuttxImsSetSmsVoiceCap),
        cmocka_unit_test(TestNuttxSendEnglishMessageInSmsVoiceCap),
        cmocka_unit_test(TestNuttxSendChineseMessageInSmsVoiceCap),
        cmocka_unit_test(TestNuttxSendLongEnglishMessageInSmsVoiceCap),
        cmocka_unit_test(TestNuttxSendLongChineseMessageInSmsVoiceCap),
        cmocka_unit_test(TestNuttxSendEnglishDataMessageInSmsVoiceCap),
        cmocka_unit_test(TestNuttxSendChineseDataMessageInSmsVoiceCap),
        cmocka_unit_test(TestNuttxSendLongEnglishDataMessageInSmsVoiceCap),
        cmocka_unit_test(TestNuttxSendLongChineseDataMessageInSmsVoiceCap),
        cmocka_unit_test(TestNuttxSetSmsDefaultSlot),
        cmocka_unit_test(TestNuttxGetSmsDefaultSlot),
        cmocka_unit_test(TestNuttxSmsSetCellBroadcastPower),
        cmocka_unit_test(TestNuttxSmsGetCellBroadcastPower),
        cmocka_unit_test(TestNuttxSmsSetCellBroadcastTopics),
        cmocka_unit_test(TestNuttxSmsGetCellBroadcastTopics),
        cmocka_unit_test(TestNuttxImsResetImsCap),
        cmocka_unit_test(TestNuttxImsTurnOff),
        cmocka_unit_test(TestNuttxUnlistenCall),
    };

    const struct CMUnitTest NetTestSuites[] = {
        cmocka_unit_test(TestNuttxNetSelectManual),
        cmocka_unit_test(TestNuttxNetSelectAuto),
        cmocka_unit_test(TestNuttxNetScan),
        cmocka_unit_test(TestNuttxNetGetServingCellinfos),
        cmocka_unit_test(TestNuttxNetGetNeighbouringCellInfos),
        cmocka_unit_test(TestNuttxNetRegistrationInfo),
        cmocka_unit_test(TestNuttxNetGetOperatorName),
        cmocka_unit_test(TestNuttxNetQuerySignalstrength),
        //      cmocka_unit_test(TestNuttxNetSetCellInfoListRate),
        cmocka_unit_test(TestNuttxNetGetVoiceRegistered),
        cmocka_unit_test(TestNuttxGetVoiceNwType),
        cmocka_unit_test(TestNuttxGetVoiceRoaming),
    };

    const struct CMUnitTest ImsTestSuits[] = {
        cmocka_unit_test(TestNuttxImsTurnOn),
        cmocka_unit_test(TestNuttxImsGetRegistration),
        cmocka_unit_test(TestNuttxImsGetEnabled),
        cmocka_unit_test(TestNuttxImsSetServiceStatus),
        cmocka_unit_test(TestNuttxImsResetImsCap),
        cmocka_unit_test(TestNuttxImsTurnOff),
        cmocka_unit_test(TestNuttxImsTurnOnOff),
    };

    const struct CMUnitTest SSTestSuits[] = {
        cmocka_unit_test(TestNuttxSSRegister),
        cmocka_unit_test(TestNuttxSSUnRegister),
        cmocka_unit_test(TestNuttxRequestCallBarring),
        cmocka_unit_test(TestNuttxSetCallBarring),
        cmocka_unit_test(TestNuttxGetCallBarring),
        cmocka_unit_test(TestNuttxChangeCallBarringPassword),
        cmocka_unit_test(TestNuttxResetCallBarringPassword),
        cmocka_unit_test(TestNuttxDisableAllIncoming),
        cmocka_unit_test(TestNuttxDisableAllOutgoing),
        cmocka_unit_test(TestNuttxDisableAllCallBarrings),
        cmocka_unit_test(TestNuttxSetCallForwardingUnConditional),
        cmocka_unit_test(TestNuttxGetCallForwardingUnConditional),
        cmocka_unit_test(TestNuttxClearCallForwardingUnconditional),
        cmocka_unit_test(TestNuttxSetCallForwardingBusy),
        cmocka_unit_test(TestNuttxGetCallForwardingBusy),
        cmocka_unit_test(TestNuttxClearCallForwardingBusy),
        cmocka_unit_test(TestNuttxSetCallForwardingNoReply),
        cmocka_unit_test(TestNuttxGetCallForwardingNoReply),
        cmocka_unit_test(TestNuttxClearCallForwardingNoReply),
        cmocka_unit_test(TestNuttxSetCallForwardingNotReachable),
        cmocka_unit_test(TestNuttxGetCallForwardingNotReachable),
        cmocka_unit_test(TestNuttxClearCallForwardingNotReachable),
        cmocka_unit_test(TestNuttxEnableCallWaiting),
        cmocka_unit_test(TestNuttxGetEnableCallWaiting),
        cmocka_unit_test(TestNuttxDisableCallWaiting),
        cmocka_unit_test(TestNuttxGetDisableCallWaiting),
        cmocka_unit_test(TestNuttxEnableFdn),
        cmocka_unit_test(TestNuttxGetFdnEnabled),
        cmocka_unit_test(TestNuttxDisableFdn),
        cmocka_unit_test(TestNuttxGetFdnDisabled),
    };

    const struct CMUnitTest CommonTestSuites[] = {
        cmocka_unit_test(TestNuttxModemGetImei),
        // cmocka_unit_test(TestNuttxModemSetUmtsPrefNetMode),
        // cmocka_unit_test(TestNuttxModemSetGsmOnlyPrefNetMode),
        // cmocka_unit_test(TestNuttxModemSetWcdmaOnlyPrefNetMode),
        // cmocka_unit_test(TestNuttxModemSetLteOnlyPrefNetMode),
        // cmocka_unit_test(TestNuttxModemSetLteWcdmaPrefNetMode),
        // cmocka_unit_test(TestNuttxModemSetLteGsmWcdmaPrefNetMode),
        cmocka_unit_test(TestNuttxModemGetPrefNetMode),
        cmocka_unit_test(TestNuttxModemRegister),
        cmocka_unit_test(TestNuttxModemUnregister),
        cmocka_unit_test(TestNuttxModemInvokeOemShotRilRequestRaw),
        cmocka_unit_test(TestNuttxModemInvokeOemLongRilRequestRaw),
        cmocka_unit_test(TestNuttxModemInvokeOemNormalRilRequestRaw),
        cmocka_unit_test(TestNuttxModemInvokeOemSeperateRilRequestRaw),
        cmocka_unit_test(TestNuttxModemInvokeOemRilRequestATCmdStrings),
        cmocka_unit_test(TestNuttxModemInvokeOemRilRequestNotATCmdStrings),
        cmocka_unit_test(TestNuttxModemInvokeOemRilRequestHexStrings),
        cmocka_unit_test(TestNuttxImsListen),
        cmocka_unit_test(TestNuttxGetModemRevision),
        cmocka_unit_test(TestNuttxModemDisable),
        cmocka_unit_test(TestNuttxGetModemDsiableStatus),
        cmocka_unit_test(TestNuttxModemEnableDisableRepeatedly),
        cmocka_unit_test(TestNuttxModemEnable),
        cmocka_unit_test(TestNuttxGetModemEnableStatus),
        cmocka_unit_test(TestNuttxModemSetRadioPowerOff),
        cmocka_unit_test(TestNuttxModemSetRadioPowerOnOffRepeatedly),
        cmocka_unit_test(TestNuttxModemSetRadioPowerOn),
        cmocka_unit_test(TestNuttxModemDisable),

        //         cmocka_unit_test_setup_teardown(TestNuttxModemSetRadioPowerOnOffRepeatedly,
        //             setup_normal_mode, free_mode),
        //         // Airplane mode
        //         cmocka_unit_test(TestNuttxModemSetRadioPowerOff),
        //         // cmocka_unit_test_setup_teardown(TestNuttxImsServiceStatus,
        //         //     setup_airplane_mode, free_mode),
        //         cmocka_unit_test(TestNuttxModemSetRadioPowerOn),

        //         // Call dialing
        //         cmocka_unit_test(TestNuttxModemDialCall),
        //         // TODO: enable disable modem
        //         cmocka_unit_test(TestNuttxModemSetRadioPowerOn),
        //         // cmocka_unit_test_setup_teardown(TestNuttxImsServiceStatus,
        //         //     setup_call_dialing, free_mode),
        //         cmocka_unit_test(TestNuttxModemHangupCall),

        //         // Modem poweroff
        //
        //         // cmocka_unit_test_setup_teardown(TestNuttxImsServiceStatus,
        //         //     setup_modem_poweroff, free_mode),
        //         cmocka_unit_test_setup_teardown(TestNuttxModemSetRadioPowerOnOffRepeatedly,
        //             setup_modem_poweroff, free_mode),
        //     // FIXME: Cannot enable because of RADIO_NOT_AVAILABLE
        //     // cmocka_unit_test(TestNuttxModemEnable),
    };

    sleep(3);
    cmocka_run_group_tests(SimTestSuites, NULL, NULL);

    cmocka_run_group_tests(CallTestSuites, NULL, NULL);

    cmocka_run_group_tests(DataTestSuites, NULL, NULL);

    cmocka_run_group_tests(SmsTestSuites, NULL, NULL);

    cmocka_run_group_tests(NetTestSuites, NULL, NULL);

    cmocka_run_group_tests(ImsTestSuits, NULL, NULL);

    cmocka_run_group_tests(SSTestSuits, NULL, NULL);

    cmocka_run_group_tests(CommonTestSuites, NULL, NULL);

do_exit:
    uv_async_send(&g_uv_exit);

    pthread_join(thread, NULL);
    uv_close((uv_handle_t*)&g_uv_exit, NULL);

    return 0;
}
