// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#define SIGNAL_FINISH 1
#define SIGNAL_NOT_FINISH 0

void emit_reset_status(int finish, int status);

void emit_device_detect_status(int finish, int status);

void emit_make_cred_status(const char *user, int finish, int status);

void emit_get_assert_status(const char *user, int finish, int status);
