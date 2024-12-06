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

static void TestTeleHasIccCard(void** state)
{
    (void)state;
    int ret = tapi_sim_has_icc_card_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleHasIccCardNumerousTimes(void** state)
{
    (void)state;
    int ret = tapi_sim_multi_has_icc_card_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleGetSimOperator(void** state)
{
    (void)state;
    int ret = tapi_sim_get_sim_operator_test(0, "310260");
    assert_int_equal(ret, OK);
}

static void TestTeleGetSimOperatorName(void** state)
{
    (void)state;
    int ret = tapi_sim_get_sim_operator_name_test(0, "T-Mobile");
    assert_int_equal(ret, OK);
}

static void TestTeleGetSimOperatorNameNumerousTimes(void** state)
{
    (void)state;
    int ret = tapi_sim_get_sim_operator_name_numerous(0, "T-Mobile");
    assert_int_equal(ret, OK);
}

static void TestTeleGetSimOperatorNumerousTimes(void** state)
{
    (void)state;
    int ret = tapi_sim_multi_get_sim_operator(0, "310260");
    assert_int_equal(ret, OK);
}

static void TestTeleGetSimSubscriberId(void** state)
{
    (void)state;
    int ret = tapi_sim_get_sim_subscriber_id_test(0, "310260000000000");
    assert_int_equal(ret, OK);
}

static void TestTeleGetSimSubscriberIdNumerousTimes(void** state)
{
    (void)state;
    int ret = tapi_sim_multi_get_sim_subscriber_id_test(0, "310260000000000");
    assert_int_equal(ret, OK);
}

static void TestTeleGetIccId(void** state)
{
    (void)state;
    int ret = tapi_sim_get_sim_iccid_test(0, "89860318640220133897");
    assert_int_equal(ret, OK);
}

static void TestTeleGetIccIdNumerousTimes(void** state)
{
    (void)state;
    int ret = tapi_sim_multi_get_sim_iccid_test(0, "89860318640220133897");
    assert_int_equal(ret, OK);
}

static void TestTeleGetMSISDN(void** state)
{
    (void)state;
    int ret = tapi_sim_get_ef_msisdn_test(0, "+15551234567");
    assert_int_equal(ret, OK);
}

static void TestTeleGetMSISDNNumerousTimes(void** state)
{
    (void)state;
    int ret = tapi_sim_multi_get_ef_msisdn_test(0, "+15551234567");
    assert_int_equal(ret, OK);
}

static void TestTeleTransmitAPDUInBasicChannel(void** state)
{
    (void)state;
    int ret = tapi_transmit_apdu_basic_channel_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleGetSimState(void** state)
{
    (void)state;
    int ret = tapi_sim_get_state_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleOpenLogicalChannel(void** state)
{
    (void)state;
    int ret = tapi_open_logical_channel_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleCloseLogicalChannel(void** state)
{
    (void)state;
    int ret = tapi_close_logical_channel_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleLogicalChannelOpenCloseNumerous(void** state)
{
    (void)state;
    int ret = sim_open_close_logical_channel_numerous(0);
    assert_int_equal(ret, OK);
}

static void TestTeleTransmitAPDUInLogicalChannel(void** state)
{
    int ret = sim_transmit_apdu_by_logical_channel(0);
    assert_int_equal(ret, OK);
}

static void TestTeleSetUiccEnablement(void** state)
{
    (void)state;
    int ret = tapi_sim_set_uicc_enablement_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleGetUiccEnablement(void** state)
{
    (void)state;
    int ret = tapi_sim_get_uicc_enablement_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleTransmitAPDUBasicChannel(void** state)
{
    (void)state;
    int ret = tapi_transmit_apdu_basic_channel_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleEnterPin(void** state)
{
    (void)state;
    int ret = tapi_sim_enter_pin_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleChangePin(void** state)
{
    (void)state;
    int ret = sim_change_pin_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleLockPin(void** state)
{
    (void)state;
    int ret = tapi_sim_lock_pin_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleUnlockPin(void** state)
{
    (void)state;
    int ret = tapi_sim_unlock_pin_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleLoadAdnEntries(void** state)
{
    (void)state;
    int ret = tapi_phonebook_load_adn_entries_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleLoadFdnEntries(void** state)
{
    (void)state;
    int ret = tapi_phonebook_load_fdn_entries_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleInsertFdnEntry(void** state)
{
    (void)state;
    int ret = tapi_phonebook_insert_fdn_entry_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleUpdateFdnEntry(void** state)
{
    (void)state;
    int ret = tapi_phonebook_update_fdn_entry_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDeleteFdnEntry(void** state)
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

// static void TestTeleLoadEccList(void** state)
// {
//     (void)state;
//     int ret = tapi_call_load_ecc_list_test(0);
//     assert_int_equal(ret, 0);
// }

static void TestTeleSetVoicecallSlot(void** state)
{
    (void)state;
    int ret = tapi_call_set_default_voicecall_slot_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleGetVoicecallSlot(void** state)
{
    (void)state;
    int ret = tapi_call_get_default_voicecall_slot_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleClearVoicecallSlotSet(void** state)
{
    (void)state;
    int ret = call_clear_voicecall_slot_set();
    assert_int_equal(ret, 0);
}

static void TestTeleListenCall(void** state)
{
    (void)state;
    int ret = tapi_call_listen_call_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleUnlistenCall(void** state)
{
    (void)state;
    int ret = tapi_call_unlisten_call_test();
    assert_int_equal(ret, 0);
}

static void TestTeleStartDtmf(void** state)
{
    (void)state;
    int ret = tapi_start_dtmf_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleStopDtmf(void** state)
{
    (void)state;
    int ret = tapi_stop_dtmf_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleHangupBetweenDialingAndAnswering(void** state)
{
    (void)state;
    sleep(2);
    int ret = call_hangup_between_dialing_and_answering(0);
    assert_int_equal(ret, 0);
}

static void TestTeleIncomingCallAnswerAndHangup(void** state)
{
    (void)state;
    int ret = incoming_call_answer_and_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestTeleIncomingCallAnswerAndRemoteHangup(void** state)
{
    (void)state;
    int ret = incoming_call_answer_and_remote_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestTeleCallReleaseAndAnswer(void** state)
{
    (void)state;
    int ret = call_release_and_answer(0);
    assert_int_equal(ret, 0);
}

static void TestTeleCallHoldAndAnswerOtherCall(void** state)
{
    (void)state;
    int ret = call_hold_first_call_and_answer_second_call(0);
    assert_int_equal(ret, 0);
}

static void TestTeleCallMergeByUser(void** state)
{
    (void)state;
    int ret = call_merge_by_user(0);
    assert_int_equal(ret, 0);
}

static void TestTeleCallSeparateByUser(void** state)
{
    (void)state;
    int ret = call_separate_by_user(0);
    assert_int_equal(ret, 0);
}

static void TestTeleCallReleaseAndSwapOtherCall(void** state)
{
    (void)state;
    int ret = call_release_and_swap_other_call(0);
    assert_int_equal(ret, 0);
}

// static void TestTeleDialAgainAfterRemoteHangup(void **state) {
//     int ret = call_dial_after_caller_reject(0);
//     assert_int_equal(ret, 0);
// }

// static void TestTeleHangupCallAfterRemoteAnswer(void **state) {
//     int ret = call_hangup_after_caller_answer(0);
//     assert_int_equal(ret, 0);
// }

static void TestTeleOutgoingCallRemoteAnswerAndHangup(void** state)
{
    int ret = outgoing_call_remote_answer_and_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestTeleOutgoingCallHoldAndHangup(void** state)
{
    (void)state;
    int ret = outgoing_call_hold_and_unhold_by_caller(0);
    assert_int_equal(ret, OK);
}

static void TestTeleOutgoingCallActiveAndSendtones(void** state)
{
    (void)state;
    int ret = outgoing_call_active_and_send_tones(0);
    assert_int_equal(ret, OK);
}

static void TestTeleCallConnectAndLocalHangup(void** state)
{
    int ret = call_connect_and_local_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestTeleDialAndRemoteHangup(void** state)
{
    int ret = dial_and_remote_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestTeleIncomingCallandLocalHangup(void** state)
{
    (void)state;
    int ret = incoming_call_and_local_hangup(0);
    assert_int_equal(ret, 0);
}

// static void TestTeleRemoteHangupThenIncomingNewCall(void **state) {
//     int ret = call_dial_caller_reject_and_incoming(0);
//     assert_int_equal(ret, 0);
// }

// static void TestTeleRemoteHangupThenDialAnother(void **state) {
//     int ret = call_dial_caller_reject_and_dial_another(0);
//     assert_int_equal(ret, 0);
// }

static void TestTeleHangupCurrentCall(void** state)
{
    (void)state;
    int ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleHangupAllCall(void** state)
{
    (void)state;
    int ret = tapi_call_hangup_all_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleGetCallCount(void** state)
{
    (void)state;
    int ret = tapi_get_call_count(0);
    assert_true(ret >= 0);
}

static void TestTeleDialNumber(void** state)
{
    (void)state;
    int ret = tapi_dial_number(0);
    assert_int_equal(ret, OK);
    sleep(2);
}

static void TestTeleDialEccNumber(void** state)
{
    (void)state;
    int ret = tapi_dial_ecc_number(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDialLongPhoneNumber(void** state)
{
    (void)state;
    int ret = tapi_dial_with_long_phone_number(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDialShotPhoneNumber(void** state)
{
    (void)state;
    int ret = tapi_dial_with_short_phone_number(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDialWithEnableHideCallId(void** state)
{
    (void)state;
    int ret = tapi_dial_with_enable_hide_callerid(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDialWithDisabledHideCallId(void** state)
{
    (void)state;
    int ret = tapi_dial_with_disabled_hide_callerid(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDialWithDefaultHideCallId(void** state)
{
    (void)state;
    int ret = tapi_dial_with_default_hide_callerid(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

// static void TestTeleIncomingNewCallThenRemoteHangup(void **state) {
//     int ret = call_incoming_and_hangup_by_dialer_before_answer(0);
//     assert_int_equal(ret, 0);
// }

static void TestTeleDialWithAreaCode(void** state)
{
    (void)state;
    int ret = tapi_call_dial_using_phone_number_with_area_code_test(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDialWithPauseCode(void** state)
{
    (void)state;
    int ret = tapi_call_dial_using_phone_number_with_pause_code_test(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDialWithWaitCode(void** state)
{
    (void)state;
    int ret = tapi_call_dial_using_phone_number_with_wait_code_test(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDialWithNumerousCode(void** state)
{
    (void)state;
    int ret = tapi_call_dial_using_phone_number_with_numerous_code_test(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDialConference(void** state)
{
    (void)state;
    int ret = tapi_call_dial_conference_test(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hangup_all_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleCheckAleringStatusAfterDial(void** state)
{
    (void)state;
    int ret = call_check_alerting_status_after_dial(0);
    assert_int_equal(ret, OK);
}

// static void TestTeleCheckStatusInDialing(void **state)
// {
//     int ret = call_check_status_in_dialing(0);
//     assert_int_equal(ret, 0);
// }

// data testcases
static void TestTeleDataLoadApnContexts(void** state)
{
    (void)state;
    int ret = tapi_data_load_apn_contexts_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDataSaveApnContextSupl(void** state)
{
    (void)state;
    int ret = tapi_data_save_apn_context_test("0", "3", "supl", "supl", "2", "2");
    assert_int_equal(ret, OK);
}

static void TestTeleDataSaveApnContextEmergency(void** state)
{
    (void)state;
    int ret = tapi_data_save_apn_context_test("0", "7", "emergency",
        "emergency", "2", "2");
    assert_int_equal(ret, OK);
}

static void TestTeleDataSaveLongApnContex(void** state)
{
    (void)state;
    int ret = tapi_data_save_apn_context_test("0", "1",
        "longname-----------------------------------------"
        "-----------------------------------------longname",
        "cmnet4", "2", "2");
    assert_int_equal(ret, OK);
}

static void TestTeleDataSaveApnContext(void** state)
{
    (void)state;
    int ret = tapi_data_save_apn_context_test("0", "1", "cmcc1", "cmnet1", "2", "2");
    assert_int_equal(ret, 0);
}

static void TestTeleDataRemoveApnContext(void** state)
{
    (void)state;
    int ret = tapi_data_remove_apn_context_test("0", "/ril_0/context3");
    assert_int_equal(ret, OK);
}

static void TestTeleDataResetApnContexts(void** state)
{
    (void)state;
    int ret = tapi_data_reset_apn_contexts_test("0");
    assert_int_equal(ret, OK);
}

static void TestTeleDataResetApnContextsRepeatedly(void** state)
{
    (void)state;
    REPEAT_TEST_MORE_FOR
    {
        int ret = tapi_data_reset_apn_contexts_test("0");
        assert_int_equal(ret, OK);
    }
}

static void TestTeleDataEditApnName(void** state)
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

static void TestTeleDataEditApnType(void** state)
{
    (void)state;
    int ret = tapi_data_edit_apn_context_test("0", "/ril_0/context3", "3", "cmname", "cmname", "2", "2");
    assert_int_equal(ret, OK);
}

static void TestTeleDataEditApnProto(void** state)
{
    (void)state;
    int ret = tapi_data_edit_apn_context_test("0", "/ril_0/context3", "3", "cmname", "cmname", "0", "2");
    assert_int_equal(ret, OK);
}

static void TestTeleDataEditApnAuth(void** state)
{
    (void)state;
    int ret = tapi_data_edit_apn_context_test("0", "/ril_0/context3", "3", "cmname", "cmname", "0", "0");
    assert_int_equal(ret, OK);
}

static void TestTeleDataEditApnAll(void** state)
{
    (void)state;
    int ret;
    ret = tapi_data_edit_apn_context_test("0", "/ril_0/context3", "2", "cmnameall", "cmnameall", "1", "1");
    assert_int_equal(ret, OK);
    ret = tapi_data_load_apn_contexts_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDataEditApnAndRemove(void** state)
{
    (void)state;
    int ret;
    ret = tapi_data_edit_apn_context_test("0", "/ril_0/context3", "2", "cmnameall", "cmnameall", "2", "2");
    assert_int_equal(ret, OK);
    ret = tapi_data_remove_apn_context_test("0", "/ril_0/context3");
    assert_int_equal(ret, OK);
}

static void TestTeleDataEditApnAndReset(void** state)
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

static void TestTeleDataEditApnRepeatedlyAndLoad(void** state)
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

static void TestTeleDataEnable(void** state)
{
    (void)state;
    int ret = data_enabled_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDataDisable(void** state)
{
    (void)state;
    int ret = data_disabled_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDataEnableRepeatedly(void** state)
{
    (void)state;
    REPEAT_TEST_MORE_FOR
    {
        TestTeleDataEnable(state);
        TestTeleDataDisable(state);
    }
}

static void TestTeleDataIsEnable(void** state)
{
    (void)state;
    bool enable = false;
    int ret = tapi_data_get_enabled_test(&enable);
    assert_int_equal(ret, OK);
    assert_int_equal(enable, 1);
}

static void TestTeleDataIsDisable(void** state)
{
    (void)state;
    bool enable = true;
    int ret = tapi_data_get_enabled_test(&enable);
    assert_int_equal(ret, OK);
    assert_int_equal(enable, 0);
}

static void TestTeleDataRegister(void** state)
{
    (void)state;
    int ret = tapi_data_listen_data_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDataUnregister(void** state)
{
    (void)state;
    int ret = tapi_data_unlisten_data_test();
    assert_int_equal(ret, OK);
}

static void TestTeleDataRequestNetworkInternet(void** state)
{
    (void)state;
    int ret = data_request_network_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDataReleaseNetworkInternet(void** state)
{
    (void)state;
    int ret = data_release_network_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDataReleaseNetworkInternetRepeatedly(void** state)
{
    (void)state;
    REPEAT_TEST_MORE_FOR
    {
        TestTeleDataReleaseNetworkInternet(state);
        TestTeleDataRequestNetworkInternet(state);
    }
}

static void TestTeleDataRequestNetworkIms(void** state)
{
    (void)state;
    int ret = data_request_ims(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDataReleaseNetworkIms(void** state)
{
    (void)state;
    int ret = data_release_ims(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDataReleaseNetworkImsRepeatedly(void** state)
{
    (void)state;
    REPEAT_TEST_MORE_FOR
    {
        TestTeleDataRequestNetworkIms(state);
        TestTeleDataReleaseNetworkIms(state);
    }
}

static void TestTeleDataSetPreferredApn(void** state)
{
    (void)state;
    int ret = tapi_data_set_preferred_apn_test(0, "/ril_0/context1");
    assert_int_equal(ret, OK);
}

static void TestTeleDataGetPreferredApn(void** state)
{
    (void)state;
    int ret = tapi_data_get_preferred_apn_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDataSendScreenState(void** state)
{
    (void)state;
    int ret = tapi_data_send_screen_stat_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDataIsPsAttached(void** state)
{
    (void)state;
    int ret = tapi_data_is_ps_attached_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDataGetNetworkType(void** state)
{
    (void)state;
    int ret = tapi_data_get_network_type_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDataSetDefaultDataSlot(void** state)
{
    (void)state;
    int ret = tapi_data_set_default_data_slot_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleDataGetDefaultDataSlot(void** state)
{
    (void)state;
    sleep(2);
    int ret = tapi_data_get_default_data_slot_test();
    assert_int_equal(ret, OK);
}

static void TestTeleDataSetDataAllow(void** state)
{
    (void)state;
    int ret = tapi_data_set_data_allow_test(0);
    assert_true(ret == OK);
}

static void TestTeleDataGetCallList(void** state)
{
    (void)state;
    int ret = data_get_call_list(0);
    assert_true(ret == OK);
}

static void TestTeleEnableDataRoaming(void** state)
{
    (void)state;
    int ret = data_enable_roaming_test();
    assert_true(ret == OK);
}

static void TestTeleDisableDataRoaming(void** state)
{
    (void)state;
    int ret = data_disable_roaming_test();
    assert_true(ret == OK);
}

static void TestTeleSmsSetServiceCenterNum(void** state)
{
    (void)state;
    int ret = sms_set_service_center_number_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleSmsGetServiceCenterNum(void** state)
{
    (void)state;
    sleep(5);
    int ret = sms_check_service_center_number_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleSmsSendShortMessageInEnglish(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSmsSendShortMessageInChinese(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSmsSendShortDataMessageInEnglish(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSmsSendShortDataMessageInChinese(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSmsSendLongMessageInEnglish(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSmsSendLongMessageInChinese(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSmsSendLongDataMessageInEnglish(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSmsSendLongDataMessageInChinese(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSmsSendShortEnglishMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_message_in_dialing(0, phone_num, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSmsSendShortChineseMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_message_in_dialing(0, phone_num, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSmsSendLongEnglishMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_message_in_dialing(0, phone_num, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSmsSendLongChineseMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_message_in_dialing(0, phone_num, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSmsSendShortEnglishDataMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_dialing(0, phone_num, short_english_text, 0);
    assert_int_equal(ret, 0);
}

static void TestTeleSmsSendShortChineseDataMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_dialing(0, phone_num, short_chinese_text, 0);
    assert_int_equal(ret, 0);
}

static void TestTeleSmsSendLongEnglishDataMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_dialing(0, phone_num, long_english_text, 0);
    assert_int_equal(ret, 0);
}

static void TestTeleSmsSendLongChineseDataMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_dialing(0, phone_num, long_chinese_text, 0);
    assert_int_equal(ret, 0);
}

static void TestTeleSendEnglishMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendChineseMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendLongEnglishMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendLongChineseMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendEnglishDataMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendChineseDataMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendLongEnglishDataMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendLongChineseDataMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendEnglishMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendChineseMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendLongEnglishMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendLongChineseMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendEnglishDataMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendChineseDataMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendLongEnglishDataMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendLongChineseDataMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendEnglishMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendChineseMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendLongEnglishMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendLongChineseMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendEnglishDataMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendChineseDataMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendLongEnglishDataMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSendLongChineseDataMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleSetSmsDefaultSlot(void** state)
{
    (void)state;
    int ret = tapi_sms_set_default_slot(get_tapi_ctx(), 0);
    sleep(5);
    assert_int_equal(ret, 0);
}

static void TestTeleGetSmsDefaultSlot(void** state)
{
    (void)state;
    int result = -1;
    int ret = tapi_sms_get_default_slot(context, &result);
    syslog(LOG_INFO, "%s, ret: %d, result: %d", __func__, ret, result);
    assert_int_equal(ret, 0);
    assert_int_equal(result, 0);
}

static void TestTeleSmsSetCellBroadcastPower(void** state)
{
    (void)state;
    int ret = tapi_sms_set_cell_broadcast_power_on(get_tapi_ctx(), 0, 1);
    sleep(5);
    assert_int_equal(ret, 0);
}

static void TestTeleSmsGetCellBroadcastPower(void** state)
{
    (void)state;
    bool result = false;
    int ret = tapi_sms_get_cell_broadcast_power_on(get_tapi_ctx(), 0, &result);
    syslog(LOG_INFO, "%s, ret: %d, result: %d", __func__, 0, (int)result);
    assert_int_equal(ret, 0);
    assert_int_equal(result, 1);
}

static void TestTeleSmsSetCellBroadcastTopics(void** state)
{
    (void)state;
    int ret = tapi_sms_set_cell_broadcast_topics(get_tapi_ctx(), 0, "1");
    sleep(5);
    assert_int_equal(ret, 0);
}

static void TestTeleSmsGetCellBroadcastTopics(void** state)
{
    (void)state;
    char* result = NULL;
    int ret = tapi_sms_get_cell_broadcast_topics(get_tapi_ctx(), 0, &result);
    assert_int_equal(ret, 0);
    assert_int_equal(strcmp(result, "1"), 0);
}

static void TestTeleNetSelectAuto(void** state)
{
    (void)state;
    int ret = tapi_net_select_auto_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleNetSelectManual(void** state)
{
    (void)state;
    sleep(4);
    int ret = tapi_net_select_manual_test(0, "310", "260", "lte");
    sleep(4);
    assert_int_equal(ret, OK);
}

static void TestTeleNetScan(void** state)
{
    (void)state;
    int ret = tapi_net_scan_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleNetGetServingCellinfos(void** state)
{
    (void)state;
    int ret = tapi_net_get_serving_cellinfos_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleNetGetNeighbouringCellInfos(void** state)
{
    (void)state;
    int ret = tapi_net_get_neighbouring_cellInfos_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleNetRegistrationInfo(void** state)
{
    (void)state;
    int ret = tapi_net_registration_info_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleNetGetOperatorName(void** state)
{
    (void)state;
    int ret = tapi_net_get_operator_name_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleNetQuerySignalstrength(void** state)
{
    (void)state;
    int ret = tapi_net_query_signalstrength_test(0);
    assert_int_equal(ret, OK);
}

// static void TestTeleNetSetCellInfoListRate(void **state)
// {
//     int ret = tapi_net_set_cell_info_list_rate_test(0, 10);
//     assert_int_equal(ret, OK);
// }

static void TestTeleNetGetVoiceRegistered(void** state)
{
    (void)state;
    int ret = tapi_net_get_voice_registered_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleGetVoiceNwType(void** state)
{
    (void)state;
    tapi_network_type type = NETWORK_TYPE_UNKNOWN;
    int ret = tapi_network_get_voice_network_type(get_tapi_ctx(), 0, &type);
    syslog(LOG_INFO, "%s, ret: %d, type: %d", __func__, ret, (int)type);
    assert_int_equal(ret, OK);
    assert_int_equal((int)type, 13);
}

static void TestTeleGetVoiceRoaming(void** state)
{
    (void)state;
    bool value = true;
    int ret = tapi_network_is_voice_roaming(get_tapi_ctx(), 0, &value);
    syslog(LOG_INFO, "%s, ret: %d, value: %d", __func__, ret, (int)value);
    assert_int_equal(ret, OK);
    assert_int_equal((int)value, 0);
}

// modem
static void TestTeleModemGetImei(void** state)
{
    (void)state;
    int ret = tapi_get_imei_test(0);
    assert_int_equal(ret, OK);
}

// static void TestTeleModemSetUmtsPrefNetMode(void** state)
// {
//     // defalut rat mode is NETWORK_PREF_NET_TYPE_LTE_GSM_WCDMA (9)
//     tapi_pref_net_mode set_value = NETWORK_PREF_NET_TYPE_UMTS;
//     int ret = tapi_set_pref_net_mode_test(0, set_value);
//     assert_int_equal(ret, OK);
// }

// static void TestTeleModemSetGsmOnlyPrefNetMode(void** state)
// {
//     tapi_pref_net_mode set_value = NETWORK_PREF_NET_TYPE_GSM_ONLY;
//     int ret = tapi_set_pref_net_mode_test(0, set_value);
//     assert_int_equal(ret, OK);
// }

// static void TestTeleModemSetWcdmaOnlyPrefNetMode(void** state)
// {
//     tapi_pref_net_mode set_value = NETWORK_PREF_NET_TYPE_WCDMA_ONLY;
//     int ret = tapi_set_pref_net_mode_test(0, set_value);
//     assert_int_equal(ret, OK);
// }

// static void TestTeleModemSetLteOnlyPrefNetMode(void** state)
// {
//     tapi_pref_net_mode set_value = NETWORK_PREF_NET_TYPE_LTE_ONLY;
//     int ret = tapi_set_pref_net_mode_test(0, set_value);
//     assert_int_equal(ret, OK);
// }

// static void TestTeleModemSetLteWcdmaPrefNetMode(void** state)
// {
//     tapi_pref_net_mode set_value = NETWORK_PREF_NET_TYPE_LTE_WCDMA;
//     int ret = tapi_set_pref_net_mode_test(0, set_value);
//     assert_int_equal(ret, OK);
// }

// static void TestTeleModemSetLteGsmWcdmaPrefNetMode(void** state)
// {
//     tapi_pref_net_mode set_value = NETWORK_PREF_NET_TYPE_LTE_GSM_WCDMA;
//     int ret = tapi_set_pref_net_mode_test(0, set_value);
//     assert_int_equal(ret, OK);
// }

static void TestTeleModemGetPrefNetMode(void** state)
{
    sleep(5);
    tapi_pref_net_mode get_value = NETWORK_PREF_NET_TYPE_ANY;
    int ret = tapi_get_pref_net_mode_test(0, &get_value);
    assert_int_equal(ret, OK);
}

static void TestTeleModemSetRadioPowerOn(void** state)
{
    (void)state;
    int ret = tapi_set_radio_power_test(0, 1);
    assert_int_equal(ret, OK);
    sleep(10);
}

static void TestTeleModemSetRadioPowerOff(void** state)
{
    (void)state;
    int ret = tapi_set_radio_power_test(0, 0);
    assert_int_equal(ret, OK);
    sleep(10);
}

static void TestTeleModemSetRadioPowerOnOffRepeatedly(void** state)
{
    (void)state;
    REPEAT_TEST_LESS_FOR
    {
        TestTeleModemSetRadioPowerOn(state);
        TestTeleModemSetRadioPowerOff(state);
    }
}

static void TestTeleGetModemEnableStatus(void** state)
{
    (void)state;
    int get_state = 1;
    int ret = tapi_get_modem_status_test(0, &get_state);
    assert_int_equal(ret, OK);
}

static void TestTeleGetModemDsiableStatus(void** state)
{
    (void)state;
    int get_state = 0;
    int ret = tapi_get_modem_status_test(0, &get_state);
    assert_int_equal(ret, OK);
}

static void TestTeleModemEnable(void** state)
{
    (void)state;
    int ret = tapi_enable_modem_test(0, 1);
    assert_int_equal(ret, OK);
    sleep(10);
}

static void TestTeleModemDisable(void** state)
{
    (void)state;
    int ret = tapi_enable_modem_test(0, 0);
    assert_int_equal(ret, OK);
    sleep(10);
}

static void TestTeleModemEnableDisableRepeatedly(void** state)
{
    (void)state;
    REPEAT_TEST_LESS_FOR
    {
        TestTeleModemEnable(state);
        TestTeleGetModemEnableStatus(state);
        TestTeleModemDisable(state);
        TestTeleGetModemDsiableStatus(state);
    }
}

static void TestTeleModemRegister(void** state)
{
    (void)state;
    int ret = tapi_modem_register_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleModemUnregister(void** state)
{
    (void)state;
    int ret = tapi_modem_unregister_test();
    assert_true(ret == OK);
}

static void TestTeleGetModemRevision(void** state)
{
    int ret;
    ret = tapi_get_modem_revision_test(0);
    assert_int_equal(ret, OK);
}

// static void TestTeleImsServiceStatus(void** state)
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

// static void TestTeleModemDialCall(void** state)
// {
//     int slot_id = 0;
//     int ret1 = tapi_call_listen_call_test(slot_id);
//     int ret2 = tapi_call_dial_test(slot_id, phone_num, 0);
//     int ret = ret1 || ret2;
//     assert_int_equal(ret, OK);
//     sleep(2);
// }

// static void TestTeleModemHangupCall(void** state)
// {
//     int slot_id = 0;
//     int ret1 = tapi_call_hanup_current_call_test(slot_id);
//     int ret2 = tapi_call_unlisten_call_test();
//     int ret = ret1 || ret2;
//     assert_int_equal(ret, OK);
// }

static void TestTeleModemInvokeOemShotRilRequestRaw(void** state)
{
    int ret = tapi_invoke_oem_ril_request_raw_test(0, "01A0B023", 4);
    assert_int_equal(ret, OK);
}

static void TestTeleModemInvokeOemLongRilRequestRaw(void** state)
{
    int ret = tapi_invoke_oem_ril_request_raw_test(0, "01A0B02301A0B02301A0B02301A0B02301A0B02301", 21);
    assert_int_equal(ret, OK);
}

static void TestTeleModemInvokeOemSeperateRilRequestRaw(void** state)
{
    int ret = tapi_invoke_oem_ril_request_raw_test(0, "10|22", 2);
    assert_int_equal(ret, OK);
}

static void TestTeleModemInvokeOemNormalRilRequestRaw(void** state)
{
    int ret = tapi_invoke_oem_ril_request_raw_test(0, "01A0B023", 2);
    assert_int_equal(ret, OK);
}

static void TestTeleModemInvokeOemRilRequestATCmdStrings(void** state)
{
    char* req_data = "AT+CPIN?";
    int ret = tapi_invoke_oem_ril_request_strings_test(0, req_data, 1);
    assert_int_equal(ret, OK);
}

static void TestTeleModemInvokeOemRilRequestNotATCmdStrings(void** state)
{
    // not AT cmd
    char* req_data = "10|22";
    int ret = tapi_invoke_oem_ril_request_strings_test(0, req_data, 2);
    assert_int_equal(ret, OK);
}

static void TestTeleModemInvokeOemRilRequestHexStrings(void** state)
{
    char* req_data = "0x10|0x01";
    int ret = tapi_invoke_oem_ril_request_strings_test(0, req_data, 2);
    assert_int_equal(ret, OK);
}

// static void TestTeleModemInvokeOemRilRequestLongStrings(void **state)
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

static void TestTeleImsListen(void** state)
{
    (void)state;
    int ret = tapi_ims_listen_ims_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleImsTurnOn(void** state)
{
    (void)state;
    int ret = tapi_ims_turn_on_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleImsGetRegistration(void** state)
{
    (void)state;
    int ret = tapi_ims_get_registration_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleImsGetEnabled(void** state)
{
    (void)state;
    int ret = tapi_ims_get_enabled_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleImsSetServiceStatus(void** state)
{
    (void)state;
    int ret = tapi_ims_set_service_status_test(0, 5);
    assert_int_equal(ret, 0);
    sleep(5);
}

static void TestTeleImsSetSmsCap(void** state)
{
    (void)state;
    int ret = tapi_ims_set_service_status_test(0, 4);
    assert_int_equal(ret, 0);
}

static void TestTeleImsSetSmsVoiceCap(void** state)
{
    (void)state;
    int ret = tapi_ims_set_service_status_test(0, 5);
    assert_int_equal(ret, 0);
}

static void TestTeleImsResetImsCap(void** state)
{
    (void)state;
    int ret = tapi_ims_set_service_status_test(0, 1);
    assert_int_equal(ret, 0);
    sleep(10);
}

static void TestTeleImsTurnOff(void** state)
{
    (void)state;
    int ret = tapi_ims_turn_off_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleImsTurnOnOff(void** state)
{
    (void)state;
    REPEAT_TEST_LESS_FOR
    {
        TestTeleImsTurnOn(state);
        TestTeleImsGetEnabled(state);
        TestTeleImsGetRegistration(state);
        TestTeleImsTurnOff(state);
    }
}

static void TestTeleSSRegister(void** state)
{
    (void)state;
    int ret = tapi_listen_ss_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleSSUnRegister(void** state)
{
    (void)state;
    int ret = tapi_unlisten_ss_test();
    assert_int_equal(ret, 0);
}

static void TestTeleRequestCallBarring(void** state)
{
    (void)state;
    int ret = tapi_ss_request_call_barring_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleSetCallBarring(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_barring_option_test(0, "AI", "1234");
    assert_int_equal(ret, 0);
}

static void TestTeleGetCallBarring(void** state)
{
    (void)state;
    sleep(5);
    int ret = tapi_ss_get_call_barring_option_test(0, "VoiceIncoming", "always");
    assert_int_equal(ret, 0);
}

static void TestTeleChangeCallBarringPassword(void** state)
{
    (void)state;
    int ret = tapi_ss_change_call_barring_password_test(0, "1234", "2345");
    assert_int_equal(ret, 0);
}

static void TestTeleResetCallBarringPassword(void** state)
{
    (void)state;
    int ret = tapi_ss_change_call_barring_password_test(0, "2345", "1234");
    assert_int_equal(ret, 0);
}

static void TestTeleDisableAllIncoming(void** state)
{
    (void)state;
    int ret = tapi_ss_disable_all_incoming_test(0, "1234");
    assert_int_equal(ret, 0);
}

static void TestTeleDisableAllOutgoing(void** state)
{
    (void)state;
    int ret = tapi_ss_disable_all_outgoing_test(0, "1234");
    assert_int_equal(ret, 0);
}

static void TestTeleDisableAllCallBarrings(void** state)
{
    (void)state;
    int ret = tapi_ss_disable_all_call_barrings_test(0, "1234");
    assert_int_equal(ret, 0);
}

static void TestTeleSetCallForwardingUnConditional(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 0, "10086");
    assert_int_equal(ret, 0);
}

static void TestTeleGetCallForwardingUnConditional(void** state)
{
    (void)state;
    int ret = tapi_ss_get_call_forwarding_option_test(0, 0);
    assert_int_equal(ret, 0);
}

static void TestTeleClearCallForwardingUnconditional(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 0, "\0");
    assert_int_equal(ret, 0);
}

static void TestTeleSetCallForwardingBusy(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 1, "10086");
    assert_int_equal(ret, 0);
}

static void TestTeleGetCallForwardingBusy(void** state)
{
    (void)state;
    int ret = tapi_ss_get_call_forwarding_option_test(0, 1);
    assert_int_equal(ret, 0);
}

static void TestTeleClearCallForwardingBusy(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 1, "\0");
    assert_int_equal(ret, 0);
}

static void TestTeleSetCallForwardingNoReply(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 2, "10086");
    assert_int_equal(ret, 0);
}

static void TestTeleGetCallForwardingNoReply(void** state)
{
    (void)state;
    int ret = tapi_ss_get_call_forwarding_option_test(0, 2);
    assert_int_equal(ret, 0);
}

static void TestTeleClearCallForwardingNoReply(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 2, "\0");
    assert_int_equal(ret, 0);
}

static void TestTeleSetCallForwardingNotReachable(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 3, "10086");
    assert_int_equal(ret, 0);
}

static void TestTeleGetCallForwardingNotReachable(void** state)
{
    (void)state;
    int ret = tapi_ss_get_call_forwarding_option_test(0, 3);
    assert_int_equal(ret, 0);
}

static void TestTeleClearCallForwardingNotReachable(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 3, "\0");
    assert_int_equal(ret, 0);
}

static void TestTeleEnableCallWaiting(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_waiting_test(0, true);
    assert_int_equal(ret, 0);
}

static void TestTeleGetEnableCallWaiting(void** state)
{
    (void)state;
    int ret = tapi_ss_get_call_waiting_test(0, true);
    assert_int_equal(ret, 0);
}

static void TestTeleDisableCallWaiting(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_waiting_test(0, false);
    assert_int_equal(ret, 0);
}

static void TestTeleGetDisableCallWaiting(void** state)
{
    (void)state;
    int ret = tapi_ss_get_call_waiting_test(0, false);
    assert_int_equal(ret, 0);
}

static void TestTeleEnableFdn(void** state)
{
    (void)state;
    int ret = tapi_ss_enable_fdn_test(0, true, "1234");
    assert_int_equal(ret, 0);
}

static void TestTeleGetFdnEnabled(void** state)
{
    (void)state;
    int ret = tapi_ss_query_fdn_test(0, true);
    assert_int_equal(ret, 0);
}

static void TestTeleDisableFdn(void** state)
{
    (void)state;
    int ret = tapi_ss_enable_fdn_test(0, false, "1234");
    assert_int_equal(ret, 0);
}

static void TestTeleGetFdnDisabled(void** state)
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
        cmocka_unit_test(TestTeleHasIccCard),
        cmocka_unit_test(TestTeleHasIccCardNumerousTimes),
        cmocka_unit_test(TestTeleGetSimOperatorName),
        cmocka_unit_test(TestTeleGetSimOperatorNameNumerousTimes),
        cmocka_unit_test(TestTeleGetSimOperator),
        cmocka_unit_test(TestTeleGetSimOperatorNumerousTimes),
        cmocka_unit_test(TestTeleGetSimSubscriberId),
        cmocka_unit_test(TestTeleGetSimSubscriberIdNumerousTimes),
        cmocka_unit_test(TestTeleGetIccId),
        cmocka_unit_test(TestTeleGetIccIdNumerousTimes),
        cmocka_unit_test(TestTeleGetMSISDN),
        cmocka_unit_test(TestTeleGetMSISDNNumerousTimes),
        cmocka_unit_test(TestTeleTransmitAPDUInBasicChannel),
        cmocka_unit_test(TestTeleOpenLogicalChannel),
        cmocka_unit_test(TestTeleCloseLogicalChannel),
        cmocka_unit_test(TestTeleLogicalChannelOpenCloseNumerous),
        cmocka_unit_test(TestTeleTransmitAPDUInLogicalChannel),
        cmocka_unit_test(TestTeleSetUiccEnablement),
        cmocka_unit_test(TestTeleGetUiccEnablement),
        cmocka_unit_test(TestTeleTransmitAPDUBasicChannel),
        cmocka_unit_test(TestTeleGetSimState),
        cmocka_unit_test(TestTeleEnterPin),
        cmocka_unit_test(TestTeleChangePin),
        cmocka_unit_test(TestTeleLockPin),
        cmocka_unit_test(TestTeleUnlockPin),
        cmocka_unit_test(TestTeleLoadAdnEntries),
        cmocka_unit_test(TestTeleLoadFdnEntries),
        cmocka_unit_test(TestTeleInsertFdnEntry),
        cmocka_unit_test(TestTeleUpdateFdnEntry),
        cmocka_unit_test(TestTeleDeleteFdnEntry),
    };

    const struct CMUnitTest CallTestSuites[] = {
        cmocka_unit_test(TestTeleListenCall),
        cmocka_unit_test(TestTeleDialNumber),
        cmocka_unit_test(TestTeleHangupCurrentCall),
        cmocka_unit_test(TestTeleDialEccNumber),
        cmocka_unit_test(TestTeleDialLongPhoneNumber),
        cmocka_unit_test(TestTeleDialShotPhoneNumber),
        cmocka_unit_test(TestTeleDialWithEnableHideCallId),
        cmocka_unit_test(TestTeleDialWithDisabledHideCallId),
        cmocka_unit_test(TestTeleDialWithDefaultHideCallId),
        cmocka_unit_test(TestTeleDialWithAreaCode),
        cmocka_unit_test(TestTeleDialWithPauseCode),
        cmocka_unit_test(TestTeleDialWithWaitCode),
        cmocka_unit_test(TestTeleDialWithNumerousCode),
        cmocka_unit_test(TestTeleDialConference),
        cmocka_unit_test(TestTeleCheckAleringStatusAfterDial),
        cmocka_unit_test(TestTeleStartDtmf),
        cmocka_unit_test(TestTeleStopDtmf),
        cmocka_unit_test(TestTeleGetCallCount),
        cmocka_unit_test(TestTeleHangupAllCall),
        // cmocka_unit_test(TestTeleLoadEccList),
        cmocka_unit_test(TestTeleSetVoicecallSlot),
        cmocka_unit_test(TestTeleGetVoicecallSlot),
        cmocka_unit_test(TestTeleClearVoicecallSlotSet),
        // hangup between dialing and answering
        cmocka_unit_test(TestTeleHangupBetweenDialingAndAnswering),
        cmocka_unit_test(TestTeleIncomingCallAnswerAndHangup),
        cmocka_unit_test(TestTeleIncomingCallAnswerAndRemoteHangup),
        cmocka_unit_test(TestTeleEnableCallWaiting),
        cmocka_unit_test(TestTeleCallReleaseAndAnswer),
        cmocka_unit_test(TestTeleCallHoldAndAnswerOtherCall),
        cmocka_unit_test(TestTeleCallMergeByUser),
        cmocka_unit_test(TestTeleCallSeparateByUser),
        cmocka_unit_test(TestTeleCallReleaseAndSwapOtherCall),
        cmocka_unit_test(TestTeleDisableCallWaiting),
        cmocka_unit_test(TestTeleOutgoingCallRemoteAnswerAndHangup),
        cmocka_unit_test(TestTeleOutgoingCallHoldAndHangup),
        cmocka_unit_test(TestTeleOutgoingCallActiveAndSendtones),
        cmocka_unit_test(TestTeleCallConnectAndLocalHangup),
        cmocka_unit_test(TestTeleDialAndRemoteHangup),
        cmocka_unit_test(TestTeleIncomingCallandLocalHangup),
        cmocka_unit_test(TestTeleUnlistenCall),
#if 0

        // answer the incoming call then hangup it
        cmocka_unit_test(TestTeleAnswerAndHangupTheIncomingCall),

        // dial again after remote hangup
        // cmocka_unit_test(TestTeleDialAgainAfterRemoteHangup),

        // hangup after remote answer
        cmocka_unit_test(TestTeleHangupCallAfterRemoteAnswer),

        // dial then remote answer and hangup
        // cmocka_unit_test(TestTeleDialThenRemoteAnswerAndHangup),

        // remote hangup then incoming new call
        // cmocka_unit_test(TestTeleRemoteHangupThenIncomingNewCall),

        // remote hangup then dial another
        // cmocka_unit_test(TestTeleRemoteHangupThenDialAnother),

        // dial with numerous hide call id
        cmocka_unit_test(TestTeleDialWithNumerousHideCallId),

        // incoming new call then remote hangup
        // cmocka_unit_test(TestTeleIncomingNewCallThenRemoteHangup),

        // dial using a phone number with area code
        cmocka_unit_test(TestTeleDialWithAreaCode),
#endif
    };

    const struct CMUnitTest DataTestSuites[] = {
        cmocka_unit_test(TestTeleDataRegister),
        cmocka_unit_test(TestTeleDataUnregister),
        cmocka_unit_test(TestTeleDataRegister),
        cmocka_unit_test(TestTeleDataLoadApnContexts),
        cmocka_unit_test(TestTeleDataSaveApnContext),
        cmocka_unit_test(TestTeleDataRemoveApnContext),
        cmocka_unit_test(TestTeleDataResetApnContexts),
        cmocka_unit_test(TestTeleDataResetApnContextsRepeatedly),
        cmocka_unit_test(TestTeleDataEditApnName),
        cmocka_unit_test(TestTeleDataEditApnType),
        cmocka_unit_test(TestTeleDataEditApnProto),
        cmocka_unit_test(TestTeleDataEditApnAuth),
        cmocka_unit_test(TestTeleDataEditApnAll),
        cmocka_unit_test(TestTeleDataEditApnAndRemove),
        cmocka_unit_test(TestTeleDataEditApnAndReset),
        cmocka_unit_test(TestTeleDataEditApnRepeatedlyAndLoad),
        cmocka_unit_test(TestTeleDataResetApnContexts),
        cmocka_unit_test(TestTeleDataEnable),
        cmocka_unit_test(TestTeleDataIsEnable),
        cmocka_unit_test(TestTeleDataDisable),
        cmocka_unit_test(TestTeleDataIsDisable),
        cmocka_unit_test(TestTeleDataEnableRepeatedly),
        cmocka_unit_test(TestTeleDataEnable),
        cmocka_unit_test(TestTeleDataReleaseNetworkInternet),
        cmocka_unit_test(TestTeleDataRequestNetworkInternet),
        cmocka_unit_test(TestTeleDataReleaseNetworkInternetRepeatedly),
        cmocka_unit_test(TestTeleDataDisable),
        cmocka_unit_test(TestTeleDataRequestNetworkIms),
        cmocka_unit_test(TestTeleDataReleaseNetworkIms),
        cmocka_unit_test(TestTeleDataReleaseNetworkImsRepeatedly),
        cmocka_unit_test(TestTeleDataSaveApnContextSupl),
        cmocka_unit_test(TestTeleDataSaveApnContextEmergency),
        cmocka_unit_test(TestTeleDataResetApnContexts),
        cmocka_unit_test(TestTeleDataSetPreferredApn),
        cmocka_unit_test(TestTeleDataGetPreferredApn),
        cmocka_unit_test(TestTeleDataSendScreenState),
        cmocka_unit_test(TestTeleDataGetNetworkType),
        cmocka_unit_test(TestTeleDataIsPsAttached),
        cmocka_unit_test(TestTeleDataSetDefaultDataSlot),
        cmocka_unit_test(TestTeleDataGetDefaultDataSlot),
        cmocka_unit_test(TestTeleDataSetDataAllow),
        cmocka_unit_test(TestTeleDataGetCallList),
        cmocka_unit_test(TestTeleDataSaveLongApnContex),
        cmocka_unit_test(TestTeleDataResetApnContexts),
        cmocka_unit_test(TestTeleEnableDataRoaming),
        cmocka_unit_test(TestTeleDisableDataRoaming),
        cmocka_unit_test(TestTeleDataUnregister),
    };

    const struct CMUnitTest SmsTestSuites[] = {
        cmocka_unit_test(TestTeleListenCall),
        cmocka_unit_test(TestTeleImsListen),
        cmocka_unit_test(TestTeleImsTurnOn),
        cmocka_unit_test(TestTeleSmsSetServiceCenterNum),
        cmocka_unit_test(TestTeleSmsGetServiceCenterNum),
        cmocka_unit_test(TestTeleSmsSendShortMessageInEnglish),
        cmocka_unit_test(TestTeleSmsSendShortMessageInChinese),
        cmocka_unit_test(TestTeleSmsSendShortDataMessageInEnglish),
        cmocka_unit_test(TestTeleSmsSendShortDataMessageInChinese),
        cmocka_unit_test(TestTeleSmsSendLongMessageInEnglish),
        cmocka_unit_test(TestTeleSmsSendLongMessageInChinese),
        cmocka_unit_test(TestTeleSmsSendLongDataMessageInEnglish),
        cmocka_unit_test(TestTeleSmsSendLongDataMessageInChinese),
        cmocka_unit_test(TestTeleSmsSendShortEnglishMessageInDialing),
        cmocka_unit_test(TestTeleSmsSendShortChineseMessageInDialing),
        cmocka_unit_test(TestTeleSmsSendLongEnglishMessageInDialing),
        cmocka_unit_test(TestTeleSmsSendLongChineseMessageInDialing),
        cmocka_unit_test(TestTeleSmsSendShortEnglishDataMessageInDialing),
        cmocka_unit_test(TestTeleSmsSendShortChineseDataMessageInDialing),
        cmocka_unit_test(TestTeleSmsSendLongEnglishDataMessageInDialing),
        cmocka_unit_test(TestTeleSmsSendLongChineseDataMessageInDialing),
        cmocka_unit_test(TestTeleSendEnglishMessageInVoiceImsCap),
        cmocka_unit_test(TestTeleSendChineseMessageInVoiceImsCap),
        cmocka_unit_test(TestTeleSendLongEnglishMessageInVoiceImsCap),
        cmocka_unit_test(TestTeleSendLongChineseMessageInVoiceImsCap),
        cmocka_unit_test(TestTeleSendEnglishDataMessageInVoiceImsCap),
        cmocka_unit_test(TestTeleSendChineseDataMessageInVoiceImsCap),
        cmocka_unit_test(TestTeleSendLongEnglishDataMessageInVoiceImsCap),
        cmocka_unit_test(TestTeleSendLongChineseDataMessageInVoiceImsCap),
        cmocka_unit_test(TestTeleImsSetSmsCap),
        cmocka_unit_test(TestTeleSendEnglishMessageInSmsImsCap),
        cmocka_unit_test(TestTeleSendChineseMessageInSmsImsCap),
        cmocka_unit_test(TestTeleSendLongEnglishMessageInSmsImsCap),
        cmocka_unit_test(TestTeleSendLongChineseMessageInSmsImsCap),
        cmocka_unit_test(TestTeleSendEnglishDataMessageInSmsImsCap),
        cmocka_unit_test(TestTeleSendChineseDataMessageInSmsImsCap),
        cmocka_unit_test(TestTeleSendLongEnglishDataMessageInSmsImsCap),
        cmocka_unit_test(TestTeleSendLongChineseDataMessageInSmsImsCap),
        cmocka_unit_test(TestTeleImsSetSmsVoiceCap),
        cmocka_unit_test(TestTeleSendEnglishMessageInSmsVoiceCap),
        cmocka_unit_test(TestTeleSendChineseMessageInSmsVoiceCap),
        cmocka_unit_test(TestTeleSendLongEnglishMessageInSmsVoiceCap),
        cmocka_unit_test(TestTeleSendLongChineseMessageInSmsVoiceCap),
        cmocka_unit_test(TestTeleSendEnglishDataMessageInSmsVoiceCap),
        cmocka_unit_test(TestTeleSendChineseDataMessageInSmsVoiceCap),
        cmocka_unit_test(TestTeleSendLongEnglishDataMessageInSmsVoiceCap),
        cmocka_unit_test(TestTeleSendLongChineseDataMessageInSmsVoiceCap),
        cmocka_unit_test(TestTeleSetSmsDefaultSlot),
        cmocka_unit_test(TestTeleGetSmsDefaultSlot),
        cmocka_unit_test(TestTeleSmsSetCellBroadcastPower),
        cmocka_unit_test(TestTeleSmsGetCellBroadcastPower),
        cmocka_unit_test(TestTeleSmsSetCellBroadcastTopics),
        cmocka_unit_test(TestTeleSmsGetCellBroadcastTopics),
        cmocka_unit_test(TestTeleImsResetImsCap),
        cmocka_unit_test(TestTeleImsTurnOff),
        cmocka_unit_test(TestTeleUnlistenCall),
    };

    const struct CMUnitTest NetTestSuites[] = {
        cmocka_unit_test(TestTeleNetSelectManual),
        cmocka_unit_test(TestTeleNetSelectAuto),
        cmocka_unit_test(TestTeleNetScan),
        cmocka_unit_test(TestTeleNetGetServingCellinfos),
        cmocka_unit_test(TestTeleNetGetNeighbouringCellInfos),
        cmocka_unit_test(TestTeleNetRegistrationInfo),
        cmocka_unit_test(TestTeleNetGetOperatorName),
        cmocka_unit_test(TestTeleNetQuerySignalstrength),
        //      cmocka_unit_test(TestTeleNetSetCellInfoListRate),
        cmocka_unit_test(TestTeleNetGetVoiceRegistered),
        cmocka_unit_test(TestTeleGetVoiceNwType),
        cmocka_unit_test(TestTeleGetVoiceRoaming),
    };

    const struct CMUnitTest ImsTestSuits[] = {
        cmocka_unit_test(TestTeleImsTurnOn),
        cmocka_unit_test(TestTeleImsGetRegistration),
        cmocka_unit_test(TestTeleImsGetEnabled),
        cmocka_unit_test(TestTeleImsSetServiceStatus),
        cmocka_unit_test(TestTeleImsResetImsCap),
        cmocka_unit_test(TestTeleImsTurnOff),
        cmocka_unit_test(TestTeleImsTurnOnOff),
    };

    const struct CMUnitTest SSTestSuits[] = {
        cmocka_unit_test(TestTeleSSRegister),
        cmocka_unit_test(TestTeleSSUnRegister),
        cmocka_unit_test(TestTeleRequestCallBarring),
        cmocka_unit_test(TestTeleSetCallBarring),
        cmocka_unit_test(TestTeleGetCallBarring),
        cmocka_unit_test(TestTeleChangeCallBarringPassword),
        cmocka_unit_test(TestTeleResetCallBarringPassword),
        cmocka_unit_test(TestTeleDisableAllIncoming),
        cmocka_unit_test(TestTeleDisableAllOutgoing),
        cmocka_unit_test(TestTeleDisableAllCallBarrings),
        cmocka_unit_test(TestTeleSetCallForwardingUnConditional),
        cmocka_unit_test(TestTeleGetCallForwardingUnConditional),
        cmocka_unit_test(TestTeleClearCallForwardingUnconditional),
        cmocka_unit_test(TestTeleSetCallForwardingBusy),
        cmocka_unit_test(TestTeleGetCallForwardingBusy),
        cmocka_unit_test(TestTeleClearCallForwardingBusy),
        cmocka_unit_test(TestTeleSetCallForwardingNoReply),
        cmocka_unit_test(TestTeleGetCallForwardingNoReply),
        cmocka_unit_test(TestTeleClearCallForwardingNoReply),
        cmocka_unit_test(TestTeleSetCallForwardingNotReachable),
        cmocka_unit_test(TestTeleGetCallForwardingNotReachable),
        cmocka_unit_test(TestTeleClearCallForwardingNotReachable),
        cmocka_unit_test(TestTeleEnableCallWaiting),
        cmocka_unit_test(TestTeleGetEnableCallWaiting),
        cmocka_unit_test(TestTeleDisableCallWaiting),
        cmocka_unit_test(TestTeleGetDisableCallWaiting),
        cmocka_unit_test(TestTeleEnableFdn),
        cmocka_unit_test(TestTeleGetFdnEnabled),
        cmocka_unit_test(TestTeleDisableFdn),
        cmocka_unit_test(TestTeleGetFdnDisabled),
    };

    const struct CMUnitTest CommonTestSuites[] = {
        cmocka_unit_test(TestTeleModemGetImei),
        // cmocka_unit_test(TestTeleModemSetUmtsPrefNetMode),
        // cmocka_unit_test(TestTeleModemSetGsmOnlyPrefNetMode),
        // cmocka_unit_test(TestTeleModemSetWcdmaOnlyPrefNetMode),
        // cmocka_unit_test(TestTeleModemSetLteOnlyPrefNetMode),
        // cmocka_unit_test(TestTeleModemSetLteWcdmaPrefNetMode),
        // cmocka_unit_test(TestTeleModemSetLteGsmWcdmaPrefNetMode),
        cmocka_unit_test(TestTeleModemGetPrefNetMode),
        cmocka_unit_test(TestTeleModemRegister),
        cmocka_unit_test(TestTeleModemUnregister),
        cmocka_unit_test(TestTeleModemInvokeOemShotRilRequestRaw),
        cmocka_unit_test(TestTeleModemInvokeOemLongRilRequestRaw),
        cmocka_unit_test(TestTeleModemInvokeOemNormalRilRequestRaw),
        cmocka_unit_test(TestTeleModemInvokeOemSeperateRilRequestRaw),
        cmocka_unit_test(TestTeleModemInvokeOemRilRequestATCmdStrings),
        cmocka_unit_test(TestTeleModemInvokeOemRilRequestNotATCmdStrings),
        cmocka_unit_test(TestTeleModemInvokeOemRilRequestHexStrings),
        cmocka_unit_test(TestTeleImsListen),
        cmocka_unit_test(TestTeleGetModemRevision),
        cmocka_unit_test(TestTeleModemDisable),
        cmocka_unit_test(TestTeleGetModemDsiableStatus),
        cmocka_unit_test(TestTeleModemEnableDisableRepeatedly),
        cmocka_unit_test(TestTeleModemEnable),
        cmocka_unit_test(TestTeleGetModemEnableStatus),
        cmocka_unit_test(TestTeleModemSetRadioPowerOff),
        cmocka_unit_test(TestTeleModemSetRadioPowerOnOffRepeatedly),
        cmocka_unit_test(TestTeleModemSetRadioPowerOn),
        cmocka_unit_test(TestTeleModemDisable),

        //         cmocka_unit_test_setup_teardown(TestTeleModemSetRadioPowerOnOffRepeatedly,
        //             setup_normal_mode, free_mode),
        //         // Airplane mode
        //         cmocka_unit_test(TestTeleModemSetRadioPowerOff),
        //         // cmocka_unit_test_setup_teardown(TestTeleImsServiceStatus,
        //         //     setup_airplane_mode, free_mode),
        //         cmocka_unit_test(TestTeleModemSetRadioPowerOn),

        //         // Call dialing
        //         cmocka_unit_test(TestTeleModemDialCall),
        //         // TODO: enable disable modem
        //         cmocka_unit_test(TestTeleModemSetRadioPowerOn),
        //         // cmocka_unit_test_setup_teardown(TestTeleImsServiceStatus,
        //         //     setup_call_dialing, free_mode),
        //         cmocka_unit_test(TestTeleModemHangupCall),

        //         // Modem poweroff
        //
        //         // cmocka_unit_test_setup_teardown(TestTeleImsServiceStatus,
        //         //     setup_modem_poweroff, free_mode),
        //         cmocka_unit_test_setup_teardown(TestTeleModemSetRadioPowerOnOffRepeatedly,
        //             setup_modem_poweroff, free_mode),
        //     // FIXME: Cannot enable because of RADIO_NOT_AVAILABLE
        //     // cmocka_unit_test(TestTeleModemEnable),
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
