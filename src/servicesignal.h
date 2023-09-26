// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "serviceframe/servicebase.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SIGNAL_FINISH 1
#define SIGNAL_NOT_FINISH 0

void emit_reset_status(MethodContext *mc, int finish, int status);

void emit_device_detect_status(MethodContext *mc, int finish, int status);

void emit_make_cred_status(MethodContext *mc, const char *user, int finish, int status);

void emit_get_assert_status(MethodContext *mc, const char *user, int finish, int status);

#ifdef __cplusplus
}
#endif
