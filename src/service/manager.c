// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "manager.h"

#include "common/common.h"
#include "common/errcode.h"
#include "common/log.h"
#include "decode/b64.h"
#include "decode/decode.h"
#include "dev/cred.h"
#include "dev/dev.h"
#include "dev/info.h"
#include "rp/rp.h"
#include "servicedata.h"
#include "servicesignal.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <threads.h>

// 获取选中设备，封装了检测，使用该函数，对获取到的“findDev”，需要使用service_selected_device_use_end
static int dpk_manager_find_device_by_select(MethodContext *mc, fido_dev_t **findDev)
{
    fido_dev_t *dev = NULL;
    Service *srv = NULL;
    fido_cbor_info_t *info = NULL;
    int callRet = FIDO_ERR_INTERNAL;

    LOG(LOG_INFO, "to find selected device");

    if (mc == NULL || mc->serviceData == NULL) {
        LOG(LOG_ERR, "param invalid");
        goto end;
    }
    srv = (Service *)mc->serviceData;
    if (service_selected_device_use_start(srv, mc->sender, &dev) < 0) {
        goto end;
    }

    if (dev == NULL) {
        // 如果是不存在selected的设备，那么不认为是出错，而是认为没有select设备
        callRet = FIDO_OK;
        goto end;
    }

    if ((info = fido_cbor_info_new()) == NULL) {
        LOG(LOG_ERR, "fido_cbor_info_new failed");
        goto end;
    }
    callRet = fido_dev_get_cbor_info(dev, info);
    if (callRet != FIDO_OK) {
        LOG(LOG_WARNING, "fido_dev_get_cbor_info ret (%d) %s", callRet, fido_strerr(callRet)); // FIDO_ERR_TX
        goto end;
    }
    LOG(LOG_INFO, "find device by select.");

    *findDev = dev;
    callRet = FIDO_OK;
end:
    if (callRet != FIDO_OK) {
        // 屏蔽内部错误码，使用统一错误码，便于区分
        callRet = DEEPIN_ERR_DEVICE_NOT_FOUND;
        if (dev != NULL) {
            service_selected_device_use_end(srv, mc->sender, dev);
        }
        service_selected_device_delete(srv, mc->sender);
    }
    if (info != NULL) {
        fido_cbor_info_free(&info);
    }

    return callRet;
}

int dpk_manager_get_pin_status(MethodContext *mc, int *status)
{
    fido_dev_t *selectedDev = NULL;
    fido_dev_t *defaultDev = NULL;
    fido_dev_t *currentDev = NULL;
    fido_dev_info_t *devInfoList = NULL;
    size_t nDevs = 0;
    int callRet = FIDO_ERR_INTERNAL;
    Service *srv = NULL;

    if (mc == NULL || mc->serviceData == NULL) {
        LOG(LOG_ERR, "param invalid");
        goto end;
    }
    srv = (Service *)mc->serviceData;

    fido_init(0);

    if ((callRet = dpk_manager_find_device_by_select(mc, &selectedDev)) != FIDO_OK) {
        LOG(LOG_WARNING, "Unable to use selected device.");
        goto end;
    }
    if (selectedDev == NULL) {
        if ((callRet = dpk_dev_info_find_existed(&devInfoList, &nDevs)) != FIDO_OK) {
            LOG(LOG_ERR, "Unable to discover device(s)");
            goto end;
        }
        if (nDevs == 0) {
            LOG(LOG_ERR, "Unable to discover device(s)");
            callRet = DEEPIN_ERR_DEVICE_NOT_FOUND;
            goto end;
        }
        if ((callRet = dpk_dev_open_default_dev(devInfoList, nDevs, &defaultDev)) != FIDO_OK) {
            LOG(LOG_ERR, "Unable to open default device");
            goto end;
        }
        if (defaultDev == NULL) {
            LOG(LOG_ERR, "Unable to open default device");
            goto end;
        }
        currentDev = defaultDev;
    } else {
        currentDev = selectedDev;
    }
    *status = 0;
    if (fido_dev_supports_pin(currentDev)) {
        *status = *status | 0x10;
    }
    if (fido_dev_has_pin(currentDev)) {
        *status = *status | 0x1;
    }

    LOG(LOG_WARNING, "get pin status %x", *status);

    callRet = FIDO_OK;
end:
    if (defaultDev != NULL) {
        fido_dev_close(defaultDev);
        fido_dev_free(&defaultDev);
    }
    if (devInfoList != NULL) {
        fido_dev_info_free(&devInfoList, nDevs);
    }
    if (selectedDev != NULL) {
        service_selected_device_use_end(srv, mc->sender, selectedDev);
    }
    return callRet;
}

