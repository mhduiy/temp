// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/log.h"
#include "service.h"

#include <libintl.h>

#include <locale.h>
#include <stdio.h>

int main()
{
    LOG(LOG_INFO, "deepin-passkey start.");

    setlocale(LC_ALL, "");
    textdomain("deepin-passkey-service");

    dpk_service_start();

    LOG(LOG_INFO, "deepin-passkey exit.");

    return 0;
}
