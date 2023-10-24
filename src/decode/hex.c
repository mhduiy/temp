/*
 * This file was modified by UnionTech Software Technology Co., Ltd. in 2023
 *
 * Copyright (C) 2018 Yubico AB - See COPYING
 */

#include "hex.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

int hex_decode(const char *asciiHex, unsigned char **blob, size_t *blobLen)
{
    *blob = NULL;
    *blobLen = 0;

    if (asciiHex == NULL || (strlen(asciiHex) % 2) != 0) {
        return (0);
    }

    *blobLen = strlen(asciiHex) / 2;
    *blob = calloc(1, *blobLen);
    if (*blob == NULL) {
        return (0);
    }

    for (size_t i = 0; i < *blobLen; i++) {
        unsigned int c;
        int n = -1;
        int r = sscanf(asciiHex, "%02x%n", &c, &n);
        if (r != 1 || n != 2 || c > UCHAR_MAX) {
            free(*blob);
            *blob = NULL;
            *blobLen = 0;
            return (0);
        }
        (*blob)[i] = (unsigned char)c;
        asciiHex += n;
    }

    return (1);
}