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

#include "telephony_test.h"

#include "remote_operation.h"
#include "telephony_call_test.h"
#include "telephony_common_test.h"
#include "telephony_data_test.h"
#include "telephony_ims_test.h"
#include "telephony_network_test.h"
#include "telephony_phone_service_test.h"
#include "telephony_sim_test.h"
#include "telephony_sms_test.h"
#include "telephony_ss_test.h"

#define REPEAT_TEST_MORE_FOR for (int _i = 0; _i < 10; _i++)
#define REPEAT_TEST_LESS_FOR for (int _i = 0; _i < 3; _i++)

#define TAPI_TEST_NAME_MAX_LEN 256
#define TAPI_TEST_DBUS_NAME "vela.telephony.test"

char* phone_num = NULL;
static bool g_uv_exit_flag = false;
static uv_async_t g_uv_exit;
static uv_async_t g_uv_cmd_tapi;
static int ready_done;
tapi_context g_context = NULL;
static int count = 0;

#ifdef CONFIG_PHONE_SERVICE
static async_message_t g_uv_message;
#endif
typedef enum {
    CASE_NORMAL_MODE = 0,
    CASE_AIRPLANE_MODE = 1,
    CASE_CALL_DIALING = 2,
    CASE_MODEM_POWEROFF = 3,
} case_type;

struct uv_tapi_cmd_data_s {
    unsigned int spec_service;
    char dbus_name[TAPI_TEST_NAME_MAX_LEN];
    bool is_open;
};

struct judge_type judge_data;

bool response_flag[MAX_MESSAGE_COUNT];
int response_ret[MAX_MESSAGE_COUNT];

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

#ifndef CONFIG_TELEPHONY_DFX
struct dfx_judge_data dfx_data;

void dfx_data_init(void)
{
    memset(&dfx_data, 0, sizeof(dfx_data));
}

bool check_dfx_value(void)
{
    bool res = true;

    for (int i = 0; i < dfx_data.expected_dfx_count; i++) {
        if (!dfx_data.received_dfx_flag[i]) {
            res = false;
            break;
        }
    }
    return res;
}
#endif

static void exit_async_cleanup(uv_async_t* handle)
{
#ifdef CONFIG_PHONE_SERVICE
    tapi_stop_phone_service_client();
#endif
    if (g_context) {
        g_uv_exit_flag = true;
        tapi_close(g_context);
        g_context = NULL;
    } else {
        syslog(LOG_ERR, "tapi is already close, stop default loop");
        uv_stop(uv_default_loop());
    }
}

tapi_context get_tapi_ctx(void)
{
    return g_context;
}

void init_response_flag(int wait_message_count)
{
    for (int i = 0; i < MAX_MESSAGE_COUNT; i++) {
        if (i < wait_message_count) {
            response_flag[i] = FALSE;
        } else {
            response_flag[i] = TRUE;
        }
    }
}

int wait_response(int wait_message_count)
{
    int timeout;
    if (wait_message_count == MID_MESSAGE_COUNT) {
        timeout = MID_TIMEOUT;
    } else {
        timeout = MAX_TIMEOUT;
    }

    while (timeout-- > 0) {
        bool response = TRUE;
        for (int i = 0; i < wait_message_count; i++) {
            if (!response_flag[i]) {
                response = FALSE;
                break;
            }
        }
        if (!response) {
            sleep(1);
            syslog(LOG_INFO, "There is %d second(s) remain.\n", timeout);
        } else {
            break;
        }
    }

    if (timeout > 0) {
        for (int i = 0; i < wait_message_count; i++) {
            if (response_ret[i] != 0) {
                syslog(LOG_INFO, "wait response:i=%d,ret=%d", i, response_ret[i]);
                return response_ret[i];
            }
        }
        return 0;
    } else {
        syslog(LOG_INFO, "wait response timeout");
        return -ETIME;
    }
}

int judge(void)
{
    int timeout = TIMEOUT;

    while (timeout-- > 0) {

        if (judge_data.flag == judge_data.expect) {
            if (judge_data.result != 0)
                syslog(LOG_ERR, "judge expect(%d): result error\n", judge_data.expect);
            else
                syslog(LOG_INFO, "judge expect(%d): result correct\n", judge_data.expect);

            return 0;
        }

        sleep(1);
        syslog(LOG_INFO, "There is %d second(s) remain.\n", timeout);
    }

    syslog(LOG_ERR, "judge expect(%d) timeout\n", judge_data.expect);
    return -ETIME;
}

void judge_data_init(void)
{
    judge_data.flag = INVALID_VALUE;
    judge_data.expect = INVALID_VALUE;
    judge_data.result = INVALID_VALUE;
    judge_data.phone_state_value = INVALID_VALUE;
    judge_data.sim_channel_error_code = INVALID_VALUE;
}

