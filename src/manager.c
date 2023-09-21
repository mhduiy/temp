// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "manager.h"

#include "common/common.h"
#include "common/log.h"
#include "dev/cred.h"
#include "dev/dev.h"
#include "dev/info.h"
#include "rp/rp.h"
#include "servicesignal.h"

#include <fido.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <threads.h>

int dpk_manager_get_pin_status(int *status)
{
    fido_dev_t *dev = NULL;
    int callRet = FIDO_ERR_INTERNAL;

    fido_init(0);

    if ((callRet = dpk_dev_get_default_dev(false, &dev)) != FIDO_OK) {
        goto end;
    }
    if (dev == NULL) {
        goto end;
    }
    *status = 0;
    if (fido_dev_supports_pin(dev)) {
        *status = *status | 0x10;
    }
    if (fido_dev_has_pin(dev)) {
        *status = *status | 0x1;
    }

    LOG(LOG_DEBUG, "get pin status %x", *status);

    callRet = FIDO_OK;
end:
    if (dev != NULL) {
        fido_dev_close(dev);
        fido_dev_free(&dev);
    }
    return callRet;
}

int dpk_manager_set_pin(const char *pin, const char *oldPin)
{
    fido_dev_t *dev = NULL;
    int callRet = FIDO_ERR_INTERNAL;

    fido_init(0);

    if ((callRet = dpk_dev_get_default_dev(false, &dev)) != FIDO_OK) {
        LOG(LOG_WARNING, "can not find dev");
        goto end;
    }
    if (dev == NULL) {
        LOG(LOG_WARNING, "can not find dev");
        goto end;
    }

    if (!fido_dev_supports_pin(dev)) {
        LOG(LOG_WARNING, "is not support pin");
        callRet = FIDO_ERR_UNSUPPORTED_OPTION;
        goto end;
    }

    if (fido_dev_has_pin(dev) && oldPin == NULL) {
        LOG(LOG_WARNING, "pin is existed, need old pin");
        callRet = FIDO_ERR_INVALID_PARAMETER;
        goto end;
    }

    callRet = fido_dev_set_pin(dev, pin, oldPin);
    if (callRet != FIDO_OK) {
        LOG(LOG_ERR, "error: fido_dev_set_pin (%d) %s", callRet, fido_strerr(callRet));
        goto end;
    }
    LOG(LOG_INFO, "set pin success.");
    callRet = FIDO_OK;
end:
    if (dev != NULL) {
        fido_dev_close(dev);
        fido_dev_free(&dev);
    }
    return callRet;
}

// 如果满足所有条件，authenticator将返回CTAP2 OK。
// 如果对所使用的传输禁用了此命令，则authenticator会返回CTAP2_ERR_OPERATION_DENIED。
// 如果明确拒绝用户存在，则验证器返回CTAP2_ERR_OPERATION_DENIED
// 如果发生用户操作超时，验证器将返回CTAP2_ERR_USER_ACTION_TIMEOUT。
// 如果请求是在通电10秒后发出的，则验证器返回CTAP2_ERR_NOT_ALLOWED。
int dpk_manager_reset()
{
    fido_dev_t *dev = NULL;
    int callRet = FIDO_ERR_INTERNAL;

    fido_init(0);

    if ((callRet = dpk_dev_get_default_dev(false, &dev)) != FIDO_OK) {
        callRet = FIDO_ERR_NOTFOUND;
        goto end;
    }
    if (dev == NULL) {
        callRet = FIDO_ERR_NOTFOUND;
        goto end;
    }
    LOG(LOG_DEBUG, "will to reset!");
    emit_reset_status(SIGNAL_NOT_FINISH, FIDO_ERR_USER_ACTION_PENDING);
    callRet = fido_dev_reset(dev);
    if (callRet != FIDO_OK) {
        fido_dev_cancel(dev);
        LOG(LOG_WARNING, "error: fido_dev_reset (%d) %s", callRet, fido_strerr(callRet));
        goto end;
    }
    LOG(LOG_INFO, "reset success!");
    callRet = FIDO_OK;
end:
    if (dev != NULL) {
        fido_dev_close(dev);
        fido_dev_free(&dev);
    }
    return callRet;
}

