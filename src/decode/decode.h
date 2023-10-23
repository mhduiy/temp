// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define DP_SUPPORT_AES_ECB128 11100
#define DP_SUPPORT_SM4_ECB 12100
#define DP_SUPPORT_RSA_2048_PKCS1_PADDING_PKCS1 20120
#define DP_SUPPORT_RSA_4096_PKCS1_PADDING_PKCS1 20130
#define DP_SUPPORT_RSA_2048_PKCS8_PADDING_PKCS1 20220
#define DP_SUPPORT_RSA_4096_PKCS8_PADDING_PKCS1 20230
#define DP_SUPPORT_SM2_SM3 21100

int dp_get_default_asym_key_type();
int dp_get_default_sym_key_type();

// 对外统一封装，外部只需要指定算法类型，不用区分算法
int dp_asym_key_check_support(int type);
int dp_asym_key_create_private(int type, unsigned char **privateKey);
int dp_asym_key_private_to_public(int type, const unsigned char *privateKey, unsigned char **publicKey);
int dp_asym_key_decrypt(int type, const unsigned char *privateKey, const unsigned char *in, int inLen, unsigned char **out, int *outLen);

int dp_sym_key_check_support(int type);
int dp_sym_key_decrypt(int type, const unsigned char *key, const unsigned char *in, int inLen, unsigned char **out, int *outLen);

#ifdef __cplusplus
}
#endif