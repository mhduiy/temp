// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "global_test_env.h"

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    AddGlobalTestEnvironment(new TestEnv());

    return RUN_ALL_TESTS();
}