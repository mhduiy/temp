// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "evp.h"

int dp_aes_ecb128_encrypt(const unsigned char *key, const unsigned char *in, int inLen, unsigned char **out, int *outLen)
{
    return dp_evp_deal(EVP_aes_128_ecb(), EVP_PADDING_PKCS7, EVP_TYPE_ENC, key, NULL, in, inLen, out, outLen);
}

int dp_aes_ecb128_decrypt(const unsigned char *key, const unsigned char *in, int inLen, unsigned char **out, int *outLen)
{
    return dp_evp_deal(EVP_aes_128_ecb(), EVP_PADDING_PKCS7, EVP_TYPE_DEC, key, NULL, in, inLen, out, outLen);
}
