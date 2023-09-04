// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "service.h"

#include "manager.h"
#include "servicebase.h"
#include "servicesignal.h"

#include <common/common.h>
#include <common/log.h>
#include <fido.h>
#include <gio/gio.h>
#include <glib.h>

#include <stdio.h>

static int dpk_service_get_pin_status(MethodContext *mc)
{
    int status = 0;
    int callRet = FIDO_ERR_INTERNAL;
    LOG(LOG_INFO, "service-called: GetPinStatus");

    if (mc == NULL) {
        LOG(LOG_ERR, "method context invalid");
        goto end;
    }

    if ((callRet = dpk_manager_get_pin_status(&status)) != FIDO_OK) {
        goto end;
    }
    int support = 0;
    int hasPin = 0;
    if (status & 0x10) {
        support = 1;
    }
    if (status & 0x1) {
        hasPin = 1;
    }

    g_dbus_method_invocation_return_value(mc->invocation, g_variant_new("(ii)", support, hasPin));

    callRet = FIDO_OK;
end:
    if (callRet != FIDO_OK) {
        gchar *retStr = g_strdup_printf("%d", callRet);
        g_dbus_method_invocation_return_dbus_error(mc->invocation, "com.deepin.Passkey.Error", retStr);
        g_free(retStr);
    }
    return 0;
}

static int dpk_service_set_pin(MethodContext *mc)
{
    int callRet = FIDO_ERR_INTERNAL;
    const gchar *pin = NULL;
    const gchar *oldPin = NULL;

    if (mc == NULL || mc->parameters == NULL) {
        LOG(LOG_ERR, "method context invalid");
        goto end;
    }

    g_variant_get(mc->parameters, "(&ss)", &oldPin, &pin);

    LOG(LOG_INFO, "service-called: SetPin");

    // LOG(LOG_INFO, "change pin %s, oldpin %s", pin, oldPin);
    if (strlen(oldPin) == 0) {
        callRet = dpk_manager_set_pin(pin, NULL);
    } else {
        callRet = dpk_manager_set_pin(pin, oldPin);
    }
    if (callRet != FIDO_OK) {
        goto end;
    }
    g_dbus_method_invocation_return_value(mc->invocation, g_variant_new("()"));
    callRet = FIDO_OK;
end:
    if (callRet != FIDO_OK) {
        gchar *retStr = g_strdup_printf("%d", callRet);
        g_dbus_method_invocation_return_dbus_error(mc->invocation, "com.deepin.Passkey.Error", retStr);
        g_free(retStr);
    }
    return 0;
}

static int dpk_service_reset(MethodContext *mc)
{
    int callRet = FIDO_ERR_INTERNAL;
    LOG(LOG_INFO, "service-called: Reset");

    if (mc == NULL) {
        LOG(LOG_ERR, "method context invalid");
        goto end;
    }

    callRet = dpk_manager_reset();
    if (callRet != FIDO_OK) {
        goto end;
    }

    callRet = FIDO_OK;

end:
    emit_reset_status(SIGNAL_FINISH, callRet);
    return 0;
}

static int dpk_service_make_cred(MethodContext *mc)
{
    int callRet = FIDO_ERR_INTERNAL;
    const gchar *userName = NULL;
    const gchar *credName = NULL;
    const gchar *pin = NULL;

    LOG(LOG_INFO, "service-called: MakeCredential");

    if (mc == NULL || mc->parameters == NULL) {
        LOG(LOG_ERR, "method context invalid");
        goto end;
    }

    g_variant_get(mc->parameters, "(&sss)", &userName, &credName, &pin);
    if (userName == NULL || credName == NULL || pin == NULL) {
        LOG(LOG_INFO, "service make cred param error");
        goto end;
    }

    LOG(LOG_INFO, "service make cred by %s:%s", userName, credName);

    if ((callRet = dpk_manager_make_cred(userName, credName, pin)) != FIDO_OK) {
        goto end;
    }

    callRet = FIDO_OK;
end:
    emit_make_cred_status(userName, SIGNAL_FINISH, callRet);
    return 0;
}