//  注册，生产证书
int dpk_manager_make_cred(const char *userName, const char *credName, const char *pin)
{
    fido_dev_t *dev = NULL;
    fido_cbor_info_t *info = NULL;
    fido_cred_t *cred = NULL;
    char *rpId = NULL;
    CredArgs args = { 0 };
    int algorithm = 0;
    int callRet = FIDO_ERR_INTERNAL;

    fido_init(0);

    if (!is_user_exist(userName)) {
        LOG(LOG_ERR, "user %s is not exist", userName);
        goto end;
    }

    // 1 dev
    if ((callRet = dpk_dev_get_default_dev(false, &dev)) != FIDO_OK) {
        LOG(LOG_WARNING, "failed to find dev");
        callRet = FIDO_ERR_NOTFOUND;
        goto end;
    }
    if (dev == NULL) {
        LOG(LOG_WARNING, "failed to find dev");
        callRet = FIDO_ERR_NOTFOUND;
        goto end;
    }
    // 2 info
    if ((callRet = dpk_dev_get_info(dev, &info)) != FIDO_OK) {
        LOG(LOG_WARNING, "failed to get dev-info");
        goto end;
    }
    // 3 cred
    if ((callRet = dpk_dev_get_support_algorithm(info, &algorithm)) != FIDO_OK) {
        LOG(LOG_WARNING, "algorithm not support");
        callRet = FIDO_ERR_UNSUPPORTED_ALGORITHM;
        goto end;
    }
    args.type = algorithm;
    if ((callRet = dpk_rp_get_rp_id(&rpId)) != FIDO_OK) {
        LOG(LOG_WARNING, "failed to get rp-id");
        goto end;
    }
    args.origin = rpId;
    args.userName = userName;
    args.credName = credName;
    // args.noUserPresence = 1;
    if (pin != NULL && strlen(pin) > 0) {
        args.pinVerification = 1;
    } else {
        args.pinVerification = 0;
    }
    args.userVerification = 0; // 目前不支持

    if ((callRet = dpk_dev_prepare_cred(&args, &cred)) != FIDO_OK) {
        LOG(LOG_ERR, "prepare cred failed");
        goto end;
    }

    emit_make_cred_status(userName, SIGNAL_NOT_FINISH, FIDO_ERR_USER_ACTION_PENDING);
    if ((callRet = dpk_dev_make_cred(&args, dev, cred, info, pin)) != FIDO_OK) {
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
    if (dev != NULL) {
        fido_dev_close(dev);
        fido_dev_free(&dev);
    }
    if (rpId != NULL) {
        free(rpId);
    }

    return callRet;
}

int dpk_manager_get_assertion(const char *userName, const char *credName, const char *pin)
{
    int callRet = FIDO_ERR_INTERNAL;
    char *rpId = NULL;
    CredInfo *creds = NULL;
    unsigned int credsCount = 0;
    char *credFile = NULL;
    AssertArgs args = { 0 };

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

    if (args.manual == 0) {
        if ((callRet = dk_dev_do_authentication(&args, creds, credsCount)) != FIDO_OK) {
            LOG(LOG_WARNING, "do_authentication failed.");
            goto end;
        }
    } else {
        LOG(LOG_ERR, "not support manual.");
        goto end;
    }

    LOG(LOG_INFO, "user %s authenticate success!!", userName);

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
    return callRet;
}

int dpk_manager_get_valid_cred_count(const char *userName, const char *pin, unsigned int *validCredCount)
{
    char *rpId = NULL;
    CredInfo *creds = NULL;
    unsigned int credsCount = 0;
    char *credFile = NULL;
    AssertArgs args = { 0 };
    int callRet = FIDO_ERR_INTERNAL;

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

    callRet = dk_dev_has_valid_cred_count(&args, creds, credsCount, validCredCount);
    if (callRet != FIDO_OK) {
        goto end;
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

    if ((callRet = dpk_dev_devs_find_existed(&devList, &nDevs)) != FIDO_OK) {
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

int dpk_manager_test()
{
    fido_dev_t *dev = NULL;
    int callRet = FIDO_ERR_INTERNAL;

    fido_init(0);

    if ((callRet = dpk_dev_dev_select(&dev)) != FIDO_OK) {
        LOG(LOG_WARNING, "dev select erorr");
        goto end;
    }
    if (dev == NULL) {
        goto end;
    }

    LOG(LOG_WARNING, "dev select success");

    callRet = FIDO_OK;
end:
    if (dev != NULL) {
        fido_dev_close(dev);
        fido_dev_free(&dev);
    }

    return callRet;
}

// timeout：检测的超时时间，限制了上限避免服务一直处于检测设备状态消耗系统资源
// stopWhenExist：如果为1，如果存在设备时，检测立刻结束。用于等待有设备插入。
// stopWhenNotExist：如果为1，如果不存在设备时，检测立刻结束。用于等待设备全部拔出。
int dpk_manager_device_detect(int timeout, int stopWhenExist, int stopWhenNotExist)
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
            emit_device_detect_status(SIGNAL_NOT_FINISH, nDevsCurrent);
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