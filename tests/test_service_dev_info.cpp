#include <gtest/gtest.h>

#include "common/common.h"
#include "global_test_env.h"
#include "service/dev/dev.h"
#include "service/dev/info.h"

#include <fstream>
#include <iostream>
#include <tuple>
#include <vector>

#include <stdbool.h>

// deepin-passkey-tests --gtest_also_run_disabled_tests --gtest_filter=DISABLED_TestServiceDevInfo*
class DISABLED_TestServiceDevInfo : public testing::Test
{
public:
    void SetUp() override { ASSERT_NE(TestEnv::testWorkDir, ""); }

    void TearDown() override { }
};

TEST_F(DISABLED_TestServiceDevInfo, DevInfo)
{
    fido_dev_info_t *devInfoList = NULL;
    size_t nDevInfos = 0;
    fido_dev_t *defaultDev = NULL;
    fido_cbor_info_t *info = NULL;
    int callRet = FIDO_ERR_INTERNAL;
    int infoAlgorithm = 0;
    int infoOpt = 0;

    int versionsCount;
    char **versions = nullptr;

    auto printInfoOptSupport = [](const char *param, int type) {
        // 0 1 2 不支持、支持且已有设置、支持但未设置
        if (type == 0) {
            printf("[DEV-INFO]opt-param:%s, Not support.\n", param);
        } else if (type == 1) {
            printf("[DEV-INFO]opt-param:%s, Support and exist.\n", param);
        } else if (type == 2) {
            printf("[DEV-INFO]opt-param:%s, Support but not exist.\n", param);
        } else {
            printf("[DEV-INFO]opt-param:%s, invalid type:%d.\n", param, type);
        }
    };

    fido_init(FIDO_DEBUG);

    if ((callRet = dpk_dev_info_find_existed(&devInfoList, &nDevInfos)) != FIDO_OK) {
        printf("Unable to discover device(s)\n");
        goto end;
    }

    printf("to open default device\n");
    if ((callRet = dpk_dev_open_default_dev(devInfoList, nDevInfos, &defaultDev)) != FIDO_OK) {
        printf("Unable to open default device\n");
        goto end;
    }
    if (defaultDev == NULL) {
        printf("Unable to open default device.\n");
        goto end;
    }

    if ((callRet = dpk_dev_get_info(defaultDev, &info)) != FIDO_OK) {
        printf("failed to get dev-info\n");
        goto end;
    }

    // info
    if ((callRet = dpk_dev_get_version(info, &versions, &versionsCount)) != FIDO_OK) {
        printf("versions not support\n");
        callRet = FIDO_ERR_INVALID_COMMAND;
        goto end;
    }
    for (int i = 0; i < versionsCount; i++) {
        printf("[DEV-INFO]version:%s\n", versions[i]);
    }

    if ((callRet = dpk_dev_get_fido_default_support_algorithm(info, &infoAlgorithm)) != FIDO_OK) {
        printf("algorithm not support\n");
        callRet = FIDO_ERR_UNSUPPORTED_ALGORITHM;
        goto end;
    }
    printf("[DEV-INFO]algorithm = %d\n", infoAlgorithm);

    if ((callRet = dpk_dev_get_options_support_pin(info, &infoOpt)) != FIDO_OK) {
        callRet = FIDO_ERR_NO_OPERATIONS;
        goto end;
    }
    printInfoOptSupport("ClientPin", infoOpt);

    if ((callRet = dpk_dev_get_options_support_rk(info, &infoOpt)) != FIDO_OK) {
        callRet = FIDO_ERR_NO_OPERATIONS;
        goto end;
    }
    printInfoOptSupport("rk", infoOpt);

    if ((callRet = dpk_dev_get_options_support_uv(info, &infoOpt)) != FIDO_OK) {
        callRet = FIDO_ERR_NO_OPERATIONS;
        goto end;
    }
    printInfoOptSupport("uv", infoOpt);

    if ((callRet = dpk_dev_get_options_support_up(info, &infoOpt)) != FIDO_OK) {
        callRet = FIDO_ERR_NO_OPERATIONS;
        goto end;
    }
    printInfoOptSupport("up", infoOpt);

    if ((callRet = dpk_dev_get_options_support_bio(info, &infoOpt)) != FIDO_OK) {
        callRet = FIDO_ERR_NO_OPERATIONS;
        goto end;
    }
    printInfoOptSupport("BioEnroll", infoOpt);
    if ((callRet = dpk_dev_get_options_support_mcuvnr(info, &infoOpt)) != FIDO_OK) {
        callRet = FIDO_ERR_NO_OPERATIONS;
        goto end;
    }
    printInfoOptSupport("MakeCredUvNotRqd", infoOpt);
    if ((callRet = dpk_dev_get_options_support_pin_uv_auth_token(info, &infoOpt)) != FIDO_OK) {
        callRet = FIDO_ERR_NO_OPERATIONS;
        goto end;
    }
    printInfoOptSupport("pinUvAuthToken", infoOpt);
    if ((callRet = dpk_dev_get_options_support_always_uv(info, &infoOpt)) != FIDO_OK) {
        callRet = FIDO_ERR_NO_OPERATIONS;
        goto end;
    }
    printInfoOptSupport("alwaysUv", infoOpt);

    if ((callRet = dpk_dev_get_options_support_ep(info, &infoOpt)) != FIDO_OK) {
        callRet = FIDO_ERR_NO_OPERATIONS;
        goto end;
    }
    printInfoOptSupport("ep", infoOpt);

    callRet = FIDO_OK;
end:
    if (info != NULL) {
        fido_cbor_info_free(&info);
    }
    if (devInfoList != NULL) {
        fido_dev_info_free(&devInfoList, nDevInfos);
    }
    if (defaultDev != NULL) {
        fido_dev_close(defaultDev);
        fido_dev_free(&defaultDev);
    }
    if (callRet != FIDO_OK) {
        printf("test error:%d, %s\n", callRet, fido_strerr(callRet));
    }
    return;
}