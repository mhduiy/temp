/*
 * This file was modified by UnionTech Software Technology Co., Ltd. in 2023
 *
 * Copyright (C) 2018 Yubico AB - See COPYING
 */

#include "hex.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

int hex_decode(const char *ascii_hex, unsigned char **blob, size_t *blob_len)
{
    *blob = NULL;
    *blob_len = 0;

    if (ascii_hex == NULL || (strlen(ascii_hex) % 2) != 0) {
        return (0);
    }

    *blob_len = strlen(ascii_hex) / 2;
    *blob = calloc(1, *blob_len);
    if (*blob == NULL) {
        return (0);
    }

    for (size_t i = 0; i < *blob_len; i++) {
        unsigned int c;
        int n = -1;
        int r = sscanf(ascii_hex, "%02x%n", &c, &n);
        if (r != 1 || n != 2 || c > UCHAR_MAX) {
            free(*blob);
            *blob = NULL;
            *blob_len = 0;
            return (0);
        }
        (*blob)[i] = (unsigned char)c;
        ascii_hex += n;
    }

    return (1);
}