int dpk_manager_set_pin(MethodContext *mc, const char *pin, const char *oldPin)
{
    fido_dev_t *selectedDev = NULL;
    fido_dev_t *defaultDev = NULL;
    fido_dev_t *currentDev = NULL;
    fido_dev_info_t *devInfoList = NULL;
    size_t nDevs = 0;
    int callRet = FIDO_ERR_INTERNAL;
    Service *srv = NULL;
    int symKeyType = 0;
    unsigned char *symKey = NULL;
    unsigned char *newPinB64Dec = NULL;
    size_t newPinB64DecLen = 0;
    unsigned char *oldPinB64Dec = NULL;
    size_t oldPinB64DecLen = 0;
    unsigned char *newPinDec = NULL;
    int newPinDecLen = 0;
    unsigned char *oldPinDec = NULL;
    int oldPinDecLen = 0;

    if (mc == NULL || mc->serviceData == NULL) {
        LOG(LOG_ERR, "param invalid");
        goto end;
    }
    srv = (Service *)mc->serviceData;

#ifndef NO_USE_ENCRYPT
    if (service_client_sym_key_get(srv, mc->sender, &symKeyType, &symKey) < 0) {
        LOG(LOG_ERR, "failed to get client(%s) sym key, and stop to set pin.", mc->sender);
        goto end;
    }
    if (symKey == NULL) {
        LOG(LOG_ERR, "client(%s) sym key is not exist, and stop to set pin.", mc->sender);
        goto end;
    }

    if (pin != NULL) {
        if (b64_decode(pin, (void **)&newPinB64Dec, &newPinB64DecLen) < 0) {
            LOG(LOG_ERR, "fail to decode new pin by b64.");
            goto end;
        }
        if (newPinB64Dec == NULL) {
            LOG(LOG_ERR, "fail to get new pin by b64.");
            goto end;
        }
        if (dp_sym_key_decrypt(symKeyType, symKey, (unsigned char *)newPinB64Dec, newPinB64DecLen, &newPinDec, &newPinDecLen) < 0) {
            LOG(LOG_ERR, "failed to decrypt new pin.");
            goto end;
        }
        if (newPinDec == NULL) {
            LOG(LOG_ERR, "failed to decrypt new pin.");
            goto end;
        }
    }
    if (oldPin != NULL) {
        if (b64_decode(oldPin, (void **)&oldPinB64Dec, &oldPinB64DecLen) < 0) {
            LOG(LOG_ERR, "fail to decode new pin by b64.");
            goto end;
        }
        if (oldPinB64Dec == NULL) {
            LOG(LOG_ERR, "fail to get old pin by b64.");
            goto end;
        }
        if (dp_sym_key_decrypt(symKeyType, symKey, (unsigned char *)oldPinB64Dec, oldPinB64DecLen, &oldPinDec, &oldPinDecLen) < 0) {
            LOG(LOG_ERR, "failed to decrypt old pin.");
            goto end;
        }
        if (oldPinDec == NULL) {
            LOG(LOG_ERR, "failed to decrypt old pin.");
            goto end;
        }
    }
#else
    if (pin != NULL) {
        int newPinLength = strlen(pin);
        newPinDec = (unsigned char *)calloc(newPinLength + 1, sizeof(char));
        if (newPinDec != NULL) {
            strncpy(newPinDec, pin, newPinLength);
            newPinDec[newPinLength] = 0;
        }
    }
    if (oldPin != NULL) {
        int oldPinLength = strlen(oldPin);
        oldPinDec = (unsigned char *)calloc(oldPinLength + 1, sizeof(char));
        if (oldPinDec != NULL) {
            strncpy(oldPinDec, oldPin, oldPinLength);
            oldPinDec[oldPinLength] = 0;
        }
    }
#endif

    fido_init(0);

    if ((callRet = dpk_manager_find_device_by_select(mc, &selectedDev)) != FIDO_OK) {
        LOG(LOG_WARNING, "Unable to use selected device.");
        goto end;
    }
    if (selectedDev == NULL) {
        if ((callRet = dpk_dev_info_find_existed(&devInfoList, &nDevs)) != FIDO_OK) {
            LOG(LOG_ERR, "Unable to discover device(s)");
            goto end;
        }
        if (nDevs == 0) {
            LOG(LOG_ERR, "Unable to discover device(s)");
            callRet = DEEPIN_ERR_DEVICE_NOT_FOUND;
            goto end;
        }
        if ((callRet = dpk_dev_open_default_dev(devInfoList, nDevs, &defaultDev)) != FIDO_OK) {
            LOG(LOG_ERR, "Unable to open default device");
            goto end;
        }
        if (defaultDev == NULL) {
            LOG(LOG_ERR, "Unable to open default device");
            goto end;
        }
        currentDev = defaultDev;
    } else {
        currentDev = selectedDev;
    }

    if (!fido_dev_supports_pin(currentDev)) {
        LOG(LOG_WARNING, "is not support pin");
        callRet = FIDO_ERR_UNSUPPORTED_OPTION;
        goto end;
    }

    if (fido_dev_has_pin(currentDev) && oldPinDec == NULL) {
        LOG(LOG_WARNING, "pin is existed, need old pin");
        callRet = FIDO_ERR_INVALID_PARAMETER;
        goto end;
    }

    callRet = fido_dev_set_pin(currentDev, (char *)newPinDec, (char *)oldPinDec);
    if (callRet != FIDO_OK) {
        LOG(LOG_ERR, "error: fido_dev_set_pin (%d) %s", callRet, fido_strerr(callRet));
        goto end;
    }
    LOG(LOG_INFO, "set pin success.");
    callRet = FIDO_OK;
end:
    if (defaultDev != NULL) {
        fido_dev_close(defaultDev);
        fido_dev_free(&defaultDev);
    }
    if (devInfoList != NULL) {
        fido_dev_info_free(&devInfoList, nDevs);
    }
    if (selectedDev != NULL) {
        service_selected_device_use_end(srv, mc->sender, selectedDev);
    }
    if (symKey != NULL) {
        free(symKey);
    }
    if (oldPinDec != NULL) {
        free(oldPinDec);
    }
    if (newPinDec != NULL) {
        free(newPinDec);
    }
    if (newPinB64Dec != NULL) {
        free(newPinB64Dec);
    }
    if (oldPinB64Dec != NULL) {
        free(oldPinB64Dec);
    }
    return callRet;
}

