// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int dp_sm4_ecb_encrypt(const unsigned char *key, const unsigned char *in, int inLen, unsigned char **out, int *outLen);

int dp_sm4_ecb_decrypt(const unsigned char *key, const unsigned char *in, int inLen, unsigned char **out, int *outLen);

int dp_sm4_cbc_encrypt(const unsigned char *key, const unsigned char *iv, const unsigned char *in, int inLen, unsigned char **out, int *outLen);

int dp_sm4_cbc_decrypt(const unsigned char *key, const unsigned char *iv, const unsigned char *in, int inLen, unsigned char **out, int *outLen);

#ifdef __cplusplus
}
#endif