/*
 * This file was modified by UnionTech Software Technology Co., Ltd. in 2023
 *
 * Copyright (C) 2018 Yubico AB - See COPYING
 */

#include <stddef.h>

#ifndef B64_H
#  define B64_H

int b64_encode(const void *, size_t, char **);
int b64_decode(const char *, void **, size_t *);
char *normal_b64(const char *websafe_b64);

#endif /* B64_H */
