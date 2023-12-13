#include <gtest/gtest.h>

#include "global_test_env.h"
#include "service/dev/dev.h"
#include "service/dev/info.h"

#include <fido/bio.h>

#include <fstream>
#include <iostream>
#include <tuple>
#include <vector>

#include <stdbool.h>

// deepin-passkey-tests --gtest_also_run_disabled_tests --gtest_filter=DISABLED_TestServiceDevBio*
class DISABLED_TestServiceDevBio : public testing::Test
{
public:
    void SetUp() override { ASSERT_NE(TestEnv::testWorkDir, ""); }

    void TearDown() override { }
};

TEST_F(DISABLED_TestServiceDevBio, Status)
{
    using TestData = std::vector<std::tuple<std::string, std::string, bool>>;
    TestData tests = {
        { "test_bio_test1", R"({"code":123, "msg":"abc"})", true },
    };

    auto testFunc = [](const auto &data) {
        std::string json = std::get<1>(data);
        bool success = std::get<2>(data);
        bool ret = true;

        std::cout << "Testing: " << json << std::endl;
    };

    for (const auto &test : tests) {
        testFunc(test);
    }
}

TEST_F(DISABLED_TestServiceDevBio, Demo)
{
    fido_dev_info_t *devInfoList = NULL;
    size_t nDevInfos = 0;
    fido_dev_t *defaultDev = NULL;
    fido_cbor_info_t *info = NULL;
    int infoOpt = 0;
    fido_bio_info_t *bioInfo = NULL;
    uint8_t bioInfoMaxSamples = 0;
    uint8_t bioInfoType = 0; // sensor type: 1-touch, 2-swipe, 3-unknown
    uint64_t bioModality;

    fido_bio_template_t *bioTemplate = NULL;
    fido_bio_enroll_t *bioEnroll = NULL;

    size_t templateCount = 0;
    fido_bio_template_array_t *bioTemplateArray = NULL;
    char buf[1024];
    int callRet = FIDO_ERR_INTERNAL;

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

    auto enroll_strerr = [](uint8_t n) {
        switch (n) {
        case FIDO_BIO_ENROLL_FP_GOOD:
            return "Sample ok";
        case FIDO_BIO_ENROLL_FP_TOO_HIGH:
            return "Sample too high";
        case FIDO_BIO_ENROLL_FP_TOO_LOW:
            return "Sample too low";
        case FIDO_BIO_ENROLL_FP_TOO_LEFT:
            return "Sample too left";
        case FIDO_BIO_ENROLL_FP_TOO_RIGHT:
            return "Sample too right";
        case FIDO_BIO_ENROLL_FP_TOO_FAST:
            return "Sample too fast";
        case FIDO_BIO_ENROLL_FP_TOO_SLOW:
            return "Sample too slow";
        case FIDO_BIO_ENROLL_FP_POOR_QUALITY:
            return "Poor quality sample";
        case FIDO_BIO_ENROLL_FP_TOO_SKEWED:
            return "Sample too skewed";
        case FIDO_BIO_ENROLL_FP_TOO_SHORT:
            return "Sample too short";
        case FIDO_BIO_ENROLL_FP_MERGE_FAILURE:
            return "Sample merge failure";
        case FIDO_BIO_ENROLL_FP_EXISTS:
            return "Sample exists";
        case FIDO_BIO_ENROLL_FP_DATABASE_FULL:
            return "Fingerprint database full";
        case FIDO_BIO_ENROLL_NO_USER_ACTIVITY:
            return "No user activity";
        case FIDO_BIO_ENROLL_NO_USER_PRESENCE_TRANSITION:
            return "No user presence transition";
        default:
            return "Unknown error";
        }
    };

    // fido_init(FIDO_DEBUG);

    if ((callRet = dpk_dev_info_find_existed(&devInfoList, &nDevInfos)) != FIDO_OK) {
        printf("Unable to discover device(s)\n");
        goto end;
    }

    if ((callRet = dpk_dev_open_default_dev(devInfoList, nDevInfos, &defaultDev)) != FIDO_OK) {
        printf("Unable to open default device\n");
        goto end;
    }
    if (defaultDev == NULL) {
        printf("Unable to open default device.\n");
        goto end;
    }

    printf("to open default device success.\n");

    // 1, 获取是否支持指纹
    if ((callRet = dpk_dev_get_info(defaultDev, &info)) != FIDO_OK) {
        printf("failed to get dev-info\n");
        goto end;
    }

    if ((callRet = dpk_dev_get_options_support_bio(info, &infoOpt)) != FIDO_OK) {
        callRet = FIDO_ERR_NO_OPERATIONS;
        goto end;
    }
    printInfoOptSupport("BioEnroll", infoOpt);

    if ((callRet = dpk_dev_get_uv_modality(info, &bioModality)) != FIDO_OK) {
        callRet = FIDO_ERR_NO_OPERATIONS;
        goto end;
    }
    if ((bioModality & FIDO_UV_MODE_FP) == 0) {
        printf("not support fingerprint (modality:0x%x)\n", bioModality);
        callRet = FIDO_ERR_INTERNAL;
        goto end;
    }
    printf("support fingerprint (modality:0x%x)\n", bioModality);

    // 2, 获取指纹传感器信息
    bioInfo = fido_bio_info_new();
    if (bioInfo == NULL) {
        printf("Unable to new bio info.\n");
        goto end;
    }
    if ((callRet = fido_bio_dev_get_info(defaultDev, bioInfo)) != FIDO_OK) {
        printf("Unable to get bio info.\n");
        goto end;
    }
    bioInfoMaxSamples = fido_bio_info_max_samples(bioInfo);
    bioInfoType = fido_bio_info_type(bioInfo);
    printf("bio info, type:%d, max-samples:%d.\n", bioInfoType, bioInfoMaxSamples);

    // 3, 查询指纹数量, 貌似无法提前获取到指纹容量大小，只能在容量满时报错FIDO_ERR_FP_DATABASE_FULL
    bioTemplateArray = fido_bio_template_array_new();
    if ((callRet = fido_bio_dev_get_template_array(defaultDev, bioTemplateArray, "1111")) != FIDO_OK) {
        printf("Unable to begin bio enroll: %s.\n", fido_strerr(callRet));
        goto end;
    }
    templateCount = fido_bio_template_array_count(bioTemplateArray);
    printf("\n> Get fingerprint count: %d!!\n", templateCount);
    for (int i = 0; i < templateCount; i++) {
        const fido_bio_template_t *tmpTemplate = fido_bio_template(bioTemplateArray, i);
        printf("index:%d, template id:%s, name:%s\n", i, fido_bio_template_id_ptr(tmpTemplate), fido_bio_template_name(tmpTemplate));
        if (i >= 3) {
            printf("index:%d, to be delete\n", i);
            if ((callRet = fido_bio_dev_enroll_remove(defaultDev, tmpTemplate, "1111")) != FIDO_OK) {
                printf("Unable to delete bio enroll: %s.\n", fido_strerr(callRet));
                goto end;
            }
        }
    }

    // 4, 录入指纹。
    // 循环录入，认证器返错、平台主动取消、认证完成（remaining_samples为0）时，录入结束
    printf("\n> Start to enroll fingerprint!!\n");
    bioEnroll = fido_bio_enroll_new();
    bioTemplate = fido_bio_template_new();
    // sprintf(buf, "fp%d", templateCount + 1);
    // sprintf(buf, "ZnBw");
    // fido_bio_template_set_id(bioTemplate, (unsigned char *)buf, strlen(buf));
    if ((callRet = fido_bio_dev_enroll_begin(defaultDev, bioTemplate, bioEnroll, 10 * 1000, "1111")) != FIDO_OK) {
        printf("Unable to begin bio enroll: %s.\n", fido_strerr(callRet));
        goto end;
    }
    printf("%s.\n", enroll_strerr(fido_bio_enroll_last_status(bioEnroll)));

    while (fido_bio_enroll_remaining_samples(bioEnroll) > 0) {
        // 通过remaining_samples这个数据可以实现录入进度百分比
        printf("Touch your security key (%u sample left).\n", (unsigned)fido_bio_enroll_remaining_samples(bioEnroll));
        if ((callRet = fido_bio_dev_enroll_continue(defaultDev, bioTemplate, bioEnroll, 10 * 1000)) != FIDO_OK) {
            printf("Unable to continue bio enroll:%s.\n", fido_strerr(callRet));
            goto end;
        }
        printf("%s.\n", enroll_strerr(fido_bio_enroll_last_status(bioEnroll)));
        // 测试取消
        // if (fido_bio_enroll_remaining_samples(bioEnroll) == 2) {
        //     fido_bio_dev_enroll_cancel(defaultDev);
        //     printf("to cancel bio enroll.\n");
        //     break;
        // }
    }

    memset(buf, 0, 1024);
    sprintf(buf, "指纹%d", templateCount + 1);
    fido_bio_template_set_name(bioTemplate, buf);
    // 通过设置名字方便前端管理
    if ((callRet = fido_bio_dev_set_template_name(defaultDev, bioTemplate, "1111")) != FIDO_OK) {
        printf("Unable to set fp name: %s.\n", fido_strerr(callRet));
        goto end;
    }
    printf("> finish to enroll fingerprint!!\n\n");

    // 认证指纹的测试，通过MakeCred、GetAssert等接口进行

end:
    if (bioTemplateArray != NULL) {
        fido_bio_template_array_free(&bioTemplateArray);
    }
    if (bioEnroll != NULL) {
        fido_bio_enroll_free(&bioEnroll);
    }
    if (bioTemplate != NULL) {
        fido_bio_template_free(&bioTemplate);
    }
    if (bioInfo != NULL) {
        fido_bio_info_free(&bioInfo);
    }
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