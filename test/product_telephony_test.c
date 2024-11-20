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
#include "telephony_test.h"

#define REPEAT_TEST_MORE_FOR for (int _i = 0; _i < 10; _i++)
#define REPEAT_TEST_LESS_FOR for (int _i = 0; _i < 3; _i++)

char* phone_num = NULL;
static uv_async_t g_uv_exit;
static int ready_done;
tapi_context context = NULL;
typedef enum {
    CASE_NORMAL_MODE = 0,
    CASE_AIRPLANE_MODE = 1,
    CASE_CALL_DIALING = 2,
    CASE_MODEM_POWEROFF = 3,
} case_type;

int modem_status = -1;

struct judge_type judge_data;

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

static void TestNuttxDataEnable(void** state)
{
    (void)state;
    int ret = data_enabled_test(0);
    assert_int_equal(ret, OK);
    sleep(20);
}

static void TestNuttxDataDisable(void** state)
{
    (void)state;
    int ret = data_disabled_test(0);
    assert_int_equal(ret, OK);
    sleep(20);
}

static void TestNuttxDataIsEnable(void** state)
{
    (void)state;
    bool enable = false;
    sleep(5);
    int ret = tapi_data_get_enabled_test(&enable);
    assert_int_equal(ret, OK);
    assert_int_equal(enable, 1);
}

static void TestNuttxDataIsDisable(void** state)
{
    (void)state;
    bool enable = true;
    sleep(5);
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

// modem
static void TestNuttxModemGetImei(void** state)
{
    (void)state;
    int ret = tapi_get_imei_test(0);
    assert_int_equal(ret, OK);
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
    sleep(60);
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
    REPEAT_TEST_LESS_FOR
    {
        TestNuttxModemEnable(state);
        sleep(60);
        TestNuttxGetModemEnableStatus(state);
        TestNuttxModemDisable(state);
        sleep(60);
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

static void modem_status_cb(tapi_async_result* result)
{
    syslog(LOG_DEBUG, "%s : \n", __func__);
    syslog(LOG_DEBUG, "result->msg_id : %d\n", result->msg_id);
    syslog(LOG_DEBUG, "result->status : %d\n", result->status);
    syslog(LOG_DEBUG, "result->arg1 : %d\n", result->arg1);
    syslog(LOG_DEBUG, "result->arg2 : %d\n", result->arg2);

    if (result->status != OK) {
        syslog(LOG_DEBUG, "%s msg id : %d result err, return.\n", __func__, result->msg_id);
        assert(0);
        return;
    }

    if (result->msg_id == EVENT_MODEM_STATUS_QUERY_DONE) {
        modem_status = result->arg2;
    }
}

static void on_tapi_client_ready(const char* client_name, void* user_data)
{
    if (client_name != NULL)
        syslog(LOG_DEBUG, "tapi is ready for %s\n", client_name);

    ready_done = 1;

    /**
     * both args is NULL, that's tapi received disconnect message
     * so we here stop the default loop
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

static void TestNuttxNetGetOperatorName(void** state)
{
    (void)state;
    int ret = tapi_net_get_operator_name_test(0);
    assert_int_equal(ret, OK);
}

static void TestNuttxDataIsPsAttached(void** state)
{
    (void)state;
    int ret = tapi_data_is_ps_attached_test(0);
    assert_int_equal(ret, OK);
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

static void TestNuttxDialCall(void** state)
{
    sleep(30);
    (void)state;
    int ret = tapi_call_dial_test(0, "10086", 0);
    assert_int_equal(ret, 0);
    sleep(30);
}

static void TestNuttxHangupCall(void** state)
{
    (void)state;
    int ret = tapi_call_hanup_current_call_test(0);
    assert_int_equal(ret, 0);
    sleep(30);
}

static void* run_test_loop(void* args)
{
    context = tapi_open("vela.telephony.test", on_tapi_client_ready, NULL);
    if (context == NULL) {
        return 0;
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

    const struct CMUnitTest StabilityTestSuites[] = {
        cmocka_unit_test(TestNuttxListenCall),
        cmocka_unit_test(TestNuttxModemRegister),
        cmocka_unit_test(TestNuttxModemUnregister),
        cmocka_unit_test(TestNuttxDataRegister),
        cmocka_unit_test(TestNuttxDataUnregister),
        cmocka_unit_test(TestNuttxDataRegister),
        cmocka_unit_test(TestNuttxModemEnable),
        cmocka_unit_test(TestNuttxGetModemEnableStatus),
        cmocka_unit_test(TestNuttxHasIccCard),
        cmocka_unit_test(TestNuttxHasIccCardNumerousTimes),
        cmocka_unit_test(TestNuttxDataIsPsAttached),
        cmocka_unit_test(TestNuttxNetGetOperatorName),
        cmocka_unit_test(TestNuttxModemGetImei),
        cmocka_unit_test(TestNuttxGetModemRevision),
        cmocka_unit_test(TestNuttxDialCall),
        cmocka_unit_test(TestNuttxHangupCall),
        cmocka_unit_test(TestNuttxDataEnable),
        cmocka_unit_test(TestNuttxDataIsEnable),
        cmocka_unit_test(TestNuttxDataDisable),
        cmocka_unit_test(TestNuttxDataIsDisable),
        cmocka_unit_test(TestNuttxDataUnregister),
        cmocka_unit_test(TestNuttxModemDisable),
        cmocka_unit_test(TestNuttxGetModemDsiableStatus),
        cmocka_unit_test(TestNuttxModemEnableDisableRepeatedly),
        cmocka_unit_test(TestNuttxUnlistenCall),
    };

    sleep(120);
    tapi_get_modem_status(get_tapi_ctx(), 0,
        EVENT_MODEM_STATUS_QUERY_DONE, modem_status_cb);
    sleep(30);
    if (modem_status == -1) {
        assert(0);
    } else if (modem_status == 1) {
        tapi_data_enable_data(get_tapi_ctx(), false);
        sleep(30);
        tapi_enable_modem(get_tapi_ctx(), 0,
            EVENT_MODEM_ENABLE_DONE, false, NULL);
        sleep(60);
    }

    cmocka_run_group_tests(StabilityTestSuites, NULL, NULL);

    uv_async_send(&g_uv_exit);

    pthread_join(thread, NULL);
    uv_close((uv_handle_t*)&g_uv_exit, NULL);

    return 0;
}
