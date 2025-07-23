// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rsa.h"

#include "common/log.h"

#include <openssl/pem.h>
#include <openssl/rsa.h>

#include <string.h>

int dp_rsa_key_generate(int keyLen, EVP_PKEY **key)
{
    EVP_PKEY_CTX *ctx = NULL;
    EVP_PKEY *newKey = NULL;
    int ret = -1;

    if (key == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_keygen_init.");
        goto end;
    }

    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, keyLen) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_CTX_set_rsa_keygen_bits.");
        goto end;
    }

    if (EVP_PKEY_keygen(ctx, &newKey) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_keygen.");
        goto end;
    }

    *key = newKey;

    ret = 0;
end:
    if (ctx != NULL) {
        EVP_PKEY_CTX_free(ctx);
    }
    if (ret != 0 && newKey != NULL) {
        EVP_PKEY_free(newKey);
    }
    return ret;
}

int dp_rsa_public_key_create_by_string(const unsigned char *keyStr, EVP_PKEY **key)
{
    BIO *bio = NULL;
    RSA *rsa = NULL;
    EVP_PKEY *newKey = NULL;
    int ret = -1;

    if (key == NULL || keyStr == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    bio = BIO_new(BIO_s_mem());
    if (bio == NULL) {
        LOG(LOG_WARNING, "failed to call BIO_new.");
        goto end;
    }

    if (BIO_puts(bio, (char *)keyStr) <= 0) {
        LOG(LOG_WARNING, "failed to call BIO_puts.");
        goto end;
    }

    if (0 == strncmp((char *)keyStr, RSA_PKCS8_HEADER, strlen(RSA_PKCS8_HEADER))) {
        PEM_read_bio_RSA_PUBKEY(bio, &rsa, NULL, NULL);
    } else if (0 == strncmp((char *)keyStr, RSA_PKCS1_HEADER, strlen(RSA_PKCS1_HEADER))) {
        PEM_read_bio_RSAPublicKey(bio, &rsa, NULL, NULL);
    } else {
        LOG(LOG_WARNING, "invalid key.");
        goto end;
    }

    if (rsa == NULL) {
        LOG(LOG_WARNING, "rsa is not exist.");
        goto end;
    }

    newKey = EVP_PKEY_new();
    if (newKey == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_new.");
        goto end;
    }
    if (EVP_PKEY_assign_RSA(newKey, rsa) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_assign_RSA.");
        goto end;
    }
    *key = newKey;

    ret = 0;
end:
    if (bio != NULL) {
        BIO_free(bio);
    }
    if (ret != 0 && newKey != NULL) {
        EVP_PKEY_free(newKey);
    }
    return ret;
}

int dp_rsa_private_key_create_by_string(const unsigned char *keyStr, EVP_PKEY **key)
{
    BIO *bio = NULL;
    RSA *rsa = NULL;
    EVP_PKEY *newKey = NULL;
    int ret = -1;

    if (key == NULL || keyStr == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    bio = BIO_new(BIO_s_mem());
    if (bio == NULL) {
        LOG(LOG_WARNING, "failed to call BIO_new.");
        goto end;
    }

    if (BIO_puts(bio, (char *)keyStr) <= 0) {
        LOG(LOG_WARNING, "failed to call BIO_puts.");
        goto end;
    }

    PEM_read_bio_RSAPrivateKey(bio, &rsa, NULL, NULL);

    if (rsa == NULL) {
        LOG(LOG_WARNING, "failed to call PEM_read_bio_RSAPrivateKey.");
        goto end;
    }

    newKey = EVP_PKEY_new();
    if (newKey == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_new.");
        goto end;
    }
    if (EVP_PKEY_assign_RSA(newKey, rsa) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_assign_RSA.");
        goto end;
    }
    *key = newKey;

    ret = 0;
end:
    if (bio != NULL) {
        BIO_free(bio);
    }
    if (ret != 0 && newKey != NULL) {
        EVP_PKEY_free(newKey);
    }
    return ret;
}

int dp_rsa_get_public_key(EVP_PKEY *key, unsigned char **publicKey)
{
    // 默认用PKCS1
    return dp_rsa_get_public_key_ex(key, RSA_PUBLIC_KEY_FORMAT_PKCS1, publicKey);
}

int dp_rsa_get_public_key_ex(EVP_PKEY *key, int farmot, unsigned char **publicKey)
{
    BIO *bio = NULL;
    RSA *rsa = NULL; // not need free
    unsigned char *pkey = NULL;
    int ret = -1;

    if (key == NULL || publicKey == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    if (key == NULL || publicKey == NULL) {
        LOG(LOG_WARNING, "invalid param.");
        goto end;
    }

    bio = BIO_new(BIO_s_mem());
    if (bio == NULL) {
        LOG(LOG_WARNING, "failed to call BIO_new.");
        goto end;
    }

    rsa = EVP_PKEY_get0_RSA(key);
    if (rsa == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_get0_RSA.");
        goto end;
    }

    if (farmot == RSA_PUBLIC_KEY_FORMAT_PKCS1) {
        if (PEM_write_bio_RSAPublicKey(bio, rsa) <= 0) {
            LOG(LOG_WARNING, "failed to call PEM_write_bio_RSAPublicKey.");
            goto end;
        }
    } else if (farmot == RSA_PUBLIC_KEY_FORMAT_PKCS8) {
        if (PEM_write_bio_RSA_PUBKEY(bio, rsa) <= 0) {
            LOG(LOG_WARNING, "failed to call PEM_write_bio_RSA_PUBKEY.");
            goto end;
        }
    } else {
        goto end;
    }

    size_t len = BIO_pending(bio);
    if (len == 0) {
        LOG(LOG_WARNING, "failed to call BIO_pending.");
        goto end;
    }
    pkey = (unsigned char *)malloc(len + 1);
    if (pkey == NULL) {
        LOG(LOG_WARNING, "failed to call malloc.");
        goto end;
    }
    if (BIO_read(bio, pkey, len) <= 0) {
        LOG(LOG_WARNING, "failed to call BIO_read.");
        goto end;
    }
    pkey[len] = '\0';

    *publicKey = pkey;

    ret = 0;
end:
    if (bio != NULL) {
        BIO_free(bio);
    }
    if (ret != 0 && pkey != NULL) {
        free(pkey);
    }
    return ret;
}

int dp_rsa_get_private_key(EVP_PKEY *key, unsigned char **privateKey)
{
    BIO *bio = NULL;
    RSA *rsa = NULL; // not need free
    unsigned char *pkey = NULL;
    int ret = -1;

    if (key == NULL || privateKey == NULL) {
        LOG(LOG_WARNING, "invalid param.");
        goto end;
    }

    bio = BIO_new(BIO_s_mem());
    if (bio == NULL) {
        LOG(LOG_WARNING, "failed to call BIO_new.");
        goto end;
    }

    rsa = EVP_PKEY_get0_RSA(key);
    if (rsa == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_get0_RSA.");
        goto end;
    }

    if (PEM_write_bio_RSAPrivateKey(bio, rsa, NULL, NULL, 0, 0, NULL) <= 0) {
        LOG(LOG_WARNING, "failed to call PEM_write_bio_RSAPrivateKey.");
        goto end;
    }

    size_t len = BIO_pending(bio);
    if (len == 0) {
        LOG(LOG_WARNING, "failed to call BIO_pending.");
        goto end;
    }
    pkey = (unsigned char *)malloc(len + 1);
    if (pkey == NULL) {
        LOG(LOG_WARNING, "failed to call malloc.");
        goto end;
    }
    if (BIO_read(bio, pkey, len) <= 0) {
        LOG(LOG_WARNING, "failed to call BIO_read.");
        goto end;
    }
    pkey[len] = '\0';

    *privateKey = pkey;

    ret = 0;
end:
    if (bio != NULL) {
        BIO_free(bio);
    }
    if (ret != 0 && pkey != NULL) {
        free(pkey);
    }
    return ret;
}

int dp_rsa_encrypt_ex(EVP_PKEY *key, int padding, const unsigned char *in, size_t inLen, unsigned char **out, size_t *outLen)
{
    int ret = -1;
    unsigned char *outData = NULL;
    size_t outDataLen = 0;
    EVP_PKEY_CTX *ctx = NULL;

    if (key == NULL || in == NULL || out == NULL || outLen == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    ctx = EVP_PKEY_CTX_new(key, NULL);
    if (ctx == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_CTX_new.");
        goto end;
    }

    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_encrypt_init.");
        goto end;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, padding) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_CTX_set_rsa_padding.");
        goto end;
    }
    if (EVP_PKEY_encrypt(ctx, NULL, &outDataLen, in, inLen) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_encrypt.");
        goto end;
    }

    if (outDataLen == 0) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_encrypt.");
        goto end;
    }

    outData = OPENSSL_malloc(outDataLen);
    if (outData == NULL) {
        LOG(LOG_WARNING, "failed to call OPENSSL_malloc.");
        goto end;
    }

    if (EVP_PKEY_encrypt(ctx, outData, &outDataLen, in, inLen) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_encrypt.");
        goto end;
    }

    if (outData == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_encrypt.");
        goto end;
    }

    *out = outData;
    *outLen = outDataLen;

    ret = 0;
end:
    if (ctx != NULL) {
        EVP_PKEY_CTX_free(ctx);
    }
    if (ret != 0 && outData != NULL) {
        OPENSSL_free(outData);
    }
    return ret;
}

int dp_rsa_decrypt_ex(EVP_PKEY *key, int padding, const unsigned char *in, size_t inLen, unsigned char **out, size_t *outLen)
{
    int ret = -1;
    unsigned char *outData = NULL;
    size_t outDataLen = 0;
    EVP_PKEY_CTX *ctx = NULL;

    if (key == NULL || in == NULL || out == NULL || outLen == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    ctx = EVP_PKEY_CTX_new(key, NULL);
    if (ctx == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_CTX_new.");
        goto end;
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_decrypt_init.");
        goto end;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, padding) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_CTX_set_rsa_padding.");
        goto end;
    }

    if (EVP_PKEY_decrypt(ctx, NULL, &outDataLen, in, inLen) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_decrypt.");
        goto end;
    }

    if (outDataLen == 0) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_decrypt.");
        goto end;
    }

    outData = OPENSSL_zalloc(outDataLen+1);
    if (outData == NULL) {
        LOG(LOG_WARNING, "failed to call OPENSSL_malloc.");
        goto end;
    }

    if (EVP_PKEY_decrypt(ctx, outData, &outDataLen, in, inLen) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_decrypt.");
        goto end;
    }

    if (outData == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_decrypt.");
        goto end;
    }

    *out = outData;
    *outLen = outDataLen;

    ret = 0;
end:
    if (ctx != NULL) {
        EVP_PKEY_CTX_free(ctx);
    }
    if (ret != 0 && outData != NULL) {
        OPENSSL_free(outData);
    }
    return ret;
}

