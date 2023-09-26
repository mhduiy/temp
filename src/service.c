// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "service.h"

#include "errcode.h"
#include "manager.h"
#include "serviceframe/servicebase.h"
#include "servicesignal.h"

#include <common/common.h>
#include <common/log.h>
#include <fido.h>
#include <gio/gio.h>
#include <glib.h>

#include <stdio.h>

const gchar *PASSKEY_SERVICE_DBUS_NAME = "com.deepin.Passkey";
const gchar *PASSKEY_SERVICE_DBUS_PATH = "/com/deepin/Passkey";
const gchar *PASSKEY_SERVICE_DBUS_INTERFACE = "com.deepin.Passkey";
// 源自xml生成的文件
extern const gchar *PASSKEY_SERVICE_DBUS_XML_DATA;

typedef struct _PasskeyServiceData
{
    // customData 提供一个string->string的kv结构
    // individual data structure instances are not automatically locked for performance reasons.
    // So, for example you must coordinate accesses to the same GHashTable from multiple threads.
    GHashTable *customData;
    mtx_t customDataMtx;
} PasskeyServiceData;

int service_custom_data_get(Service *srv, const gchar *key, gchar **value)
{
    if (srv == NULL || srv->customData == NULL) {
        LOG(LOG_ERR, "param invalid");
        return -1;
    }
    PasskeyServiceData *passkeyData = (PasskeyServiceData *)srv->customData;
    mtx_lock(&(passkeyData->customDataMtx));
    gchar *v = (gchar *)g_hash_table_lookup(passkeyData->customData, key);
    *value = g_strdup(v);
    mtx_unlock(&(passkeyData->customDataMtx));

    return 0;
}

int service_custom_data_set(Service *srv, const gchar *key, const gchar *value)
{
    if (srv == NULL || srv->customData == NULL) {
        LOG(LOG_ERR, "param invalid");
        return -1;
    }
    PasskeyServiceData *passkeyData = (PasskeyServiceData *)srv->customData;
    mtx_lock(&(passkeyData->customDataMtx));
    g_hash_table_insert(passkeyData->customData, g_strdup(key), g_strdup(value));
    mtx_unlock(&(passkeyData->customDataMtx));

    return 0;
}

int service_custom_data_delete(Service *srv, const gchar *key)
{
    if (srv == NULL || srv->customData == NULL) {
        LOG(LOG_ERR, "param invalid");
        return -1;
    }
    PasskeyServiceData *passkeyData = (PasskeyServiceData *)srv->customData;
    mtx_lock(&(passkeyData->customDataMtx));
    g_hash_table_remove(passkeyData->customData, key);
    mtx_unlock(&(passkeyData->customDataMtx));

    return 0;
}

static void free_custom_data(gpointer data)
{
    if (data == NULL) {
        return;
    }
    g_free(data);
}

int service_passkey_data_init(PasskeyServiceData *passkeyData)
{
    if (passkeyData == NULL) {
        return -1;
    }
    passkeyData->customData = g_hash_table_new_full(g_str_hash, g_str_equal, free_custom_data, free_custom_data);
    mtx_init(&(passkeyData->customDataMtx), mtx_plain);
    return 0;
}

int service_passkey_data_free(PasskeyServiceData *passkeyData)
{
    if (passkeyData == NULL) {
        return -1;
    }
    mtx_destroy(&(passkeyData->customDataMtx));
    if (passkeyData->customData != NULL) {
        g_hash_table_destroy(passkeyData->customData);
    }
    return 0;
}

// 使用PasskeyServiceData的kv结构，实现全局claim处理
static int do_claim(MethodContext *mc)
{
    int callRet = FIDO_ERR_INTERNAL;
    gchar *claim = NULL;
    if (mc == NULL || mc->serviceData == NULL || mc->sender == NULL || strlen(mc->sender) == 0) {
        LOG(LOG_ERR, "param invalid");
        goto end;
    }
    Service *srv = (Service *)mc->serviceData;

    if (service_custom_data_get(srv, "claim", &claim) != 0) {
        goto end;
    }
    if (claim != NULL) {
        // claimed
        LOG(LOG_ERR, "device claimed.");
        callRet = DEEPIN_ERR_DEVICE_CLAIMED;
        goto end;
    }
    if (service_custom_data_set(srv, "claim", mc->sender) != 0) {
        goto end;
    }

    callRet = FIDO_OK;
end:
    if (claim != NULL) {
        g_free(claim);
    }
    return callRet;
}

