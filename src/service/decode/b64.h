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

int b64_encode(const void *, size_t, char **);
int b64_decode(const char *, void **, size_t *);
char *normal_b64(const char *websafe_b64);

#ifdef __cplusplus
}
#endif