int dp_rsa_sign_ex(EVP_PKEY *key, int padding, const EVP_MD *md, const unsigned char *in, size_t inLen, unsigned char **sign, size_t *signLen)
{
    int ret = -1;
    unsigned char *outData = NULL;
    unsigned int outDataLen = 0;
    EVP_MD_CTX *mdCtx = NULL;

    if (key == NULL || in == NULL || sign == NULL || signLen == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    mdCtx = EVP_MD_CTX_new();
    if (mdCtx == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_MD_CTX_new.");
        goto end;
    }

    EVP_MD_CTX_set_flags(mdCtx, padding);

    if (EVP_SignInit_ex(mdCtx, md, NULL) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_SignInit_ex.");
        goto end;
    }

    if (EVP_SignUpdate(mdCtx, in, inLen) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_SignUpdate.");
        goto end;
    }

    if (EVP_SignFinal(mdCtx, NULL, &outDataLen, key) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_SignFinal.");
        goto end;
    }

    if (outDataLen == 0) {
        LOG(LOG_WARNING, "failed to call EVP_SignFinal.");
        goto end;
    }

    outData = OPENSSL_malloc(outDataLen);
    if (outData == NULL) {
        LOG(LOG_WARNING, "failed to call OPENSSL_malloc.");
        goto end;
    }

    if (EVP_SignFinal(mdCtx, outData, &outDataLen, key) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_SignFinal.");
        goto end;
    }

    if (outData == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_SignFinal.");
        goto end;
    }

    *sign = outData;
    *signLen = outDataLen;

    ret = 0;
end:
    if (mdCtx != NULL) {
        EVP_MD_CTX_free(mdCtx);
    }
    if (ret != 0 && outData != NULL) {
        OPENSSL_free(outData);
    }
    return ret;
}

int dp_rsa_verify_ex(EVP_PKEY *key, int padding, const EVP_MD *md, const unsigned char *in, size_t inLen, const unsigned char *sign, size_t signLen)
{
    int ret = -1;
    EVP_MD_CTX *mdCtx = NULL;

    if (key == NULL || in == NULL || sign == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    mdCtx = EVP_MD_CTX_new();
    if (mdCtx == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_MD_CTX_new.");
        goto end;
    }

    EVP_MD_CTX_set_flags(mdCtx, padding);

    if (EVP_VerifyInit_ex(mdCtx, md, NULL) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_VerifyInit_ex.");
        goto end;
    }

    if (EVP_VerifyUpdate(mdCtx, in, inLen) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_VerifyUpdate.");
        goto end;
    }

    if (EVP_VerifyFinal(mdCtx, sign, signLen, key) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_VerifyFinal.");
        goto end;
    }

    ret = 0;
end:
    if (mdCtx != NULL) {
        EVP_MD_CTX_free(mdCtx);
    }
    return ret;
}

int dp_rsa_encrypt(EVP_PKEY *key, const unsigned char *in, size_t inLen, unsigned char **out, size_t *outLen)
{
    // 默认使用RSA_PKCS1_PADDING
    return dp_rsa_encrypt_ex(key, RSA_PKCS1_PADDING, in, inLen, out, outLen);
}

int dp_rsa_decrypt(EVP_PKEY *key, const unsigned char *in, size_t inLen, unsigned char **out, size_t *outLen)
{
    // 默认使用RSA_PKCS1_PADDING
    return dp_rsa_decrypt_ex(key, RSA_PKCS1_PADDING, in, inLen, out, outLen);
}

int dp_rsa_sign(EVP_PKEY *key, const unsigned char *in, size_t inLen, unsigned char **sign, size_t *signLen)
{
    return dp_rsa_sign_ex(key, EVP_MD_CTX_FLAG_PAD_PKCS1, EVP_sha256(), in, inLen, sign, signLen);
}

int dp_rsa_verify(EVP_PKEY *key, const unsigned char *in, size_t inLen, const unsigned char *sign, size_t signLen)
{
    return dp_rsa_verify_ex(key, EVP_MD_CTX_FLAG_PAD_PKCS1, EVP_sha256(), in, inLen, sign, signLen);
}