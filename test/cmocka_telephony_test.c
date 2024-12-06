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

static void TestTeleFunc_CI_SimHasIccCard(void** state)
{
    (void)state;
    int ret = tapi_sim_has_icc_card_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimHasIccCardNumerousTimes(void** state)
{
    (void)state;
    int ret = tapi_sim_multi_has_icc_card_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimGetOperatorName(void** state)
{
    (void)state;
    int ret = tapi_sim_get_sim_operator_test(0, "310260");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimGetOperator(void** state)
{
    (void)state;
    int ret = tapi_sim_get_sim_operator_name_test(0, "T-Mobile");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimGetOperatorNameNumerousTimes(void** state)
{
    (void)state;
    int ret = tapi_sim_get_sim_operator_name_numerous(0, "T-Mobile");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimGetOperatorNumerousTimes(void** state)
{
    (void)state;
    int ret = tapi_sim_multi_get_sim_operator(0, "310260");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimGetSubscriberId(void** state)
{
    (void)state;
    int ret = tapi_sim_get_sim_subscriber_id_test(0, "310260000000000");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimGetSubscriberIdNumerousTimes(void** state)
{
    (void)state;
    int ret = tapi_sim_multi_get_sim_subscriber_id_test(0, "310260000000000");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimGetIccId(void** state)
{
    (void)state;
    int ret = tapi_sim_get_sim_iccid_test(0, "89860318640220133897");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimGetIccIdNumerousTimes(void** state)
{
    (void)state;
    int ret = tapi_sim_multi_get_sim_iccid_test(0, "89860318640220133897");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimGetMSISDN(void** state)
{
    (void)state;
    int ret = tapi_sim_get_ef_msisdn_test(0, "+15551234567");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimGetMSISDNNumerousTimes(void** state)
{
    (void)state;
    int ret = tapi_sim_multi_get_ef_msisdn_test(0, "+15551234567");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimTransmitAPDUInBasicChannel(void** state)
{
    (void)state;
    int ret = tapi_transmit_apdu_basic_channel_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimGetState(void** state)
{
    (void)state;
    int ret = tapi_sim_get_state_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimOpenLogicalChannel(void** state)
{
    (void)state;
    int ret = tapi_open_logical_channel_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimCloseLogicalChannel(void** state)
{
    (void)state;
    int ret = tapi_close_logical_channel_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimLogicalChannelOpenCloseNumerous(void** state)
{
    (void)state;
    int ret = sim_open_close_logical_channel_numerous(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimTransmitAPDUInLogicalChannel(void** state)
{
    int ret = sim_transmit_apdu_by_logical_channel(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimSetUiccEnablement(void** state)
{
    (void)state;
    int ret = tapi_sim_set_uicc_enablement_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimGetUiccEnablement(void** state)
{
    (void)state;
    int ret = tapi_sim_get_uicc_enablement_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimTransmitAPDUBasicChannel(void** state)
{
    (void)state;
    int ret = tapi_transmit_apdu_basic_channel_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimEnterPin(void** state)
{
    (void)state;
    int ret = tapi_sim_enter_pin_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimChangePin(void** state)
{
    (void)state;
    int ret = sim_change_pin_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimLockPin(void** state)
{
    (void)state;
    int ret = tapi_sim_lock_pin_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimUnlockPin(void** state)
{
    (void)state;
    int ret = tapi_sim_unlock_pin_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimLoadAdnEntries(void** state)
{
    (void)state;
    int ret = tapi_phonebook_load_adn_entries_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimLoadFdnEntries(void** state)
{
    (void)state;
    int ret = tapi_phonebook_load_fdn_entries_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimInsertFdnEntry(void** state)
{
    (void)state;
    int ret = tapi_phonebook_insert_fdn_entry_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimUpdateFdnEntry(void** state)
{
    (void)state;
    int ret = tapi_phonebook_update_fdn_entry_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimDeleteFdnEntry(void** state)
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

static void TestTeleFunc_CallSetVoicecallSlot(void** state)
{
    (void)state;
    int ret = tapi_call_set_default_voicecall_slot_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallGetVoicecallSlot(void** state)
{
    (void)state;
    int ret = tapi_call_get_default_voicecall_slot_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallClearVoicecallSlot(void** state)
{
    (void)state;
    int ret = call_clear_voicecall_slot_set();
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_CallListen(void** state)
{
    (void)state;
    int ret = tapi_call_listen_call_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_CallUnlisten(void** state)
{
    (void)state;
    int ret = tapi_call_unlisten_call_test();
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_CallStartDtmf(void** state)
{
    (void)state;
    int ret = tapi_start_dtmf_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_CallStopDtmf(void** state)
{
    (void)state;
    int ret = tapi_stop_dtmf_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_CallHangupAfterDialing(void** state)
{
    (void)state;
    sleep(2);
    int ret = call_hangup_after_dialing(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallIncomingAnswerAndHangup(void** state)
{
    (void)state;
    int ret = incoming_call_answer_and_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallIncomingAnswerAndRemoteHangup(void** state)
{
    (void)state;
    int ret = incoming_call_answer_and_remote_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallReleaseAndAnswer(void** state)
{
    (void)state;
    int ret = call_release_and_answer(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallHoldAndAnswer(void** state)
{
    (void)state;
    int ret = call_hold_first_call_and_answer_second_call(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallMergeByUser(void** state)
{
    (void)state;
    int ret = call_merge_by_user(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallSeparateByUser(void** state)
{
    (void)state;
    int ret = call_separate_by_user(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallReleaseAndSwap(void** state)
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

static void TestTeleFunc_CallRemoteAnswerAndHangup(void** state)
{
    int ret = outgoing_call_remote_answer_and_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallHoldAndHangup(void** state)
{
    (void)state;
    int ret = outgoing_call_hold_and_unhold_by_caller(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallActiveAndSendtones(void** state)
{
    (void)state;
    int ret = outgoing_call_active_and_send_tones(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallConnectAndLocalHangup(void** state)
{
    int ret = call_connect_and_local_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallDialAndRemoteHangup(void** state)
{
    int ret = dial_and_remote_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallIncomingandLocalHangup(void** state)
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

static void TestTeleFunc_CI_CallHangupAll(void** state)
{
    (void)state;
    int ret = tapi_call_hangup_all_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_CallGetCount(void** state)
{
    (void)state;
    int ret = tapi_get_call_count(0);
    assert_true(ret >= 0);
}

static void TestTeleFunc_CI_CallDialNumber(void** state)
{
    (void)state;
    int ret = tapi_dial_number(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_CallDialEccNumber(void** state)
{
    (void)state;
    int ret = tapi_dial_ecc_number(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallDialLongPhoneNumber(void** state)
{
    (void)state;
    int ret = tapi_dial_with_long_phone_number(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallDialShotPhoneNumber(void** state)
{
    (void)state;
    int ret = tapi_dial_with_short_phone_number(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallDialWithEnableHideCallId(void** state)
{
    (void)state;
    int ret = tapi_dial_with_enable_hide_callerid(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallDialWithDisabledHideCallId(void** state)
{
    (void)state;
    int ret = tapi_dial_with_disabled_hide_callerid(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallDialWithDefaultHideCallId(void** state)
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

static void TestTeleFunc_CallDialWithAreaCode(void** state)
{
    (void)state;
    int ret = tapi_call_dial_using_phone_number_with_area_code_test(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallDialWithPauseCode(void** state)
{
    (void)state;
    int ret = tapi_call_dial_using_phone_number_with_pause_code_test(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallDialWithWaitCode(void** state)
{
    (void)state;
    int ret = tapi_call_dial_using_phone_number_with_wait_code_test(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_CallDialWithNumerousCode(void** state)
{
    (void)state;
    int ret = tapi_call_dial_using_phone_number_with_numerous_code_test(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallDialConference(void** state)
{
    (void)state;
    int ret = tapi_call_dial_conference_test(0);
    assert_int_equal(ret, OK);
    sleep(2);
    ret = tapi_call_hangup_all_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_CallCheckAleringStatus(void** state)
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
static void TestTeleFunc_CI_DataLoadApnContexts(void** state)
{
    (void)state;
    int ret = tapi_data_load_apn_contexts_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataSaveApnContextSupl(void** state)
{
    (void)state;
    int ret = tapi_data_save_apn_context_test("0", "3", "supl", "supl", "2", "2");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataSaveApnContextEmergency(void** state)
{
    (void)state;
    int ret = tapi_data_save_apn_context_test("0", "7", "emergency",
        "emergency", "2", "2");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataSaveLongApnContex(void** state)
{
    (void)state;
    int ret = tapi_data_save_apn_context_test("0", "1",
        "longname-----------------------------------------"
        "-----------------------------------------longname",
        "cmnet4", "2", "2");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataSaveApnContext(void** state)
{
    (void)state;
    int ret = tapi_data_save_apn_context_test("0", "1", "cmcc1", "cmnet1", "2", "2");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_DataRemoveApnContext(void** state)
{
    (void)state;
    int ret = tapi_data_remove_apn_context_test("0", "/ril_0/context3");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataResetApnContexts(void** state)
{
    (void)state;
    int ret = tapi_data_reset_apn_contexts_test("0");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataResetApnContextsNTimes(void** state)
{
    (void)state;
    REPEAT_TEST_MORE_FOR
    {
        int ret = tapi_data_reset_apn_contexts_test("0");
        assert_int_equal(ret, OK);
    }
}

static void TestTeleFunc_DataEditApnName(void** state)
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

static void TestTeleFunc_DataEditApnType(void** state)
{
    (void)state;
    int ret = tapi_data_edit_apn_context_test("0", "/ril_0/context3", "3", "cmname", "cmname", "2", "2");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataEditApnProto(void** state)
{
    (void)state;
    int ret = tapi_data_edit_apn_context_test("0", "/ril_0/context3", "3", "cmname", "cmname", "0", "2");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataEditApnAuth(void** state)
{
    (void)state;
    int ret = tapi_data_edit_apn_context_test("0", "/ril_0/context3", "3", "cmname", "cmname", "0", "0");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataEditApnAll(void** state)
{
    (void)state;
    int ret;
    ret = tapi_data_edit_apn_context_test("0", "/ril_0/context3", "2", "cmnameall", "cmnameall", "1", "1");
    assert_int_equal(ret, OK);
    ret = tapi_data_load_apn_contexts_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataEditApnAndRemove(void** state)
{
    (void)state;
    int ret;
    ret = tapi_data_edit_apn_context_test("0", "/ril_0/context3", "2", "cmnameall", "cmnameall", "2", "2");
    assert_int_equal(ret, OK);
    ret = tapi_data_remove_apn_context_test("0", "/ril_0/context3");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataEditApnAndReset(void** state)
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

static void TestTeleFunc_DataEditApnRepeatedlyAndLoad(void** state)
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

static void TestTeleFunc_CI_DataEnable(void** state)
{
    (void)state;
    int ret = data_enabled_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_DataDisable(void** state)
{
    (void)state;
    int ret = data_disabled_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataEnableNTimes(void** state)
{
    (void)state;
    REPEAT_TEST_MORE_FOR
    {
        TestTeleFunc_CI_DataEnable(state);
        TestTeleFunc_CI_DataDisable(state);
    }
}

static void TestTeleFunc_CI_DataIsEnable(void** state)
{
    (void)state;
    bool enable = false;
    int ret = tapi_data_get_enabled_test(&enable);
    assert_int_equal(ret, OK);
    assert_int_equal(enable, 1);
}

static void TestTeleFunc_CI_DataIsDisable(void** state)
{
    (void)state;
    bool enable = true;
    int ret = tapi_data_get_enabled_test(&enable);
    assert_int_equal(ret, OK);
    assert_int_equal(enable, 0);
}

static void TestTeleFunc_CI_DataRegister(void** state)
{
    (void)state;
    int ret = tapi_data_listen_data_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_DataUnregister(void** state)
{
    (void)state;
    int ret = tapi_data_unlisten_data_test();
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_DataRequestNetworkInternet(void** state)
{
    (void)state;
    int ret = data_request_network_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_DataReleaseNetworkInternet(void** state)
{
    (void)state;
    int ret = data_release_network_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataReleaseNetworkInternetNTimes(void** state)
{
    (void)state;
    REPEAT_TEST_MORE_FOR
    {
        TestTeleFunc_CI_DataReleaseNetworkInternet(state);
        TestTeleFunc_CI_DataRequestNetworkInternet(state);
    }
}

static void TestTeleFunc_DataRequestNetworkIms(void** state)
{
    (void)state;
    int ret = data_request_ims(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataReleaseNetworkIms(void** state)
{
    (void)state;
    int ret = data_release_ims(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataReleaseNetworkImsNTimes(void** state)
{
    (void)state;
    REPEAT_TEST_MORE_FOR
    {
        TestTeleFunc_DataRequestNetworkIms(state);
        TestTeleFunc_DataReleaseNetworkIms(state);
    }
}

static void TestTeleFunc_CI_DataSetPreferredApn(void** state)
{
    (void)state;
    int ret = tapi_data_set_preferred_apn_test(0, "/ril_0/context1");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_DataGetPreferredApn(void** state)
{
    (void)state;
    int ret = tapi_data_get_preferred_apn_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_DataSendScreenState(void** state)
{
    (void)state;
    int ret = tapi_data_send_screen_stat_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_DataIsPsAttached(void** state)
{
    (void)state;
    int ret = tapi_data_is_ps_attached_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_DataGetNetworkType(void** state)
{
    (void)state;
    int ret = tapi_data_get_network_type_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataSetDefaultDataSlot(void** state)
{
    (void)state;
    int ret = tapi_data_set_default_data_slot_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataGetDefaultDataSlot(void** state)
{
    (void)state;
    sleep(2);
    int ret = tapi_data_get_default_data_slot_test();
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_DataSetDataAllow(void** state)
{
    (void)state;
    int ret = tapi_data_set_data_allow_test(0);
    assert_true(ret == OK);
}

static void TestTeleFunc_CI_DataGetCallList(void** state)
{
    (void)state;
    int ret = data_get_call_list(0);
    assert_true(ret == OK);
}

static void TestTeleFunc_CI_DataEnableRoaming(void** state)
{
    (void)state;
    int ret = data_enable_roaming_test();
    assert_true(ret == OK);
}

static void TestTeleFunc_CI_DataDisableRoaming(void** state)
{
    (void)state;
    int ret = data_disable_roaming_test();
    assert_true(ret == OK);
}

static void TestTeleFunc_CI_SmsSetServiceCenterNum(void** state)
{
    (void)state;
    int ret = sms_set_service_center_number_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SmsGetServiceCenterNum(void** state)
{
    (void)state;
    sleep(5);
    int ret = sms_check_service_center_number_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendShortMessageInEnglish(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SmsSendShortMessageInChinese(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendShortDataMessageInEnglish(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendShortDataMessageInChinese(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongMessageInEnglish(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SmsSendLongMessageInChinese(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongDataMessageInEnglish(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongDataMessageInChinese(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendShortEnglishMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_message_in_dialing(0, phone_num, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SmsSendShortChineseMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_message_in_dialing(0, phone_num, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongEnglishMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_message_in_dialing(0, phone_num, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongChineseMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_message_in_dialing(0, phone_num, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendShortEnglishDataMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_dialing(0, phone_num, short_english_text, 0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendShortChineseDataMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_dialing(0, phone_num, short_chinese_text, 0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongEnglishDataMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_dialing(0, phone_num, long_english_text, 0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongChineseDataMessageInDialing(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_dialing(0, phone_num, long_chinese_text, 0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendEnglishMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendChineseMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongEnglishMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongChineseMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendEnglishDataMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendChineseDataMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongEnglishDataMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongChineseDataMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendEnglishMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendChineseMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongEnglishMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongChineseMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendEnglishDataMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendChineseDataMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongEnglishDataMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongChineseDataMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendEnglishMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendChineseMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongEnglishMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongChineseMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_message_test(get_tapi_ctx(), 0, phone_num, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendEnglishDataMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendChineseDataMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, short_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongEnglishDataMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongChineseDataMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_test(0, phone_num, 0, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSetDefaultSlot(void** state)
{
    (void)state;
    int ret = tapi_sms_set_default_slot(get_tapi_ctx(), 0);
    sleep(5);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsGetDefaultSlot(void** state)
{
    (void)state;
    int result = -1;
    int ret = tapi_sms_get_default_slot(context, &result);
    syslog(LOG_INFO, "%s, ret: %d, result: %d", __func__, ret, result);
    assert_int_equal(ret, 0);
    assert_int_equal(result, 0);
}

static void TestTeleFunc_SmsSetCellBroadcastPower(void** state)
{
    (void)state;
    int ret = tapi_sms_set_cell_broadcast_power_on(get_tapi_ctx(), 0, 1);
    sleep(5);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsGetCellBroadcastPower(void** state)
{
    (void)state;
    bool result = false;
    int ret = tapi_sms_get_cell_broadcast_power_on(get_tapi_ctx(), 0, &result);
    syslog(LOG_INFO, "%s, ret: %d, result: %d", __func__, 0, (int)result);
    assert_int_equal(ret, 0);
    assert_int_equal(result, 1);
}

static void TestTeleFunc_SmsSetCellBroadcastTopics(void** state)
{
    (void)state;
    int ret = tapi_sms_set_cell_broadcast_topics(get_tapi_ctx(), 0, "1");
    sleep(5);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsGetCellBroadcastTopics(void** state)
{
    (void)state;
    char* result = NULL;
    int ret = tapi_sms_get_cell_broadcast_topics(get_tapi_ctx(), 0, &result);
    assert_int_equal(ret, 0);
    assert_int_equal(strcmp(result, "1"), 0);
}

static void TestTeleFunc_NetSelectAuto(void** state)
{
    (void)state;
    int ret = tapi_net_select_auto_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_NetSelectManual(void** state)
{
    (void)state;
    sleep(4);
    int ret = tapi_net_select_manual_test(0, "310", "260", "lte");
    sleep(4);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_NetScan(void** state)
{
    (void)state;
    int ret = tapi_net_scan_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_NetGetServingCellinfos(void** state)
{
    (void)state;
    int ret = tapi_net_get_serving_cellinfos_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_NetGetNeighbouringCellInfos(void** state)
{
    (void)state;
    int ret = tapi_net_get_neighbouring_cellInfos_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_NetRegistrationInfo(void** state)
{
    (void)state;
    int ret = tapi_net_registration_info_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_NetGetOperatorName(void** state)
{
    (void)state;
    int ret = tapi_net_get_operator_name_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_NetQuerySignalstrength(void** state)
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

static void TestTeleFunc_CI_NetGetVoiceRegistered(void** state)
{
    (void)state;
    int ret = tapi_net_get_voice_registered_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_NetGetVoiceNwType(void** state)
{
    (void)state;
    tapi_network_type type = NETWORK_TYPE_UNKNOWN;
    int ret = tapi_network_get_voice_network_type(get_tapi_ctx(), 0, &type);
    syslog(LOG_INFO, "%s, ret: %d, type: %d", __func__, ret, (int)type);
    assert_int_equal(ret, OK);
    assert_int_equal((int)type, 13);
}

static void TestTeleFunc_CI_NetGetVoiceRoaming(void** state)
{
    (void)state;
    bool value = true;
    int ret = tapi_network_is_voice_roaming(get_tapi_ctx(), 0, &value);
    syslog(LOG_INFO, "%s, ret: %d, value: %d", __func__, ret, (int)value);
    assert_int_equal(ret, OK);
    assert_int_equal((int)value, 0);
}

// modem
static void TestTeleFunc_CI_ModemGetImei(void** state)
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

static void TestTeleFunc_CI_ModemGetPrefNetMode(void** state)
{
    sleep(5);
    tapi_pref_net_mode get_value = NETWORK_PREF_NET_TYPE_ANY;
    int ret = tapi_get_pref_net_mode_test(0, &get_value);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_ModemSetRadioPowerOn(void** state)
{
    (void)state;
    int ret = tapi_set_radio_power_test(0, 1);
    assert_int_equal(ret, OK);
    sleep(10);
}

static void TestTeleFunc_CI_ModemSetRadioPowerOff(void** state)
{
    (void)state;
    int ret = tapi_set_radio_power_test(0, 0);
    assert_int_equal(ret, OK);
    sleep(10);
}

static void TestTeleFunc_CI_ModemSetRadioPowerOnOffNTimes(void** state)
{
    (void)state;
    REPEAT_TEST_LESS_FOR
    {
        TestTeleFunc_CI_ModemSetRadioPowerOn(state);
        TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    }
}

static void TestTeleFunc_CI_ModemEnableStatus(void** state)
{
    (void)state;
    int get_state = 1;
    int ret = tapi_get_modem_status_test(0, &get_state);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_ModemDsiableStatus(void** state)
{
    (void)state;
    int get_state = 0;
    int ret = tapi_get_modem_status_test(0, &get_state);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_ModemEnable(void** state)
{
    (void)state;
    int ret = tapi_enable_modem_test(0, 1);
    assert_int_equal(ret, OK);
    sleep(10);
}

static void TestTeleFunc_CI_ModemDisable(void** state)
{
    (void)state;
    int ret = tapi_enable_modem_test(0, 0);
    assert_int_equal(ret, OK);
    sleep(10);
}

static void TestTeleFunc_CI_ModemEnableDisableNTimes(void** state)
{
    (void)state;
    REPEAT_TEST_LESS_FOR
    {
        TestTeleFunc_CI_ModemEnable(state);
        TestTeleFunc_CI_ModemEnableStatus(state);
        TestTeleFunc_CI_ModemDisable(state);
        TestTeleFunc_CI_ModemDsiableStatus(state);
    }
}

static void TestTeleFunc_CI_ModemRegister(void** state)
{
    (void)state;
    int ret = tapi_modem_register_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_ModemUnregister(void** state)
{
    (void)state;
    int ret = tapi_modem_unregister_test();
    assert_true(ret == OK);
}

static void TestTeleFunc_CI_ModemGetRevision(void** state)
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

static void TestTeleFunc_ModemInvokeOemShotRilRequestRaw(void** state)
{
    int ret = tapi_invoke_oem_ril_request_raw_test(0, "01A0B023", 4);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemInvokeOemLongRilRequestRaw(void** state)
{
    int ret = tapi_invoke_oem_ril_request_raw_test(0, "01A0B02301A0B02301A0B02301A0B02301A0B02301", 21);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemInvokeOemSeperateRilRequestRaw(void** state)
{
    int ret = tapi_invoke_oem_ril_request_raw_test(0, "10|22", 2);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemInvokeOemNormalRilRequestRaw(void** state)
{
    int ret = tapi_invoke_oem_ril_request_raw_test(0, "01A0B023", 2);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemInvokeOemRilRequestATCmdStrings(void** state)
{
    char* req_data = "AT+CPIN?";
    int ret = tapi_invoke_oem_ril_request_strings_test(0, req_data, 1);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemInvokeOemRilRequestNotATCmdStrings(void** state)
{
    // not AT cmd
    char* req_data = "10|22";
    int ret = tapi_invoke_oem_ril_request_strings_test(0, req_data, 2);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemInvokeOemRilRequestHexStrings(void** state)
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

static void TestTeleFunc_CI_ImsListen(void** state)
{
    (void)state;
    int ret = tapi_ims_listen_ims_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_ImsTurnOn(void** state)
{
    (void)state;
    int ret = tapi_ims_turn_on_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_ImsGetRegistration(void** state)
{
    (void)state;
    int ret = tapi_ims_get_registration_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_ImsGetEnabled(void** state)
{
    (void)state;
    int ret = tapi_ims_get_enabled_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_ImsSetServiceStatus(void** state)
{
    (void)state;
    int ret = tapi_ims_set_service_status_test(0, 5);
    assert_int_equal(ret, 0);
    sleep(5);
}

static void TestTeleFunc_CI_ImsSetSmsCap(void** state)
{
    (void)state;
    int ret = tapi_ims_set_service_status_test(0, 4);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_ImsSetSmsVoiceCap(void** state)
{
    (void)state;
    int ret = tapi_ims_set_service_status_test(0, 5);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_ImsResetImsCap(void** state)
{
    (void)state;
    int ret = tapi_ims_set_service_status_test(0, 1);
    assert_int_equal(ret, 0);
    sleep(10);
}

static void TestTeleFunc_CI_ImsTurnOff(void** state)
{
    (void)state;
    int ret = tapi_ims_turn_off_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_ImsTurnOnOff(void** state)
{
    (void)state;
    REPEAT_TEST_LESS_FOR
    {
        TestTeleFunc_CI_ImsTurnOn(state);
        TestTeleFunc_CI_ImsGetEnabled(state);
        TestTeleFunc_CI_ImsGetRegistration(state);
        TestTeleFunc_CI_ImsTurnOff(state);
    }
}

static void TestTeleFunc_CI_SSRegister(void** state)
{
    (void)state;
    int ret = tapi_listen_ss_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSUnRegister(void** state)
{
    (void)state;
    int ret = tapi_unlisten_ss_test();
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SSRequestCallBarring(void** state)
{
    (void)state;
    int ret = tapi_ss_request_call_barring_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SSSetCallBarring(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_barring_option_test(0, "AI", "1234");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SSGetCallBarring(void** state)
{
    (void)state;
    sleep(5);
    int ret = tapi_ss_get_call_barring_option_test(0, "VoiceIncoming", "always");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SSChangeCallBarringPassword(void** state)
{
    (void)state;
    int ret = tapi_ss_change_call_barring_password_test(0, "1234", "2345");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SSResetCallBarringPassword(void** state)
{
    (void)state;
    int ret = tapi_ss_change_call_barring_password_test(0, "2345", "1234");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SSDisableAllIncoming(void** state)
{
    (void)state;
    int ret = tapi_ss_disable_all_incoming_test(0, "1234");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SSDisableAllOutgoing(void** state)
{
    (void)state;
    int ret = tapi_ss_disable_all_outgoing_test(0, "1234");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SSDisableAllCallBarrings(void** state)
{
    (void)state;
    int ret = tapi_ss_disable_all_call_barrings_test(0, "1234");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSSetCallForwardingUnConditional(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 0, "10086");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSGetCallForwardingUnConditional(void** state)
{
    (void)state;
    int ret = tapi_ss_get_call_forwarding_option_test(0, 0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSClearCallForwardingUnconditional(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 0, "\0");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSSetCallForwardingBusy(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 1, "10086");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSGetCallForwardingBusy(void** state)
{
    (void)state;
    int ret = tapi_ss_get_call_forwarding_option_test(0, 1);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSClearCallForwardingBusy(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 1, "\0");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSSetCallForwardingNoReply(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 2, "10086");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSGetCallForwardingNoReply(void** state)
{
    (void)state;
    int ret = tapi_ss_get_call_forwarding_option_test(0, 2);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSClearCallForwardingNoReply(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 2, "\0");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSSetCallForwardingNotReachable(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 3, "10086");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSGetCallForwardingNotReachable(void** state)
{
    (void)state;
    int ret = tapi_ss_get_call_forwarding_option_test(0, 3);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSClearCallForwardingNotReachable(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_forwarding_option_test(0, 3, "\0");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSEnableCallWaiting(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_waiting_test(0, true);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSGetEnableCallWaiting(void** state)
{
    (void)state;
    int ret = tapi_ss_get_call_waiting_test(0, true);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSDisableCallWaiting(void** state)
{
    (void)state;
    int ret = tapi_ss_set_call_waiting_test(0, false);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSGetDisableCallWaiting(void** state)
{
    (void)state;
    int ret = tapi_ss_get_call_waiting_test(0, false);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SSEnableFdn(void** state)
{
    (void)state;
    int ret = tapi_ss_enable_fdn_test(0, true, "1234");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SSGetFdnEnabled(void** state)
{
    (void)state;
    int ret = tapi_ss_query_fdn_test(0, true);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SSDisableFdn(void** state)
{
    (void)state;
    int ret = tapi_ss_enable_fdn_test(0, false, "1234");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SSGetFdnDisabled(void** state)
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
        cmocka_unit_test(TestTeleFunc_CI_SimHasIccCard),
        cmocka_unit_test(TestTeleFunc_SimHasIccCardNumerousTimes),
        cmocka_unit_test(TestTeleFunc_CI_SimGetOperatorName),
        cmocka_unit_test(TestTeleFunc_SimGetOperatorNameNumerousTimes),
        cmocka_unit_test(TestTeleFunc_CI_SimGetOperator),
        cmocka_unit_test(TestTeleFunc_SimGetOperatorNumerousTimes),
        cmocka_unit_test(TestTeleFunc_CI_SimGetSubscriberId),
        cmocka_unit_test(TestTeleFunc_SimGetSubscriberIdNumerousTimes),
        cmocka_unit_test(TestTeleFunc_CI_SimGetIccId),
        cmocka_unit_test(TestTeleFunc_SimGetIccIdNumerousTimes),
        cmocka_unit_test(TestTeleFunc_CI_SimGetMSISDN),
        cmocka_unit_test(TestTeleFunc_SimGetMSISDNNumerousTimes),
        cmocka_unit_test(TestTeleFunc_CI_SimTransmitAPDUInBasicChannel),
        cmocka_unit_test(TestTeleFunc_CI_SimOpenLogicalChannel),
        cmocka_unit_test(TestTeleFunc_CI_SimCloseLogicalChannel),
        cmocka_unit_test(TestTeleFunc_SimLogicalChannelOpenCloseNumerous),
        cmocka_unit_test(TestTeleFunc_CI_SimTransmitAPDUInLogicalChannel),
        cmocka_unit_test(TestTeleFunc_SimSetUiccEnablement),
        cmocka_unit_test(TestTeleFunc_SimGetUiccEnablement),
        cmocka_unit_test(TestTeleFunc_CI_SimTransmitAPDUBasicChannel),
        cmocka_unit_test(TestTeleFunc_CI_SimGetState),
        cmocka_unit_test(TestTeleFunc_SimEnterPin),
        cmocka_unit_test(TestTeleFunc_SimChangePin),
        cmocka_unit_test(TestTeleFunc_SimLockPin),
        cmocka_unit_test(TestTeleFunc_SimUnlockPin),
        cmocka_unit_test(TestTeleFunc_SimLoadAdnEntries),
        cmocka_unit_test(TestTeleFunc_SimLoadFdnEntries),
        cmocka_unit_test(TestTeleFunc_SimInsertFdnEntry),
        cmocka_unit_test(TestTeleFunc_SimUpdateFdnEntry),
        cmocka_unit_test(TestTeleFunc_SimDeleteFdnEntry),
    };

    const struct CMUnitTest CallTestSuites[] = {
        cmocka_unit_test(TestTeleFunc_CI_CallListen),
        cmocka_unit_test(TestTeleFunc_CI_CallDialNumber),
        cmocka_unit_test(TestTeleFunc_CI_CallDialEccNumber),
        cmocka_unit_test(TestTeleFunc_CallDialLongPhoneNumber),
        cmocka_unit_test(TestTeleFunc_CallDialShotPhoneNumber),
        cmocka_unit_test(TestTeleFunc_CallDialWithEnableHideCallId),
        cmocka_unit_test(TestTeleFunc_CallDialWithDisabledHideCallId),
        cmocka_unit_test(TestTeleFunc_CallDialWithDefaultHideCallId),
        cmocka_unit_test(TestTeleFunc_CallDialWithAreaCode),
        cmocka_unit_test(TestTeleFunc_CallDialWithPauseCode),
        cmocka_unit_test(TestTeleFunc_CallDialWithWaitCode),
        cmocka_unit_test(TestTeleFunc_CI_CallDialWithNumerousCode),
        cmocka_unit_test(TestTeleFunc_CallDialConference),
        cmocka_unit_test(TestTeleFunc_CI_CallCheckAleringStatus),
        cmocka_unit_test(TestTeleFunc_CI_CallStartDtmf),
        cmocka_unit_test(TestTeleFunc_CI_CallStopDtmf),
        cmocka_unit_test(TestTeleFunc_CI_CallGetCount),
        cmocka_unit_test(TestTeleFunc_CI_CallHangupAll),
        // cmocka_unit_test(TestTeleLoadEccList),
        // hangup between dialing and answering
        cmocka_unit_test(TestTeleFunc_CI_CallHangupAfterDialing),
        cmocka_unit_test(TestTeleFunc_CallIncomingAnswerAndHangup),
        cmocka_unit_test(TestTeleFunc_CallIncomingAnswerAndRemoteHangup),
        cmocka_unit_test(TestTeleFunc_CallReleaseAndAnswer),
        cmocka_unit_test(TestTeleFunc_CallHoldAndAnswer),
        cmocka_unit_test(TestTeleFunc_CallMergeByUser),
        cmocka_unit_test(TestTeleFunc_CallSeparateByUser),
        cmocka_unit_test(TestTeleFunc_CallReleaseAndSwap),
        cmocka_unit_test(TestTeleFunc_CallRemoteAnswerAndHangup),
        cmocka_unit_test(TestTeleFunc_CallHoldAndHangup),
        cmocka_unit_test(TestTeleFunc_CallActiveAndSendtones),
        cmocka_unit_test(TestTeleFunc_CallConnectAndLocalHangup),
        cmocka_unit_test(TestTeleFunc_CallDialAndRemoteHangup),
        cmocka_unit_test(TestTeleFunc_CallIncomingandLocalHangup),
        cmocka_unit_test(TestTeleFunc_CallSetVoicecallSlot),
        cmocka_unit_test(TestTeleFunc_CallGetVoicecallSlot),
        cmocka_unit_test(TestTeleFunc_CallClearVoicecallSlot),
        cmocka_unit_test(TestTeleFunc_CI_CallUnlisten),
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
        cmocka_unit_test(TestTeleFunc_CallDialWithAreaCode),
#endif
    };

    const struct CMUnitTest DataTestSuites[] = {
        cmocka_unit_test(TestTeleFunc_CI_DataRegister),
        cmocka_unit_test(TestTeleFunc_CI_DataLoadApnContexts),
        cmocka_unit_test(TestTeleFunc_DataSaveApnContext),
        cmocka_unit_test(TestTeleFunc_DataRemoveApnContext),
        cmocka_unit_test(TestTeleFunc_DataResetApnContexts),
        cmocka_unit_test(TestTeleFunc_DataResetApnContextsNTimes),
        cmocka_unit_test(TestTeleFunc_DataEditApnName),
        cmocka_unit_test(TestTeleFunc_DataEditApnType),
        cmocka_unit_test(TestTeleFunc_DataEditApnProto),
        cmocka_unit_test(TestTeleFunc_DataEditApnAuth),
        cmocka_unit_test(TestTeleFunc_DataEditApnAll),
        cmocka_unit_test(TestTeleFunc_DataEditApnAndRemove),
        cmocka_unit_test(TestTeleFunc_DataEditApnAndReset),
        cmocka_unit_test(TestTeleFunc_DataEditApnRepeatedlyAndLoad),
        cmocka_unit_test(TestTeleFunc_DataResetApnContexts),
        cmocka_unit_test(TestTeleFunc_DataEnableNTimes),
        cmocka_unit_test(TestTeleFunc_CI_DataEnable),
        cmocka_unit_test(TestTeleFunc_CI_DataIsEnable),
        cmocka_unit_test(TestTeleFunc_CI_DataReleaseNetworkInternet),
        cmocka_unit_test(TestTeleFunc_CI_DataRequestNetworkInternet),
        cmocka_unit_test(TestTeleFunc_DataReleaseNetworkInternetNTimes),
        cmocka_unit_test(TestTeleFunc_CI_DataDisable),
        cmocka_unit_test(TestTeleFunc_CI_DataIsDisable),
        cmocka_unit_test(TestTeleFunc_DataRequestNetworkIms),
        cmocka_unit_test(TestTeleFunc_DataReleaseNetworkIms),
        cmocka_unit_test(TestTeleFunc_DataReleaseNetworkImsNTimes),
        cmocka_unit_test(TestTeleFunc_DataSaveApnContextSupl),
        cmocka_unit_test(TestTeleFunc_DataSaveApnContextEmergency),
        cmocka_unit_test(TestTeleFunc_DataResetApnContexts),
        cmocka_unit_test(TestTeleFunc_CI_DataSetPreferredApn),
        cmocka_unit_test(TestTeleFunc_CI_DataGetPreferredApn),
        cmocka_unit_test(TestTeleFunc_CI_DataSendScreenState),
        cmocka_unit_test(TestTeleFunc_CI_DataGetNetworkType),
        cmocka_unit_test(TestTeleFunc_CI_DataIsPsAttached),
        cmocka_unit_test(TestTeleFunc_DataSetDefaultDataSlot),
        cmocka_unit_test(TestTeleFunc_DataGetDefaultDataSlot),
        cmocka_unit_test(TestTeleFunc_CI_DataSetDataAllow),
        cmocka_unit_test(TestTeleFunc_CI_DataGetCallList),
        cmocka_unit_test(TestTeleFunc_DataSaveLongApnContex),
        cmocka_unit_test(TestTeleFunc_DataResetApnContexts),
        cmocka_unit_test(TestTeleFunc_CI_DataEnableRoaming),
        cmocka_unit_test(TestTeleFunc_CI_DataDisableRoaming),
        cmocka_unit_test(TestTeleFunc_CI_DataUnregister),
    };

    const struct CMUnitTest SmsTestSuites[] = {
        cmocka_unit_test(TestTeleFunc_CI_CallListen),
        cmocka_unit_test(TestTeleFunc_CI_ImsListen),
        cmocka_unit_test(TestTeleFunc_CI_ImsTurnOn),
        cmocka_unit_test(TestTeleFunc_CI_SmsSetServiceCenterNum),
        cmocka_unit_test(TestTeleFunc_CI_SmsGetServiceCenterNum),
        cmocka_unit_test(TestTeleFunc_SmsSendShortMessageInEnglish),
        cmocka_unit_test(TestTeleFunc_CI_SmsSendShortMessageInChinese),
        cmocka_unit_test(TestTeleFunc_SmsSendShortDataMessageInEnglish),
        cmocka_unit_test(TestTeleFunc_SmsSendShortDataMessageInChinese),
        cmocka_unit_test(TestTeleFunc_SmsSendLongMessageInEnglish),
        cmocka_unit_test(TestTeleFunc_CI_SmsSendLongMessageInChinese),
        cmocka_unit_test(TestTeleFunc_SmsSendLongDataMessageInEnglish),
        cmocka_unit_test(TestTeleFunc_SmsSendLongDataMessageInChinese),
        cmocka_unit_test(TestTeleFunc_SmsSendShortEnglishMessageInDialing),
        cmocka_unit_test(TestTeleFunc_CI_SmsSendShortChineseMessageInDialing),
        cmocka_unit_test(TestTeleFunc_SmsSendLongEnglishMessageInDialing),
        cmocka_unit_test(TestTeleFunc_SmsSendLongChineseMessageInDialing),
        cmocka_unit_test(TestTeleFunc_SmsSendShortEnglishDataMessageInDialing),
        cmocka_unit_test(TestTeleFunc_SmsSendShortChineseDataMessageInDialing),
        cmocka_unit_test(TestTeleFunc_SmsSendLongEnglishDataMessageInDialing),
        cmocka_unit_test(TestTeleFunc_SmsSendLongChineseDataMessageInDialing),
        cmocka_unit_test(TestTeleFunc_SmsSendEnglishMessageInVoiceImsCap),
        cmocka_unit_test(TestTeleFunc_SmsSendChineseMessageInVoiceImsCap),
        cmocka_unit_test(TestTeleFunc_SmsSendLongEnglishMessageInVoiceImsCap),
        cmocka_unit_test(TestTeleFunc_SmsSendLongChineseMessageInVoiceImsCap),
        cmocka_unit_test(TestTeleFunc_SmsSendEnglishDataMessageInVoiceImsCap),
        cmocka_unit_test(TestTeleFunc_SmsSendChineseDataMessageInVoiceImsCap),
        cmocka_unit_test(TestTeleFunc_SmsSendLongEnglishDataMessageInVoiceImsCap),
        cmocka_unit_test(TestTeleFunc_SmsSendLongChineseDataMessageInVoiceImsCap),
        cmocka_unit_test(TestTeleFunc_CI_ImsSetSmsCap),
        cmocka_unit_test(TestTeleFunc_SmsSendEnglishMessageInSmsImsCap),
        cmocka_unit_test(TestTeleFunc_SmsSendChineseMessageInSmsImsCap),
        cmocka_unit_test(TestTeleFunc_SmsSendLongEnglishMessageInSmsImsCap),
        cmocka_unit_test(TestTeleFunc_SmsSendLongChineseMessageInSmsImsCap),
        cmocka_unit_test(TestTeleFunc_SmsSendEnglishDataMessageInSmsImsCap),
        cmocka_unit_test(TestTeleFunc_SmsSendChineseDataMessageInSmsImsCap),
        cmocka_unit_test(TestTeleFunc_SmsSendLongEnglishDataMessageInSmsImsCap),
        cmocka_unit_test(TestTeleFunc_SmsSendLongChineseDataMessageInSmsImsCap),
        cmocka_unit_test(TestTeleFunc_CI_ImsSetSmsVoiceCap),
        cmocka_unit_test(TestTeleFunc_SmsSendEnglishMessageInSmsVoiceCap),
        cmocka_unit_test(TestTeleFunc_SmsSendChineseMessageInSmsVoiceCap),
        cmocka_unit_test(TestTeleFunc_SmsSendLongEnglishMessageInSmsVoiceCap),
        cmocka_unit_test(TestTeleFunc_SmsSendLongChineseMessageInSmsVoiceCap),
        cmocka_unit_test(TestTeleFunc_SmsSendEnglishDataMessageInSmsVoiceCap),
        cmocka_unit_test(TestTeleFunc_SmsSendChineseDataMessageInSmsVoiceCap),
        cmocka_unit_test(TestTeleFunc_SmsSendLongEnglishDataMessageInSmsVoiceCap),
        cmocka_unit_test(TestTeleFunc_SmsSendLongChineseDataMessageInSmsVoiceCap),
        cmocka_unit_test(TestTeleFunc_SmsSetDefaultSlot),
        cmocka_unit_test(TestTeleFunc_SmsGetDefaultSlot),
        cmocka_unit_test(TestTeleFunc_SmsSetCellBroadcastPower),
        cmocka_unit_test(TestTeleFunc_SmsGetCellBroadcastPower),
        cmocka_unit_test(TestTeleFunc_SmsSetCellBroadcastTopics),
        cmocka_unit_test(TestTeleFunc_SmsGetCellBroadcastTopics),
        cmocka_unit_test(TestTeleFunc_CI_ImsResetImsCap),
        cmocka_unit_test(TestTeleFunc_CI_ImsTurnOff),
        cmocka_unit_test(TestTeleFunc_CI_CallUnlisten),
    };

    const struct CMUnitTest NetTestSuites[] = {
        cmocka_unit_test(TestTeleFunc_NetSelectManual),
        cmocka_unit_test(TestTeleFunc_NetSelectAuto),
        cmocka_unit_test(TestTeleFunc_NetScan),
        cmocka_unit_test(TestTeleFunc_CI_NetGetServingCellinfos),
        cmocka_unit_test(TestTeleFunc_NetGetNeighbouringCellInfos),
        cmocka_unit_test(TestTeleFunc_CI_NetRegistrationInfo),
        cmocka_unit_test(TestTeleFunc_CI_NetGetOperatorName),
        cmocka_unit_test(TestTeleFunc_CI_NetQuerySignalstrength),
        //      cmocka_unit_test(TestTeleNetSetCellInfoListRate),
        cmocka_unit_test(TestTeleFunc_CI_NetGetVoiceRegistered),
        cmocka_unit_test(TestTeleFunc_CI_NetGetVoiceNwType),
        cmocka_unit_test(TestTeleFunc_CI_NetGetVoiceRoaming),
    };

    const struct CMUnitTest ImsTestSuits[] = {
        cmocka_unit_test(TestTeleFunc_CI_ImsTurnOn),
        cmocka_unit_test(TestTeleFunc_CI_ImsGetRegistration),
        cmocka_unit_test(TestTeleFunc_CI_ImsGetEnabled),
        cmocka_unit_test(TestTeleFunc_CI_ImsSetServiceStatus),
        cmocka_unit_test(TestTeleFunc_CI_ImsResetImsCap),
        cmocka_unit_test(TestTeleFunc_CI_ImsTurnOff),
        cmocka_unit_test(TestTeleFunc_CI_ImsTurnOnOff),
    };

    const struct CMUnitTest SSTestSuits[] = {
        cmocka_unit_test(TestTeleFunc_CI_SSRegister),
        cmocka_unit_test(TestTeleFunc_CI_SSUnRegister),
        cmocka_unit_test(TestTeleFunc_SSRequestCallBarring),
        cmocka_unit_test(TestTeleFunc_SSSetCallBarring),
        cmocka_unit_test(TestTeleFunc_SSGetCallBarring),
        cmocka_unit_test(TestTeleFunc_SSChangeCallBarringPassword),
        cmocka_unit_test(TestTeleFunc_SSResetCallBarringPassword),
        cmocka_unit_test(TestTeleFunc_SSDisableAllIncoming),
        cmocka_unit_test(TestTeleFunc_SSDisableAllOutgoing),
        cmocka_unit_test(TestTeleFunc_SSDisableAllCallBarrings),
        cmocka_unit_test(TestTeleFunc_CI_SSSetCallForwardingUnConditional),
        cmocka_unit_test(TestTeleFunc_CI_SSGetCallForwardingUnConditional),
        cmocka_unit_test(TestTeleFunc_CI_SSClearCallForwardingUnconditional),
        cmocka_unit_test(TestTeleFunc_CI_SSSetCallForwardingBusy),
        cmocka_unit_test(TestTeleFunc_CI_SSGetCallForwardingBusy),
        cmocka_unit_test(TestTeleFunc_CI_SSClearCallForwardingBusy),
        cmocka_unit_test(TestTeleFunc_CI_SSSetCallForwardingNoReply),
        cmocka_unit_test(TestTeleFunc_CI_SSGetCallForwardingNoReply),
        cmocka_unit_test(TestTeleFunc_CI_SSClearCallForwardingNoReply),
        cmocka_unit_test(TestTeleFunc_CI_SSSetCallForwardingNotReachable),
        cmocka_unit_test(TestTeleFunc_CI_SSGetCallForwardingNotReachable),
        cmocka_unit_test(TestTeleFunc_CI_SSClearCallForwardingNotReachable),
        cmocka_unit_test(TestTeleFunc_CI_SSEnableCallWaiting),
        cmocka_unit_test(TestTeleFunc_CI_SSGetEnableCallWaiting),
        cmocka_unit_test(TestTeleFunc_CI_SSDisableCallWaiting),
        cmocka_unit_test(TestTeleFunc_CI_SSGetDisableCallWaiting),
        cmocka_unit_test(TestTeleFunc_SSEnableFdn),
        cmocka_unit_test(TestTeleFunc_SSGetFdnEnabled),
        cmocka_unit_test(TestTeleFunc_SSDisableFdn),
        cmocka_unit_test(TestTeleFunc_SSGetFdnDisabled),
    };

    const struct CMUnitTest CommonTestSuites[] = {
        cmocka_unit_test(TestTeleFunc_CI_ModemGetImei),
        // cmocka_unit_test(TestTeleModemSetUmtsPrefNetMode),
        // cmocka_unit_test(TestTeleModemSetGsmOnlyPrefNetMode),
        // cmocka_unit_test(TestTeleModemSetWcdmaOnlyPrefNetMode),
        // cmocka_unit_test(TestTeleModemSetLteOnlyPrefNetMode),
        // cmocka_unit_test(TestTeleModemSetLteWcdmaPrefNetMode),
        // cmocka_unit_test(TestTeleModemSetLteGsmWcdmaPrefNetMode),
        cmocka_unit_test(TestTeleFunc_CI_ModemGetPrefNetMode),
        cmocka_unit_test(TestTeleFunc_CI_ModemRegister),
        cmocka_unit_test(TestTeleFunc_CI_ModemUnregister),
        cmocka_unit_test(TestTeleFunc_ModemInvokeOemShotRilRequestRaw),
        cmocka_unit_test(TestTeleFunc_ModemInvokeOemLongRilRequestRaw),
        cmocka_unit_test(TestTeleFunc_ModemInvokeOemNormalRilRequestRaw),
        cmocka_unit_test(TestTeleFunc_ModemInvokeOemSeperateRilRequestRaw),
        cmocka_unit_test(TestTeleFunc_ModemInvokeOemRilRequestATCmdStrings),
        cmocka_unit_test(TestTeleFunc_ModemInvokeOemRilRequestNotATCmdStrings),
        cmocka_unit_test(TestTeleFunc_ModemInvokeOemRilRequestHexStrings),
        cmocka_unit_test(TestTeleFunc_CI_ImsListen),
        cmocka_unit_test(TestTeleFunc_CI_ModemGetRevision),
        cmocka_unit_test(TestTeleFunc_CI_ModemDisable),
        cmocka_unit_test(TestTeleFunc_CI_ModemDsiableStatus),
        cmocka_unit_test(TestTeleFunc_CI_ModemEnableDisableNTimes),
        cmocka_unit_test(TestTeleFunc_CI_ModemEnable),
        cmocka_unit_test(TestTeleFunc_CI_ModemEnableStatus),
        cmocka_unit_test(TestTeleFunc_CI_ModemSetRadioPowerOff),
        cmocka_unit_test(TestTeleFunc_CI_ModemSetRadioPowerOnOffNTimes),
        cmocka_unit_test(TestTeleFunc_CI_ModemSetRadioPowerOn),
        cmocka_unit_test(TestTeleFunc_CI_ModemDisable),

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
