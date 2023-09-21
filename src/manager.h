// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdbool.h>

int dpk_manager_get_pin_status(int *status);

int dpk_manager_set_pin(const char *pin, const char *oldPin);

int dpk_manager_reset();

int dpk_manager_make_cred(const char *userName, const char *credName, const char *pin);

int dpk_manager_get_assertion(const char *userName, const char *credName, const char *pin);

int dpk_manager_get_valid_cred_count(const char *userName, const char *pin, unsigned int *validCredCount);

int dpk_manager_get_creds(const char *userName, char **creds, unsigned int *credCount);

int dpk_manager_get_device_count(int *count);

int dpk_manager_device_detect(int timeout, int stopWhenExist, int stopWhenNotExist);

int dpk_manager_test();