// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "evp.h"

#include "common/log.h"

#include <openssl/err.h>

#include <string.h>

unsigned char *dp_gen_symmetric_key_16()
{
    char *key = (char *)malloc(20);
    srand(time(NULL));
    int rand_num = (10000000 + rand() % 10000000) % 100000000;
    sprintf(key, "%d%d", rand_num, rand_num);

    return (unsigned char *)key;
}

int dp_evp_deal(const EVP_CIPHER *cip, int padding, int enc, const unsigned char *key, const unsigned char *iv, const unsigned char *in, int inLen, unsigned char **out, int *outLen)
{
    EVP_CIPHER_CTX *ctx = NULL;
    int ret = -1;

    if (cip == NULL || key == NULL || in == NULL || out == NULL || outLen == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    if (strlen((char *)key) == 0 || strlen((char *)in) == 0) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_CIPHER_CTX_new.");
        goto end;
    }
    int initRet = EVP_CipherInit(ctx, cip, key, iv, enc);
    if (1 != initRet) {
        // 1 success, 0 failed
        LOG(LOG_WARNING, "failed to call EVP_CipherInit.");
        goto end;
    }

    if (padding > 0) {
        // 可选
        if (!EVP_CIPHER_CTX_set_padding(ctx, padding)) {
            LOG(LOG_WARNING, "failed to call EVP_CIPHER_CTX_set_padding.");
            goto end;
        }
    }

    unsigned char *outBuf = (unsigned char *)calloc(sizeof(unsigned char), inLen + EVP_MAX_BLOCK_LENGTH);
    int outBufLen, tmpLen;
    if (!EVP_CipherUpdate(ctx, outBuf, &outBufLen, in, inLen)) {
        LOG(LOG_WARNING, "failed to call EVP_CipherUpdate.");
        goto end;
    }

    if (!EVP_CipherFinal_ex(ctx, outBuf + outBufLen, &tmpLen)) {
        LOG(LOG_WARNING, "failed to call EVP_CipherFinal_ex.");
        goto end;
    }
    outBufLen += tmpLen;
    outBuf[outBufLen] = 0;

    *out = outBuf;
    *outLen = outBufLen;
    ret = 0;
end:
    if (ctx != NULL) {
        EVP_CIPHER_CTX_free(ctx);
    }
    return ret;
}

void printOpenSSLError()
{
    char *buf;
    BIO *bio = BIO_new(BIO_s_mem());
    ERR_print_errors(bio);

    BIO_get_mem_data(bio, &buf);

    BIO_free(bio);
}