#include <gtest/gtest.h>

#include "global_test_env.h"

class TestDecode : public testing::Test
{
public:
    void SetUp() override { ASSERT_NE(TestEnv::testWorkDir, ""); }

    void TearDown() override { }
};
