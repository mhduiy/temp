// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sm4.h"

#include "evp.h"

int dp_sm4_ecb_encrypt(const unsigned char *key, const unsigned char *in, int inLen, unsigned char **out, int *outLen)
{
    return dp_evp_deal(EVP_sm4_ecb(), -1, EVP_TYPE_ENC, key, NULL, in, inLen, out, outLen);
}

int dp_sm4_ecb_decrypt(const unsigned char *key, const unsigned char *in, int inLen, unsigned char **out, int *outLen)
{
    return dp_evp_deal(EVP_sm4_ecb(), -1, EVP_TYPE_DEC, key, NULL, in, inLen, out, outLen);
}

int dp_sm4_cbc_encrypt(const unsigned char *key, const unsigned char *iv, const unsigned char *in, int inLen, unsigned char **out, int *outLen)
{
    return dp_evp_deal(EVP_sm4_cbc(), -1, EVP_TYPE_ENC, key, iv, in, inLen, out, outLen);
}

int dp_sm4_cbc_decrypt(const unsigned char *key, const unsigned char *iv, const unsigned char *in, int inLen, unsigned char **out, int *outLen)
{
    return dp_evp_deal(EVP_sm4_cbc(), -1, EVP_TYPE_DEC, key, iv, in, inLen, out, outLen);
}
