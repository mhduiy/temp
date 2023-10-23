// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int dp_aes_ecb128_encrypt(const unsigned char *key, const unsigned char *in, int inLen, unsigned char **out, int *outLen);
int dp_aes_ecb128_decrypt(const unsigned char *key, const unsigned char *in, int inLen, unsigned char **out, int *outLen);

#ifdef __cplusplus
}
#endif