// 如果满足所有条件，authenticator将返回CTAP2 OK。
// 如果对所使用的传输禁用了此命令，则authenticator会返回CTAP2_ERR_OPERATION_DENIED。
// 如果明确拒绝用户存在，则验证器返回CTAP2_ERR_OPERATION_DENIED
// 如果发生用户操作超时，验证器将返回CTAP2_ERR_USER_ACTION_TIMEOUT。
// 如果请求是在通电10秒后发出的，则验证器返回CTAP2_ERR_NOT_ALLOWED。
int dpk_manager_reset(MethodContext *mc)
{
    fido_dev_t *selectedDev = NULL;
    fido_dev_t *defaultDev = NULL;
    fido_dev_t *currentDev = NULL;
    fido_dev_info_t *devInfoList = NULL;
    size_t nDevs = 0;
    int callRet = FIDO_ERR_INTERNAL;
    Service *srv = NULL;

    if (mc == NULL || mc->serviceData == NULL) {
        LOG(LOG_ERR, "param invalid");
        goto end;
    }
    srv = (Service *)mc->serviceData;

    fido_init(0);

    if ((callRet = dpk_manager_find_device_by_select(mc, &selectedDev)) != FIDO_OK) {
        LOG(LOG_WARNING, "Unable to use selected device.");
        goto end;
    }
    if (selectedDev == NULL) {
        if ((callRet = dpk_dev_info_find_existed(&devInfoList, &nDevs)) != FIDO_OK) {
            LOG(LOG_ERR, "Unable to discover device(s)");
            goto end;
        }
        if (nDevs == 0) {
            LOG(LOG_ERR, "Unable to discover device(s)");
            callRet = DEEPIN_ERR_DEVICE_NOT_FOUND;
            goto end;
        }
        if ((callRet = dpk_dev_open_default_dev(devInfoList, nDevs, &defaultDev)) != FIDO_OK) {
            LOG(LOG_ERR, "Unable to open default device");
            goto end;
        }
        if (defaultDev == NULL) {
            LOG(LOG_ERR, "Unable to open default device");
            goto end;
        }
        currentDev = defaultDev;
        GList *list = NULL;
        list = g_list_append(list, defaultDev);
        service_deal_devices_add_list(srv, mc->sender, mc->callId, list);
    } else {
        currentDev = selectedDev;
    }
    emit_reset_status(mc, SIGNAL_NOT_FINISH, DEEPIN_ERR_DEVICE_OPEN);

    LOG(LOG_DEBUG, "will to reset!");
    emit_reset_status(mc, SIGNAL_NOT_FINISH, FIDO_ERR_USER_ACTION_PENDING);
    callRet = fido_dev_reset(currentDev);
    if (callRet != FIDO_OK) {
        fido_dev_cancel(currentDev);
        LOG(LOG_WARNING, "error: fido_dev_reset (%d) %s", callRet, fido_strerr(callRet));
        goto end;
    }
    LOG(LOG_INFO, "reset success!");
    callRet = FIDO_OK;
end:
    if (selectedDev != NULL) {
        service_selected_device_use_end(srv, mc->sender, selectedDev);
    }
    if (defaultDev != NULL) {
        service_deal_devices_delete_list(srv, mc->sender, mc->callId);
    }
    if (devInfoList != NULL) {
        fido_dev_info_free(&devInfoList, nDevs);
    }

    return callRet;
}