static int do_unclaim(MethodContext *mc)
{
    int callRet = FIDO_ERR_INTERNAL;
    gchar *claim = NULL;
    if (mc == NULL || mc->serviceData == NULL || mc->sender == NULL || strlen(mc->sender) == 0) {
        LOG(LOG_ERR, "param invalid");
        goto end;
    }
    Service *srv = (Service *)mc->serviceData;

    if (service_custom_data_get(srv, "claim", &claim) != 0) {
        goto end;
    }
    if (claim != NULL) {
        // claimed
        if (service_custom_data_delete(srv, "claim") != 0) {
            goto end;
        }
    }
    callRet = FIDO_OK;

end:
    if (claim != NULL) {
        g_free(claim);
    }
    return callRet;
}

static int do_claim_check(MethodContext *mc)
{
    int callRet = FIDO_ERR_INTERNAL;
    gchar *claim = NULL;
    if (mc == NULL || mc->serviceData == NULL || mc->sender == NULL || strlen(mc->sender) == 0) {
        LOG(LOG_ERR, "param invalid");
        goto end;
    }
    Service *srv = (Service *)mc->serviceData;

    if (service_custom_data_get(srv, "claim", &claim) != 0) {
        goto end;
    }
    if (claim != NULL) {
        // claimed
        if (g_strcmp0(claim, mc->sender) != 0) {
            LOG(LOG_ERR, "device claimed.");
            callRet = DEEPIN_ERR_DEVICE_CLAIMED;
            goto end;
        }
    }
    callRet = FIDO_OK;

end:
    if (claim != NULL) {
        g_free(claim);
    }
    return callRet;
}

