#include <gtest/gtest.h>

#include "common/common.h"
#include "global_test_env.h"

#include <fstream>
#include <iostream>
#include <tuple>
#include <vector>

#include <stdbool.h>

class TestCommon : public testing::Test
{
public:
    void SetUp() override { ASSERT_NE(TestEnv::testWorkDir, ""); }

    void TearDown() override { }
};

TEST_F(TestCommon, RandomBytes)
{
    unsigned char data[32] = { 0 };
    ASSERT_TRUE(random_bytes(data, sizeof(data)));
}

TEST_F(TestCommon, GetUserId)
{
    using TestData = std::vector<std::tuple<std::string, std::string, bool, std::string>>;
    TestData tests = { { "user_test1",
                         "92255d43-016b-449b-9189-74150f6e25b5",
                         true,
                         R"([User]
XSession=deepin
SystemAccount=false
Layout=cn;
Locale=zh_CN.UTF-8
Icon="file:///var/lib/AccountsService/icons/6.png"
HistoryLayout=cn\;;
UUID=92255d43-016b-449b-9189-74150f6e25b5
Workspace=test
)" },
                       { "user_test2",
                         "",
                         true,
                         R"([User]
XSession=deepin
SystemAccount=false
Layout=cn;
Locale=zh_CN.UTF-8
Icon=file:///var/lib/AccountsService/icons/6.png
HistoryLayout=cn\;;
UUID=
Workspace=test
)" },
                       { "user_test3",
                         "user_test3",
                         true,
                         R"([User]
XSession=deepin
)" },
                       { "user_test4",
                         "92255d43-016b-449b-9189-",
                         false,
                         R"([User]
XSession=deepin
SystemAccount=false
Layout=cn;
Locale=zh_CN.UTF-8
Icon="file:///var/lib/AccountsService/icons/6.png"
HistoryLayout=cn\;;
UUID=92255d43-016b-449b-9189-74150f6e25b5
Workspace=test
)" } };

    auto testFunc = [](const auto &data) {
        std::ofstream userFile(TestEnv::testWorkDir + "/" + std::get<0>(data));
        userFile << std::get<3>(data);
        userFile.close();

        char *id = nullptr;
        do {
            EXPECT_TRUE(get_user_id_from_path(std::get<0>(data).c_str(), TestEnv::testWorkDir.c_str(), &id));
            if (HasFailure()) {
                break;
            }
            bool success = std::get<2>(data);
            if (success) {
                EXPECT_STREQ(std::get<1>(data).c_str(), id);
            } else {
                EXPECT_STRNE(std::get<1>(data).c_str(), id);
            }
            if (HasFailure()) {
                break;
            }
        } while (false);
        if (id != nullptr) {
            free(id);
        }
    };

    for (const auto &test : tests) {
        testFunc(test);
    }
}

TEST_F(TestCommon, ReadFileLine)
{
    using TestData = std::vector<std::tuple<std::string, std::string, bool, std::string>>;
    TestData tests = { { "test_read_file_line_file1", "abc", true, R"(abc
)" },
                       { "test_read_file_line_file2", "abc", true, R"(abc)" },
                       { "test_read_file_line_file4", "111", false, R"(222)" },
                       { "test_read_file_line_file5",
                         "abc",
                         true,
                         R"(abc
222
333
)" } };

    auto testFunc = [](const auto &data) {
        std::string path = TestEnv::testWorkDir + "/" + std::get<0>(data);
        std::ofstream userFile(path);
        userFile << std::get<3>(data);
        userFile.close();
        char *value = nullptr;
        do {
            EXPECT_TRUE(read_file_line(path.c_str(), &value));
            if (HasFailure()) {
                break;
            }
            bool success = std::get<2>(data);
            if (success) {
                EXPECT_STREQ(std::get<1>(data).c_str(), value);
            } else {
                EXPECT_STRNE(std::get<1>(data).c_str(), value);
            }
            if (HasFailure()) {
                break;
            }
        } while (false);
        if (value != nullptr) {
            free(value);
        }
    };

    for (const auto &test : tests) {
        testFunc(test);
    }
}