//  注册，生产证书
int dpk_manager_make_cred(MethodContext *mc, const char *userName, const char *credName, const char *pin)
{
    fido_cbor_info_t *info = NULL;
    fido_cred_t *cred = NULL;
    char *rpId = NULL;
    fido_dev_t *selectedDev = NULL;
    fido_dev_t *defaultDev = NULL;
    fido_dev_t *currentDev = NULL;
    fido_dev_info_t *devInfoList = NULL;
    size_t nDevInfos = 0;
    CredArgs args = { 0 };
    int algorithm = 0;
    int supportNoPinReq = 0;
    char **versions = NULL;
    int versionsCount = 0;
    int callRet = FIDO_ERR_INTERNAL;
    Service *srv = NULL;

    if (mc == NULL || mc->serviceData == NULL) {
        LOG(LOG_ERR, "param invalid");
        goto end;
    }
    srv = (Service *)mc->serviceData;

    fido_init(0);

    if (!is_user_exist(userName)) {
        LOG(LOG_ERR, "user %s is not exist", userName);
        goto end;
    }

    // 1 dev
    if ((callRet = dpk_manager_find_device_by_select(mc, &selectedDev)) != FIDO_OK) {
        LOG(LOG_WARNING, "Unable to use selected device.");
        goto end;
    }
    if (selectedDev == NULL) {
        if ((callRet = dpk_dev_info_find_existed(&devInfoList, &nDevInfos)) != FIDO_OK) {
            LOG(LOG_ERR, "Unable to discover device(s)");
            goto end;
        }
        if (nDevInfos == 0) {
            LOG(LOG_ERR, "Unable to discover device(s)");
            callRet = DEEPIN_ERR_DEVICE_NOT_FOUND;
            goto end;
        }
        if ((callRet = dpk_dev_open_default_dev(devInfoList, nDevInfos, &defaultDev)) != FIDO_OK) {
            LOG(LOG_ERR, "Unable to open default device");
            goto end;
        }
        if (defaultDev == NULL) {
            LOG(LOG_ERR, "Unable to open default device");
            goto end;
        }
        currentDev = defaultDev;
        GList *list = NULL;
        list = g_list_append(list, defaultDev);
        service_deal_devices_add_list(srv, mc->sender, mc->callId, list);
    } else {
        currentDev = selectedDev;
    }
    emit_make_cred_status(mc, userName, SIGNAL_NOT_FINISH, DEEPIN_ERR_DEVICE_OPEN);

    // 2 info
    if ((callRet = dpk_dev_get_info(currentDev, &info)) != FIDO_OK) {
        LOG(LOG_WARNING, "failed to get dev-info");
        goto end;
    }
    // 3 cred
    if ((callRet = dpk_rp_get_rp_id(&rpId)) != FIDO_OK) {
        LOG(LOG_WARNING, "failed to get rp-id");
        goto end;
    }
    args.origin = rpId;
    args.userName = userName;
    args.credName = credName;
    if ((callRet = dpk_dev_get_version(info, &versions, &versionsCount)) != FIDO_OK) {
        LOG(LOG_ERR, "prepare cred failed");
        goto end;
    }
    if (versionsCount == 0 || versions == NULL) {
        callRet = FIDO_ERR_INTERNAL;
        LOG(LOG_ERR, "can not get dev versions");
        goto end;
    }
    // 设置当前证书使用的版本, 优先设置能支持的最高版本
    if (dpk_dev_check_version_exist((const char **)versions, versionsCount, INFO_VERSION_FIDO_2_1)) {
        snprintf(args.version, INFO_VERSION_MAX_LEN, INFO_VERSION_FIDO_2_1);
    } else if (dpk_dev_check_version_exist((const char **)versions, versionsCount, INFO_VERSION_FIDO_2_1_PRE)) {
        snprintf(args.version, INFO_VERSION_MAX_LEN, INFO_VERSION_FIDO_2_1_PRE);
    } else if (dpk_dev_check_version_exist((const char **)versions, versionsCount, INFO_VERSION_FIDO_2_0)) {
        snprintf(args.version, INFO_VERSION_MAX_LEN, INFO_VERSION_FIDO_2_0);
    } else if (dpk_dev_check_version_exist((const char **)versions, versionsCount, INFO_VERSION_U2F_V2)) {
        snprintf(args.version, INFO_VERSION_MAX_LEN, INFO_VERSION_U2F_V2);
    } else {
        LOG(LOG_ERR, "version can not support!!");
        callRet = FIDO_ERR_UNSUPPORTED_ALGORITHM;
        goto end;
    }

    // args.noUserPresence = 1;
    if (pin != NULL && strlen(pin) > 0) {
        args.pinVerification = 1;
    } else {
        args.pinVerification = 0;
        // 设备可能会要求必须使用pin
        if ((callRet = dpk_dev_check_fido2_support_no_pin_req(info, &supportNoPinReq)) != FIDO_OK) {
            LOG(LOG_ERR, "check support no-pin-req failed");
            goto end;
        }
        if (!supportNoPinReq) {
            // 不支持no-pin-req的话，通过u2f去创建证书
            LOG(LOG_INFO, "not support support no-pin-req, and to use u2f.");
            if (!dpk_dev_check_version_exist((const char **)versions, versionsCount, INFO_VERSION_U2F_V2)) {
                callRet = FIDO_ERR_PIN_REQUIRED;
                LOG(LOG_ERR, "device not support no-pin-req and not support u2f");
                goto end;
            }
            memset(args.version, 0, INFO_VERSION_MAX_LEN * sizeof(char));
            snprintf(args.version, INFO_VERSION_MAX_LEN, INFO_VERSION_U2F_V2);
        }
    }
    if (strcmp(args.version, INFO_VERSION_U2F_V2) == 0) {
        if ((callRet = dpk_dev_get_u2f_default_support_algorithm(info, &algorithm)) != FIDO_OK) {
            LOG(LOG_WARNING, "algorithm not support");
            callRet = FIDO_ERR_UNSUPPORTED_ALGORITHM;
            goto end;
        }
    } else {
        if ((callRet = dpk_dev_get_fido_default_support_algorithm(info, &algorithm)) != FIDO_OK) {
            LOG(LOG_WARNING, "algorithm not support");
            callRet = FIDO_ERR_UNSUPPORTED_ALGORITHM;
            goto end;
        }
    }
    args.type = algorithm;
    args.resident = 0;         // 默认使用非可发现凭据
    args.userVerification = 0; // 目前不支持

    if ((callRet = dpk_dev_prepare_cred(&args, &cred)) != FIDO_OK) {
        LOG(LOG_ERR, "prepare cred failed");
        goto end;
    }

    emit_make_cred_status(mc, userName, SIGNAL_NOT_FINISH, FIDO_ERR_USER_ACTION_PENDING);
    if ((callRet = dpk_dev_make_cred(&args, currentDev, cred, info, pin)) != FIDO_OK) {
        LOG(LOG_ERR, "create cred failed");
        goto end;
    }

    if ((callRet = dpk_dev_verify_cred(cred)) != FIDO_OK) {
        LOG(LOG_ERR, "verify cred failed");
        goto end;
    }

    if ((callRet = dk_dev_save_cred(&args, cred)) != FIDO_OK) {
        LOG(LOG_ERR, "save cred failed");
        goto end;
    }

    LOG(LOG_INFO, "success to make cred");

    callRet = FIDO_OK;
end:
    if (info != NULL) {
        fido_cbor_info_free(&info);
    }
    if (cred != NULL) {
        fido_cred_free(&cred);
    }

    if (rpId != NULL) {
        free(rpId);
    }
    if (selectedDev != NULL) {
        service_selected_device_use_end(srv, mc->sender, selectedDev);
    }
    if (defaultDev != NULL) {
        service_deal_devices_delete_list(srv, mc->sender, mc->callId);
    }
    if (devInfoList != NULL) {
        fido_dev_info_free(&devInfoList, nDevInfos);
    }
    return callRet;
}

