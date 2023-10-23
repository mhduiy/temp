// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sm2.h"

#include "common/log.h"

#include <openssl/ec.h>
#include <openssl/pem.h>
#include <openssl/sm2.h>

int dp_sm2_key_generate(EVP_PKEY **key)
{
    EC_KEY *ecKey;
    EC_GROUP *ecGroup;
    EVP_PKEY *newKey = NULL;
    int ret = -1;

    if (key == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    ecKey = EC_KEY_new();
    if (ecKey == NULL) {
        LOG(LOG_WARNING, "failed to call EC_KEY_new.");
        goto end;
    }
    ecGroup = EC_GROUP_new_by_curve_name(NID_sm2);
    if (ecGroup == NULL) {
        LOG(LOG_WARNING, "failed to call EC_GROUP_new_by_curve_name.");
        goto end;
    }

    if (EC_KEY_set_group(ecKey, ecGroup) <= 0) {
        LOG(LOG_WARNING, "failed to call EC_KEY_set_group.");
        goto end;
    }

    if (EC_KEY_generate_key(ecKey) <= 0) {
        LOG(LOG_WARNING, "failed to call EC_KEY_generate_key.");
        goto end;
    }

    newKey = EVP_PKEY_new();
    if (newKey == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_new.");
        goto end;
    }
    if (EVP_PKEY_set1_EC_KEY(newKey, ecKey) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_set1_EC_KEY.");
        goto end;
    }
    *key = newKey;

    ret = 0;
end:
    if (ecGroup != NULL) {
        EC_GROUP_free(ecGroup);
    }
    if (ecKey != NULL) {
        EC_KEY_free(ecKey);
    }
    if (ret != 0 && newKey != NULL) {
        EVP_PKEY_free(newKey);
    }
    return ret;
}

int dp_sm2_get_public_key(EVP_PKEY *key, unsigned char **publicKey)
{
    BIO *bio = NULL;
    EC_KEY *ecKey = NULL; // not need free
    unsigned char *pkey = NULL;
    int ret = -1;

    if (key == NULL || publicKey == NULL) {
        LOG(LOG_WARNING, "invalid param.");
        goto end;
    }

    bio = BIO_new(BIO_s_mem());
    if (bio == NULL) {
        LOG(LOG_WARNING, "failed to call BIO_new.");
        goto end;
    }

    ecKey = EVP_PKEY_get0_EC_KEY(key);
    if (ecKey == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_get0_EC_KEY.");
        goto end;
    }

    if (PEM_write_bio_EC_PUBKEY(bio, ecKey) <= 0) {
        LOG(LOG_WARNING, "failed to call PEM_write_bio_EC_PUBKEY.");
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

int dp_sm2_get_private_key(EVP_PKEY *key, unsigned char **privateKey)
{
    BIO *bio = NULL;
    EC_KEY *ecKey = NULL; // not need free
    unsigned char *pkey = NULL;
    int ret = -1;

    if (key == NULL || privateKey == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    bio = BIO_new(BIO_s_mem());
    if (bio == NULL) {
        LOG(LOG_WARNING, "failed to call BIO_new.");
        goto end;
    }

    ecKey = EVP_PKEY_get0_EC_KEY(key);
    if (ecKey == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_get0_EC_KEY.");
        goto end;
    }

    if (PEM_write_bio_ECPrivateKey(bio, ecKey, NULL, NULL, 0, NULL, NULL) <= 0) {
        LOG(LOG_WARNING, "failed to call PEM_write_bio_ECPrivateKey.");
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

int dp_sm2_public_key_create_by_string(const unsigned char *keyStr, EVP_PKEY **key)
{
    BIO *bio = NULL;
    EVP_PKEY *newKey = NULL;
    EC_KEY *ecKey = NULL;
    int ret = -1;

    if (keyStr == NULL || key == NULL) {
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

    ecKey = (EC_KEY *)PEM_read_bio_EC_PUBKEY(bio, NULL, NULL, NULL);
    if (ecKey == NULL) {
        LOG(LOG_WARNING, "failed to call PEM_read_bio_PUBKEY.");
        goto end;
    }

    newKey = EVP_PKEY_new();
    if (newKey == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_new.");
        goto end;
    }
    if (EVP_PKEY_set1_EC_KEY(newKey, ecKey) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_set1_EC_KEY.");
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
    if (ecKey != NULL) {
        EC_KEY_free(ecKey);
    }
    return ret;
}

int dp_sm2_private_key_create_by_string(const unsigned char *keyStr, EVP_PKEY **key)
{
    BIO *bio = NULL;
    EVP_PKEY *newKey = NULL;
    EC_KEY *ecKey = NULL;
    int ret = -1;

    if (keyStr == NULL || key == NULL) {
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

    ecKey = (EC_KEY *)PEM_read_bio_ECPrivateKey(bio, NULL, NULL, NULL);
    if (ecKey == NULL) {
        LOG(LOG_WARNING, "failed to call PEM_read_bio_ECPrivateKey.");
        goto end;
    }

    newKey = EVP_PKEY_new();
    if (newKey == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_new.");
        goto end;
    }
    if (EVP_PKEY_set1_EC_KEY(newKey, ecKey) <= 0) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_set1_EC_KEY.");
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
    if (ecKey != NULL) {
        EC_KEY_free(ecKey);
    }
    return ret;
}

int dp_sm2_encrypt(EVP_PKEY *key, const unsigned char *in, size_t inLen, unsigned char **out, size_t *outLen)
{
    int ret = -1;
    unsigned char *outData = NULL;
    size_t outDataLen = 0;
    EC_KEY *ecKey = NULL; // not need free

    if (in == NULL || key == NULL || out == NULL || outLen == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    ecKey = EVP_PKEY_get0_EC_KEY(key);
    if (ecKey == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_get0_EC_KEY.");
        goto end;
    }

    if (sm2_ciphertext_size(ecKey, EVP_sm3(), inLen, &outDataLen) <= 0) {
        LOG(LOG_WARNING, "failed to call sm2_ciphertext_size.");
        goto end;
    }

    outData = OPENSSL_malloc(outDataLen);
    if (outData == NULL) {
        LOG(LOG_WARNING, "failed to call OPENSSL_malloc.");
        goto end;
    }

    if (sm2_encrypt(ecKey, EVP_sm3(), in, inLen, outData, &outDataLen) <= 0) {
        LOG(LOG_WARNING, "failed to call sm2_encrypt.");
        goto end;
    }
    *out = outData;
    *outLen = outDataLen;
    ret = 0;
end:
    if (ret != 0 && outData != NULL) {
        OPENSSL_free(outData);
    }
    return ret;
}

int dp_sm2_decrypt(EVP_PKEY *key, const unsigned char *in, size_t inLen, unsigned char **out, size_t *outLen)
{
    int ret = -1;
    unsigned char *outData = NULL;
    size_t outDataLen = 0;
    EC_KEY *ecKey = NULL; // not need free

    if (in == NULL || key == NULL || out == NULL || outLen == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    ecKey = EVP_PKEY_get0_EC_KEY(key);
    if (ecKey == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_get0_EC_KEY.");
        goto end;
    }

    if (sm2_plaintext_size(in, inLen, &outDataLen) <= 0) {
        LOG(LOG_WARNING, "failed to call sm2_plaintext_size.");
        goto end;
    }
    outData = OPENSSL_malloc(outDataLen);
    if (outData == NULL) {
        LOG(LOG_WARNING, "failed to call OPENSSL_malloc.");
        goto end;
    }

    if (sm2_decrypt(ecKey, EVP_sm3(), in, inLen, outData, &outDataLen) <= 0) {
        LOG(LOG_WARNING, "failed to call sm2_decrypt.");
        goto end;
    }

    *out = outData;
    *outLen = outDataLen;
    ret = 0;
end:
    if (ret != 0 && outData != NULL) {
        OPENSSL_free(outData);
    }
    return ret;
}

int dp_sm2_sign(EVP_PKEY *key, const unsigned char *in, size_t inLen, unsigned char **sign, size_t *signLen)
{
    int ret = -1;
    unsigned char *outData = NULL;
    unsigned int outDataLen = 0;
    EC_KEY *ecKey = NULL; // not need free

    if (in == NULL || key == NULL || sign == NULL || signLen == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    ecKey = EVP_PKEY_get0_EC_KEY(key);
    if (ecKey == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_get0_EC_KEY.");
        goto end;
    }

    outData = OPENSSL_malloc(256);
    if (outData == NULL) {
        LOG(LOG_WARNING, "failed to call OPENSSL_malloc.");
        goto end;
    }

    if (sm2_sign(in, inLen, outData, &outDataLen, ecKey) <= 0) {
        LOG(LOG_WARNING, "failed to call sm2_sign.");
        goto end;
    }

    *sign = outData;
    *signLen = outDataLen;
    ret = 0;
end:
    if (ret != 0 && outData != NULL) {
        OPENSSL_free(outData);
    }
    return ret;
}

int dp_sm2_verify(EVP_PKEY *key, const unsigned char *in, size_t inLen, const unsigned char *sign, size_t signLen)
{
    int ret = -1;
    EC_KEY *ecKey = NULL; // not need free

    if (in == NULL || key == NULL || sign == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    ecKey = EVP_PKEY_get0_EC_KEY(key);
    if (ecKey == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_get0_EC_KEY.");
        goto end;
    }

    if (sm2_verify(in, inLen, sign, signLen, ecKey) <= 0) {
        LOG(LOG_WARNING, "failed to call sm2_verify.");
        goto end;
    }

    ret = 0;
end:

    return ret;
}