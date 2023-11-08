// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "info.h"

#include "common/common.h"
#include "common/log.h"
#include "cred.h"

#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 从设备获取info信息
int dpk_dev_get_info(fido_dev_t *dev, fido_cbor_info_t **info)
{
    fido_cbor_info_t *infoRes;
    int ret = 0;
    int callRet = FIDO_ERR_INTERNAL;

    if (!fido_dev_is_fido2(dev)) {
        LOG(LOG_ERR, "device is not fido2");
        goto end;
    }

    if ((infoRes = fido_cbor_info_new()) == NULL) {
        LOG(LOG_ERR, "fido_cbor_info_new failed");
        goto end;
    }
    if ((ret = fido_dev_get_cbor_info(dev, infoRes)) != FIDO_OK) {
        LOG(LOG_ERR, "fido_dev_get_cbor_info: %s (%d)", fido_strerr(ret), ret);
        goto end;
    }

    *info = infoRes;
    callRet = FIDO_OK;
end:
    if (callRet != FIDO_OK && infoRes != NULL) {
        fido_cbor_info_free(&infoRes);
    }
    return callRet;
}

int dpk_dev_get_version(fido_cbor_info_t *info, char ***versions, int *count)
{
    int callRet = FIDO_ERR_INTERNAL;

    if (info == NULL) {
        LOG(LOG_ERR, "info is null");
        goto end;
    }
    // 查询设备支持的算法列表
    size_t versionsCount = fido_cbor_info_versions_len(info);
    LOG(LOG_DEBUG, "support versions count %ld", versionsCount);
    if (versionsCount > 0) {
        *versions = fido_cbor_info_versions_ptr(info);
    }

    *count = versionsCount;

    callRet = FIDO_OK;
end:
    return callRet;
}

int dpk_dev_get_fido_default_support_algorithm(fido_cbor_info_t *info, int *algorithm)
{
    int callRet = FIDO_ERR_INTERNAL;

    if (info == NULL) {
        LOG(LOG_ERR, "info is null");
        goto end;
    }
    // 查询设备支持的算法列表
    size_t supportAlgorithmCount = fido_cbor_info_algorithm_count(info);
    LOG(LOG_DEBUG, "support algorithm count %ld", supportAlgorithmCount);
    // dde期望使用的算法列表，这个列表顺序决定了优先级，一般默认用ES256，所以ES256在第一个，其他算法顺序暂时未要求
    int algorithmList[] = { COSE_ES256, COSE_EDDSA, COSE_ECDH_ES256, COSE_ES384, COSE_RS256, COSE_RS1 };
    int useAlgorithm = COSE_UNSPEC;
    for (size_t i = 0; i < sizeof(algorithmList) / sizeof(algorithmList[0]); i++) {
        for (size_t j = 0; j < supportAlgorithmCount; j++) {
            int id = fido_cbor_info_algorithm_cose(info, j);
            if (id == algorithmList[i]) {
                useAlgorithm = id;
                break;
            }
        }
        if (useAlgorithm != COSE_UNSPEC) {
            break;
        }
    }

    if (useAlgorithm == COSE_UNSPEC) {
        LOG(LOG_ERR, "can not find algorithm to support the device");
        goto end;
    }

    *algorithm = useAlgorithm;
    LOG(LOG_DEBUG, "use algorithm - %d", useAlgorithm);

    callRet = FIDO_OK;
end:
    return callRet;
}

int dpk_dev_get_u2f_default_support_algorithm(fido_cbor_info_t *info, int *algorithm)
{
    int callRet = FIDO_ERR_INTERNAL;

    if (info == NULL) {
        LOG(LOG_ERR, "info is null");
        goto end;
    }
    // 查询设备支持的算法列表
    size_t supportAlgorithmCount = fido_cbor_info_algorithm_count(info);
    LOG(LOG_DEBUG, "support algorithm count %ld", supportAlgorithmCount);
    // u2f只使用COSE_ES256
    int algorithmList[] = { COSE_ES256 };
    int useAlgorithm = COSE_UNSPEC;
    for (size_t i = 0; i < sizeof(algorithmList) / sizeof(algorithmList[0]); i++) {
        for (size_t j = 0; j < supportAlgorithmCount; j++) {
            int id = fido_cbor_info_algorithm_cose(info, j);
            if (id == algorithmList[i]) {
                useAlgorithm = id;
                break;
            }
        }
        if (useAlgorithm != COSE_UNSPEC) {
            break;
        }
    }

    if (useAlgorithm == COSE_UNSPEC) {
        LOG(LOG_ERR, "can not find algorithm to support the device");
        goto end;
    }

    *algorithm = useAlgorithm;
    LOG(LOG_DEBUG, "use algorithm - %d", useAlgorithm);

    callRet = FIDO_OK;
end:
    return callRet;
}