int dpk_manager_get_assertion(MethodContext *mc, const char *userName, const char *credName, const char *pin)
{
    int callRet = FIDO_ERR_INTERNAL;
    char *rpId = NULL;
    CredInfo *creds = NULL;
    unsigned int credsCount = 0;
    char *credFile = NULL;
    AssertArgs args = { 0 };
    fido_dev_t *selectedDev = NULL;
    fido_dev_info_t *devInfoList = NULL;
    size_t nDevs = 0;
    fido_dev_t **openDevList = NULL;
    Service *srv = NULL;

    if (mc == NULL || mc->serviceData == NULL) {
        LOG(LOG_ERR, "param invalid");
        goto end;
    }
    srv = (Service *)mc->serviceData;

    fido_init(0);

    if (userName == NULL || strlen(userName) == 0) {
        LOG(LOG_ERR, "user name invalid.");
        goto end;
    }
    if (!is_user_exist(userName)) {
        LOG(LOG_ERR, "user %s is not exist", userName);
        goto end;
    }

    if ((callRet = dpk_rp_get_rp_id(&rpId)) != FIDO_OK) {
        LOG(LOG_WARNING, "failed to get rp-id");
        goto end;
    }
    args.origin = rpId;
    args.appid = args.origin;
    args.maxDevs = DEV_NUM_MAX;
    args.manual = 0;
    args.userName = userName;
    args.credName = credName;
    args.pin = pin;
    if (pin != NULL && strlen(pin) > 0) {
        args.pinVerification = 1;
    } else {
        args.pinVerification = 0;
    }
    args.userVerification = 0; // 目前不支持
    args.userPresence = 1;     // 默认认证开启在场证明

    creds = calloc(args.maxDevs, sizeof(CredInfo));
    if (!creds) {
        LOG(LOG_ERR, "Unable to allocate memory");
        goto end;
    }

    LOG(LOG_INFO, "Requesting authentication for user %s", userName);

    // 寻找证书
    if ((callRet = dk_dev_get_cred_file_path(userName, &credFile)) != FIDO_OK) {
        LOG(LOG_ERR, "failed to get cred file path:%s (%d)", fido_strerr(callRet), callRet);
        goto end;
    }
    args.authFile = credFile;
    if ((callRet = dk_dev_get_creds_from_file(args.authFile, userName, creds, &credsCount)) != FIDO_OK) {
        LOG(LOG_ERR, "Unable to get devices from authentication file");
        goto end;
    }
    if (credsCount == 0) {
        LOG(LOG_ERR, "Found no devices. Aborting.");
        goto end;
    }
    LOG(LOG_INFO, "Found %d cred of user %s, and will to authenticate.", credsCount, userName);

    // 寻找设备
    if ((callRet = dpk_manager_find_device_by_select(mc, &selectedDev)) != FIDO_OK) {
        LOG(LOG_WARNING, "Unable to use selected device.");
        goto end;
    }

    if (selectedDev == NULL) {
        // 没有选择设备，开启所有设备的认证
        if ((callRet = dpk_dev_info_find_existed(&devInfoList, &nDevs)) != FIDO_OK) {
            LOG(LOG_ERR, "Unable to discover device(s)");
            goto end;
        }
        if (nDevs == 0) {
            LOG(LOG_ERR, "Unable to discover device(s)");
            callRet = DEEPIN_ERR_DEVICE_NOT_FOUND;
            goto end;
        }
        openDevList = calloc(nDevs, sizeof(fido_dev_t *));
        if (!openDevList) {
            LOG(LOG_ERR, "Unable to allocate authenticator list");
            goto end;
        }
        if ((callRet = dpk_dev_open_all_dev(devInfoList, nDevs, openDevList, nDevs)) != FIDO_OK) {
            LOG(LOG_ERR, "Unable to open device(s)");
            goto end;
        }

        GList *list = NULL;
        for (size_t i = 0; i < nDevs; i++) {
            if (openDevList[i] == NULL) {
                continue;
            }
            list = g_list_append(list, openDevList[i]);
        }
        service_deal_devices_add_list(srv, mc->sender, mc->callId, list);
    } else {
        nDevs = 1;
        openDevList = calloc(nDevs, sizeof(fido_dev_t *));
        if (!openDevList) {
            LOG(LOG_ERR, "Unable to allocate authenticator list");
            goto end;
        }
        openDevList[0] = selectedDev;
    }

    emit_get_assert_status(mc, userName, SIGNAL_NOT_FINISH, DEEPIN_ERR_DEVICE_OPEN);

    bool isSuccess = false;
    for (size_t i = 0; i < nDevs; i++) {
        if (args.manual == 0) {
            callRet = dk_dev_do_authentication(mc, &args, creds, credsCount, openDevList[i]);
            if (callRet == FIDO_OK) {
                isSuccess = true;
                break;
            }
        } else {
            LOG(LOG_ERR, "not support manual.");
            goto end;
        }
    }

    if (isSuccess) {
        LOG(LOG_INFO, "user %s authenticate success.", userName);
    } else {
        LOG(LOG_INFO, "user %s authenticate fail.", userName);
        goto end;
    }

    callRet = FIDO_OK;
end:
    if (rpId != NULL) {
        free(rpId);
    }
    if (credFile != NULL) {
        free(credFile);
    }
    if (creds != NULL) {
        for (unsigned int i = 0; i < credsCount; i++) {
            dk_dev_reset_cred(&creds[i]);
        }
        free(creds);
    }
    if (selectedDev != NULL) {
        service_selected_device_use_end(srv, mc->sender, selectedDev);
    }
    if (devInfoList != NULL) {
        fido_dev_info_free(&devInfoList, nDevs);
    }
    if (openDevList != NULL) {
        if (selectedDev == NULL) {
            service_deal_devices_delete_list(srv, mc->sender, mc->callId);
        }
        free(openDevList);
    }
    return callRet;
}

