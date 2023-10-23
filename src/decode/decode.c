// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "decode.h"

#include "aes.h"
#include "rsa.h"
#include "sm2.h"
#include "sm4.h"

int dp_get_default_asym_key_type()
{
    return DP_SUPPORT_SM2_SM3;
}

int dp_get_default_sym_key_type()
{
    return DP_SUPPORT_SM4_ECB;
}

int dp_asym_key_check_support(int type)
{
    if (type == DP_SUPPORT_RSA_2048_PKCS1_PADDING_PKCS1 || type == DP_SUPPORT_RSA_2048_PKCS8_PADDING_PKCS1 || type == DP_SUPPORT_SM2_SM3) {
        return 0;
    }
    return -1;
}

int dp_asym_key_create_private(int type, unsigned char **privateKey)
{
    EVP_PKEY *priKey = NULL;
    unsigned char *priKeyData = NULL;
    int ret = -1;

    if (type == DP_SUPPORT_RSA_2048_PKCS1_PADDING_PKCS1 || type == DP_SUPPORT_RSA_2048_PKCS8_PADDING_PKCS1) {
        //
        if (dp_rsa_key_generate(2048, &priKey) < 0) {
            // error
            goto end;
        }
        if (dp_rsa_get_private_key(priKey, &priKeyData) < 0) {
            // error
            goto end;
        }
    } else if (type == DP_SUPPORT_SM2_SM3) {
        //
        if (dp_sm2_key_generate(&priKey) < 0) {
            // error
            goto end;
        }
        if (dp_sm2_get_private_key(priKey, &priKeyData) < 0) {
            // error
            goto end;
        }
    } else {
        goto end;
    }
    *privateKey = priKeyData;
    ret = 0;
end:
    if (priKey != NULL) {
        EVP_PKEY_free(priKey);
    }
    if (ret != 0 && priKeyData != NULL) {
        free(priKeyData);
    }
    return ret;
}

int dp_asym_key_private_to_public(int type, const unsigned char *privateKey, unsigned char **publicKey)
{
    EVP_PKEY *priKey = NULL;
    int ret = -1;
    unsigned char *pubKey = NULL;

    if (type == DP_SUPPORT_RSA_2048_PKCS1_PADDING_PKCS1) {
        if (dp_rsa_private_key_create_by_string(privateKey, &priKey) < 0) {
            goto end;
        }
        if (dp_rsa_get_public_key_ex(priKey, RSA_PUBLIC_KEY_FORMAT_PKCS1, &pubKey) < 0) {
            goto end;
        }
    } else if (type == DP_SUPPORT_RSA_2048_PKCS8_PADDING_PKCS1) {
        if (dp_rsa_private_key_create_by_string(privateKey, &priKey) < 0) {
            goto end;
        }
        if (dp_rsa_get_public_key_ex(priKey, RSA_PUBLIC_KEY_FORMAT_PKCS8, &pubKey) < 0) {
            goto end;
        }
    } else if (type == DP_SUPPORT_SM2_SM3) {
        if (dp_sm2_private_key_create_by_string(privateKey, &priKey) < 0) {
            goto end;
        }
        if (dp_sm2_get_public_key(priKey, &pubKey) < 0) {
            goto end;
        }
    } else {
        goto end;
    }
    *publicKey = pubKey;
    ret = 0;
end:
    if (priKey != NULL) {
        EVP_PKEY_free(priKey);
    }
    if (ret != 0 && pubKey != NULL) {
        free(pubKey);
    }
    return ret;
}

int dp_asym_key_decrypt(int type, const unsigned char *privateKey, const unsigned char *in, int inLen, unsigned char **out, int *outLen)
{
    EVP_PKEY *priKey = NULL;
    unsigned char *outData = NULL;
    size_t outDataLen = 0;
    int ret = -1;

    if (type == DP_SUPPORT_RSA_2048_PKCS1_PADDING_PKCS1) {
        if (dp_rsa_private_key_create_by_string(privateKey, &priKey) < 0) {
            goto end;
        }
        if (dp_rsa_decrypt_ex(priKey, RSA_PUBLIC_KEY_FORMAT_PKCS1, in, inLen, &outData, &outDataLen) < 0) {
            goto end;
        }
    } else if (type == DP_SUPPORT_RSA_2048_PKCS8_PADDING_PKCS1) {
        if (dp_rsa_private_key_create_by_string(privateKey, &priKey) < 0) {
            goto end;
        }
        if (dp_rsa_decrypt_ex(priKey, RSA_PUBLIC_KEY_FORMAT_PKCS8, in, inLen, &outData, &outDataLen) < 0) {
            goto end;
        }
    } else if (type == DP_SUPPORT_SM2_SM3) {
        if (dp_sm2_private_key_create_by_string(privateKey, &priKey) < 0) {
            goto end;
        }
        if (dp_sm2_decrypt(priKey, in, inLen, &outData, &outDataLen) < 0) {
            goto end;
        }
    } else {
        goto end;
    }
    *out = outData;
    *outLen = outDataLen;
    ret = 0;
end:
    if (priKey != NULL) {
        EVP_PKEY_free(priKey);
    }
    if (ret != 0 && outData != NULL) {
        free(outData);
    }
    return ret;
}

int dp_sym_key_check_support(int type)
{
    if (type == DP_SUPPORT_AES_ECB128 || type == DP_SUPPORT_SM4_ECB) {
        return 0;
    }
    return -1;
}

int dp_sym_key_decrypt(int type, const unsigned char *key, const unsigned char *in, int inLen, unsigned char **out, int *outLen)
{
    int ret = -1;
    unsigned char *outData = NULL;
    int outDataLen = 0;

    if (type == DP_SUPPORT_AES_ECB128) {
        if (dp_aes_ecb128_decrypt(key, in, inLen, &outData, &outDataLen) < 0) {
            goto end;
        }
    } else if (type == DP_SUPPORT_SM4_ECB) {
        if (dp_sm4_ecb_decrypt(key, in, inLen, &outData, &outDataLen) < 0) {
            goto end;
        }
    } else {
        goto end;
    }
    *out = outData;
    *outLen = outDataLen;
    ret = 0;
end:
    if (ret != 0 && outData != NULL) {
        free(outData);
    }
    return ret;
}
