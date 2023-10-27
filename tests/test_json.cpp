// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "common/json.h"
#include "global_test_env.h"

#include <tuple>
#include <vector>

#include <stdbool.h>

class TestJson : public testing::Test
{
public:
    void SetUp() override { ASSERT_NE(TestEnv::testWorkDir, ""); }

    void TearDown() override { }
};

TEST_F(TestJson, Test1)
{
    using TestData = std::vector<std::tuple<std::string, std::string, int, std::string, bool>>;
    TestData tests = {
        { "test_json_test1", R"({"code":123, "msg":"abc"})", 123, "abc", true },
        { "test_json_test1", R"({"code":1234444, "msg":"abc"})", 123, "abc", false },
        { "test_json_test1", R"({"code":123, "msg":"abcdddd"})", 123, "abc", false },
    };

    auto testFunc = [](const auto &data) {
        std::string jsonData = std::get<1>(data);
        int code = std::get<2>(data);
        std::string msg = std::get<3>(data);
        bool success = std::get<4>(data);
        bool ret = true;

        DpApiResult resultData;
        EXPECT_EQ(dp_json_to_result(jsonData.c_str(), &resultData), 0);
        if (HasFailure()) {
            return;
        }

        if (resultData.code != code) {
            ret = false;
        }

        if (std::string(resultData.msg) != msg) {
            ret = false;
        }

        EXPECT_EQ(success, ret);
        if (HasFailure()) {
            return;
        }
    };

    for (const auto &test : tests) {
        testFunc(test);
    }
}

TEST_F(TestJson, Test2)
{
    using TestData = std::vector<std::tuple<std::string, int, std::string, bool>>;
    TestData tests = {
        { "test_json_test1", 123, "abc", true },
        { "test_json_test1", 64645, "gsdfgdsfgdf", true },
    };

    auto testFunc = [](const auto &data) {
        int code = std::get<1>(data);
        std::string msg = std::get<2>(data);
        bool success = std::get<3>(data);
        char *jsonData = nullptr;
        DpApiResult resultData2;
        DpApiResult resultData;
        bool ret = true;

        do {
            resultData.code = code;
            strncpy(resultData.msg, msg.c_str(), DP_API_RESULT_MSG_LEN);

            EXPECT_EQ(dp_result_to_json(&resultData, &jsonData), 0);
            if (HasFailure()) {
                break;
            }

            EXPECT_EQ(dp_json_to_result(jsonData, &resultData2), 0);
            if (HasFailure()) {
                break;
            }

            if (resultData.code != resultData2.code) {
                ret = false;
            }

            if (std::string(resultData.msg) != std::string(resultData2.msg)) {
                ret = false;
            }

            EXPECT_EQ(success, ret);
            if (HasFailure()) {
                break;
            }
        } while (0);

        if (jsonData != nullptr) {
            free(jsonData);
        }
    };

    for (const auto &test : tests) {
        testFunc(test);
    }
}