int dpk_manager_get_valid_cred_count(MethodContext *mc, const char *userName, const char *pin, unsigned int *validCredCount)
{
    char *rpId = NULL;
    CredInfo *creds = NULL;
    unsigned int credsCount = 0;
    char *credFile = NULL;
    fido_dev_t *selectedDev = NULL;
    fido_dev_info_t *devInfoList = NULL;
    size_t nDevs = 0;
    fido_dev_t **openDevList = NULL;
    AssertArgs args = { 0 };
    int callRet = FIDO_ERR_INTERNAL;

    Service *srv = NULL;

    if (mc == NULL || mc->serviceData == NULL) {
        LOG(LOG_ERR, "param invalid");
        goto end;
    }
    srv = (Service *)mc->serviceData;

    fido_init(0);

    if (userName == NULL || strlen(userName) == 0) {
        LOG(LOG_ERR, "user name invalid.");
        goto end;
    }
    if (!is_user_exist(userName)) {
        LOG(LOG_ERR, "user %s is not exist", userName);
        goto end;
    }
    *validCredCount = 0;

    if ((callRet = dpk_rp_get_rp_id(&rpId)) != FIDO_OK) {
        LOG(LOG_WARNING, "failed to get rp-id");
        goto end;
    }
    args.origin = rpId;
    args.appid = args.origin;
    args.maxDevs = DEV_NUM_MAX;
    args.manual = 0;
    args.pin = pin;

    creds = calloc(args.maxDevs, sizeof(CredInfo));
    if (!creds) {
        LOG(LOG_ERR, "Unable to allocate memory");
        goto end;
    }

    if ((callRet = dk_dev_get_cred_file_path(userName, &credFile)) != FIDO_OK) {
        LOG(LOG_ERR, "failed to get cred file path:%s (%d)", fido_strerr(callRet), callRet);
        goto end;
    }
    if (credFile == NULL) {
        LOG(LOG_ERR, "failed to get cred file path.");
        goto end;
    }
    args.authFile = credFile;

    LOG(LOG_INFO, "get valid cred for user %s %s", userName, credFile);

    if ((callRet = dk_dev_get_creds_from_file(args.authFile, userName, creds, &credsCount)) != FIDO_OK) {
        LOG(LOG_WARNING, "Unable to get devices from authentication file");
    }
    if (credsCount == 0) {
        LOG(LOG_INFO, "Found no creds. Aborting.");
        // 证书文件不存在或者文件内不存在用户证书，跳过验证证书环节，认为是查询成功，结果为0
        callRet = FIDO_OK;
        goto end;
    }

    // 寻找设备
    if ((callRet = dpk_manager_find_device_by_select(mc, &selectedDev)) != FIDO_OK) {
        LOG(LOG_WARNING, "Unable to use selected device.");
        goto end;
    }

    if (selectedDev == NULL) {
        // 没有选择设备，开启所有设备的认证
        LOG(LOG_INFO, "Unable to use selected device and to search devices.");
        if ((callRet = dpk_dev_info_find_existed(&devInfoList, &nDevs)) != FIDO_OK) {
            LOG(LOG_ERR, "Unable to discover device(s)");
            goto end;
        }

        if (nDevs == 0) {
            LOG(LOG_ERR, "Unable to discover device(s)");
            callRet = DEEPIN_ERR_DEVICE_NOT_FOUND;
            goto end;
        }

        openDevList = calloc(nDevs, sizeof(fido_dev_t *));
        if (!openDevList) {
            LOG(LOG_ERR, "Unable to allocate authenticator list");
            goto end;
        }
        if ((callRet = dpk_dev_open_all_dev(devInfoList, nDevs, openDevList, nDevs)) != FIDO_OK) {
            LOG(LOG_ERR, "Unable to open device(s)");
            goto end;
        }
    } else {
        nDevs = 1;
        openDevList = calloc(nDevs, sizeof(fido_dev_t *));
        if (!openDevList) {
            LOG(LOG_ERR, "Unable to allocate authenticator list");
            goto end;
        }
        openDevList[0] = selectedDev;
    }

    LOG(LOG_INFO, "to check creds.");

    for (size_t i = 0; i < nDevs; i++) {
        if (openDevList[i] == NULL) {
            continue;
        }
        unsigned int count;
        callRet = dk_dev_has_valid_cred_count(&args, creds, credsCount, openDevList[i], &count);
        if (callRet != FIDO_OK) {
            goto end;
        }
        *validCredCount += count;
    }

    LOG(LOG_INFO, "the valid cerd count of user %s is %d", userName, *validCredCount);

    callRet = FIDO_OK;
end:
    if (rpId != NULL) {
        free(rpId);
    }
    if (credFile != NULL) {
        free(credFile);
    }
    if (creds != NULL) {
        for (unsigned int i = 0; i < credsCount; i++) {
            dk_dev_reset_cred(&creds[i]);
        }
        free(creds);
    }
    if (selectedDev != NULL) {
        service_selected_device_use_end(srv, mc->sender, selectedDev);
    }
    if (devInfoList != NULL) {
        fido_dev_info_free(&devInfoList, nDevs);
    }
    if (openDevList != NULL) {
        if (selectedDev == NULL) {
            for (size_t i = 0; i < nDevs; i++) {
                if (openDevList[i] != NULL) {
                    fido_dev_close(openDevList[i]);
                    fido_dev_free(&openDevList[i]);
                }
            }
        }
        free(openDevList);
    }
    return callRet;
}

int dpk_manager_get_creds(const char *userName, char **creds, unsigned int *credCount)
{
    char *rpId = NULL;
    CredInfo *credsInfo = NULL;
    unsigned int credsCountTemp = 0;
    char *credFile = NULL;
    AssertArgs args = { 0 };
    int callRet = FIDO_ERR_INTERNAL;

    fido_init(0);

    if (userName == NULL || strlen(userName) == 0 || credCount == NULL) {
        LOG(LOG_ERR, "param invalid.");
        goto end;
    }

    *credCount = 0;
    if (!is_user_exist(userName)) {
        callRet = FIDO_OK;
        goto end;
    }

    if ((callRet = dpk_rp_get_rp_id(&rpId)) != FIDO_OK) {
        LOG(LOG_WARNING, "failed to get rp-id");
        goto end;
    }
    args.origin = rpId;
    args.appid = args.origin;
    args.maxDevs = CRED_NUM_MAX;
    args.manual = 0;

    credsInfo = calloc(args.maxDevs, sizeof(CredInfo));
    if (!credsInfo) {
        LOG(LOG_ERR, "Unable to allocate memory");
        goto end;
    }

    if ((callRet = dk_dev_get_cred_file_path(userName, &credFile)) != FIDO_OK) {
        LOG(LOG_ERR, "failed to get cred file path:%s (%d)", fido_strerr(callRet), callRet);
        goto end;
    }
    if (credFile == NULL) {
        LOG(LOG_ERR, "failed to get cred file path.");
        goto end;
    }
    args.authFile = credFile;

    LOG(LOG_INFO, "get creds for user %s %s", userName, credFile);

    if ((callRet = dk_dev_get_creds_from_file(args.authFile, userName, credsInfo, &credsCountTemp)) != FIDO_OK) {
        LOG(LOG_WARNING, "Unable to get devices from authentication file");
    }
    if (credsCountTemp == 0) {
        LOG(LOG_INFO, "Found no creds. Aborting.");
        callRet = FIDO_OK;
        goto end;
    }

    for (unsigned int i = 0; i < credsCountTemp; i++) {
        creds[i] = strdup((&credsInfo[i])->name);
    }
    *credCount = credsCountTemp;

    LOG(LOG_INFO, "the cerd count of user %s is %d", userName, credsCountTemp);

    callRet = FIDO_OK;
end:
    if (rpId != NULL) {
        free(rpId);
    }
    if (credFile != NULL) {
        free(credFile);
    }
    if (credsInfo != NULL) {
        for (unsigned int i = 0; i < credsCountTemp; i++) {
            dk_dev_reset_cred(&credsInfo[i]);
        }
        free(credsInfo);
    }
    return callRet;
}

