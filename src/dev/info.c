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

int dpk_dev_get_support_algorithm(fido_cbor_info_t *info, int *algorithm)
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

int dpk_dev_get_options_support_uv(fido_cbor_info_t *info, int *support)
{
    return dpk_dev_get_options(info, "uv", support);
}

int dpk_dev_get_options_support_bio(fido_cbor_info_t *info, int *support)
{
    return dpk_dev_get_options(info, "bioEnroll", support);
}

int dpk_dev_get_options_support_mcuvnr(fido_cbor_info_t *info, int *support)
{
    return dpk_dev_get_options(info, "makeCredUvNotRqd", support);
}