static void TestTeleFunc_CI_SimListen(void** state)
{
    (void)state;
    int ret = sim_listen_sim_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimUnListen(void** state)
{
    (void)state;
    int ret = sim_unlisten_sim_test();
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimListenAndUnListen(void** state)
{
    TestTeleFunc_CI_SimListen(state);
    TestTeleFunc_CI_SimUnListen(state);
}

static void TestTeleFunc_CI_SimCheckSimInvalid(void** state)
{
    (void)state;
    int ret = sim_set_and_check_sim_invalid(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimHasIccCard(void** state)
{
    (void)state;
    int ret = sim_has_icc_card_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleStab_SimHasIccCardNumerousTimes(void** state)
{
    (void)state;
    int ret = sim_multi_has_icc_card_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimGetOperatorName(void** state)
{
    (void)state;
    char operator[MAX_MCC_LENGTH + MAX_MNC_LENGTH + 1];
    memset(operator, 0, sizeof(operator));
    int ret = sim_get_sim_operator_test(0, operator);
    assert_int_equal(ret, OK);
    assert_string_equal(operator, "310260");
}

static void TestTeleFunc_CI_SimGetOperator(void** state)
{
    (void)state;
    int ret = sim_get_sim_operator_name_test(0, "T-Mobile");
    assert_int_equal(ret, OK);
}

static void TestTeleStab_SimGetOperatorNameNumerousTimes(void** state)
{
    (void)state;
    int ret = sim_get_sim_operator_name_numerous(0, "T-Mobile");
    assert_int_equal(ret, OK);
}

static void TestTeleStab_SimGetOperatorNumerousTimes(void** state)
{
    (void)state;
    REPEAT_TEST_MORE_FOR
    {
        TestTeleFunc_CI_SimGetOperatorName(state);
    }
}

static void TestTeleFunc_CI_SimGetSubscriberId(void** state)
{
    (void)state;
    int ret = sim_get_sim_subscriber_id_test(0, "310260000000000");
    assert_int_equal(ret, OK);
}

static void TestTeleStab_SimGetSubscriberIdNumerousTimes(void** state)
{
    (void)state;
    int ret = sim_multi_get_sim_subscriber_id_test(0, "310260000000000");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimGetIccId(void** state)
{
    (void)state;
    int ret = sim_get_sim_iccid_test(0, "89860318640220133897");
    assert_int_equal(ret, OK);
}

static void TestTeleStab_SimGetIccIdNumerousTimes(void** state)
{
    (void)state;
    int ret = sim_multi_get_sim_iccid_test(0, "89860318640220133897");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimGetMSISDN(void** state)
{
    (void)state;
    int ret = sim_get_ef_msisdn_test(0, "+15551234567");
    assert_int_equal(ret, OK);
}

static void TestTeleStab_SimGetMSISDNNumerousTimes(void** state)
{
    (void)state;
    int ret = sim_multi_get_ef_msisdn_test(0, "+15551234567");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimTransmitAPDUInBasicChannel(void** state)
{
    (void)state;
    int ret = sim_transmit_apdu_basic_channel_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimRmoteAbsentInsertOperator(void** state)
{
    (void)state;
    int ret = remote_sim_absent_insert_operation_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimGetState(void** state)
{
    (void)state;
    int ret = sim_get_state_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimOpenAndCloseLogicalChannel(void** state)
{
    (void)state;
    int ret = sim_open_logical_channel_test(0);
    assert_int_equal(ret, OK);
    sleep(3);
    ret = sim_close_logical_channel_test(0);
}

static void TestTeleAbn_CI_SimOpenLogicalChannelWithErrorCode(void** state)
{
    (void)state;
    int ret = sim_open_logical_channel_with_error_code_test(0, 3);
    assert_int_equal(ret, OK);
}

static void TestTeleStab_SimLogicalChannelOpenCloseNumerous(void** state)
{
    (void)state;
    int ret = sim_open_close_logical_channel_numerous(0);
    assert_int_equal(ret, OK);
}

static void TestTeleStab_SimLogicalChannelOpenCloseWithErrorCodeNumerous(void** state)
{
    (void)state;
    int ret = sim_open_close_logical_channel_with_error_code_numerous(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimTransmitAPDUInLogicalChannel(void** state)
{
    int ret = sim_transmit_apdu_by_logical_channel(0);
    assert_int_equal(ret, OK);
}

static void TestTeleAbn_CI_SimTransmitAPDUInLogicalChannelWithErrorCode(void** state)
{
    int ret = sim_transmit_apdu_by_logical_channel_with_error_code(0, 0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimSetUiccEnablement(void** state)
{
    (void)state;
    int ret = sim_set_uicc_enablement_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimGetUiccEnablement(void** state)
{
    (void)state;
    int ret = sim_get_uicc_enablement_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_SimTransmitAPDUBasicChannel(void** state)
{
    (void)state;
    int ret = sim_transmit_apdu_basic_channel_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleAbn_CI_SimTransmitAPDUBasicChannelWithErrorCode(void** state)
{
    (void)state;
    int ret = sim_transmit_apdu_basic_channel_with_error_code_test(0, 0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimEnterPin(void** state)
{
    (void)state;
    int ret = sim_enter_pin_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimChangePin(void** state)
{
    (void)state;
    int ret = sim_change_and_restore_pin_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimLockPin(void** state)
{
    (void)state;
    int ret = sim_lock_pin_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimUnlockPin(void** state)
{
    (void)state;
    int ret = sim_unlock_pin_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimLoadAdnEntries(void** state)
{
    (void)state;
    int ret = phonebook_load_adn_entries_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimLoadFdnEntries(void** state)
{
    (void)state;
    int ret = phonebook_load_fdn_entries_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimInsertFdnEntry(void** state)
{
    (void)state;
    int ret = phonebook_insert_fdn_entry_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimUpdateFdnEntry(void** state)
{
    (void)state;
    int ret = phonebook_update_fdn_entry_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SimDeleteFdnEntry(void** state)
{
    (void)state;
    int ret = phonebook_delete_fdn_entry_test(0);
    assert_int_equal(ret, OK);
}

// call testcases
static void TestTeleFunc_CallPerformLoadAndCompareEccWithChinaSimCard(void** state)
{
    (void)state;
    int ret = call_load_and_compare_ecclist_with_china_sim_card_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformLoadAndCompareEccWithoutSimCard(void** state)
{
    (void)state;
    int ret = call_load_and_compare_ecclist_without_sim_card_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_CallDialNumber(void** state)
{
    (void)state;
    int ret = call_dial_number_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallStatusError(void** state)
{
    int ret = incoming_call_with_unexpected_status(6);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_CallDialNumberWithoutSimCard(void** state)
{
    (void)state;
    int ret = call_dial_without_sim_card(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_CallDialEccNumber(void** state)
{
    (void)state;
    int ret = call_dial_ecc_number_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallDialEccNumberWithoutSimCard(void** state)
{
    (void)state;
    int ret = call_dial_ecc_number_without_sim_card(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallDialLongPhoneNumber(void** state)
{
    (void)state;
    int ret = call_dial_long_phone_number_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallDialShotPhoneNumber(void** state)
{
    (void)state;
    int ret = call_dial_short_phone_number_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleStab_CallDialingAndHangupNTimes(void** state)
{
    int ret;
    (void)state;

    for (int i = 0; i < 200; i++) {
        ret = call_dial_number_test(0);
        assert_int_equal(ret, OK);
        ret = call_hangup_all_test(0);
        assert_int_equal(ret, OK);
    }
}

static void TestTeleFunc_CallDialWithEnableHideCallId(void** state)
{
    (void)state;
    int ret = call_dial_with_enable_hide_callerid_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallDialWithDisabledHideCallId(void** state)
{
    (void)state;
    int ret = call_dial_with_disabled_hide_callerid_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallDialWithDefaultHideCallId(void** state)
{
    (void)state;
    int ret = call_dial_with_default_hide_callerid_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallDialWithAreaCode(void** state)
{
    (void)state;
    int ret = call_dial_with_area_code_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallDialWithPauseCode(void** state)
{
    (void)state;
    int ret = call_dial_with_pause_code_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallDialWithWaitCode(void** state)
{
    (void)state;
    int ret = call_dial_with_wait_code_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_CallDialWithNumerousCode(void** state)
{
    (void)state;
    int ret = call_dial_with_numerous_code_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallDialConference(void** state)
{
    (void)state;
    int ret = call_dial_conference_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_CallDtmfAfterDial(void** state)
{
    (void)state;
    int ret = call_dtmf_after_dial_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallIncomingAnswerAndHangup(void** state)
{
    (void)state;
    int ret = call_incoming_answer_and_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallIncomingAndCheckNumber(void** state)
{
    (void)state;
    int ret = call_incoming_and_check_number(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallIncomingAndCheckNumberInCall(void** state)
{
    (void)state;
    int ret = call_incoming_and_check_number_in_call(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallIncomingAnswerAndRemoteHangup(void** state)
{
    (void)state;
    int ret = call_incoming_answer_and_remote_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformReleaseAndAnswer(void** state)
{
    (void)state;
    int ret = call_release_and_answer(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformHoldAndAnswer(void** state)
{
    (void)state;
    int ret = call_hold_first_call_and_answer_second_call(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformMergeByUser(void** state)
{
    (void)state;
    int ret = call_merge_by_user(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformSeparateByUser(void** state)
{
    (void)state;
    int ret = call_separate_by_user(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformReleaseAndSwap(void** state)
{
    (void)state;
    int ret = call_release_and_swap_other_call(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformSwapInTwoCalling(void** state)
{
    (void)state;
    int ret = call_swap_in_two_calling(0);
    assert_int_equal(ret, 0);
}

static void TestTeleStab_CallPerformSwapTimesInTwoCalling(void** state)
{
    (void)state;
    int ret = call_swap_two_call_times_in_second_active(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformRejectSecondCallInCallActive(void** state)
{
    int ret = call_reject_second_call_in_call_active(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformRemoteAnswerAndHangup(void** state)
{
    int ret = call_outgoing_remote_answer_and_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformRemoteAnswerAndNetworkHangup(void** state)
{
    int ret = call_outgoing_remote_answer_and_network_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformRemoteHoldAfterLocalhold(void** state)
{
    int ret = call_remote_hold_after_local_hold_in_actve(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformRemoteHoldAfterLocalUnhold(void** state)
{
    int ret = call_remote_hold_after_local_unhold_in_actve(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformRemoteHoldUnholdAfterAnswer(void** state)
{
    int ret = call_remote_hold_and_unhold_after_incoming_answer(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformHoldAndHangup(void** state)
{
    (void)state;
    int ret = call_outgoing_hold_and_unhold_by_caller(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallPerformHoldCallAndRejectNewCall(void** state)
{
    (void)state;
    int ret = call_hold_current_call_and_reject_new_incoming(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallPerformUnHoldIncomingCallAfterHangupSecondCall(void** state)
{
    (void)state;
    int ret = call_unhold_first_incoming_call_after_hangup_second_call(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallPerformHangupAndResumeCall(void** state)
{
    (void)state;
    int ret = call_hangup_current_call_and_resume_call(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformHangupHoldCallInTwoCalls(void** state)
{
    (void)state;
    int ret = call_hangup_hold_call_in_two_calls(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformActiveAndSendtones(void** state)
{
    (void)state;
    int ret = call_outgoing_active_and_send_tones(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallDialAndRemoteActive(void** state)
{
    int ret = call_dial_and_remote_active(0, phone_num);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallDialAndRemoteHangup(void** state)
{
    int ret = call_dial_and_remote_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestTeleStab_CallDialAndRemoteHangupNTimes(void** state)
{
    (void)state;
    REPEAT_TEST_MORE_FOR
    {
        int ret = call_dial_and_remote_hangup(0);
        assert_int_equal(ret, 0);
        sleep(3);
    }
}

static void TestTeleFunc_CallDialAfterReject(void** state)
{
    (void)state;
    int ret = call_dial_after_caller_reject(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallDialOtherAfterReject(void** state)
{
    (void)state;
    int ret = call_dial_another_after_reject(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallDialCheckStatusInCallActive(void** state)
{
    (void)state;
    int ret = call_dial_and_check_status_in_call_active(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformCheckDialingStausWithMultiCall(void** state)
{
    (void)state;
    int ret = call_check_dialing_status_with_multi_call(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallDialAndKeepInActive(void** state)
{
    (void)state;
    int ret = call_dial_and_keep_in_call_active(0, phone_num);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallDialSecondCallAndRejectByCaller(void** state)
{
    (void)state;
    int ret = call_dial_second_call_and_reject_by_caller(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallDialSecondCallAndHangupByCaller(void** state)
{
    (void)state;
    int ret = call_dial_second_call_active_and_hangup_by_caller(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallIncomingandLocalHangup(void** state)
{
    (void)state;
    int ret = call_incoming_and_local_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallIncomingandRemoteHangup(void** state)
{
    (void)state;
    int ret = call_incoming_and_remote_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestTeleStab_CallIncomingandRemoteHangupNTimes(void** state)
{
    (void)state;
    int ret = call_incoming_and_remote_hangup_for_times(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallIncomingAfterRemoteHangup(void** state)
{
    (void)state;
    int ret = call_incoming_after_remote_hangup(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallIncomingAndHangupNewCall(void** state)
{
    (void)state;
    int ret = call_incoming_and_hangup_new_call(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallIncomingHangupFirstAnswerCall(void** state)
{
    (void)state;
    int ret = call_incoming_hangup_first_answer_call(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallIncomingHoldAndResume(void** state)
{
    (void)state;
    int ret = call_incoming_hold_and_resume_by_caller(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformSetVoicecallSlot(void** state)
{
    (void)state;
    int ret = call_set_voicecall_slot(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformGetVoicecallSlot(void** state)
{
    (void)state;
    int ret = call_get_default_voicecall_slot_test();
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallPerformClearVoicecallSlot(void** state)
{
    (void)state;
    int ret = call_clear_voicecall_slot();
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_CallListen(void** state)
{
    (void)state;
    int ret = call_listen_call_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_CallUnlisten(void** state)
{
    (void)state;
    int ret = call_unlisten_call_test();
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_CallListenAndUnlisten(void** state)
{
    TestTeleFunc_CI_CallListen(state);
    TestTeleFunc_CI_CallUnlisten(state);
}

static void TestTeleAbn_CallAnswerAgain(void** state)
{
    (void)state;
    int ret = call_abnormal_answer_again_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleAbn_CallDialError(void** state)
{
    (void)state;
    remote_command_response_fail(0, 1);
    int ret = call_dial_test(0, phone_num, 0);
    remote_command_response_fail(0, 0);
    assert_int_equal(ret, -1);
}

static void TestTeleAbn_CallAnswerError(void** state)
{
    (void)state;
    int ret = call_answer_error(0);
    assert_int_equal(ret, -1);
}

static void TestTeleAbn_CallRejectError(void** state)
{
    (void)state;
    remote_operation_call_incoming_test(0, "10086");
    remote_command_response_fail(0, 1);
    int ret = call_hangup_all_test(0);
    remote_command_response_fail(0, 0);
    assert_int_equal(ret, -1);
}

static void TestTeleAbn_CallHangupError(void** state)
{
    (void)state;
    int ret = call_dial_test(0, phone_num, 0);
    assert_int_equal(ret, 0);
    remote_command_response_fail(0, 1);
    ret = call_hangup_all_test(0);
    remote_command_response_fail(0, 0);
    assert_int_equal(ret, -1);
}

static void TestTeleAbn_CallConferenceError(void** state)
{
    (void)state;
    remote_command_response_fail(0, 1);
    int ret = call_dial_conference_test(0);
    remote_command_response_fail(0, 0);
    assert_int_equal(ret, -1);
}

// data testcases
static void TestTeleFunc_CI_DataRegister(void** state)
{
    (void)state;
    int ret = data_listen_data_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_DataUnregister(void** state)
{
    (void)state;
    int ret = data_unlisten_data_test();
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_DataRegisterAndUnregister(void** state)
{
    TestTeleFunc_CI_DataRegister(state);
    TestTeleFunc_CI_DataUnregister(state);
}

static void TestTeleFunc_CI_DataLoadApnContexts(void** state)
{
    (void)state;
    int ret = data_load_apn_contexts_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataSaveApnContextSupl(void** state)
{
    (void)state;
    int ret = data_save_apn_context_test("0", "3", "supl", "supl", "2", "2");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataSaveApnContextEmergency(void** state)
{
    (void)state;
    int ret = data_save_apn_context_test("0", "7", "emergency",
        "emergency", "2", "2");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataSaveLongApnContex(void** state)
{
    (void)state;
    int ret = data_save_apn_context_test("0", "1",
        "longname-----------------------------------------"
        "-----------------------------------------longname",
        "cmnet4", "2", "2");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataSaveApnContext(void** state)
{
    (void)state;
    int ret = data_save_apn_context_test("0", "1", "cmcc1", "cmnet1", "2", "2");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_DataRemoveApnContext(void** state)
{
    (void)state;
    int ret = data_save_apn_context_test("0", "1", "cmcc1", "cmnet1", "2", "2");
    assert_int_equal(ret, OK);
    ret = data_remove_apn_context_test("0", "/ril_0/context3");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataResetApnContexts(void** state)
{
    (void)state;
    int ret = data_reset_apn_contexts_test("0");
    assert_int_equal(ret, OK);
}

static void TestTeleStab_DataResetApnContextsNTimes(void** state)
{
    (void)state;
    REPEAT_TEST_MORE_FOR
    {
        int ret = data_reset_apn_contexts_test("0");
        assert_int_equal(ret, OK);
    }
}

static void TestTeleFunc_DataEditApnName(void** state)
{
    (void)state;
    int ret;
    ret = data_save_apn_context_test("0", "1", "cmcc1", "cmnet1", "2", "2");
    assert_int_equal(ret, OK);
    ret = data_edit_apn_context_test("0", "/ril_0/context3", "1", "cmname", "cmname", "2", "2");
    assert_int_equal(ret, OK);
    ret = data_load_apn_contexts_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataEditApnType(void** state)
{
    (void)state;
    int ret = data_save_apn_context_test("0", "1", "cmcc1", "cmnet1", "2", "2");
    assert_int_equal(ret, OK);
    ret = data_edit_apn_context_test("0", "/ril_0/context3", "3", "cmname", "cmname", "2", "2");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataEditApnProto(void** state)
{
    (void)state;
    int ret = data_save_apn_context_test("0", "1", "cmcc1", "cmnet1", "2", "2");
    assert_int_equal(ret, OK);
    ret = data_edit_apn_context_test("0", "/ril_0/context3", "3", "cmname", "cmname", "0", "2");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataEditApnAuth(void** state)
{
    (void)state;
    int ret = data_save_apn_context_test("0", "1", "cmcc1", "cmnet1", "2", "2");
    assert_int_equal(ret, OK);
    ret = data_edit_apn_context_test("0", "/ril_0/context3", "3", "cmname", "cmname", "0", "0");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataEditApnAll(void** state)
{
    (void)state;
    int ret;
    ret = data_save_apn_context_test("0", "1", "cmcc1", "cmnet1", "2", "2");
    assert_int_equal(ret, OK);
    ret = data_edit_apn_context_test("0", "/ril_0/context3", "2", "cmnameall", "cmnameall", "1", "1");
    assert_int_equal(ret, OK);
    ret = data_load_apn_contexts_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataEditApnAndRemove(void** state)
{
    (void)state;
    int ret;
    ret = data_save_apn_context_test("0", "1", "cmcc1", "cmnet1", "2", "2");
    assert_int_equal(ret, OK);
    ret = data_edit_apn_context_test("0", "/ril_0/context3", "2", "cmnameall", "cmnameall", "2", "2");
    assert_int_equal(ret, OK);
    ret = data_remove_apn_context_test("0", "/ril_0/context3");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataEditApnAndReset(void** state)
{
    (void)state;
    int ret;
    ret = data_save_apn_context_test("0", "1", "cmcc1", "cmnet1", "2", "2");
    assert_int_equal(ret, OK);
    ret = data_edit_apn_context_test("0", "/ril_0/context3", "1", "cmname", "cmname", "2", "2");
    assert_int_equal(ret, OK);
    ret = data_reset_apn_contexts_test("0");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataEditApnRepeatedlyAndLoad(void** state)
{
    (void)state;
    int ret;
    ret = data_save_apn_context_test("0", "1", "cmcc1", "cmnet1", "2", "2");
    assert_int_equal(ret, OK);
    ret = data_edit_apn_context_test("0", "/ril_0/context1", "1", "cmname11", "cmname", "2", "2");
    assert_int_equal(ret, OK);
    ret = data_edit_apn_context_test("0", "/ril_0/context1", "1", "cmname22", "cmname", "2", "2");
    assert_int_equal(ret, OK);
    ret = data_edit_apn_context_test("0", "/ril_0/context1", "1", "cmname33", "cmname", "2", "2");
    assert_int_equal(ret, OK);
    ret = data_load_apn_contexts_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_DataEnable(void** state)
{
    (void)state;
    int ret = data_enabled_test(1);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_DataDisable(void** state)
{
    (void)state;
    int ret = data_disabled_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleStab_DataEnableNTimes(void** state)
{
    (void)state;
    REPEAT_TEST_MORE_FOR
    {
        TestTeleFunc_CI_DataEnable(state);
        TestTeleFunc_CI_DataDisable(state);
    }
}

static void TestTeleFunc_CI_DataReleaseAndRequestNetworkInternet(void** state)
{
    (void)state;
    int ret;
    ret = data_release_internet_network_test(0);
    assert_int_equal(ret, OK);
    ret = data_request_internet_network_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleStab_DataReleaseAndRequestNetworkInternetNTimes(void** state)
{
    (void)state;
    REPEAT_TEST_MORE_FOR
    {
        TestTeleFunc_CI_DataReleaseAndRequestNetworkInternet(state);
    }
}

static void TestTeleFunc_DataRequestAndReleaseNetworkIms(void** state)
{
    (void)state;
    int ret;
    ret = data_request_ims_network_test(0);
    assert_int_equal(ret, OK);
    ret = data_release_ims_network_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleStab_DataRequestAndReleaseNetworkImsNTimes(void** state)
{
    (void)state;
    REPEAT_TEST_MORE_FOR
    {
        TestTeleFunc_DataRequestAndReleaseNetworkIms(state);
    }
}

static void TestTeleFunc_CI_DataSetAndGetPreferredApn(void** state)
{
    (void)state;
    int ret = data_set_and_get_preferred_apn_test(0, "/ril_0/context1");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_DataSendScreenState(void** state)
{
    (void)state;
    int ret = data_send_screen_stat_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_DataIsPsAttached(void** state)
{
    (void)state;
    int ret = data_is_ps_attached_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_DataGetNetworkType(void** state)
{
    (void)state;
    int ret = data_get_network_type_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataSetAndGetDefaultDataSlot(void** state)
{
    (void)state;
    int ret = data_set_and_get_default_data_slot_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_DataSetDataAllow(void** state)
{
    (void)state;
    int ret = data_set_data_allow_test(0);
    assert_true(ret == OK);
}

static void TestTeleFunc_CI_DataGetCallList(void** state)
{
    (void)state;
    int ret = data_get_call_list(0, 1);
    assert_true(ret == OK);
}

static void TestTeleFunc_DataSetRoamingWhenDataOff(void** state)
{
    (void)state;
    bool enable = false;
    int ret = data_get_enabled_test(&enable);
    assert_int_equal(ret, OK);
    if (enable) {
        ret = data_enable_data_test(0);
        assert_int_equal(ret, OK);
    }

    ret = data_enable_and_get_roaming_test();
    assert_int_equal(ret, OK);
    ret = data_disable_and_get_roaming_test();
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_DataEnableRoaming(void** state)
{
    (void)state;
    int ret = data_enable_and_get_roaming_test();
    assert_true(ret == OK);
}

static void TestTeleFunc_CI_DataDisableRoaming(void** state)
{
    (void)state;
    int ret = data_disable_and_get_roaming_test();
    assert_true(ret == OK);
}

static void TestTeleStab_DataToggleRoamingRepeatedly(void** state)
{
    (void)state;
    REPEAT_TEST_MORE_FOR
    {
        TestTeleFunc_CI_DataEnableRoaming(state);
        TestTeleFunc_CI_DataDisableRoaming(state);
    }
}

static void TestTeleFunc_DataRequestNetworksAndCheck(void** state)
{
    (void)state;
    int ret = data_request_ims_network_test(0);
    assert_int_equal(ret, OK);
    ret = data_get_call_list(0, 2);
    assert_int_equal(ret, OK);
    ret = data_release_ims_network_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_DataActivateAndCheckRAT(void** state)
{
    (void)state;
    int ret = data_release_internet_network_test(0);
    assert_int_equal(ret, OK);
    ret = data_request_internet_network_test(0);
    assert_int_equal(ret, OK);
    ret = data_is_ps_attached_test(0);
    assert_int_equal(ret, OK);
    ret = data_get_network_type_test(0);
    assert_int_equal(ret, OK);
}

void TestTeleFunc_DataAirplaneOffAutoReconnect(void** state)
{
    (void)state;
    int ret = data_enable_auto_when_airplane_mode_close_test();
    assert_int_equal(ret, OK);
}

void TestTeleFunc_DataTypeChangedWhileChangeRAT(void** state)
{
    (void)state;
    int ret = data_network_type_changed_while_change_rat_test(0, NETWORK_PREF_NET_TYPE_UMTS, NETWORK_TYPE_HSPA);
    assert_int_equal(ret, OK);
    sleep(5);
    ret = data_network_type_changed_while_change_rat_test(0, NETWORK_PREF_NET_TYPE_LTE_ONLY, NETWORK_TYPE_LTE);
    assert_int_equal(ret, OK);
}

void TestTeleFunc_DataRegStateChangedWhileChangeAP(void** state)
{
    (void)state;
    int ret = data_registration_changed_while_change_radio_power(0, 0, 0);
    assert_int_equal(ret, OK);
    sleep(5);
    ret = data_registration_changed_while_change_radio_power(0, 1, 1);
    assert_int_equal(ret, OK);
}

void TestTeleFunc_DataAPNAfterFlightMode(void** state)
{
    (void)state;
    int ret = data_apn_after_flight_mode_test(0);
    assert_int_equal(ret, OK);
}

void TestTeleFunc_DataLoadChinaMobileAPN(void** state)
{
    (void)state;
    int ret = data_load_carrier_apn_test(0, "46000");
    assert_int_equal(ret, OK);
}

void TestTeleFunc_DataLoadChinaUnicomAPN(void** state)
{
    (void)state;
    int ret = data_load_carrier_apn_test(0, "46001");
    assert_int_equal(ret, OK);
}

void TestTeleFunc_DataLoadChinaTelecomAPN(void** state)
{
    (void)state;
    int ret = data_load_carrier_apn_test(0, "46003");
    assert_int_equal(ret, OK);
}

void TestTeleFunc_DataLoadChinaBroadcacstNetworkAPN(void** state)
{
    (void)state;
    int ret = data_load_carrier_apn_test(0, "46013");
    assert_int_equal(ret, OK);
}

// sms testcases
static void TestTeleFunc_CI_SmsListen(void** state)
{
    (void)state;
    int ret = sms_listen_sms_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SmsUnListen(void** state)
{
    (void)state;
    int ret = sms_unlisten_sms_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SmsListenAndUnListen(void** state)
{
    TestTeleFunc_CI_SmsListen(state);
    TestTeleFunc_CI_SmsUnListen(state);
}

static void TestTeleFunc_CI_SmsSetAndGetServiceCenterNum(void** state)
{
    (void)state;
    int ret = sms_set_and_get_service_center_number_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendShortMessageInEnglish(void** state)
{
    (void)state;
    int result = 1;
    int ret = sms_send_message_test(0, phone_num, short_english_text, &result);
    assert_int_equal(ret, 0);
    assert_int_equal(result, 0);
}

static void TestTeleFunc_CI_SmsSendShortMessageInChinese(void** state)
{
    (void)state;
    int result = 1;
    int ret = sms_send_message_test(0, phone_num, short_chinese_text, &result);
    assert_int_equal(ret, 0);
    assert_int_equal(result, 0);
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
    int result = 1;
    int ret = sms_send_message_test(0, phone_num, long_english_text, &result);
    assert_int_equal(ret, 0);
    assert_int_equal(result, 0);
}

static void TestTeleFunc_CI_SmsSendLongMessageInChinese(void** state)
{
    (void)state;
    int result = 1;
    int ret = sms_send_message_test(0, phone_num, long_chinese_text, &result);
    assert_int_equal(ret, 0);
    assert_int_equal(result, 0);
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

static void TestTeleStab_SmsSendMessageContinuous(void** state)
{
    (void)state;
    int ret = sms_send_short_sms_continuous(0, phone_num);
    assert_int_equal(ret, 0);
}

static void TestTeleStab_SmsSendDataMessageContinuous(void** state)
{
    (void)state;
    int ret = sms_send_short_data_sms_continuous(0, phone_num);
    assert_int_equal(ret, 0);
}

static void TestTeleStab_SmsSendDataMessageAndMessageContinuous(void** state)
{
    (void)state;
    int ret = sms_send_short_mix_sms_continuous(0, phone_num);
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
    int ret = sms_send_message_in_special_ims_cap(0, phone_num, short_english_text, 1);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendChineseMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_in_special_ims_cap(0, phone_num, short_chinese_text, 1);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongEnglishMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_in_special_ims_cap(0, phone_num, long_english_text, 1);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongChineseMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_in_special_ims_cap(0, phone_num, long_chinese_text, 1);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendEnglishDataMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_special_ims_cap(0, phone_num, 0, short_english_text, 1);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendChineseDataMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_special_ims_cap(0, phone_num, 0, short_chinese_text, 1);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongEnglishDataMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_special_ims_cap(0, phone_num, 0, long_english_text, 1);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongChineseDataMessageInVoiceImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_special_ims_cap(0, phone_num, 0, long_chinese_text, 1);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendEnglishMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_in_special_ims_cap(0, phone_num, short_english_text, 4);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendChineseMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_in_special_ims_cap(0, phone_num, short_chinese_text, 4);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongEnglishMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_in_special_ims_cap(0, phone_num, long_english_text, 4);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongChineseMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_message_in_special_ims_cap(0, phone_num, long_chinese_text, 4);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendEnglishDataMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_special_ims_cap(0, phone_num, 0, short_english_text, 4);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendChineseDataMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_special_ims_cap(0, phone_num, 0, short_chinese_text, 4);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongEnglishDataMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_special_ims_cap(0, phone_num, 0, long_english_text, 4);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongChineseDataMessageInSmsImsCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_special_ims_cap(0, phone_num, 0, long_chinese_text, 4);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendEnglishMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_message_in_special_ims_cap(0, phone_num, short_english_text, 5);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendChineseMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_message_in_special_ims_cap(0, phone_num, short_chinese_text, 5);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongEnglishMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_message_in_special_ims_cap(0, phone_num, long_english_text, 5);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongChineseMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_message_in_special_ims_cap(0, phone_num, long_chinese_text, 5);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendEnglishDataMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_special_ims_cap(0, phone_num, 0, short_english_text, 5);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendChineseDataMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_special_ims_cap(0, phone_num, 0, short_chinese_text, 5);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongEnglishDataMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_special_ims_cap(0, phone_num, 0, long_english_text, 5);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendLongChineseDataMessageInSmsVoiceCap(void** state)
{
    (void)state;
    int ret = sms_send_data_message_in_special_ims_cap(0, phone_num, 0, long_chinese_text, 5);
    assert_int_equal(ret, 0);
}

static void TestTeleAbn_SmsSendMessageFailInAirplane(void** state)
{
    (void)state;
    int ret = sms_send_message_fail_in_airplane_test(0, phone_num, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSendMessageAfterDataOnOff(void** state)
{
    TestTeleFunc_CI_DataRegister(state);
    TestTeleFunc_CI_DataEnable(state);
    TestTeleFunc_CI_DataDisable(state);
    TestTeleFunc_CI_DataUnregister(state);
    TestTeleFunc_SmsSendShortMessageInEnglish(state);
}

static void TestTeleFunc_SmsReceiveMessage(void** state)
{
    (void)state;
    int ret = sms_receive_message_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsReceiveMessageAfterDataOnOff(void** state)
{
    TestTeleFunc_CI_DataRegister(state);
    TestTeleFunc_CI_DataEnable(state);
    TestTeleFunc_CI_DataDisable(state);
    TestTeleFunc_CI_DataUnregister(state);
    TestTeleFunc_SmsReceiveMessage(state);
}

static void TestTeleFunc_SmsReceiveMessageInActive(void** state)
{
    TestTeleFunc_CallDialAndKeepInActive(state);
    TestTeleFunc_SmsReceiveMessage(state);
}

static void TestTeleFunc_SmsReceiveMessageInDialing(void** state)
{
    TestTeleFunc_CI_CallDialNumber(state);
    TestTeleFunc_SmsReceiveMessage(state);
}

static void TestTeleFunc_SmsReceiveEnglishLongMessage(void** state)
{
    (void)state;
    int ret = sms_receive_english_long_message_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsReceiveEnglishLongMessageInActive(void** state)
{
    TestTeleFunc_CallDialAndKeepInActive(state);
    TestTeleFunc_SmsReceiveEnglishLongMessage(state);
}

static void TestTeleFunc_SmsReceiveEnglishLongMessageInDialing(void** state)
{
    TestTeleFunc_CI_CallDialNumber(state);
    TestTeleFunc_SmsReceiveEnglishLongMessage(state);
}

static void TestTeleFunc_SmsReceiveChineseLongMessage(void** state)
{
    (void)state;
    int ret = sms_receive_chinese_long_message_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsReceiveChineseLongMessageInActive(void** state)
{
    TestTeleFunc_CallDialAndKeepInActive(state);
    TestTeleFunc_SmsReceiveChineseLongMessage(state);
}

static void TestTeleFunc_SmsReceiveChineseLongMessageInDialing(void** state)
{
    TestTeleFunc_CI_CallDialNumber(state);
    TestTeleFunc_SmsReceiveChineseLongMessage(state);
}

static void TestTeleFunc_SmsSetAndGetDefaultSlot(void** state)
{
    (void)state;
    int result = -1;
    int ret = tapi_sms_set_default_slot(get_tapi_ctx(), 0);
    sleep(5);
    assert_int_equal(ret, 0);
    tapi_sms_get_default_slot(get_tapi_ctx(), &result);
    assert_int_equal(result, 0);
}

static void TestTeleFunc_SmsSetAndGetCellBroadcastPower(void** state)
{
    (void)state;
    int ret = sms_set_and_get_cell_broadcast_power(0, 1);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsSetAndGetCellBroadcastTopics(void** state)
{
    (void)state;
    int ret = sms_set_and_get_cell_broadcast_topics(0, "1");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallIncomingRejectandSendMessage(void** state)
{
    (void)state;
    TestTeleFunc_CallIncomingandLocalHangup(state);
    TestTeleFunc_SmsSendShortMessageInEnglish(state);
}

static void TestTeleFunc_NetSelectAuto(void** state)
{
    (void)state;
    int ret = net_select_auto_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_NetSelectManual(void** state)
{
    (void)state;
    sleep(4);
    int ret = net_select_manual_test(0, "310", "260", "lte");
    sleep(4);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_NetSelectManualAndRestoreAuto(void** state)
{
    TestTeleFunc_NetSelectManual(state);
    TestTeleFunc_NetSelectAuto(state);
}

static void TestTeleFunc_NetScan(void** state)
{
    (void)state;
    int ret = net_scan_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_NetGetServingCellinfos(void** state)
{
    (void)state;
    int ret = net_get_serving_cellinfos_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_NetGetNeighbouringCellInfos(void** state)
{
    (void)state;
    int ret = net_get_neighbouring_cellInfos_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_NetRegistrationInfo(void** state)
{
    (void)state;
    int ret = net_registration_info_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_NetGetOperatorName(void** state)
{
    (void)state;
    int ret = net_get_operator_name_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_NetQuerySignalstrength(void** state)
{
    (void)state;
    int ret = net_query_signalstrength_test(0);
    assert_int_equal(ret, OK);
}

// static void TestTeleNetSetCellInfoListRate(void **state)
// {
//     int ret = net_set_cell_info_list_rate_test(0, 10);
//     assert_int_equal(ret, OK);
// }

static void TestTeleFunc_CI_NetGetVoiceRegistered(void** state)
{
    (void)state;
    int ret = net_get_voice_registered_test(0);
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

// modem testcases
static void TestTeleFunc_CI_ModemGetImei(void** state)
{
    (void)state;
    int ret = get_imei_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemSetUmtsPrefNetMode(void** state)
{
    // defalut rat mode is NETWORK_PREF_NET_TYPE_LTE_GSM_WCDMA (9)
    tapi_pref_net_mode set_value = NETWORK_PREF_NET_TYPE_UMTS;
    int ret = set_pref_net_mode_test(0, set_value);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemSetGsmOnlyPrefNetMode(void** state)
{
    tapi_pref_net_mode set_value = NETWORK_PREF_NET_TYPE_GSM_ONLY;
    int ret = set_pref_net_mode_test(0, set_value);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemSetWcdmaOnlyPrefNetMode(void** state)
{
    tapi_pref_net_mode set_value = NETWORK_PREF_NET_TYPE_WCDMA_ONLY;
    int ret = set_pref_net_mode_test(0, set_value);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemSetLteOnlyPrefNetMode(void** state)
{
    tapi_pref_net_mode set_value = NETWORK_PREF_NET_TYPE_LTE_ONLY;
    int ret = set_pref_net_mode_test(0, set_value);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemSetLteWcdmaPrefNetMode(void** state)
{
    tapi_pref_net_mode set_value = NETWORK_PREF_NET_TYPE_LTE_WCDMA;
    int ret = set_pref_net_mode_test(0, set_value);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemSetLteGsmWcdmaPrefNetMode(void** state)
{
    tapi_pref_net_mode set_value = NETWORK_PREF_NET_TYPE_LTE_GSM_WCDMA;
    int ret = set_pref_net_mode_test(0, set_value);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_ModemVerifyPrefNetMode(void** state)
{
    tapi_pref_net_mode value, get_value;
    int ret;

    (void)state;
    value = NETWORK_PREF_NET_TYPE_UMTS;
    ret = set_pref_net_mode_test(0, value);
    assert_int_equal(ret, OK);
    ret = get_pref_net_mode_test(0, &get_value);
    assert_int_equal(ret, OK);
    assert_true(value == get_value);

    value = NETWORK_PREF_NET_TYPE_GSM_ONLY;
    ret = set_pref_net_mode_test(0, value);
    assert_int_equal(ret, OK);
    ret = get_pref_net_mode_test(0, &get_value);
    assert_int_equal(ret, OK);
    assert_true(value == get_value);

    value = NETWORK_PREF_NET_TYPE_WCDMA_ONLY;
    ret = set_pref_net_mode_test(0, value);
    assert_int_equal(ret, OK);
    ret = get_pref_net_mode_test(0, &get_value);
    assert_int_equal(ret, OK);
    assert_true(value == get_value);

    value = NETWORK_PREF_NET_TYPE_LTE_GSM_WCDMA;
    ret = set_pref_net_mode_test(0, value);
    assert_int_equal(ret, OK);
    ret = get_pref_net_mode_test(0, &get_value);
    assert_int_equal(ret, OK);
    assert_true(value == get_value);

    value = NETWORK_PREF_NET_TYPE_LTE_ONLY;
    ret = set_pref_net_mode_test(0, value);
    assert_int_equal(ret, OK);
    ret = get_pref_net_mode_test(0, &get_value);
    assert_int_equal(ret, OK);
    assert_true(value == get_value);

    value = NETWORK_PREF_NET_TYPE_LTE_WCDMA;
    ret = set_pref_net_mode_test(0, value);
    assert_int_equal(ret, OK);
    ret = get_pref_net_mode_test(0, &get_value);
    assert_int_equal(ret, OK);
    assert_true(value == get_value);
}

static void TestTeleFunc_CI_ModemGetPrefNetMode(void** state)
{
    tapi_pref_net_mode get_value = NETWORK_PREF_NET_TYPE_ANY;
    int ret = get_pref_net_mode_test(0, &get_value);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_ModemSetRadioPowerOn(void** state)
{
    (void)state;
    int ret = set_radio_power_test(0, true);
    assert_int_equal(ret, OK);
    sleep(5);
}

static void TestTeleStab_ModemSetRadioPowerOnAndOffContinuous(void** state)
{
    (void)state;
    int ret = radio_power_on_off_pending_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemSetRadioPowerOnAndModemDisable(void** state)
{
    (void)state;
    int ret = radio_power_on_modem_disable_pending_test(0);
    assert_int_equal(ret, OK);
    sleep(10);
}

static void TestTeleFunc_ModemSetRadioPowerOffOrModemDisableAfterotherAction(void** state)
{
    (void)state;
    int ret = modem_disable_power_off_pending_test(0);
    assert_int_equal(ret, OK);
    sleep(10);
}

static void TestTeleFunc_CI_ModemSetRadioPowerOff(void** state)
{
    (void)state;
    int ret = set_radio_power_test(0, false);
    assert_int_equal(ret, OK);
    sleep(5);
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

static void TestTeleFunc_SmsReceiveMessageAfterRadioOnOff(void** state)
{
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
    TestTeleFunc_SmsReceiveMessage(state);
}

static void TestTeleFunc_SmsSendMessageAfterRadioOnOff(void** state)
{
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
    TestTeleFunc_SmsSendShortMessageInEnglish(state);
}

static void TestTeleFunc_SmsReceiveEnglishLongMessageAfterRadioOnOff(void** state)
{
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
    TestTeleFunc_SmsReceiveEnglishLongMessage(state);
}

static void TestTeleFunc_SmsReceiveChineseLongMessageAfterRadioOnOff(void** state)
{
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
    TestTeleFunc_SmsReceiveChineseLongMessage(state);
}

static void TestTeleFunc_CI_ModemEnableStatus(void** state)
{
    (void)state;
    int get_state = -1;
    int ret = get_modem_status_test(0, &get_state);
    assert_int_equal(ret, OK);
    assert_int_equal(get_state, 1);
}

static void TestTeleFunc_CI_ModemDsiableStatus(void** state)
{
    (void)state;
    int get_state = -1;
    int ret = get_modem_status_test(0, &get_state);
    assert_int_equal(ret, OK);
    assert_int_equal(get_state, 0);
}

static void TestTeleFunc_CI_ModemEnable(void** state)
{
    (void)state;
    int ret = modem_enable_status_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CI_ModemDisable(void** state)
{
    (void)state;
    int ret = modem_disable_status_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleStab_ModemEnableAndDisableContinuous(void** state)
{
    (void)state;
    int ret = modem_enable_disable_pending_test(0);
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

static void TestTeleStab_ModemEnableDisableNTimesUnderRadioPowerOff(void** state)
{
    (void)state;
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    TestTeleFunc_CI_ModemDisable(state);
    TestTeleFunc_CI_ModemDsiableStatus(state);
    TestTeleFunc_CI_ModemEnableDisableNTimes(state);
}

static void TestTeleFunc_ModemDisableEnableRadioPowerOff(void** state)
{
    (void)state;
    bool value;
    int ret;

    TestTeleFunc_CI_ModemDisable(state);
    TestTeleFunc_CI_ModemDsiableStatus(state);
    TestTeleFunc_CI_ModemEnable(state);
    TestTeleFunc_CI_ModemEnableStatus(state);
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    ret = get_radio_power_test(0, &value);
    assert_int_equal(ret, OK);
    assert_false(value);
}

static void TestTeleFunc_ModemDisableRadioPowerOff(void** state)
{
    (void)state;
    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
    TestTeleFunc_CI_ModemDisable(state);
    TestTeleFunc_CI_ModemDsiableStatus(state);
    int ret = set_radio_power_test(0, 0);
    assert_int_equal(ret, -1);
    TestTeleFunc_CI_ModemEnable(state);
    TestTeleFunc_CI_ModemEnableStatus(state);
    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
}

static void TestTeleFunc_ModemSetSignalReportThreshold(void** state)
{
    int ret;

    (void)state;
    ret = set_signal_report_threshold_test(0, 0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemSuppressMsgReport(void** state)
{
    int ret;

    (void)state;
    ret = suppress_message_report(0, true);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemEnableModemStationary(void** state)
{
    int ret;

    (void)state;
    ret = enable_modem_stationary(0, true);
    assert_int_equal(ret, OK);
    ret = enable_modem_stationary(0, false);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemSetModemStationaryThreshold(void** state)
{
    int ret;

    (void)state;
    ret = set_modem_stationary_threshold(0, 1);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemUpgradeIndication(void** state)
{
    int ret;

    TestTeleFunc_CI_ModemEnable(state);
    ret = trigger_modem_upgrade_state_test(0, 0);
    assert_int_equal(ret, OK);
    ret = trigger_modem_upgrade_state_test(0, 2);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemUpgradeStateCheck(void** state)
{
    int ret;

    (void)state;
    ret = check_modem_upgrade_state_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemUpgradeCMD(void** state)
{
    int ret;

    (void)state;
    ret = send_modem_upgrade_cmd_test(0, 0);
    assert_int_equal(ret, OK);
}

#ifndef CONFIG_TELEPHONY_DFX
static void TestTeleFunc_ModemAbnormalEventReport(void** state)
{
    int ret;

    (void)state;
    ret = check_abnormal_event_report(false, 2); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 3); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 5); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 6); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 7); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 8); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 9); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 10); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 11); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 12); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 13); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 14); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 15); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 16); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 17); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 20); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 21); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 200); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 201); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 202); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 203); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(false, 208); // expect normal data
    assert_int_equal(ret, OK);
    ret = check_abnormal_event_report(true, 208); // unexpected data
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemEnableDataFailDFX(void** state)
{
    (void)state;

    int ret = data_enabled_fail_test();
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemOOSDFX(void** state)
{
    (void)state;

    int ret = check_oos_dfx();
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemDisableDFX(void** state)
{
    (void)state;

    int ret = check_disable_modem_duration_dfx();
    assert_int_equal(ret, OK);
}
#endif

static void TestTeleStab_ModemEnableDisableNTimesUnderDialingCall(void** state)
{
    (void)state;

    TestTeleFunc_CI_ModemEnable(state);
    TestTeleFunc_CI_ModemEnableStatus(state);
    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
    TestTeleFunc_CI_CallDialNumber(state);
    REPEAT_TEST_LESS_FOR
    {
        TestTeleFunc_CI_ModemDisable(state);
        TestTeleFunc_CI_ModemDsiableStatus(state);
        TestTeleFunc_CI_ModemEnable(state);
        TestTeleFunc_CI_ModemEnableStatus(state);
    }
}

static void TestTeleFunc_ModemSetRadioOffUnderDialingCall(void** state)
{
    (void)state;
    bool value;
    int ret;

    TestTeleFunc_CI_CallDialNumber(state);
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    ret = get_radio_power_test(0, &value);
    assert_int_equal(ret, OK);
    assert_false(value);
    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
}

static void TestTeleFunc_ModemSetRadioOffUnderOngoingCall(void** state)
{
    (void)state;
    bool value;
    int ret;

    ret = call_dial_and_keep_in_call_active(0, phone_num);
    assert_int_equal(ret, 0);
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    ret = get_radio_power_test(0, &value);
    assert_int_equal(ret, OK);
    assert_false(value);
    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
}

static void TestTeleStab_ModemEnableDisableNTimesUnderOngoingCall(void** state)
{
    (void)state;
    int ret = call_dial_and_keep_in_call_active(0, phone_num);
    assert_int_equal(ret, OK);
    REPEAT_TEST_LESS_FOR
    {
        TestTeleFunc_CI_ModemDisable(state);
        TestTeleFunc_CI_ModemDsiableStatus(state);
        TestTeleFunc_CI_ModemEnable(state);
        TestTeleFunc_CI_ModemEnableStatus(state);
    }
}

static void TestTeleFunc_CI_ModemRegisterOrUnregister(void** state)
{
    (void)state;
    int ret;
    ret = modem_register_test(0);
    assert_int_equal(ret, OK);
    ret = modem_unregister_test();
    assert_true(ret == OK);
}

static void TestTeleFunc_CI_ModemGetRevision(void** state)
{
    int ret;
    ret = get_modem_revision_test(0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemInvokeOemShotRilRequestRaw(void** state)
{
    int ret = modem_invoke_oem_ril_request_raw_test(0, "01A0B023", 4);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemInvokeOemLongRilRequestRaw(void** state)
{
    int ret = modem_invoke_oem_ril_request_raw_test(0, "01A0B02301A0B02301A0B02301A0B02301A0B02301", 21);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemInvokeOemSeperateRilRequestRaw(void** state)
{
    int ret = modem_invoke_oem_ril_request_raw_test(0, "10|22", 2);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemInvokeOemNormalRilRequestRaw(void** state)
{
    int ret = modem_invoke_oem_ril_request_raw_test(0, "01A0B023", 2);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemInvokeOemRilRequestATCmdStrings(void** state)
{
    char req_data[] = "AT+CPIN?";
    int ret = modem_invoke_oem_ril_request_strings_test(0, req_data, 1);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemInvokeOemRilRequestNotATCmdStrings(void** state)
{
    // not AT cmd
    char req_data[] = "10|22";
    int ret = modem_invoke_oem_ril_request_strings_test(0, req_data, 2);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemInvokeOemRilRequestHexStrings(void** state)
{
    char req_data[] = "0x10|0x01";
    int ret = modem_invoke_oem_ril_request_strings_test(0, req_data, 2);
    assert_int_equal(ret, OK);
}

// static void TestTeleModemInvokeOemRilRequestLongStrings(void **state)
// {
//     char req_data[MAX_INPUT_ARGS_LEN];

//     // test error
//     // FIXME: modem_invoke_oem_ril_request_strings_test interface buffer overflow
//     // when req_data len is 21, current max len is 20
//     strcpy(req_data,
//         "10|22|10|22|10|22|10|22|10|22|10|22|10|22|10|22|10|22|10|22");
//     int ret = modem_invoke_oem_ril_request_strings_test(0, req_data, 20);
//     assert_int_equal(ret, -1);

//     // strcpy(req_data, "10|22");
//     // // FIXME: _dbus_check_is_valid_utf8 is called by dbus_message_iter_append_basic
//     // // cannot handle \0 in char * string;
//     // ret = modem_invoke_oem_ril_request_strings_test(0, req_data, 20);
//     // assert_int_equal(ret, -1);
// }

static void TestTeleFunc_CI_ImsListen(void** state)
{
    (void)state;
    int ret = ims_listen_ims_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_ImsUnlisten(void** state)
{
    (void)state;
    int ret = ims_unlisten_ims_test();
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_ImsListenAndUnlisten(void** state)
{
    TestTeleFunc_CI_ImsListen(state);
    TestTeleFunc_CI_ImsUnlisten(state);
}

static void TestTeleFunc_CI_ImsGetRegistration(void** state)
{
    (void)state;
    int ret = ims_get_registration_test(0, 1);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_ImsSetVoiceCap(void** state)
{
    (void)state;
    int ret = ims_set_service_status_test(0, 1);
    assert_int_equal(ret, 0);
    sleep(5);
}

static void TestTeleFunc_CI_ImsSetSmsCap(void** state)
{
    (void)state;
    int ret = ims_set_service_status_test(0, 4);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_ImsSetSmsVoiceCap(void** state)
{
    (void)state;
    int ret = ims_set_service_status_test(0, 5);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_ImsResetImsCap(void** state)
{
    (void)state;
    int ret = ims_set_service_status_test(0, 5);
    assert_int_equal(ret, 0);
    sleep(10);
}

static void TestTeleFunc_ImsKeepRegOnAfterRadioOffOn(void** state)
{
    (void)state;
    int ret = ims_is_reg_after_radio_off_on_test(0, true);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_ImsKeepVolteAvailAfterRadioOffOn(void** state)
{
    (void)state;
    int ret = ims_is_volte_available_after_radio_off_on_test(0, true);
    assert_int_equal(ret, 0);
}

static void TestTeleAbn_ImsVolteAvailAfterModemOff(void** state)
{
    TestTeleFunc_CI_ModemDisable(state);
    int ret = ims_set_service_status_test(0, 5);
    assert_int_equal(ret, -EIO);
    ret = ims_is_volte_available_as_expect_test(0, false);
    assert_int_equal(ret, -EIO);
    TestTeleFunc_CI_ModemEnable(state);
}

static void TestTeleAbn_ImsVolteAvailAfterRadioOff(void** state)
{
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    int ret = ims_set_service_status_test(0, 5);
    assert_int_equal(ret, 0);
    ret = ims_is_volte_available_as_expect_test(0, false);
    assert_int_equal(ret, 0);
}

static void TestTeleAbn_ImsVolteAvailWithSimAbsent(void** state)
{
    int ret = remote_sim_absent_operation_test(0);
    assert_int_equal(ret, 0);
    ret = ims_set_service_status_test(0, 5);
    assert_int_equal(ret, 0);
    ret = ims_is_volte_available_as_expect_test(0, false);
    assert_int_equal(ret, 0);
}

static void TestTeleAbn_ImsVolteAvailWhenCallDialing(void** state)
{
    int ret = 0;

    TestTeleFunc_CI_CallDialNumber(state);
    ret = ims_set_service_status_test(0, 5);
    assert_int_equal(ret, 0);
    ret = get_current_call_state_test(0);
    assert_int_equal(ret, CALL_STATUS_DIALING);
    ret = ims_is_volte_available_as_expect_test(0, true);
    assert_int_equal(ret, 0);
    ret = call_hangup_all_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleAbn_ImsVolteAvailWhenCallActive(void** state)
{
    int ret = call_dial_in_active_test(0);
    assert_int_equal(ret, 0);
    ret = ims_set_service_status_test(0, 5);
    assert_int_equal(ret, 0);
    ret = ims_is_volte_available_as_expect_test(0, true);
    assert_int_equal(ret, 0);
    ret = call_hangup_all_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallDialInVolteReg(void** state)
{
    TestTeleFunc_CI_ImsSetVoiceCap(state);
    TestTeleFunc_CI_CallDialNumber(state);
    TestTeleFunc_CI_ImsSetSmsVoiceCap(state);
}

static void TestTeleFunc_CallDialAndRemoteActiveAfterRadioPowerOnOff(void** state)
{
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
    TestTeleFunc_CallDialAndRemoteActive(state);
}

static void TestTeleFunc_CallIncomingAnswerAfterRadioPowerOnOff(void** state)
{
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
    TestTeleFunc_CallIncomingAnswerAndHangup(state);
}

static void TestTeleFunc_CallIncomingAnswerInCallAfterRadioPowerOnOff(void** state)
{
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
    TestTeleFunc_CallPerformHoldAndAnswer(state);
}

static void TestTeleFunc_CallDialSecondCallAndActiveAfterRadioPowerOnOff(void** state)
{
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
    TestTeleFunc_CallDialSecondCallAndHangupByCaller(state);
}

static void TestTeleFunc_CallDialAndRemoteActiveAfterDataOnOff(void** state)
{
    TestTeleFunc_CI_DataEnable(state);
    TestTeleFunc_CI_DataDisable(state);
    TestTeleFunc_CallDialAndRemoteActive(state);
}

static void TestTeleFunc_CallIncomingAnswerAfterDataOnOff(void** state)
{
    TestTeleFunc_CI_DataEnable(state);
    TestTeleFunc_CI_DataDisable(state);
    TestTeleFunc_CallIncomingAnswerAndHangup(state);
}

static void TestTeleFunc_CallIncomingAnswerInCallAfterDataOnOff(void** state)
{
    TestTeleFunc_CI_DataEnable(state);
    TestTeleFunc_CI_DataDisable(state);
    TestTeleFunc_CallPerformHoldAndAnswer(state);
}

static void TestTeleFunc_CallDialSecondCallAndActiveAfterDataOnOff(void** state)
{
    TestTeleFunc_CI_DataEnable(state);
    TestTeleFunc_CI_DataDisable(state);
    TestTeleFunc_CallDialSecondCallAndHangupByCaller(state);
}

static void TestTeleFunc_CallDialAndRemoteActiveInRoming(void** state)
{
    TestTeleFunc_CI_DataRegister(state);
    TestTeleFunc_CI_DataEnableRoaming(state);
    TestTeleFunc_CallDialAndRemoteActive(state);
    TestTeleFunc_CI_DataDisableRoaming(state);
    TestTeleFunc_CI_DataUnregister(state);
}

static void TestTeleFunc_CallIncomingAnswerInRoming(void** state)
{
    TestTeleFunc_CI_DataRegister(state);
    TestTeleFunc_CI_DataEnableRoaming(state);
    TestTeleFunc_CallIncomingAnswerAndHangup(state);
    TestTeleFunc_CI_DataDisableRoaming(state);
    TestTeleFunc_CI_DataUnregister(state);
}

static void TestTeleFunc_CallIncomingAnswerInCallInRoming(void** state)
{
    TestTeleFunc_CI_DataRegister(state);
    TestTeleFunc_CI_DataEnableRoaming(state);
    TestTeleFunc_CallPerformHoldAndAnswer(state);
    TestTeleFunc_CI_DataDisableRoaming(state);
    TestTeleFunc_CI_DataUnregister(state);
}

static void TestTeleFunc_CallDialSecondCallAndActiveInRoming(void** state)
{
    TestTeleFunc_CI_DataRegister(state);
    TestTeleFunc_CI_DataEnableRoaming(state);
    TestTeleFunc_CallDialSecondCallAndHangupByCaller(state);
    TestTeleFunc_CI_DataDisableRoaming(state);
    TestTeleFunc_CI_DataUnregister(state);
}

static void TestTeleFunc_CallPerformTransfer(void** state)
{
    (void)state;
    int ret = call_transfer_in_active_and_hold_call(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CallDialingThirdCall(void** state)
{
    (void)state;
    int ret = call_dial_in_two_calling(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsReceiveMessageInVoiceImsCap(void** state)
{
    TestTeleFunc_CI_ImsSetVoiceCap(state);
    TestTeleFunc_SmsReceiveMessage(state);
    TestTeleFunc_CI_ImsSetSmsVoiceCap(state);
}

static void TestTeleFunc_SmsReceiveEnglishLongMessageInVoiceImsCap(void** state)
{
    TestTeleFunc_CI_ImsSetVoiceCap(state);
    TestTeleFunc_SmsReceiveEnglishLongMessage(state);
    TestTeleFunc_CI_ImsSetSmsVoiceCap(state);
}

static void TestTeleFunc_SmsReceiveChineseLongMessageInVoiceImsCap(void** state)
{
    TestTeleFunc_CI_ImsSetVoiceCap(state);
    TestTeleFunc_SmsReceiveChineseLongMessage(state);
    TestTeleFunc_CI_ImsSetSmsVoiceCap(state);
}

static void TestTeleFunc_SmsReceiveMessageInSmsImsCap(void** state)
{
    TestTeleFunc_CI_ImsSetSmsCap(state);
    TestTeleFunc_SmsReceiveMessage(state);
    TestTeleFunc_CI_ImsSetSmsVoiceCap(state);
}

static void TestTeleFunc_SmsReceiveEnglishLongMessageInSmsImsCap(void** state)
{
    TestTeleFunc_CI_ImsSetSmsCap(state);
    TestTeleFunc_SmsReceiveEnglishLongMessage(state);
    TestTeleFunc_CI_ImsSetSmsVoiceCap(state);
}

static void TestTeleFunc_SmsReceiveChineseLongMessageInSmsImsCap(void** state)
{
    TestTeleFunc_CI_ImsSetSmsCap(state);
    TestTeleFunc_SmsReceiveChineseLongMessage(state);
    TestTeleFunc_CI_ImsSetSmsVoiceCap(state);
}

static void TestTeleFunc_SmsReceiveReportAfterSendShortMessage(void** state)
{
    (void)state;
    int ret = sms_receive_report_test(0, phone_num, 0, short_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsReceiveReportAfterSendChineseLongMessage(void** state)
{
    (void)state;
    int ret = sms_receive_report_test(0, phone_num, 0, long_chinese_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SmsReceiveReportAfterSendEnglishLongMessage(void** state)
{
    (void)state;
    int ret = sms_receive_report_test(0, phone_num, 0, long_english_text);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSRegister(void** state)
{
    (void)state;
    int ret = ss_listen_ss_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSUnRegister(void** state)
{
    (void)state;
    int ret = ss_unlisten_ss_test();
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSResigterAndUnRegister(void** state)
{
    TestTeleFunc_CI_SSRegister(state);
    TestTeleFunc_CI_SSUnRegister(state);
}

static void TestTeleFunc_SSRequestCallBarring(void** state)
{
    (void)state;
    int ret = ss_request_call_barring_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SSSetAndGetCallBarring(void** state)
{
    (void)state;
    int ret = ss_set_and_get_call_barring_option_test(0, "AI", "1234");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SSChangeAndResetCallBarringPassword(void** state)
{
    (void)state;
    int ret = ss_change_and_reset_call_barring_password_test(0, "1234", "2345");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SSDisableAllIncoming(void** state)
{
    (void)state;
    int ret = ss_disable_all_incoming_test(0, "1234");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SSDisableAllOutgoing(void** state)
{
    (void)state;
    int ret = ss_disable_all_outgoing_test(0, "1234");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SSDisableAllCallBarrings(void** state)
{
    (void)state;
    int ret = ss_disable_all_call_barrings_test(0, "1234");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSSetAndGetCallForwardingUnConditional(void** state)
{
    (void)state;
    int ret = ss_set_and_get_call_forwarding_option_test(0, 0, "10086");
    assert_int_equal(ret, 0);
}

static void TestTeleStab_SSCallForwardingContinuous(void** state)
{
    (void)state;
    int ret = ss_call_forwarding_continuous_test(0, phone_num);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSSetAndGetCallForwardingBusy(void** state)
{
    (void)state;
    int ret = ss_set_and_get_call_forwarding_option_test(0, 1, "10086");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSSetAndGetCallForwardingNoReply(void** state)
{
    (void)state;
    int ret = ss_set_and_get_call_forwarding_option_test(0, 2, "10086");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSSetAndGetCallForwardingNotReachable(void** state)
{
    (void)state;
    int ret = ss_set_and_get_call_forwarding_option_test(0, 3, "10086");
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSEnableCallWaiting(void** state)
{
    (void)state;
    int ret = ss_set_and_get_call_waiting_test(0, true);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_CI_SSDisableCallWaiting(void** state)
{
    (void)state;
    int ret = ss_set_and_get_call_waiting_test(0, false);
    assert_int_equal(ret, 0);
}

static void TestTeleAbn_SSListenAbnormalIdFail(void** state)
{
    (void)state;
    int ret = tapi_ss_register(get_tapi_ctx(), 0, 46, NULL, NULL);
    assert_int_equal(ret, -EINVAL);
    ret = tapi_ss_register(get_tapi_ctx(), 0, 50, NULL, NULL);
    assert_int_equal(ret, -EINVAL);
}

static void TestTeleFunc_CI_SSEnableAndDisableCallWaiting(void** state)
{
    TestTeleFunc_CI_SSEnableCallWaiting(state);
    TestTeleFunc_CI_SSDisableCallWaiting(state);
}

static void TestTeleFunc_SSCallForwardingAfterRadioOffOn(void** state)
{
    int ret = ss_set_call_forwarding_option_test(0, 0, "10086");
    assert_int_equal(ret, OK);
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
    ret = ss_get_call_forwarding_option_test(0, 0);
    assert_int_equal(ret, OK);
    ret = ss_clear_call_forwarding_option_test(0, 0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_SSCallWaitingAfterRadioOffOn(void** state)
{
    TestTeleFunc_CI_SSEnableCallWaiting(state);
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
    int ret = ss_get_call_waiting_test(0, true);
    assert_int_equal(ret, OK);
    TestTeleFunc_CI_SSDisableCallWaiting(state);
}

static void TestTeleFunc_ModemGetDefaultPhoneState(void** state)
{
    tapi_phone_state target;
    int ret;

    (void)state;

    target = PHONE_IDLE;
    TestTeleFunc_CallIncomingandLocalHangup(state);
    ret = get_phone_state_test(0, target);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemGetPhoneState(void** state)
{
    tapi_phone_state target;
    int ret;

    (void)state;
    TestTeleFunc_ModemGetDefaultPhoneState(state);
    target = PHONE_OFFHOOK;
    TestTeleFunc_CI_CallDialNumber(state);
    ret = get_phone_state_test(0, target);
    assert_int_equal(ret, OK);
    ret = call_hangup_current_call_test(0);
    assert_int_equal(ret, OK);
    ret = ss_clear_call_forwarding_option_test(0, 0);
    ret = ss_clear_call_forwarding_option_test(0, 1);
    ret = ss_clear_call_forwarding_option_test(0, 2);
    ret = ss_clear_call_forwarding_option_test(0, 3);
    ret = remote_operation_call_incoming_test(0, phone_num);
    assert_int_equal(ret, OK);
    target = PHONE_RINGING;
    ret = get_phone_state_test(0, target);
    assert_int_equal(ret, OK);
    ret = call_hangup_current_call_test(0);
}

static void TestTeleFunc_ModemGetPhoneStateUnderMOCall(void** state)
{
    tapi_phone_state target;
    int ret;

    (void)state;
    ret = modem_register_test(0);
    assert_int_equal(ret, OK);
    ret = call_listen_call_test(0);
    assert_int_equal(ret, OK);
    target = PHONE_OFFHOOK;
    ret = call_dial_test(0, phone_num, 0);
    assert_int_equal(ret, OK);
    assert_true(judge_data.phone_state_value == target);
    ret = remote_operation_call_active_test(0, phone_num);
    target = PHONE_IDLE;
    ret = call_hangup_current_call_test(0);
    assert_int_equal(ret, OK);
    assert_true(judge_data.phone_state_value == target);
    ret = call_unlisten_call_test();
    assert_int_equal(ret, OK);
    ret = modem_unregister_test();
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemGetPhoneStateUnderMTCall(void** state)
{
    tapi_phone_state target;
    int ret;

    (void)state;
    ret = modem_register_test(0);
    assert_int_equal(ret, OK);
    ret = call_listen_call_test(0);
    assert_int_equal(ret, OK);
    target = PHONE_RINGING;
    ret = remote_operation_call_incoming_test(0, phone_num);
    assert_int_equal(ret, OK);
    assert_true(judge_data.phone_state_value == target);
    target = PHONE_OFFHOOK;
    ret = answer_incoming_call_test(0);
    assert_int_equal(ret, OK);
    assert_true(judge_data.phone_state_value == target);
    target = PHONE_IDLE;
    ret = call_hangup_current_call_test(0);
    assert_int_equal(ret, OK);
    assert_true(judge_data.phone_state_value == target);
    ret = modem_unregister_test();
    assert_int_equal(ret, OK);
    ret = call_unlisten_call_test();
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemGetPhoneStateUnderMTRejectCall(void** state)
{
    tapi_phone_state target;
    int ret;

    (void)state;
    ret = modem_register_test(0);
    assert_int_equal(ret, OK);
    ret = call_listen_call_test(0);
    assert_int_equal(ret, OK);
    target = PHONE_RINGING;
    ret = remote_operation_call_incoming_test(0, phone_num);
    assert_int_equal(ret, OK);
    assert_true(judge_data.phone_state_value == target);
    target = PHONE_IDLE;
    ret = call_hangup_current_call_test(0);
    assert_int_equal(ret, OK);
    assert_true(judge_data.phone_state_value == target);
    ret = modem_unregister_test();
    assert_int_equal(ret, OK);
    ret = call_unlisten_call_test();
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_ModemGetPhoneStateUnderDialingCallSms(void** state)
{
    tapi_phone_state target;
    int ret;

    (void)state;
    ret = modem_register_test(0);
    assert_int_equal(ret, OK);
    ret = call_listen_call_test(0);
    assert_int_equal(ret, OK);
    target = PHONE_OFFHOOK;
    ret = call_dial_test(0, phone_num, 0);
    assert_int_equal(ret, OK);
    assert_true(judge_data.phone_state_value == target);
    TestTeleFunc_SmsSendShortMessageInEnglish(state);
    target = PHONE_IDLE;
    ret = call_hangup_current_call_test(0);
    assert_int_equal(ret, OK);
    assert_true(judge_data.phone_state_value == target);
    ret = modem_unregister_test();
    assert_int_equal(ret, OK);
    ret = call_unlisten_call_test();
    assert_int_equal(ret, OK);
}

static void TestTeleStab_SSCallWaitingContinuous(void** state)
{
    (void)state;
    int ret = ss_call_waiting_continuous_test(0);
    assert_int_equal(ret, 0);
}

static void TestTeleFunc_SSEnableAndDisableFdn(void** state)
{
    (void)state;
    int ret = ss_set_and_get_fdn_test(0, true, "1234");
    assert_int_equal(ret, 0);
    ret = ss_set_and_get_fdn_test(0, false, "1234");
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
        if (g_context != NULL) {
            syslog(LOG_ERR, "recieve dbus disconnected msg, free tapi g_context");
            tapi_close(g_context);
            g_context = NULL;
        }

        if (g_uv_exit_flag) {
            syslog(LOG_INFO, "tapi already closed, stop default loop");
            uv_stop(uv_default_loop());
            g_uv_exit_flag = false;
        }
    }
}

#ifdef CONFIG_PHONE_SERVICE
#ifdef CONFIG_PHONE_SERVICE_WTP
static void TestTeleFunc_PhoneServiceRegisterWTPCB(void** state)
{
    (void)state;
    int ret;

    ret = phone_service_register_wtp_cb_test(&g_uv_message);
    assert_int_equal(ret, -1); // mark -1 temp
}

static void TestTeleFunc_PhoneServiceUnRegisterWTPCB(void** state)
{
    (void)state;
    int ret;

    ret = phone_service_unregister_wtp_cb_test(&g_uv_message);
    assert_int_equal(ret, -1); // mark -1 temp
}

static void TestTeleFunc_PhoneServiceUpdateLocalInfo(void** state)
{
    (void)state;
    int ret;

    ret = phone_service_update_local_info_test(&g_uv_message);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_PhoneServiceSetDiscovery(void** state)
{
    (void)state;
    int ret;

    ret = phone_service_set_discovery_test(&g_uv_message, 1);
    assert_int_equal(ret, OK);
    ret = phone_service_set_discovery_test(&g_uv_message, 0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_PhoneServiceSetVisibility(void** state)
{
    (void)state;
    int ret;

    ret = phone_service_set_visibility_test(&g_uv_message, 1);
    assert_int_equal(ret, OK);
    ret = phone_service_set_visibility_test(&g_uv_message, 0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_PhoneServiceSetAudio(void** state)
{
    (void)state;
    int ret;

    ret = phone_service_set_audio_test(&g_uv_message, 0);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_PhoneServiceDialWTP(void** state)
{
    (void)state;
    int ret;

    ret = phone_service_dial_wtp_test(&g_uv_message);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_PhoneServiceHangupWTP(void** state)
{
    (void)state;
    int ret;

    ret = phone_service_hangup_wtp_test(&g_uv_message);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_PhoneServiceAnswerWTP(void** state)
{
    (void)state;
    int ret;

    ret = phone_service_answer_wtp_test(&g_uv_message);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_PhoneServiceRejectWTP(void** state)
{
    (void)state;
    int ret;

    ret = phone_service_reject_wtp_test(&g_uv_message);
    assert_int_equal(ret, OK);
}
#endif

static void TestTeleFunc_PhoneServiceSetRadioPowerOffAndOn(void** state)
{
    (void)state;
    int ret = set_phone_radio_power_test(&g_uv_message, 0);
    assert_int_equal(ret, OK);
    sleep(1);
    ret = set_phone_radio_power_test(&g_uv_message, 1);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_PhoneServiceRegisterESIMCB(void** state)
{
    (void)state;
    int ret;

    ret = phone_service_register_esim_cb_test(&g_uv_message);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_PhoneServiceUnRegisterESIMCB(void** state)
{
    (void)state;
    int ret;

    ret = phone_service_unregister_esim_cb_test(&g_uv_message);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_PhoneServiceRegisterUnRegisterESIMCB(void** state)
{
    TestTeleFunc_PhoneServiceRegisterESIMCB(state);
    TestTeleFunc_PhoneServiceUnRegisterESIMCB(state);
}

static void TestTeleFunc_PhoneServiceDialAndHangupESIM(void** state)
{
    (void)state;
    int ret;

    ret = phone_service_dial_and_hangup_esim_test(&g_uv_message, phone_num);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_PhoneServiceIncomingAnswerAndHangupESIM(void** state)
{
    (void)state;
    int ret;

    ret = phone_service_incoming_answer_and_hangup_esim_test(&g_uv_message);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_PhoneServiceIncomingAndRejectESIM(void** state)
{
    (void)state;
    int ret;

    ret = phone_service_incoming_and_reject_esim_test(&g_uv_message);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_PhoneServiceReleaseAndAnswerESIM(void** state)
{
    (void)state;
    int ret;

    ret = phone_service_release_and_answer_esim_test(&g_uv_message);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_PhoneServiceHoldAndUnholdESIM(void** state)
{
    (void)state;
    int ret;

    ret = phone_service_hold_and_unhold_esim_test(&g_uv_message);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_PhoneServiceMergeESIM(void** state)
{
    (void)state;
    int ret;

    ret = phone_service_merge_esim_test(&g_uv_message);
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_PhoneServiceSendTonesESIM(void** state)
{
    (void)state;
    int ret;

    ret = phone_service_send_tones_esim_test(&g_uv_message);
    assert_int_equal(ret, OK);
}
#endif

static void* run_test_loop(void* args)
{
    g_context = tapi_open(TAPI_TEST_DBUS_NAME, on_tapi_client_ready, NULL);
    if (g_context == NULL) {
        return NULL;
    }

#ifdef CONFIG_PHONE_SERVICE
    if (tapi_start_phone_service_client(uv_default_loop(), NULL, false) < 0) {
        syslog(LOG_ERR, "error:phone service client init fail\n");
    }
#endif
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());

    return NULL;
}

static int tapi_close_test(void)
{
    struct uv_tapi_cmd_data_s* cmd_data;

    if (get_tapi_ctx() == NULL) {
        syslog(LOG_ERR, "context is already null!");
        return -EINVAL;
    }

    cmd_data = malloc(sizeof(struct uv_tapi_cmd_data_s));
    if (cmd_data == NULL) {
        syslog(LOG_ERR, "malloc cmd_data: failed");
        return -ENOMEM;
    }

    memset(cmd_data, 0, sizeof(struct uv_tapi_cmd_data_s));
    cmd_data->is_open = false;

    syslog(LOG_INFO, "tapi_close_test: send close tapi context(%p)", get_tapi_ctx());
    g_uv_cmd_tapi.data = cmd_data;
    if (uv_async_send(&g_uv_cmd_tapi) != 0) {
        syslog(LOG_ERR, "tapi_close_test: uv_async_send failed");
        free(cmd_data);
        return -EINVAL;
    }

    return 0;
}

static int tapi_open_test(char* tapi_name, unsigned int spec_service)
{
    struct uv_tapi_cmd_data_s* cmd_data;

    if (get_tapi_ctx() != NULL) {
        syslog(LOG_ERR, "context(%p) is not null, first call tapi-close!", get_tapi_ctx());
        return -EINVAL;
    }

    cmd_data = malloc(sizeof(struct uv_tapi_cmd_data_s));
    if (cmd_data == NULL) {
        syslog(LOG_ERR, "malloc cmd_data: failed");
        return -ENOMEM;
    }
    memset(cmd_data, 0, sizeof(struct uv_tapi_cmd_data_s));
    strncpy(cmd_data->dbus_name, tapi_name, sizeof(cmd_data->dbus_name) - 1);
    cmd_data->is_open = true;
    cmd_data->spec_service = spec_service;

    syslog(LOG_INFO, "tapi_open_test:name=%s spec_service=0x%x", cmd_data->dbus_name, cmd_data->spec_service);
    g_uv_cmd_tapi.data = cmd_data;
    if (uv_async_send(&g_uv_cmd_tapi) != 0) {
        syslog(LOG_ERR, "tapi_open_test: uv_async_send failed");
        free(cmd_data);
        return -EINVAL;
    }

    return 0;
}

static int async_cmd_tapi_open_handler(struct uv_tapi_cmd_data_s* data)
{
    if (g_context != NULL) {
        syslog(LOG_ERR, "g_context(%p) is not null, first call tapi-close cmd!", g_context);
        return -EINVAL;
    }

    if (data->spec_service == TAPI_SERVICE_NONE || data->spec_service == TAPI_SERVICE_FULL)
        g_context = tapi_open(data->dbus_name, on_tapi_client_ready, NULL);
    else
        g_context = tapi_open_service(data->dbus_name, on_tapi_client_ready, NULL, data->spec_service);

    if (g_context == NULL) {
        syslog(LOG_ERR, "tapi_open: g_context is null");
        return -EINVAL;
    }
    syslog(LOG_INFO, "tapi_open: created tapi g_context(%p) ", g_context);

    return 0;
}

static int async_cmd_tapi_close_handler(struct uv_tapi_cmd_data_s* data)
{
    if (g_context == NULL) {
        syslog(LOG_ERR, "g_context is already null");
        return -EINVAL;
    }

    syslog(LOG_DEBUG, "tapi_close: free g_context(%p)", g_context);
    tapi_close(g_context);
    g_context = NULL;

    return 0;
}

static void async_cmd_tapi(uv_async_t* handle)
{
    struct uv_tapi_cmd_data_s* data = handle->data;

    if (data == NULL) {
        syslog(LOG_ERR, "async_cmd_tapi: data is null");
        return;
    }

    if (data->is_open) {
        async_cmd_tapi_open_handler(data);
    } else {
        async_cmd_tapi_close_handler(data);
    }

    free(data);
}

static void TestTeleFunc_ModemCloseTapi(void** state)
{
    (void)state;
    int ret = tapi_close_test();
    assert_int_equal(ret, OK);

    sleep(2);
    assert_true(get_tapi_ctx() == NULL);
}

static void TestTeleFunc_CI_ModemDefaultOpenTapi(void** state)
{
    if (get_tapi_ctx() != NULL) {
        TestTeleFunc_ModemCloseTapi(state);
    }

    int ret = tapi_open_test(TAPI_TEST_DBUS_NAME, 0);
    assert_int_equal(ret, OK);

    sleep(2);
    assert_true(get_tapi_ctx() != NULL);

    tapi_enable_modem(get_tapi_ctx(), 0, 0, 1, NULL); // eanble modem anyway
    sleep(10);

    ret = sim_has_icc_card_test(0);
    assert_int_equal(ret, OK);
}

static int TearDown_OpenDefaultTapi(void** state)
{
    /* recover tapi to default open */
    sleep(3);
    TestTeleFunc_CI_ModemDefaultOpenTapi(state);
    return 0;
}

static void TestTeleFunc_CI_ModemBtTeleOpenTapi(void** state)
{
    int ret;
    unsigned int tapi_service;
    bool result = false;

    if (get_tapi_ctx() != NULL) {
        TestTeleFunc_ModemCloseTapi(state);
    }

    tapi_service = TAPI_SERVICE_MODEM | TAPI_SERVICE_CALL | TAPI_SERVICE_NETREG;
    ret = tapi_open_test("vela.bt.tele", tapi_service);
    assert_int_equal(ret, OK);

    sleep(2);
    assert_true(get_tapi_ctx() != NULL);

    tapi_enable_modem(get_tapi_ctx(), 0, 0, 0, NULL); // disable modem anyway
    sleep(10);
    TestTeleFunc_CI_ModemEnable(state);

    /* No sim service, so return no availble proxy failure */
    ret = tapi_sim_has_icc_card(get_tapi_ctx(), 0, &result);
    assert_int_equal(ret, -EIO);

    /* modem, call and net can get proxy value success */
    TestTeleFunc_CI_ModemGetRevision(state);
    TestTeleFunc_CI_NetGetOperatorName(state);
    TestTeleFunc_CI_NetGetVoiceRegistered(state);
}

static void TestTeleFunc_CallDialAndHangupEcc(void** state)
{
    (void)state;
    bool get_value;
    int ret;

    ret = sim_set_operator_test(0, "46000");
    assert_int_equal(ret, OK);
    ret = call_dial_and_remote_active(0, "120");
    assert_int_equal(ret, OK);
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    ret = get_radio_power_test(0, &get_value);
    assert_int_equal(ret, OK);
    assert_false(get_value);

    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
    ret = sim_set_operator_test(0, "000");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallPerformRadioPowerOffUnderActiveECCCall(void** state)
{
    (void)state;
    bool get_value;
    int ret;

    ret = sim_set_operator_test(0, "46000");
    assert_int_equal(ret, OK);
    ret = call_dial_and_keep_in_call_active(0, "120");
    assert_int_equal(ret, OK);
    ret = set_radio_power_test(0, 0);
    assert_int_equal(ret, -1);
    ret = get_radio_power_test(0, &get_value);
    assert_int_equal(ret, OK);
    assert_true(get_value);
    ret = call_hangup_current_call_test(0);
    assert_int_equal(ret, OK);
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    ret = get_radio_power_test(0, &get_value);
    assert_int_equal(ret, OK);
    assert_false(get_value);

    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
    ret = sim_set_operator_test(0, "000");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallPerformModemDisableUnderActiveECCCall(void** state)
{
    (void)state;
    bool get_value;
    int ret;

    ret = sim_set_operator_test(0, "46000");
    assert_int_equal(ret, OK);
    ret = call_dial_and_keep_in_call_active(0, "120");
    assert_int_equal(ret, OK);
    ret = enable_modem_test(0, 0);
    assert_int_equal(ret, -1);
    TestTeleFunc_CI_ModemEnableStatus(state);
    ret = call_hangup_current_call_test(0);
    assert_int_equal(ret, OK);
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    ret = get_radio_power_test(0, &get_value);
    assert_int_equal(ret, OK);
    assert_false(get_value);

    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
    ret = sim_set_operator_test(0, "000");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallPerformRadioPowerOffUnderDialECCCall(void** state)
{
    (void)state;
    bool get_value;
    int ret;

    ret = sim_set_operator_test(0, "46000");
    assert_int_equal(ret, OK);
    ret = call_dial_test(0, "120", 0);
    assert_int_equal(ret, OK);
    ret = set_radio_power_test(0, 0);
    assert_int_equal(ret, -1);
    ret = get_radio_power_test(0, &get_value);
    assert_int_equal(ret, OK);
    assert_true(get_value);
    ret = call_hangup_current_call_test(0);
    assert_int_equal(ret, OK);
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    ret = get_radio_power_test(0, &get_value);
    assert_int_equal(ret, OK);
    assert_false(get_value);

    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
    ret = sim_set_operator_test(0, "000");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallPerformModemDisableUnderDialECCCall(void** state)
{
    (void)state;
    bool get_value;
    int ret;

    ret = sim_set_operator_test(0, "46000");
    assert_int_equal(ret, OK);
    ret = call_dial_test(0, "120", 0);
    assert_int_equal(ret, OK);
    ret = enable_modem_test(0, 0);
    assert_int_equal(ret, -1);
    TestTeleFunc_CI_ModemEnableStatus(state);
    ret = call_hangup_current_call_test(0);
    assert_int_equal(ret, OK);
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    ret = get_radio_power_test(0, &get_value);
    assert_int_equal(ret, OK);
    assert_false(get_value);

    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
    ret = sim_set_operator_test(0, "000");
    assert_int_equal(ret, OK);
}

static void TestTeleFunc_CallDialECCCallAndSMSSend(void** state)
{
    (void)state;
    int ret;
    bool get_value;

    ret = sim_set_operator_test(0, "46000");
    assert_int_equal(ret, OK);
    ret = call_dial_test(0, "120", 0);
    assert_int_equal(ret, OK);
    ret = sms_listen_sms_test(0);
    assert_int_equal(ret, OK);
    TestTeleFunc_SmsSendShortMessageInEnglish(state);
    ret = call_hangup_current_call_test(0);
    assert_int_equal(ret, OK);
    ret = sms_unlisten_sms_test(0);
    assert_int_equal(ret, OK);
    ret = sim_set_operator_test(0, "000");
    assert_int_equal(ret, OK);
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    ret = get_radio_power_test(0, &get_value);
    assert_int_equal(ret, OK);
    assert_false(get_value);

    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
}

static void TestTeleFunc_CallDialECCCallAndSMSReceive(void** state)
{
    (void)state;
    int ret;
    bool get_value;

    ret = sim_set_operator_test(0, "46000");
    assert_int_equal(ret, OK);
    ret = call_dial_test(0, "120", 0);
    assert_int_equal(ret, OK);
    ret = sms_listen_sms_test(0);
    assert_int_equal(ret, OK);
    TestTeleFunc_SmsReceiveMessage(state);
    ret = call_hangup_current_call_test(0);
    assert_int_equal(ret, OK);
    ret = sms_unlisten_sms_test(0);
    assert_int_equal(ret, OK);
    ret = sim_set_operator_test(0, "000");
    assert_int_equal(ret, OK);
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    ret = get_radio_power_test(0, &get_value);
    assert_int_equal(ret, OK);
    assert_false(get_value);

    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
}

static void TestTeleStab_CallActiveECCCallNTimes(void** state)
{
    (void)state;
    int ret;

    ret = sim_set_operator_test(0, "46000");
    assert_int_equal(ret, OK);
    for (int _i = 0; _i < 20; _i++) {
        ret = call_dial_and_remote_active(0, "120");
        assert_int_equal(ret, OK);
    }
    ret = sim_set_operator_test(0, "000");
    assert_int_equal(ret, OK);
}

static void TestTeleStab_CallSetRadioPowerOffAfterECCCallNTimes(void** state)
{
    (void)state;
    int ret;
    bool get_value;

    TestTeleStab_CallActiveECCCallNTimes(state);
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    ret = get_radio_power_test(0, &get_value);
    assert_int_equal(ret, OK);
    assert_false(get_value);

    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
}

static void TestTeleStab_CallDialEccAfterResetModemNTimes(void** state)
{
    (void)state;
    int ret;
    bool get_value;

    for (int _i = 0; _i < 20; _i++) {
        TestTeleFunc_CI_ModemDisable(state);
        TestTeleFunc_CI_ModemDsiableStatus(state);
        TestTeleFunc_CI_ModemEnable(state);
        TestTeleFunc_CI_ModemEnableStatus(state);
    }
    ret = call_dial_and_remote_active(0, "120");
    assert_int_equal(ret, OK);
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    ret = get_radio_power_test(0, &get_value);
    assert_int_equal(ret, OK);
    assert_false(get_value);

    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
}

static void TestTeleStab_CallDialEccAfterResetRadioPowerNTimes(void** state)
{
    (void)state;

    int ret;
    bool get_value;

    for (int _i = 0; _i < 20; _i++) {
        TestTeleFunc_CI_ModemSetRadioPowerOff(state);
        TestTeleFunc_CI_ModemSetRadioPowerOn(state);
    }
    ret = call_dial_and_remote_active(0, "120");
    assert_int_equal(ret, OK);
    TestTeleFunc_CI_ModemSetRadioPowerOff(state);
    ret = get_radio_power_test(0, &get_value);
    assert_int_equal(ret, OK);
    assert_false(get_value);

    TestTeleFunc_CI_ModemSetRadioPowerOn(state);
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
    uv_async_init(uv_default_loop(), &g_uv_cmd_tapi, async_cmd_tapi);

#ifdef CONFIG_PHONE_SERVICE
    uv_async_init(uv_default_loop(), &g_uv_message.async, uv_async_callback);
#endif
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
        cmocka_unit_test(TestTeleFunc_CI_SimListenAndUnListen),
        cmocka_unit_test(TestTeleFunc_CI_SimHasIccCard),
        cmocka_unit_test(TestTeleStab_SimHasIccCardNumerousTimes),
        cmocka_unit_test(TestTeleFunc_CI_SimGetOperatorName),
        cmocka_unit_test(TestTeleStab_SimGetOperatorNameNumerousTimes),
        cmocka_unit_test(TestTeleFunc_CI_SimGetOperator),
        cmocka_unit_test(TestTeleStab_SimGetOperatorNumerousTimes),
        cmocka_unit_test(TestTeleFunc_CI_SimGetSubscriberId),
        cmocka_unit_test(TestTeleStab_SimGetSubscriberIdNumerousTimes),
        cmocka_unit_test(TestTeleFunc_CI_SimGetIccId),
        cmocka_unit_test(TestTeleStab_SimGetIccIdNumerousTimes),
        cmocka_unit_test(TestTeleFunc_CI_SimGetMSISDN),
        cmocka_unit_test(TestTeleStab_SimGetMSISDNNumerousTimes),
        cmocka_unit_test(TestTeleFunc_CI_SimTransmitAPDUInBasicChannel),
        cmocka_unit_test(TestTeleFunc_CI_SimOpenAndCloseLogicalChannel),
        cmocka_unit_test_setup_teardown(TestTeleAbn_CI_SimOpenLogicalChannelWithErrorCode, NULL, teardown_sim_channel),
        cmocka_unit_test(TestTeleStab_SimLogicalChannelOpenCloseNumerous),
        cmocka_unit_test_setup_teardown(TestTeleStab_SimLogicalChannelOpenCloseWithErrorCodeNumerous, NULL, teardown_sim_channel),
        cmocka_unit_test(TestTeleFunc_CI_SimTransmitAPDUInLogicalChannel),
        cmocka_unit_test_setup_teardown(TestTeleAbn_CI_SimTransmitAPDUInLogicalChannelWithErrorCode, NULL, teardown_sim_channel),
        cmocka_unit_test(TestTeleFunc_SimSetUiccEnablement),
        cmocka_unit_test(TestTeleFunc_SimGetUiccEnablement),
        cmocka_unit_test(TestTeleFunc_CI_SimTransmitAPDUBasicChannel),
        cmocka_unit_test_setup_teardown(TestTeleAbn_CI_SimTransmitAPDUBasicChannelWithErrorCode, NULL, teardown_sim_channel),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SimRmoteAbsentInsertOperator, setup_sim, teardown_sim),
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
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_SimCheckSimInvalid, setup_sim, teardown_sim),
    };

    const struct CMUnitTest CallTestSuites[] = {
        cmocka_unit_test(TestTeleFunc_CI_CallListenAndUnlisten),
        cmocka_unit_test(TestTeleFunc_CallPerformLoadAndCompareEccWithChinaSimCard),
        cmocka_unit_test(TestTeleFunc_CallPerformLoadAndCompareEccWithoutSimCard),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_CallDialNumber, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_CallDialNumberWithoutSimCard, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_CallDialEccNumber, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialEccNumberWithoutSimCard, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialLongPhoneNumber, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialShotPhoneNumber, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialWithEnableHideCallId, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialWithDisabledHideCallId, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialWithDefaultHideCallId, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialWithAreaCode, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialWithPauseCode, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialWithWaitCode, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_CallDialWithNumerousCode, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialConference, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_CallDtmfAfterDial, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallIncomingAnswerAndHangup, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallIncomingAndCheckNumber, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallIncomingAndCheckNumberInCall, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallIncomingAnswerAndRemoteHangup, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformReleaseAndAnswer, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformHoldAndAnswer, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformMergeByUser, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformSeparateByUser, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformReleaseAndSwap, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformSwapInTwoCalling, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleStab_CallPerformSwapTimesInTwoCalling, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformRejectSecondCallInCallActive, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformRemoteAnswerAndHangup, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformRemoteAnswerAndNetworkHangup, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformRemoteHoldAfterLocalhold, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformRemoteHoldAfterLocalUnhold, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformRemoteHoldUnholdAfterAnswer, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformHoldAndHangup, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformHoldCallAndRejectNewCall, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformUnHoldIncomingCallAfterHangupSecondCall, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformHangupAndResumeCall, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformHangupHoldCallInTwoCalls, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformActiveAndSendtones, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialAndRemoteActive, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformCheckDialingStausWithMultiCall, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialAndKeepInActive, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialSecondCallAndRejectByCaller, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialSecondCallAndHangupByCaller, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialAndRemoteHangup, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleStab_CallDialAndRemoteHangupNTimes, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialAfterReject, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialCheckStatusInCallActive, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialOtherAfterReject, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallIncomingandLocalHangup, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallIncomingandRemoteHangup, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleStab_CallIncomingandRemoteHangupNTimes, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallIncomingRejectandSendMessage, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallIncomingAfterRemoteHangup, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallIncomingAndHangupNewCall, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallIncomingHangupFirstAnswerCall, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallIncomingHoldAndResume, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformSetVoicecallSlot, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformGetVoicecallSlot, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformClearVoicecallSlot, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleAbn_CallAnswerAgain, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialAndHangupEcc, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformRadioPowerOffUnderActiveECCCall, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformModemDisableUnderActiveECCCall, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformRadioPowerOffUnderDialECCCall, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformModemDisableUnderDialECCCall, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialECCCallAndSMSSend, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialECCCallAndSMSReceive, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleStab_CallActiveECCCallNTimes, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleStab_CallSetRadioPowerOffAfterECCCallNTimes, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleStab_CallDialEccAfterResetModemNTimes, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleStab_CallDialEccAfterResetRadioPowerNTimes, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialInVolteReg, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialAndRemoteActiveAfterRadioPowerOnOff, setup_callAndRadio, teardown_callAndRadio),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallIncomingAnswerAfterRadioPowerOnOff, setup_callAndRadio, teardown_callAndRadio),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallIncomingAnswerInCallAfterRadioPowerOnOff, setup_callAndRadio, teardown_callAndRadio),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialSecondCallAndActiveAfterRadioPowerOnOff, setup_callAndRadio, teardown_callAndRadio),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialAndRemoteActiveAfterDataOnOff, setup_callAndData, teardown_callAndData),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallIncomingAnswerAfterDataOnOff, setup_callAndData, teardown_callAndData),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallIncomingAnswerInCallAfterDataOnOff, setup_callAndData, teardown_callAndData),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialSecondCallAndActiveAfterDataOnOff, setup_callAndData, teardown_callAndData),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialAndRemoteActiveInRoming, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallIncomingAnswerInRoming, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallIncomingAnswerInCallInRoming, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialSecondCallAndActiveInRoming, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallPerformTransfer, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallDialingThirdCall, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleStab_CallDialingAndHangupNTimes, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleAbn_CallDialError, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleAbn_CallAnswerError, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleAbn_CallRejectError, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleAbn_CallHangupError, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleAbn_CallConferenceError, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CallStatusError, setup_call, teardown_call),
    };

    const struct CMUnitTest DataTestSuites[] = {
        cmocka_unit_test(TestTeleFunc_CI_DataRegisterAndUnregister),
        cmocka_unit_test(TestTeleFunc_CI_DataLoadApnContexts),
        cmocka_unit_test_setup_teardown(TestTeleFunc_DataSaveApnContext, setup_data, teardown_data),
        cmocka_unit_test(TestTeleFunc_DataRemoveApnContext),
        cmocka_unit_test(TestTeleStab_DataResetApnContextsNTimes),
        cmocka_unit_test_setup_teardown(TestTeleFunc_DataEditApnName, setup_data, teardown_data),
        cmocka_unit_test_setup_teardown(TestTeleFunc_DataEditApnType, setup_data, teardown_data),
        cmocka_unit_test_setup_teardown(TestTeleFunc_DataEditApnProto, setup_data, teardown_data),
        cmocka_unit_test_setup_teardown(TestTeleFunc_DataEditApnAuth, setup_data, teardown_data),
        cmocka_unit_test_setup_teardown(TestTeleFunc_DataEditApnAll, setup_data, teardown_data),
        cmocka_unit_test_setup_teardown(TestTeleFunc_DataEditApnAndRemove, setup_data, teardown_data),
        cmocka_unit_test_setup_teardown(TestTeleFunc_DataEditApnAndReset, setup_data, teardown_data),
        cmocka_unit_test_setup_teardown(TestTeleFunc_DataEditApnRepeatedlyAndLoad, setup_data, teardown_data),
        cmocka_unit_test_setup_teardown(TestTeleStab_DataEnableNTimes, setup_data, teardown_data),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_DataEnable, setup_data_enable, teardown_data_enable),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_DataDisable, setup_data_enable, teardown_data_enable),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_DataReleaseAndRequestNetworkInternet, setup_data_enable, teardown_data_enable),
        cmocka_unit_test_setup_teardown(TestTeleStab_DataReleaseAndRequestNetworkInternetNTimes, setup_data_enable, teardown_data_enable),
        cmocka_unit_test_setup_teardown(TestTeleFunc_DataRequestAndReleaseNetworkIms, setup_data_enable, teardown_data_enable),
        cmocka_unit_test_setup_teardown(TestTeleStab_DataRequestAndReleaseNetworkImsNTimes, setup_data_enable, teardown_data_enable),
        cmocka_unit_test_setup_teardown(TestTeleFunc_DataSaveApnContextSupl, setup_data, teardown_data),
        cmocka_unit_test_setup_teardown(TestTeleFunc_DataSaveApnContextEmergency, setup_data, teardown_data),
        cmocka_unit_test(TestTeleFunc_CI_DataSetAndGetPreferredApn),
        cmocka_unit_test(TestTeleFunc_CI_DataSendScreenState),
        cmocka_unit_test(TestTeleFunc_CI_DataGetNetworkType),
        cmocka_unit_test(TestTeleFunc_CI_DataIsPsAttached),
        cmocka_unit_test(TestTeleFunc_DataSetAndGetDefaultDataSlot),
        cmocka_unit_test(TestTeleFunc_CI_DataSetDataAllow),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_DataGetCallList, setup_data_enable, teardown_data_enable),
        cmocka_unit_test_setup_teardown(TestTeleFunc_DataSaveLongApnContex, setup_data, teardown_data),
        cmocka_unit_test(TestTeleFunc_DataResetApnContexts),
        cmocka_unit_test(TestTeleFunc_CI_DataEnableRoaming),
        cmocka_unit_test(TestTeleFunc_CI_DataDisableRoaming),
        cmocka_unit_test(TestTeleFunc_DataSetRoamingWhenDataOff),
        cmocka_unit_test(TestTeleStab_DataToggleRoamingRepeatedly),
        cmocka_unit_test_setup_teardown(TestTeleFunc_DataRequestNetworksAndCheck, setup_data_enable, teardown_data_enable),
        cmocka_unit_test_setup_teardown(TestTeleFunc_DataActivateAndCheckRAT, setup_data_enable, teardown_data_enable),
        cmocka_unit_test_setup_teardown(TestTeleFunc_DataAirplaneOffAutoReconnect, setup_data_enable, teardown_data_enable),
        cmocka_unit_test_setup_teardown(TestTeleFunc_DataTypeChangedWhileChangeRAT, setup_data_enable, teardown_data_enable),
        cmocka_unit_test_setup_teardown(TestTeleFunc_DataRegStateChangedWhileChangeAP, setup_data_enable, teardown_data_enable),
        cmocka_unit_test(TestTeleFunc_DataAPNAfterFlightMode),
        cmocka_unit_test(TestTeleFunc_DataLoadChinaMobileAPN),
        cmocka_unit_test(TestTeleFunc_DataLoadChinaUnicomAPN),
        cmocka_unit_test(TestTeleFunc_DataLoadChinaTelecomAPN),
        cmocka_unit_test(TestTeleFunc_DataLoadChinaBroadcacstNetworkAPN),
    };

    const struct CMUnitTest SmsTestSuites[] = {
        cmocka_unit_test(TestTeleFunc_CI_SmsListenAndUnListen),
        cmocka_unit_test(TestTeleFunc_CI_SmsSetAndGetServiceCenterNum),
        cmocka_unit_test(TestTeleFunc_SmsSendShortMessageInEnglish),
        cmocka_unit_test(TestTeleFunc_CI_SmsSendShortMessageInChinese),
        cmocka_unit_test(TestTeleFunc_SmsSendShortDataMessageInEnglish),
        cmocka_unit_test(TestTeleFunc_SmsSendShortDataMessageInChinese),
        cmocka_unit_test(TestTeleFunc_SmsSendLongMessageInEnglish),
        cmocka_unit_test(TestTeleFunc_CI_SmsSendLongMessageInChinese),
        cmocka_unit_test(TestTeleFunc_SmsSendLongDataMessageInEnglish),
        cmocka_unit_test(TestTeleFunc_SmsSendLongDataMessageInChinese),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendShortEnglishMessageInDialing, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_SmsSendShortChineseMessageInDialing, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendLongEnglishMessageInDialing, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendLongChineseMessageInDialing, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendShortEnglishDataMessageInDialing, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendShortChineseDataMessageInDialing, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendLongEnglishDataMessageInDialing, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendLongChineseDataMessageInDialing, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendEnglishMessageInVoiceImsCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendChineseMessageInVoiceImsCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendLongEnglishMessageInVoiceImsCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendLongChineseMessageInVoiceImsCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendEnglishDataMessageInVoiceImsCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendChineseDataMessageInVoiceImsCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendLongEnglishDataMessageInVoiceImsCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendLongChineseDataMessageInVoiceImsCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendEnglishMessageInSmsImsCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendChineseMessageInSmsImsCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendLongEnglishMessageInSmsImsCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendLongChineseMessageInSmsImsCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendEnglishDataMessageInSmsImsCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendChineseDataMessageInSmsImsCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendLongEnglishDataMessageInSmsImsCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendLongChineseDataMessageInSmsImsCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendEnglishMessageInSmsVoiceCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendChineseMessageInSmsVoiceCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendLongEnglishMessageInSmsVoiceCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendLongChineseMessageInSmsVoiceCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendEnglishDataMessageInSmsVoiceCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendChineseDataMessageInSmsVoiceCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendLongEnglishDataMessageInSmsVoiceCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendLongChineseDataMessageInSmsVoiceCap, setup_ims, teardown_ims),
        cmocka_unit_test(TestTeleAbn_SmsSendMessageFailInAirplane),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendMessageAfterDataOnOff, setup_sms, teardown_sms),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsSendMessageAfterRadioOnOff, setup_sms, teardown_sms),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveMessage, setup_sms, teardown_sms),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveMessageAfterDataOnOff, setup_sms, teardown_sms),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveMessageAfterRadioOnOff, setup_sms, teardown_sms),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveMessageInActive, setup_sms_and_call, teardown_sms_and_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveMessageInDialing, setup_sms_and_call, teardown_sms_and_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveEnglishLongMessage, setup_sms, teardown_sms),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveEnglishLongMessageAfterRadioOnOff, setup_sms, teardown_sms),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveEnglishLongMessageInActive, setup_sms_and_call, teardown_sms_and_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveEnglishLongMessageInDialing, setup_sms_and_call, teardown_sms_and_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveChineseLongMessage, setup_sms, teardown_sms),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveChineseLongMessageAfterRadioOnOff, setup_sms, teardown_sms),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveChineseLongMessageInActive, setup_sms_and_call, teardown_sms_and_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveChineseLongMessageInDialing, setup_sms_and_call, teardown_sms_and_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveMessageInVoiceImsCap, setup_sms, teardown_sms),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveEnglishLongMessageInVoiceImsCap, setup_sms, teardown_sms),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveChineseLongMessageInVoiceImsCap, setup_sms, teardown_sms),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveMessageInSmsImsCap, setup_sms, teardown_sms),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveEnglishLongMessageInSmsImsCap, setup_sms, teardown_sms),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveChineseLongMessageInSmsImsCap, setup_sms, teardown_sms),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveReportAfterSendShortMessage, setup_sms, teardown_sms),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveReportAfterSendChineseLongMessage, setup_sms, teardown_sms),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SmsReceiveReportAfterSendEnglishLongMessage, setup_sms, teardown_sms),
        cmocka_unit_test(TestTeleFunc_SmsSetAndGetDefaultSlot),
        cmocka_unit_test(TestTeleFunc_SmsSetAndGetCellBroadcastPower),
        cmocka_unit_test(TestTeleFunc_SmsSetAndGetCellBroadcastTopics),
        cmocka_unit_test(TestTeleStab_SmsSendMessageContinuous),
        cmocka_unit_test(TestTeleStab_SmsSendDataMessageContinuous),
        cmocka_unit_test(TestTeleStab_SmsSendDataMessageAndMessageContinuous),
    };

    const struct CMUnitTest NetTestSuites[] = {
        cmocka_unit_test(TestTeleFunc_NetSelectManualAndRestoreAuto),
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
        cmocka_unit_test(TestTeleFunc_CI_ImsListenAndUnlisten),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_ImsGetRegistration, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_ImsSetVoiceCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_ImsResetImsCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_ImsSetSmsVoiceCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_ImsSetSmsCap, setup_ims, teardown_ims),
        cmocka_unit_test_setup_teardown(TestTeleFunc_ImsKeepRegOnAfterRadioOffOn, setup_ims, teardown_imsAndRadio),
        cmocka_unit_test_setup_teardown(TestTeleFunc_ImsKeepVolteAvailAfterRadioOffOn, setup_ims, teardown_imsAndRadio),
        cmocka_unit_test_setup_teardown(TestTeleAbn_ImsVolteAvailAfterModemOff, setup_ims, teardown_imsAndModem),
        cmocka_unit_test_setup_teardown(TestTeleAbn_ImsVolteAvailAfterRadioOff, setup_ims, teardown_imsAndRadio),
        cmocka_unit_test_setup_teardown(TestTeleAbn_ImsVolteAvailWithSimAbsent, setup_imsAndSim, teardown_imsAndSim),
        cmocka_unit_test_setup_teardown(TestTeleAbn_ImsVolteAvailWhenCallDialing, setup_imsAndCall, teardown_imsAndCall),
        cmocka_unit_test_setup_teardown(TestTeleAbn_ImsVolteAvailWhenCallActive, setup_imsAndCall, teardown_imsAndCall),
    };

    const struct CMUnitTest SSTestSuits[] = {
        cmocka_unit_test(TestTeleFunc_CI_SSResigterAndUnRegister),
        cmocka_unit_test(TestTeleFunc_SSRequestCallBarring),
        cmocka_unit_test(TestTeleFunc_SSSetAndGetCallBarring),
        cmocka_unit_test(TestTeleFunc_SSChangeAndResetCallBarringPassword),
        cmocka_unit_test(TestTeleFunc_SSDisableAllIncoming),
        cmocka_unit_test(TestTeleFunc_SSDisableAllOutgoing),
        cmocka_unit_test(TestTeleFunc_SSDisableAllCallBarrings),
        cmocka_unit_test(TestTeleFunc_CI_SSSetAndGetCallForwardingUnConditional),
        cmocka_unit_test(TestTeleFunc_CI_SSSetAndGetCallForwardingBusy),
        cmocka_unit_test(TestTeleFunc_CI_SSSetAndGetCallForwardingNoReply),
        cmocka_unit_test(TestTeleFunc_CI_SSSetAndGetCallForwardingNotReachable),
        cmocka_unit_test(TestTeleFunc_CI_SSEnableAndDisableCallWaiting),
        cmocka_unit_test(TestTeleFunc_SSEnableAndDisableFdn),
        cmocka_unit_test(TestTeleStab_SSCallForwardingContinuous),
        cmocka_unit_test(TestTeleStab_SSCallWaitingContinuous),
        cmocka_unit_test(TestTeleAbn_SSListenAbnormalIdFail),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SSCallForwardingAfterRadioOffOn, setup_ssAndRadio, teardown_ssAndRadio),
        cmocka_unit_test_setup_teardown(TestTeleFunc_SSCallWaitingAfterRadioOffOn, setup_ssAndRadio, teardown_ssAndRadio),
    };

    const struct CMUnitTest CommonTestSuites[] = {
        cmocka_unit_test(TestTeleFunc_CI_ModemGetImei),
        cmocka_unit_test(TestTeleFunc_ModemSetUmtsPrefNetMode),
        cmocka_unit_test(TestTeleFunc_ModemSetGsmOnlyPrefNetMode),
        cmocka_unit_test(TestTeleFunc_ModemSetWcdmaOnlyPrefNetMode),
        cmocka_unit_test(TestTeleFunc_ModemSetLteOnlyPrefNetMode),
        cmocka_unit_test(TestTeleFunc_ModemSetLteWcdmaPrefNetMode),
        cmocka_unit_test(TestTeleFunc_ModemSetLteGsmWcdmaPrefNetMode),
        cmocka_unit_test(TestTeleFunc_CI_ModemGetPrefNetMode),
        cmocka_unit_test(TestTeleFunc_CI_ModemRegisterOrUnregister),
        cmocka_unit_test(TestTeleFunc_ModemInvokeOemShotRilRequestRaw),
        cmocka_unit_test(TestTeleFunc_ModemInvokeOemLongRilRequestRaw),
        cmocka_unit_test(TestTeleFunc_ModemInvokeOemNormalRilRequestRaw),
        cmocka_unit_test(TestTeleFunc_ModemInvokeOemSeperateRilRequestRaw),
        cmocka_unit_test(TestTeleFunc_ModemInvokeOemRilRequestATCmdStrings),
        cmocka_unit_test(TestTeleFunc_ModemInvokeOemRilRequestNotATCmdStrings),
        cmocka_unit_test(TestTeleFunc_ModemInvokeOemRilRequestHexStrings),
        cmocka_unit_test(TestTeleFunc_CI_ModemGetRevision),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_ModemEnable, setup_modem, teardown_modem),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_ModemDisable, setup_modem, teardown_modem),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_ModemEnableDisableNTimes, setup_modem, teardown_modem),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_ModemSetRadioPowerOff, setup_modem, teardown_modem),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_ModemSetRadioPowerOnOffNTimes, setup_modem, teardown_modem),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_ModemSetRadioPowerOn, setup_modem, teardown_modem),
        cmocka_unit_test_setup_teardown(TestTeleStab_ModemEnableAndDisableContinuous, setup_modem, teardown_modem),
        cmocka_unit_test_setup_teardown(TestTeleStab_ModemSetRadioPowerOnAndOffContinuous, setup_modem, teardown_modem),
        cmocka_unit_test_setup_teardown(TestTeleFunc_ModemSetRadioPowerOnAndModemDisable, setup_modem, teardown_modem),
        cmocka_unit_test_setup_teardown(TestTeleFunc_ModemSetRadioPowerOffOrModemDisableAfterotherAction, setup_modem, teardown_modem),
        cmocka_unit_test(TestTeleFunc_CI_ModemDefaultOpenTapi),
        cmocka_unit_test_setup_teardown(TestTeleFunc_CI_ModemBtTeleOpenTapi, NULL, TearDown_OpenDefaultTapi),
        cmocka_unit_test_setup_teardown(TestTeleFunc_ModemCloseTapi, NULL, TearDown_OpenDefaultTapi),
        cmocka_unit_test_setup_teardown(TestTeleStab_ModemEnableDisableNTimesUnderRadioPowerOff, setup_modem, teardown_modem),
        cmocka_unit_test_setup_teardown(TestTeleStab_ModemEnableDisableNTimesUnderDialingCall, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleStab_ModemEnableDisableNTimesUnderOngoingCall, setup_call, teardown_call),
        cmocka_unit_test(TestTeleFunc_CI_ModemVerifyPrefNetMode),
        cmocka_unit_test_setup_teardown(TestTeleFunc_ModemGetDefaultPhoneState, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_ModemGetPhoneState, setup_call, teardown_call),
        cmocka_unit_test(TestTeleFunc_ModemGetPhoneStateUnderMOCall),
        cmocka_unit_test(TestTeleFunc_ModemGetPhoneStateUnderMTCall),
        cmocka_unit_test(TestTeleFunc_ModemGetPhoneStateUnderMTRejectCall),
        cmocka_unit_test(TestTeleFunc_ModemGetPhoneStateUnderDialingCallSms),
        cmocka_unit_test_setup_teardown(TestTeleFunc_ModemSetRadioOffUnderDialingCall, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_ModemSetRadioOffUnderOngoingCall, setup_call, teardown_call),
        cmocka_unit_test_setup_teardown(TestTeleFunc_ModemDisableEnableRadioPowerOff, setup_modem, teardown_modem),
        cmocka_unit_test(TestTeleFunc_ModemDisableRadioPowerOff),
        cmocka_unit_test(TestTeleFunc_ModemSetSignalReportThreshold),
        cmocka_unit_test(TestTeleFunc_ModemSuppressMsgReport),
        cmocka_unit_test(TestTeleFunc_ModemEnableModemStationary),
        cmocka_unit_test(TestTeleFunc_ModemSetModemStationaryThreshold),
#ifndef CONFIG_TELEPHONY_DFX
        cmocka_unit_test(TestTeleFunc_ModemAbnormalEventReport),
        cmocka_unit_test_setup_teardown(TestTeleFunc_ModemEnableDataFailDFX, setup_data, teardown_data),
        cmocka_unit_test(TestTeleFunc_ModemOOSDFX),
        cmocka_unit_test_setup_teardown(TestTeleFunc_ModemDisableDFX, setup_modem, NULL),
#endif
        cmocka_unit_test(TestTeleFunc_ModemUpgradeIndication),
        cmocka_unit_test(TestTeleFunc_ModemUpgradeStateCheck),
        cmocka_unit_test(TestTeleFunc_ModemUpgradeCMD),
    };
#ifdef CONFIG_PHONE_SERVICE
    const struct CMUnitTest PhoneServiceTestSuites[] = {
#ifdef CONFIG_PHONE_SERVICE_WTP
        cmocka_unit_test(TestTeleFunc_PhoneServiceRegisterWTPCB),
        cmocka_unit_test(TestTeleFunc_PhoneServiceUnRegisterWTPCB),
        cmocka_unit_test(TestTeleFunc_PhoneServiceUpdateLocalInfo),
        cmocka_unit_test(TestTeleFunc_PhoneServiceSetDiscovery),
        cmocka_unit_test(TestTeleFunc_PhoneServiceSetVisibility),
        cmocka_unit_test(TestTeleFunc_PhoneServiceSetAudio),
        cmocka_unit_test(TestTeleFunc_PhoneServiceDialWTP),
        cmocka_unit_test(TestTeleFunc_PhoneServiceHangupWTP),
        cmocka_unit_test(TestTeleFunc_PhoneServiceAnswerWTP),
        cmocka_unit_test(TestTeleFunc_PhoneServiceRejectWTP),
#endif
        cmocka_unit_test(TestTeleFunc_PhoneServiceSetRadioPowerOffAndOn),
        cmocka_unit_test(TestTeleFunc_PhoneServiceRegisterUnRegisterESIMCB),
        cmocka_unit_test(TestTeleFunc_PhoneServiceDialAndHangupESIM),
        cmocka_unit_test(TestTeleFunc_PhoneServiceIncomingAnswerAndHangupESIM),
        cmocka_unit_test(TestTeleFunc_PhoneServiceIncomingAndRejectESIM),
        cmocka_unit_test(TestTeleFunc_PhoneServiceReleaseAndAnswerESIM),
        cmocka_unit_test(TestTeleFunc_PhoneServiceHoldAndUnholdESIM),
        cmocka_unit_test(TestTeleFunc_PhoneServiceMergeESIM),
        cmocka_unit_test(TestTeleFunc_PhoneServiceSendTonesESIM),
    };
#endif
    sleep(5);

    cmocka_run_group_tests(SimTestSuites, NULL, NULL);

    cmocka_run_group_tests(CallTestSuites, NULL, NULL);

    cmocka_run_group_tests(DataTestSuites, NULL, NULL);

    cmocka_run_group_tests(SmsTestSuites, NULL, NULL);

    cmocka_run_group_tests(NetTestSuites, NULL, NULL);

    cmocka_run_group_tests(ImsTestSuits, NULL, NULL);

    cmocka_run_group_tests(SSTestSuits, NULL, NULL);

    cmocka_run_group_tests(CommonTestSuites, NULL, NULL);
#ifdef CONFIG_PHONE_SERVICE
    cmocka_run_group_tests(PhoneServiceTestSuites, NULL, NULL);
#endif
do_exit:
    tapi_enable_modem(get_tapi_ctx(), 0, 0, 0, NULL); // disable modem anyway
    uv_async_send(&g_uv_exit);

    pthread_join(thread, NULL);
    uv_close((uv_handle_t*)&g_uv_exit, NULL);

#ifdef CONFIG_PHONE_SERVICE
    uv_close((uv_handle_t*)&g_uv_message.async, NULL);
#endif
    return 0;
}
