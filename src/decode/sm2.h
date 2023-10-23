// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <openssl/evp.h>

#ifdef __cplusplus
extern "C" {
#endif

int dp_sm2_key_generate(EVP_PKEY **key);
int dp_sm2_private_key_create_by_string(const unsigned char *keyStr, EVP_PKEY **key);
int dp_sm2_public_key_create_by_string(const unsigned char *keyStr, EVP_PKEY **key);

int dp_sm2_get_public_key(EVP_PKEY *key, unsigned char **publicKey);
int dp_sm2_get_private_key(EVP_PKEY *key, unsigned char **privateKey);

int dp_sm2_decrypt(EVP_PKEY *key, const unsigned char *in, size_t inLen, unsigned char **out, size_t *outLen);
int dp_sm2_encrypt(EVP_PKEY *key, const unsigned char *in, size_t inLen, unsigned char **out, size_t *outLen);
int dp_sm2_sign(EVP_PKEY *key, const unsigned char *in, size_t inLen, unsigned char **sign, size_t *signLen);
int dp_sm2_verify(EVP_PKEY *key, const unsigned char *in, size_t inLen, const unsigned char *sign, size_t signLen);

#ifdef __cplusplus
}
#endif