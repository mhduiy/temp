/*
 * This file was modified by UnionTech Software Technology Co., Ltd. in 2023
 *
 * Copyright (C) 2018 Yubico AB - See COPYING
 */
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int b64_encode(const void *in, size_t inLen, char **out);
int b64_decode(const char *in, void **out, size_t *outLen);
char *normal_b64(const char *safeB64);

#ifdef __cplusplus
}
#endif