static int dpk_dev_get_options(fido_cbor_info_t *info, const char *opt, int *support)
{
    char *const *opts;
    const bool *vals;
    int callRet = FIDO_ERR_INTERNAL;

    if (info == NULL || opt == NULL) {
        LOG(LOG_ERR, "param invalid.");
        goto end;
    }

    int feature = INFO_OPTIONS_NOT_SUPPORT;
    opts = fido_cbor_info_options_name_ptr(info);
    vals = fido_cbor_info_options_value_ptr(info);
    for (size_t i = 0; i < fido_cbor_info_options_len(info); i++) {
        if (strcmp(opts[i], opt) == 0) {
            feature = vals[i] ? INFO_OPTIONS_SET : INFO_OPTIONS_UNSET;
            break;
        }
    }
    *support = feature;
    callRet = FIDO_OK;
end:
    return callRet;
}

int dpk_dev_get_options_support_pin(fido_cbor_info_t *info, int *support)
{
    return dpk_dev_get_options(info, "clientPin", support);
}

int dpk_dev_get_options_support_rk(fido_cbor_info_t *info, int *support)
{
    return dpk_dev_get_options(info, "rk", support);
}

int dpk_dev_get_options_support_uv(fido_cbor_info_t *info, int *support)
{
    return dpk_dev_get_options(info, "uv", support);
}

int dpk_dev_get_options_support_up(fido_cbor_info_t *info, int *support)
{
    return dpk_dev_get_options(info, "up", support);
}

int dpk_dev_get_options_support_bio(fido_cbor_info_t *info, int *support)
{
    return dpk_dev_get_options(info, "bioEnroll", support);
}

int dpk_dev_get_options_support_mcuvnr(fido_cbor_info_t *info, int *support)
{
    return dpk_dev_get_options(info, "makeCredUvNotRqd", support);
}

int dpk_dev_get_options_support_pin_uv_auth_token(fido_cbor_info_t *info, int *support)
{
    return dpk_dev_get_options(info, "pinUvAuthToken", support);
}

int dpk_dev_get_options_support_always_uv(fido_cbor_info_t *info, int *support)
{
    return dpk_dev_get_options(info, "alwaysUv", support);
}

int dpk_dev_get_options_support_ep(fido_cbor_info_t *info, int *support)
{
    return dpk_dev_get_options(info, "ep", support);
}

int dpk_dev_check_fido2_support_no_pin_req(fido_cbor_info_t *info, int *support)
{
    int uv = 0;
    int makeCredUvNotRqd = 0;
    int alwaysUv = 0;
    int checkSupport = 0;
    int callRet = FIDO_ERR_INTERNAL;

    if ((callRet = dpk_dev_get_options_support_uv(info, &uv)) != FIDO_OK) {
        LOG(LOG_WARNING, "get option failed.");
        goto end;
    }
    if ((callRet = dpk_dev_get_options_support_mcuvnr(info, &makeCredUvNotRqd)) != FIDO_OK) {
        LOG(LOG_WARNING, "get option failed.");
        goto end;
    }
    if ((callRet = dpk_dev_get_options_support_always_uv(info, &alwaysUv)) != FIDO_OK) {
        LOG(LOG_WARNING, "get option failed.");
        goto end;
    }

    // 协议参数综合判断
    do {
        if (makeCredUvNotRqd == INFO_OPTIONS_SET) {
            checkSupport = 1;
            break;
        }
        if (alwaysUv == INFO_OPTIONS_SET) {
            checkSupport = 0;
            break;
        }
        if (uv == INFO_OPTIONS_NOT_SUPPORT) {
            checkSupport = 0;
            break;
        }

    } while (0);

    *support = checkSupport;
    callRet = FIDO_OK;

end:
    return callRet;
}

bool dpk_dev_check_version_exist(const char **versions, int versionsCount, const char *version)
{
    if (versions == NULL || versionsCount <= 0 || version == NULL) {
        return false;
    }
    for (int i = 0; i < versionsCount; i++) {
        if (versions[i] == NULL) {
            continue;
        }
        if (strcmp(versions[i], version) == 0) {
            return true;
        }
    }
    return false;
}