static int dpk_service_api_claim(MethodContext *mc)
{
    int callRet = FIDO_ERR_INTERNAL;
    if ((callRet = do_claim(mc)) != FIDO_OK) {
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

static int dpk_service_api_unclaim(MethodContext *mc)
{
    int callRet = FIDO_ERR_INTERNAL;
    if ((callRet = do_unclaim(mc)) != FIDO_OK) {
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

static int dpk_service_api_get_pin_status(MethodContext *mc)
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

static int dpk_service_api_set_pin(MethodContext *mc)
{
    int callRet = FIDO_ERR_INTERNAL;
    const gchar *pin = NULL;
    const gchar *oldPin = NULL;

    if (mc == NULL || mc->parameters == NULL) {
        LOG(LOG_ERR, "method context invalid");
        goto end;
    }

    if ((callRet = do_claim_check(mc)) != FIDO_OK) {
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

static int dpk_service_api_reset(MethodContext *mc)
{
    int callRet = FIDO_ERR_INTERNAL;
    LOG(LOG_INFO, "service-called: Reset");

    if (mc == NULL) {
        LOG(LOG_ERR, "method context invalid");
        goto end;
    }

    if ((callRet = do_claim_check(mc)) != FIDO_OK) {
        goto end;
    }

    callRet = dpk_manager_reset(mc);
    if (callRet != FIDO_OK) {
        goto end;
    }

    callRet = FIDO_OK;

end:
    emit_reset_status(mc, SIGNAL_FINISH, callRet);
    return 0;
}

static int dpk_service_api_make_cred(MethodContext *mc)
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

    if ((callRet = do_claim_check(mc)) != FIDO_OK) {
        goto end;
    }

    g_variant_get(mc->parameters, "(&sss)", &userName, &credName, &pin);
    if (userName == NULL || credName == NULL || pin == NULL) {
        LOG(LOG_INFO, "service make cred param error");
        goto end;
    }

    LOG(LOG_INFO, "service make cred by %s:%s", userName, credName);

    if ((callRet = dpk_manager_make_cred(mc, userName, credName, pin)) != FIDO_OK) {
        goto end;
    }

    callRet = FIDO_OK;
end:
    emit_make_cred_status(mc, userName, SIGNAL_FINISH, callRet);
    return 0;
}

static int dpk_service_api_get_assert(MethodContext *mc)
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

    if ((callRet = do_claim_check(mc)) != FIDO_OK) {
        goto end;
    }

    g_variant_get(mc->parameters, "(&sss)", &userName, &credName, &pin);
    if (userName == NULL || credName == NULL || pin == NULL) {
        LOG(LOG_INFO, "service get assert param error");
        goto end;
    }

    LOG(LOG_INFO, "service get assert by %s:%s", userName, credName);
    if ((callRet = dpk_manager_get_assertion(mc, userName, credName, pin)) != FIDO_OK) {
        goto end;
    }

    callRet = FIDO_OK;
end:
    emit_get_assert_status(mc, userName, SIGNAL_FINISH, callRet);
    return 0;
}

static int dpk_service_api_get_valid_cred_count(MethodContext *mc)
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

static int dpk_service_api_get_creds(MethodContext *mc)
{
    int callRet = FIDO_ERR_INTERNAL;
    const gchar *userName = NULL;

    LOG(LOG_INFO, "service-called: GetCreds");

    if (mc == NULL || mc->parameters == NULL) {
        LOG(LOG_ERR, "method context invalid");
        goto end;
    }

    g_variant_get(mc->parameters, "(s)", &userName);

    char **creds = (char **)calloc(CRED_NUM_MAX, sizeof(char *));

    unsigned int credCount = 0;
    if ((callRet = dpk_manager_get_creds(userName, creds, &credCount)) != FIDO_OK) {
        goto end;
    }

    GVariantBuilder *builder;
    builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
    for (unsigned int i = 0; i < credCount; i++) {
        if (creds[i] == NULL) {
            continue;
        }
        g_variant_builder_add(builder, "s", creds[i]);
        free(creds[i]);
    }

    g_dbus_method_invocation_return_value(mc->invocation, g_variant_new("(as)", builder));
    g_clear_pointer(&builder, g_variant_builder_unref);
    callRet = FIDO_OK;
end:
    if (creds != NULL) {
        free(creds);
    }
    if (callRet != FIDO_OK) {
        gchar *retStr = g_strdup_printf("%d", callRet);
        g_dbus_method_invocation_return_dbus_error(mc->invocation, "com.deepin.Passkey.Error", retStr);
        g_free(retStr);
    }
    return 0;
}

static int dpk_service_api_get_device_count(MethodContext *mc)
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

static int dpk_service_api_device_detect(MethodContext *mc)
{
    gint timeout = 0;
    gboolean needSkip = FALSE;
    gboolean needDelete = FALSE;
    const gchar *deviceDetectKey = "device_detect_doing";
    gchar *deviceDetectValue = NULL;
    Service *srv = NULL;
    int callRet = FIDO_ERR_INTERNAL;

    LOG(LOG_INFO, "service-called: DeviceDetect");

    if (mc == NULL || mc->parameters == NULL || mc->serviceData == NULL) {
        LOG(LOG_ERR, "method context invalid");
        goto end;
    }

    srv = (Service *)mc->serviceData;
    if (service_custom_data_get(srv, deviceDetectKey, &deviceDetectValue) != 0) {
        goto end;
    }
    if (deviceDetectValue != NULL) {
        // doing
        callRet = FIDO_OK;
        needSkip = TRUE;
        goto end;
    }
    if (service_custom_data_set(srv, deviceDetectKey, "doing") != 0) {
        goto end;
    }
    needDelete = TRUE;
    g_variant_get(mc->parameters, "(i)", &timeout);

    if ((callRet = dpk_manager_device_detect(mc, timeout, 0, 0)) != FIDO_OK) {
        goto end;
    }
    callRet = FIDO_OK;
end:
    if (deviceDetectValue != NULL) {
        g_free(deviceDetectValue);
    }
    if (!needSkip) {
        emit_device_detect_status(mc, SIGNAL_FINISH, callRet);
    }
    if (needDelete) {
        if (srv != NULL) {
            service_custom_data_delete(srv, deviceDetectKey);
        }
    }
    return 0;
}

void dpk_service_start()
{
    Service srv;
    // 初始化PasskeyServiceData， 用于管理服务全局数据
    PasskeyServiceData passkeyData;
    service_passkey_data_init(&passkeyData);
    if (service_init(&srv, PASSKEY_SERVICE_DBUS_NAME, &passkeyData) < 0) {
        return;
    }

    if (service_register_interface(&srv, PASSKEY_SERVICE_DBUS_PATH, PASSKEY_SERVICE_DBUS_INTERFACE, PASSKEY_SERVICE_DBUS_XML_DATA) < 0) {
        return;
    }

    service_register_method(&srv, "Claim", dpk_service_api_claim, false);
    service_register_method(&srv, "UnClaim", dpk_service_api_unclaim, false);
    service_register_method(&srv, "GetPinStatus", dpk_service_api_get_pin_status, false);
    service_register_method(&srv, "SetPin", dpk_service_api_set_pin, false);
    service_register_method(&srv, "Reset", dpk_service_api_reset, true);
    service_register_method(&srv, "MakeCredential", dpk_service_api_make_cred, true);
    service_register_method(&srv, "GetAssertion", dpk_service_api_get_assert, true);
    service_register_method(&srv, "GetValidCredCount", dpk_service_api_get_valid_cred_count, false);
    service_register_method(&srv, "GetCreds", dpk_service_api_get_creds, false);
    service_register_method(&srv, "GetDeviceCount", dpk_service_api_get_device_count, false);
    service_register_method(&srv, "DeviceDetect", dpk_service_api_device_detect, true);

    service_run(&srv); // in loop, and end when exit

    service_passkey_data_free(&passkeyData);
    service_unref(&srv);

    return;
}
