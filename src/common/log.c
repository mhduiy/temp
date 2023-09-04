// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/log.h"

#include <stdarg.h>
#include <stdio.h>

void log_print(const char *id, int priority, const char *function, const int line, const char *format, ...)
{
    if (id != NULL) {
        openlog(id, LOG_PID, LOG_DAEMON);
    } else {
        openlog("deepin-passkey", LOG_PID, LOG_DAEMON);
    }
    va_list ap;
    va_start(ap, format);

    do {
        char buf[1024] = { 0 };
        int len = snprintf(buf, 1024, "[%s:%d]", function, line);
        if (len < 0) {
            syslog(LOG_ERR, "da_log_print error: len invalid.");
            break;
        }
        vsnprintf(buf + len, 1024 - len, format, ap);
        syslog(priority, buf);
    } while (0);
    closelog();
    va_end(ap);
}
