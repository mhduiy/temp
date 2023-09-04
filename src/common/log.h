// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once


#include <sys/syslog.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
syslog - priority, see syslog.h
LOG_EMERG 系统不可用
LOG_ALERT 消息需立即处理
LOG_CRIT 重要情况
LOG_ERR 错误
LOG_WARNING 警告
LOG_NOTICE 正常情况，但较为重要
LOG_INFO 信息
LOG_DEBUG 调试信息

syslog - facility, see syslog.h
LOG_DAEMON 独立进程用这个
LOG_AUTH PAM用这个
*/


void log_print(const char *id, int priority, const char *function, const int line, const char *format, ...);

#define LOG(priority, text, ...) log_print("deepin-passkey", priority, __FUNCTION__, __LINE__, text, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
