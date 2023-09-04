/*
 * This file was modified by UnionTech Software Technology Co., Ltd. in 2023
 *
 * Copyright (C) 2018 Yubico AB - See COPYING
 */
#include "b64.h"

#include <openssl/bio.h>
#include <openssl/evp.h>

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int b64_encode(const void *ptr, size_t len, char **out)
{
    BIO *bio_b64 = NULL;
    BIO *bio_mem = NULL;
    char *b64_ptr = NULL;
    long b64_len;
    int n;
    int ok = 0;

    if (ptr == NULL || out == NULL || len > INT_MAX) {
        return (0);
    }

    *out = NULL;

    bio_b64 = BIO_new(BIO_f_base64());
    if (bio_b64 == NULL) {
        goto end;
    }

    bio_mem = BIO_new(BIO_s_mem());
    if (bio_mem == NULL) {
        goto end;
    }

    BIO_set_flags(bio_b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(bio_b64, bio_mem);

    n = BIO_write(bio_b64, ptr, (int)len);
    if (n < 0 || (size_t)n != len) {
        goto end;
    }

    if (BIO_flush(bio_b64) < 0) {
        goto end;
    }

    b64_len = BIO_get_mem_data(bio_b64, &b64_ptr);
    if (b64_len < 0 || (size_t)b64_len == SIZE_MAX || b64_ptr == NULL) {
        goto end;
    }

    *out = calloc(1, (size_t)b64_len + 1);
    if (*out == NULL) {
        goto end;
    }

    memcpy(*out, b64_ptr, (size_t)b64_len);
    ok = 1;

end:
    if (bio_b64 != NULL) {
        BIO_free(bio_b64);
    }
    if (bio_mem != NULL) {
        BIO_free(bio_mem);
    }
    return (ok);
}

int b64_decode(const char *in, void **ptr, size_t *len)
{
    BIO *bio_mem = NULL;
    BIO *bio_b64 = NULL;
    size_t alloc_len;
    void *ptrTemp = NULL;
    int n;
    int ok = 0;

    if (in == NULL || ptr == NULL || len == NULL || strlen(in) > INT_MAX) {
        goto end;
    }

    *ptr = NULL;
    *len = 0;

    bio_b64 = BIO_new(BIO_f_base64());
    if (bio_b64 == NULL) {
        goto end;
    }

    bio_mem = BIO_new_mem_buf((const void *)in, -1);
    if (bio_mem == NULL) {
        goto end;
    }

    BIO_set_flags(bio_b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(bio_b64, bio_mem);

    alloc_len = strlen(in);
    ptrTemp = calloc(1, alloc_len);
    if (ptrTemp == NULL) {
        goto end;
    }

    n = BIO_read(bio_b64, ptrTemp, (int)alloc_len);
    if (n < 0 || BIO_eof(bio_b64) == 0) {
        goto end;
    }

    *ptr = ptrTemp;
    *len = (size_t)n;
    ok = 1;

end:
    if (bio_b64 != NULL) {
        BIO_free(bio_b64);
    }
    if (bio_mem != NULL) {
        BIO_free(bio_mem);
    }

    if (!ok && ptrTemp != NULL) {
        free(ptrTemp);
    }

    return (ok);
}

char *normal_b64(const char *websafe_b64)
{
    char *b64;
    char *p;
    size_t n;

    n = strlen(websafe_b64);
    if (n > SIZE_MAX - 3) {
        return (NULL);
    }

    b64 = calloc(1, n + 3);
    if (b64 == NULL) {
        return (NULL);
    }

    memcpy(b64, websafe_b64, n);
    p = b64;

    while ((p = strpbrk(p, "-_")) != NULL) {
        switch (*p) {
        case '-':
            *p++ = '+';
            break;
        case '_':
            *p++ = '/';
            break;
        }
    }

    switch (n % 4) {
    case 1:
        b64[n] = '=';
        break;
    case 2:
    case 3:
        b64[n] = '=';
        b64[n + 1] = '=';
        break;
    }

    return (b64);
}