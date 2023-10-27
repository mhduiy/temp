// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gtest/gtest.h>

class TestEnv : public ::testing::Environment
{
public:
    void SetUp() override;

    void TearDown() override;

    static std::string testWorkDir;
};