// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <openssl/evp.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EVP_TYPE_ENC 1
#define EVP_TYPE_DEC 0
#define EVP_TYPE_NONE (-1)

unsigned char *dp_gen_symmetric_key_16();

int dp_evp_deal(const EVP_CIPHER *cip, int padding, int enc, const unsigned char *key, const unsigned char *iv, const unsigned char *in, int inLen, unsigned char **out, int *outLen);

void printOpenSSLError();

#ifdef __cplusplus
}
#endif