static int dpk_service_get_assert(MethodContext *mc)
{
    int callRet = FIDO_ERR_INTERNAL;
    const gchar *userName = NULL;
    const gchar *credName = NULL;
    const gchar *pin = NULL;

    LOG(LOG_INFO, "service-called: GetAssertion");

    if (mc == NULL || mc->parameters == NULL) {
        LOG(LOG_ERR, "method context invalid");
        goto end;
    }

    g_variant_get(mc->parameters, "(&sss)", &userName, &credName, &pin);
    if (userName == NULL || credName == NULL || pin == NULL) {
        LOG(LOG_INFO, "service get assert param error");
        goto end;
    }

    LOG(LOG_INFO, "service get assert by %s:%s", userName, credName);
    if ((callRet = dpk_manager_get_assertion(userName, credName, pin)) != FIDO_OK) {
        goto end;
    }

    callRet = FIDO_OK;
end:
    emit_get_assert_status(userName, SIGNAL_FINISH, callRet);
    return 0;
}

static int dpk_service_get_valid_cred_count(MethodContext *mc)
{
    int callRet = FIDO_ERR_INTERNAL;
    const gchar *userName = NULL;

    LOG(LOG_INFO, "service-called: GetValidCredCount");

    if (mc == NULL || mc->parameters == NULL) {
        LOG(LOG_ERR, "method context invalid");
        goto end;
    }

    g_variant_get(mc->parameters, "(s)", &userName);

    unsigned int validCredCount = 0;
    if ((callRet = dpk_manager_get_valid_cred_count(userName, NULL, &validCredCount)) != FIDO_OK) {
        goto end;
    }

    g_dbus_method_invocation_return_value(mc->invocation, g_variant_new("(i)", validCredCount));
    callRet = FIDO_OK;
end:
    if (callRet != FIDO_OK) {
        gchar *retStr = g_strdup_printf("%d", callRet);
        g_dbus_method_invocation_return_dbus_error(mc->invocation, "com.deepin.Passkey.Error", retStr);
        g_free(retStr);
    }
    return 0;
}

static int dpk_service_get_device_count(MethodContext *mc)
{
    int callRet = FIDO_ERR_INTERNAL;
    int count = 0;

    LOG(LOG_INFO, "service-called: GetDeviceCount");

    if (mc == NULL) {
        LOG(LOG_ERR, "method context invalid");
        goto end;
    }

    if ((callRet = dpk_manager_get_device_count(&count)) != FIDO_OK) {
        goto end;
    }

    g_dbus_method_invocation_return_value(mc->invocation, g_variant_new("(i)", count));
    callRet = FIDO_OK;
end:
    if (callRet != FIDO_OK) {
        gchar *retStr = g_strdup_printf("%d", callRet);
        g_dbus_method_invocation_return_dbus_error(mc->invocation, "com.deepin.Passkey.Error", retStr);
        g_free(retStr);
    }
    return 0;
}

static int dpk_service_device_detect(MethodContext *mc)
{
    gint timeout = 0;
    gint stopWhenExist = 0;
    gint stopWhenNotExist = 0;
    int callRet = FIDO_ERR_INTERNAL;

    LOG(LOG_INFO, "service-called: DeviceDetect");

    if (mc == NULL || mc->parameters == NULL) {
        LOG(LOG_ERR, "method context invalid");
        goto end;
    }

    g_variant_get(mc->parameters, "(iii)", &timeout, &stopWhenExist, &stopWhenNotExist);

    if ((callRet = dpk_manager_device_detect(timeout, stopWhenExist, stopWhenNotExist)) != FIDO_OK) {
        goto end;
    }
    callRet = FIDO_OK;
end:
    emit_device_detect_status(SIGNAL_FINISH, callRet);
    return 0;
}

void dpk_service_start()
{
    Service srv;

    service_init(&srv);

    service_register_method(&srv, "GetPinStatus", dpk_service_get_pin_status, false);
    service_register_method(&srv, "SetPin", dpk_service_set_pin, false);
    service_register_method(&srv, "Reset", dpk_service_reset, true);
    service_register_method(&srv, "MakeCredential", dpk_service_make_cred, true);
    service_register_method(&srv, "GetAssertion", dpk_service_get_assert, true);
    service_register_method(&srv, "GetValidCredCount", dpk_service_get_valid_cred_count, false);
    service_register_method(&srv, "GetDeviceCount", dpk_service_get_device_count, false);
    service_register_method(&srv, "DeviceDetect", dpk_service_device_detect, true);

    service_run(&srv); // in loop, and end when exit

    service_unref(&srv);

    return;
}