int dpk_manager_get_device_count(int *count)
{
    size_t nDevs = 0;
    fido_dev_info_t *devList = NULL;
    int callRet = FIDO_ERR_INTERNAL;

    fido_init(0);

    if ((callRet = dpk_dev_info_find_existed(&devList, &nDevs)) != FIDO_OK) {
        if (callRet == FIDO_ERR_NOTFOUND) {
            callRet = FIDO_OK;
            nDevs = 0;
        } else {
            goto end;
        }
    }

    *count = nDevs;
    callRet = FIDO_OK;
end:
    if (devList != NULL) {
        fido_dev_info_free(&devList, nDevs);
    }
    return callRet;
}

int dpk_manager_select(MethodContext *mc)
{
    Service *srv = NULL;
    fido_dev_t *dev = NULL;
    size_t nDevInfos = 0;
    fido_dev_info_t *devInfoList = NULL;
    int callRet = FIDO_ERR_INTERNAL;

    if (mc == NULL || mc->serviceData == NULL) {
        LOG(LOG_ERR, "param invalid");
        goto end;
    }

    fido_init(0);

    if ((callRet = dpk_dev_info_find_existed(&devInfoList, &nDevInfos)) != FIDO_OK) {
        goto end;
    }

    if ((callRet = dpk_dev_select_dev(devInfoList, nDevInfos, &dev)) != FIDO_OK) {
        LOG(LOG_WARNING, "dev select erorr");
        goto end;
    }
    if (dev == NULL) {
        goto end;
    }

    LOG(LOG_WARNING, "dev select success");

    srv = (Service *)mc->serviceData;
    // 重复select，直接覆盖，上次选中的设备会在hash_table中释放
    if (service_selected_device_add(srv, mc->sender, dev) < 0) {
        callRet = FIDO_ERR_INTERNAL;
        goto end;
    }

    callRet = FIDO_OK;
end:
    if (callRet != FIDO_OK) {
        if (dev != NULL) {
            fido_dev_close(dev);
            fido_dev_free(&dev);
        }
    }
    if (devInfoList != NULL) {
        fido_dev_info_free(&devInfoList, nDevInfos);
    }
    return callRet;
}

// 取消选中
int dpk_manager_select_close(MethodContext *mc)
{
    Service *srv = NULL;
    fido_dev_t *dev = NULL;
    int callRet = FIDO_ERR_INTERNAL;

    if (mc == NULL || mc->serviceData == NULL) {
        LOG(LOG_ERR, "param invalid");
        goto end;
    }

    fido_init(0);

    srv = (Service *)mc->serviceData;
    if (service_selected_device_use_start(srv, mc->sender, &dev) < 0) {
        callRet = FIDO_ERR_INTERNAL;
        goto end;
    }
    if (dev != NULL) {
        fido_dev_close(dev);
        service_selected_device_delete(srv, mc->sender);
    }

    callRet = FIDO_OK;
end:
    if (dev != NULL) {
        service_selected_device_use_end(srv, mc->sender, dev);
    }

    return callRet;
}

// timeout：检测的超时时间，限制了上限避免服务一直处于检测设备状态消耗系统资源
// stopWhenExist：如果为1，如果存在设备时，检测立刻结束。用于等待有设备插入。
// stopWhenNotExist：如果为1，如果不存在设备时，检测立刻结束。用于等待设备全部拔出。
int dpk_manager_device_detect(MethodContext *mc, int timeout, int stopWhenExist, int stopWhenNotExist)
{
    fido_dev_info_t *devListTemp = NULL;
    size_t nDevsTemp = 0;
    int callRet = FIDO_ERR_INTERNAL;

    fido_init(0);

    if ((timeout <= 0 || timeout > 180) || (stopWhenExist != 0 && stopWhenExist != 1) || (stopWhenNotExist != 0 && stopWhenNotExist != 1)) {
        callRet = FIDO_ERR_INVALID_PARAMETER;
        goto end;
    }

    size_t nDevsCurrent = 999999; // 初始一个非法值
    for (int i = 0; i < timeout; i++) {
        thrd_sleep(&(struct timespec){ .tv_sec = 1 }, NULL);
        devListTemp = fido_dev_info_new(DEV_NUM_MAX);
        if (!devListTemp) {
            LOG(LOG_ERR, "Unable to allocate devlist");
            goto end;
        }
        callRet = fido_dev_info_manifest(devListTemp, DEV_NUM_MAX, &nDevsTemp);
        if (callRet != FIDO_OK) {
            LOG(LOG_ERR, "Unable to discover device(s), %s (%d)", fido_strerr(callRet), callRet);
            goto end;
        }

        if (nDevsTemp != nDevsCurrent) {
            nDevsCurrent = nDevsTemp;
            emit_device_detect_status(mc, SIGNAL_NOT_FINISH, nDevsCurrent);
        }
        LOG(LOG_DEBUG, "Detect device remain times %d, current device count %d", timeout - i, nDevsCurrent);

        if (stopWhenExist > 0 && nDevsCurrent > 0) {
            callRet = FIDO_OK;
            goto end;
        }

        if (stopWhenNotExist > 0 && nDevsCurrent == 0) {
            callRet = FIDO_OK;
            goto end;
        }

        if (devListTemp != NULL) {
            fido_dev_info_free(&devListTemp, nDevsTemp);
            devListTemp = NULL;
            nDevsTemp = 0;
        }
    }

    if (stopWhenExist > 0 && nDevsCurrent == 0) {
        callRet = FIDO_ERR_TIMEOUT;
        goto end;
    }

    if (stopWhenNotExist > 0 && nDevsCurrent > 0) {
        callRet = FIDO_ERR_TIMEOUT;
        goto end;
    }

    callRet = FIDO_OK;
end:
    if (devListTemp != NULL) {
        fido_dev_info_free(&devListTemp, nDevsTemp);
    }
    return callRet;
}

