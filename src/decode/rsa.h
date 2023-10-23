// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <openssl/evp.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RSA_PUBLIC_KEY_FORMAT_PKCS1 1
#define RSA_PUBLIC_KEY_FORMAT_PKCS8 2

#define RSA_PKCS1_HEADER "-----BEGIN RSA PUBLIC KEY-----"
#define RSA_PKCS8_HEADER "-----BEGIN PUBLIC KEY-----"

int dp_rsa_key_generate(int keyLen, EVP_PKEY **key);
int dp_rsa_private_key_create_by_string(const unsigned char *keyStr, EVP_PKEY **key);
int dp_rsa_public_key_create_by_string(const unsigned char *keyStr, EVP_PKEY **key);

int dp_rsa_get_public_key(EVP_PKEY *key, unsigned char **publicKey);
int dp_rsa_get_private_key(EVP_PKEY *key, unsigned char **privateKey);

int dp_rsa_encrypt(EVP_PKEY *key, const unsigned char *in, size_t inLen, unsigned char **out, size_t *outLen);
int dp_rsa_decrypt(EVP_PKEY *key, const unsigned char *in, size_t inLen, unsigned char **out, size_t *outLen);
int dp_rsa_sign(EVP_PKEY *key, const unsigned char *in, size_t inLen, unsigned char **sign, size_t *signLen);
int dp_rsa_verify(EVP_PKEY *key, const unsigned char *in, size_t inLen, const unsigned char *sign, size_t signLen);

// ex
int dp_rsa_get_public_key_ex(EVP_PKEY *key, int farmot, unsigned char **publicKey);
int dp_rsa_encrypt_ex(EVP_PKEY *key, int padding, const unsigned char *in, size_t inLen, unsigned char **out, size_t *outLen);
int dp_rsa_decrypt_ex(EVP_PKEY *key, int padding, const unsigned char *in, size_t inLen, unsigned char **out, size_t *outLen);
int dp_rsa_sign_ex(EVP_PKEY *key, int padding, const EVP_MD *md, const unsigned char *in, size_t inLen, unsigned char **sign, size_t *signLen);
int dp_rsa_verify_ex(EVP_PKEY *key, int padding, const EVP_MD *md, const unsigned char *in, size_t inLen, const unsigned char *sign, size_t signLen);

#ifdef __cplusplus
}
#endif
