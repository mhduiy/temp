// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "serviceframe/servicebase.h"

#include <fido.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

int dpk_manager_get_pin_status(MethodContext *mc, int *status);

int dpk_manager_set_pin(MethodContext *mc, const char *pin, const char *oldPin);

int dpk_manager_reset(MethodContext *mc);

int dpk_manager_make_cred(MethodContext *mc, const char *userName, const char *credName, const char *pin);

int dpk_manager_get_assertion(MethodContext *mc, const char *userName, const char *credName, const char *pin);

int dpk_manager_get_valid_cred_count(MethodContext *mc, const char *userName, const char *pin, unsigned int *validCredCount);

int dpk_manager_get_creds(const char *userName, char **creds, unsigned int *credCount);

int dpk_manager_get_device_count(int *count);

int dpk_manager_device_detect(MethodContext *mc, int timeout, int stopWhenExist, int stopWhenNotExist);

int dpk_manager_select(MethodContext *mc);

int dpk_manager_select_close(MethodContext *mc);

int dpk_manager_devices_close(MethodContext *mc, const char *callId);

int dpk_manager_encrypt_get_public(MethodContext *mc, int type, char **publicKey);

int dpk_manager_encrypt_set_symmetric_key(MethodContext *mc, int encType, int keyType, const char *key);

#ifdef __cplusplus
}
#endif