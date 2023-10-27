// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "common/json.h"
#include "global_test_env.h"
#include "service/err.h"

#include <tuple>
#include <vector>

#include <stdbool.h>

class TestErrCode : public testing::Test
{
public:
    void SetUp() override { ASSERT_NE(TestEnv::testWorkDir, ""); }

    void TearDown() override { }
};

TEST_F(TestErrCode, Test1)
{
    using TestData = std::vector<std::tuple<std::string, int, std::string, int, std::string, bool>>;
    TestData tests = {
        { "test_json_test1", DEEPIN_ERR_DEVICE_OPEN, R"({"code":123, "msg":"abc"})", 123, "abc", true },
    };

    auto testFunc = [](const auto &data) {
        int errCode = std::get<1>(data);
        std::string jsonData = std::get<2>(data);
        int code = std::get<3>(data);
        std::string msg = std::get<4>(data);
        bool success = std::get<5>(data);
        bool ret = true;

        char *json = nullptr;

        dp_err_code_to_json(errCode, &json);

        printf("json:%s\n", json); // TODO
    };

    for (const auto &test : tests) {
        testFunc(test);
    }
}
