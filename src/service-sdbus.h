// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// 基于systemd的sdbus方案，由于需要多线程异步接口问题，不使用sdbus，暂保留代码备份，下个提交删除
#ifdef SDBUS_SERVICE

#  include <systemd/sd-bus.h>

#  include <stdbool.h>

bool dpk_service_start();
#endif