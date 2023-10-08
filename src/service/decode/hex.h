/*
 * This file was modified by UnionTech Software Technology Co., Ltd. in 2023
 *
 * Copyright (C) 2018 Yubico AB - See COPYING
 */

#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int hex_decode(const char *ascii_hex, unsigned char **blob, size_t *blob_len);

#ifdef __cplusplus
}
#endif