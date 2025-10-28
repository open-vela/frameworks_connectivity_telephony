#include "telephony_ims_test.h"
#include "telephony_call_test.h"
#include "telephony_common_test.h"
#include "telephony_sim_test.h"

static void tele_ims_async_fun(tapi_async_result* result);
extern struct judge_type judge_data;

static struct
{
    bool ims_default_enabled;
    int ims_enabled;
    int ims_reg_watch_id;
} global_data;

int ims_listen_ims_test(int slot_id)
{
    global_data.ims_reg_watch_id = -1;
    global_data.ims_reg_watch_id = tapi_ims_register_registration_change(get_tapi_ctx(),
        slot_id, NULL, tele_ims_async_fun);

    if (global_data.ims_reg_watch_id < 0) {
        syslog(LOG_ERR, "%s, slot_id: %d, watch_id < 0\n",
            __func__, slot_id);
        return -1;
    }

    return 0;
}

int ims_unlisten_ims_test(void)
{
    int ret = tapi_unregister(get_tapi_ctx(), global_data.ims_reg_watch_id);
    syslog(LOG_INFO, "unregister ims state in %s, ret: %d", __func__, ret);
    return ret;
}

int setup_ims(void** state)
{
    (void)state;
    int ret = 0;
    global_data.ims_default_enabled = false;

    if (tapi_ims_get_enabled(get_tapi_ctx(), 0, &global_data.ims_default_enabled)) {
        syslog(LOG_ERR, "Get ims state execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (!global_data.ims_default_enabled) {
        syslog(LOG_ERR, "Get ims state is false in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (ims_get_registration_test(0, 1)) {
        syslog(LOG_ERR, "Get ims reg state execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (ims_listen_ims_test(0)) {
        syslog(LOG_ERR, "Listen ims state execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    syslog(LOG_ERR, "Get ims state %d in %s", global_data.ims_default_enabled, __func__);
    return ret;
}

int teardown_ims(void** state)
{
    (void)state;
    int ret = 0;

    // reset ims cap（5-voice&sms）
    if (ims_set_service_status_test(0, 5)) {
        syslog(LOG_ERR, "Set ims state execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (ims_unlisten_ims_test()) {
        syslog(LOG_ERR, "Unlisten ims state execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    return ret;
}

int teardown_imsAndRadio(void** state)
{
    bool radio = false;
    int ret = 0;

    /** If not teardown ims before teardown radio,radio power on will trigger ims reg notifyed,
     * that make teardown enable ims can not receive ims reg notify, because ims already notifyed
     */
    ret = teardown_ims(state);
    if (ret) {
        syslog(LOG_ERR, "teardown_ims execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    ret = get_radio_power_test(0, &radio);
    if (ret) {
        syslog(LOG_ERR, "get_radio_power_test execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (!radio) {
        syslog(LOG_INFO, "radio is off, change to on in %s", __func__);
        ret = set_radio_power_test(0, true);
        if (ret) {
            syslog(LOG_ERR, "set_radio_power_test execute fail in %s", __func__);
            ret = -1;
            goto on_exit;
        }
        sleep(3); // wait for radio on 3s
    }

on_exit:
    return ret;
}

int teardown_imsAndModem(void** state)
{
    bool radio = false;
    int ret = 0;

    ret = modem_keep_status_as_expected_test(0, true);
    if (ret) {
        syslog(LOG_ERR, "keep modem status on execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    /** If not teardown ims before teardown radio, radio power on will trigger ims reg notifyed,
     * that make teardown enable ims can not receive ims reg notify, because ims already notifyed
     */
    ret = teardown_ims(state);
    if (ret) {
        syslog(LOG_ERR, "teardown_ims execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    ret = get_radio_power_test(0, &radio);
    if (ret) {
        syslog(LOG_ERR, "get_radio_power_test execute fail in %s", __func__);
        ret = -1;
        goto on_exit;
    }

    if (!radio) {
        syslog(LOG_INFO, "radio is off, change to on in %s", __func__);
        ret = set_radio_power_test(0, true);
        if (ret) {
            syslog(LOG_ERR, "set_radio_power_test execute fail in %s", __func__);
            ret = -1;
            goto on_exit;
        }
        sleep(5);
    }

on_exit:
    return ret;
}

int setup_imsAndSim(void** state)
{
    int ret = 0;

    ret = setup_sim(state);
    if (ret) {
        syslog(LOG_ERR, "setup_sim execute fail in %s", __func__);
        goto on_exit;
    }

    ret = setup_ims(state);
    if (ret) {
        syslog(LOG_ERR, "setup_ims execute fail in %s", __func__);
        goto on_exit;
    }

on_exit:
    return ret;
}

int teardown_imsAndSim(void** state)
{
    int ret = 0;

    ret = teardown_ims(state);
    if (ret) {
        syslog(LOG_ERR, "teardown_ims execute fail in %s", __func__);
        goto on_exit;
    }

    ret = teardown_sim(state);
    if (ret) {
        syslog(LOG_ERR, "teardown_sim execute fail in %s", __func__);
        goto on_exit;
    }

on_exit:
    return ret;
}

int setup_imsAndCall(void** state)
{
    int ret = 0;

    ret = setup_call(state);
    if (ret) {
        syslog(LOG_ERR, "setup_call execute fail in %s", __func__);
        goto on_exit;
    }

    ret = setup_ims(state);
    if (ret) {
        syslog(LOG_ERR, "setup_ims execute fail in %s", __func__);
        goto on_exit;
    }

on_exit:
    return ret;
}

int teardown_imsAndCall(void** state)
{
    int ret = 0;

    ret = teardown_call(state);
    if (ret) {
        syslog(LOG_ERR, "teardown_call execute fail in %s", __func__);
        goto on_exit;
    }

    ret = teardown_ims(state);
    if (ret) {
        syslog(LOG_ERR, "teardown_sim execute fail in %s", __func__);
        goto on_exit;
    }

on_exit:
    return ret;
}

int ims_get_registration_test(int slot_id, int expect)
{
    tapi_ims_registration_info info;
    memset(&info, 0, sizeof(info));
    int ret = tapi_ims_get_registration(get_tapi_ctx(), slot_id, &info);
    syslog(LOG_ERR, "%s, slot_id: %d, reg_info: %d", __func__, slot_id, info.reg_info);

    return ret || (expect != info.reg_info);
}

int ims_set_service_status_test(int slot_id, int status)
{
    int ret = tapi_ims_set_service_status(get_tapi_ctx(), slot_id, status);
    if (ret) {
        syslog(LOG_ERR, "tapi_ims_set_service_status execute fail in %s, ret: %d",
            __func__, ret);
        goto on_exit;
    }

on_exit:
    return ret;
}

static void tele_ims_async_fun(tapi_async_result* result)
{
    syslog(LOG_DEBUG, "%s : \n", __func__);
    syslog(LOG_DEBUG, "result->msg_id : %d\n", result->msg_id);
    syslog(LOG_DEBUG, "result->status : %d\n", result->status);
    syslog(LOG_DEBUG, "result->arg1 : %d\n", result->arg1);
    syslog(LOG_DEBUG, "result->arg2 : %d\n", result->arg2);

    switch (result->arg1) {
    case IMS_REG:
        if (judge_data.expect == IMS_REG) {
            syslog(LOG_DEBUG, "IMS_REG change : %d", result->arg2);
            judge_data.result = 0;
            global_data.ims_enabled = result->arg2;
            judge_data.flag = IMS_REG;
        }
        break;
    default:
        break;
    }
}

int ims_get_enabled_test(int slot_id, bool expect)
{
    bool result = false;
    int ret = tapi_ims_get_enabled(get_tapi_ctx(), slot_id, &result);
    syslog(LOG_DEBUG, "%s, slot_id: %d, ims_enable: %d", __func__, slot_id, result);

    return ret || (result != expect);
}

int ims_is_reg_as_expect_test(int slot_id, bool expect_reg_status)
{
    int ret = 0;
    bool reg_status = false;

    ret = tapi_ims_is_registered(get_tapi_ctx(), 0, &reg_status);
    if (ret) {
        syslog(LOG_ERR, "ims is registred execute fail in %s", __func__);
        goto on_exit;
    }

    if (reg_status != expect_reg_status) {
        syslog(LOG_ERR, "ims reg status(%d) not expect status(%d) in %s", reg_status,
            expect_reg_status, __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    return ret;
}

int ims_is_volte_available_as_expect_test(int slot_id, bool expect_volte_avail)
{
    int ret = 0;
    bool volte_avail = false;

    ret = tapi_ims_is_volte_available(get_tapi_ctx(), 0, &volte_avail);
    if (ret) {
        syslog(LOG_ERR, "ims is volte available execute fail in %s", __func__);
        goto on_exit;
    }
    if (volte_avail != expect_volte_avail) {
        syslog(LOG_ERR, "ims volte available(%d) not expect(%d) in %s", volte_avail,
            expect_volte_avail, __func__);
        ret = -1;
        goto on_exit;
    }

on_exit:
    return ret;
}

int ims_is_reg_after_radio_off_on_test(int slot_id, bool expect_reg_status)
{
    int ret = 0;

    ret = ims_is_reg_as_expect_test(0, expect_reg_status);
    if (ret) {
        syslog(LOG_ERR, "ims is registred not as expect(%d) fail in %s", expect_reg_status, __func__);
        goto on_exit;
    }

    ret = set_radio_power_off_then_on(0);
    if (ret) {
        syslog(LOG_ERR, "Turn off then on radio execute fail in %s", __func__);
        goto on_exit;
    }

    ret = ims_is_reg_as_expect_test(0, expect_reg_status);
    if (ret) {
        syslog(LOG_ERR, "ims is registred not as expect(%d) fail in %s", expect_reg_status, __func__);
        goto on_exit;
    }

on_exit:
    return ret;
}

int ims_is_volte_available_after_radio_off_on_test(int slot_id, bool expect_volte_avail)
{
    int ret = 0;

    ret = ims_is_volte_available_as_expect_test(0, expect_volte_avail);
    if (ret) {
        syslog(LOG_ERR, "ims is volte available as expect(%d) fail in %s", expect_volte_avail, __func__);
        goto on_exit;
    }

    ret = set_radio_power_off_then_on(0);

    ret = ims_is_volte_available_as_expect_test(0, expect_volte_avail);
    if (ret) {
        syslog(LOG_ERR, "ims is volte available as expect(%d) fail in %s", expect_volte_avail, __func__);
        goto on_exit;
    }

on_exit:
    return ret;
}