int dpk_manager_devices_close(MethodContext *mc, const char *callId)
{
    Service *srv = NULL;
    GList *devList = NULL;
    int callRet = FIDO_ERR_INTERNAL;

    if (mc == NULL || mc->serviceData == NULL || callId == NULL) {
        LOG(LOG_ERR, "param invalid");
        goto end;
    }

    fido_init(0);

    srv = (Service *)mc->serviceData;

    if (strlen(callId) == 0) {
        callRet = FIDO_ERR_UNSUPPORTED_OPTION;
        goto end;
    } else {
        service_deal_devices_list_use_start(srv, mc->sender, callId, &devList);
        if (devList == NULL) {
            goto end;
        }
        for (size_t i = 0; i < g_list_length(devList); i++) {
            fido_dev_t *dev = (fido_dev_t *)g_list_nth_data(devList, i);
            if (dev != NULL) {
                fido_dev_cancel(dev);
                fido_dev_close(dev);
            }
        }
    }

    callRet = FIDO_OK;
end:
    if (devList != NULL) {
        service_deal_devices_list_use_end(srv, mc->sender, callId, devList);
    }

    return callRet;
}

int dpk_manager_encrypt_get_public(MethodContext *mc, int type, char **publicKey)
{
    Service *srv = NULL;
    unsigned char *priKey = NULL;
    unsigned char *pubKey = NULL;
    int callRet = FIDO_ERR_INTERNAL;

    if (mc == NULL || mc->serviceData == NULL || publicKey == NULL) {
        LOG(LOG_ERR, "param invalid");
        goto end;
    }

    srv = (Service *)mc->serviceData;

    if (dp_asym_key_check_support(type) < 0) {
        LOG(LOG_ERR, "asym-key type is not supported.");
        goto end;
    }

    if (service_service_asym_key_get(srv, type, &priKey) < 0) {
        goto end;
    }
    if (priKey == NULL) {
        // 不存在则新建
        if (dp_asym_key_create_private(type, &priKey) < 0) {
            goto end;
        }
        if (priKey == NULL) {
            goto end;
        }
        if (service_service_asym_key_set(srv, type, priKey) < 0) {
            goto end;
        }
    }

    if (dp_asym_key_private_to_public(type, priKey, &pubKey) < 0) {
        goto end;
    }
    if (pubKey == NULL) {
        goto end;
    }

    *publicKey = (char *)pubKey;

    callRet = FIDO_OK;
end:
    if (priKey != NULL) {
        free(priKey);
    }
    if (callRet != FIDO_OK && pubKey != NULL) {
        free(pubKey);
    }
    return callRet;
}

int dpk_manager_encrypt_set_symmetric_key(MethodContext *mc, int encType, int keyType, const char *encKey)
{
    Service *srv = NULL;
    unsigned char *priKey = NULL;
    unsigned char *symKey = NULL;
    size_t symKeyLen = 0;
    unsigned char *clientKey = NULL;
    int clientKeyLen = 0;
    int callRet = FIDO_ERR_INTERNAL;

    if (mc == NULL || mc->serviceData == NULL || encKey == NULL) {
        LOG(LOG_ERR, "param invalid");
        goto end;
    }

    srv = (Service *)mc->serviceData;

    if (dp_asym_key_check_support(encType) < 0) {
        LOG(LOG_ERR, "asym-key type is not supported.");
        goto end;
    }

    if (dp_sym_key_check_support(keyType) < 0) {
        LOG(LOG_ERR, "sym-key type is not supported.");
        goto end;
    }

    if (b64_decode(encKey, (void **)&symKey, &symKeyLen) < 0) {
        LOG(LOG_ERR, "failed to decode sym-key from b64.");
        goto end;
    }
    if (symKey == NULL || symKeyLen <= 0) {
        LOG(LOG_ERR, "failed to decode sym-key from b64.");
        goto end;
    }

    if (service_service_asym_key_get(srv, encType, &priKey) < 0) {
        LOG(LOG_ERR, "failed to get service asym-key.");
        goto end;
    }
    if (priKey == NULL) {
        // 服务密钥必须存在
        LOG(LOG_ERR, "service asym-key is not exist.");
        goto end;
    }

    if (dp_asym_key_decrypt(encType, priKey, symKey, symKeyLen, &clientKey, &clientKeyLen) < 0) {
        LOG(LOG_ERR, "failed to decrypt client sym-key.");
        goto end;
    }

    if (service_client_sym_key_set(srv, mc->sender, keyType, clientKey) < 0) {
        LOG(LOG_ERR, "failed to save client sym-key.");
        goto end;
    }

    callRet = FIDO_OK;
end:
    if (priKey != NULL) {
        free(priKey);
    }
    if (clientKey != NULL) {
        free(clientKey);
    }
    if (symKey != NULL) {
        free(symKey);
    }
    return callRet;
}