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
        if ((callRet = dk_dev_do_authentication(&args, creds, credsCount)) < 0) {
            LOG(LOG_WARNING, "do_authentication failed.");
            goto end;
        }
    } else {
        // dk_dev_do_manual_authentication(args, creds, credsCount);
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

int dpk_manager_device_detect(int timeout, int stopWhenExist, int stopWhenNotExist)
{
    UNUSED_VALUE(timeout);
    UNUSED_VALUE(stopWhenExist);
    UNUSED_VALUE(stopWhenNotExist);
    //     fido_dev_info_t *devListTemp = NULL;
    //     size_t nDevsTemp = 0;
    //     int callRet = STATUS_FAILED;

    // fido_init(0);

    //     if (timeout <= 0) {
    //         callRet = STATUS_FAILED_PARAM_ERROR;
    //         goto end;
    //     }

    //     for (int i = 0; i < timeout; i++) {
    //         fprintf(stderr,
    //                 "\rNo U2F device available, please insert one now, you "
    //                 "have %2d seconds\n",
    //                 timeout - i);
    //         // fflush(stderr);
    //         thrd_sleep(&(struct timespec){ .tv_sec = 1 }, NULL);
    //         int ret = fido_dev_info_manifest(devListTemp, DEV_NUM_MAX, &nDevsTemp);
    //         if (ret != FIDO_OK) {
    //             LOG(LOG_WARNING,  "\nUnable to discover device(s), %s (%d)", fido_strerr(ret), ret);
    //             break;
    //         }
    //         if (nDevsTemp != 0) {
    //             LOG(LOG_WARNING,  "\nDevice found!");
    //             emit_device_detect_status(callRet);
    //         } else {
    //             //
    //         }
    //     }

    //     callRet = STATUS_FAILED_TIMEOUT;
    // end:
    //     emit_device_detect_status(callRet);
    return 0;
}