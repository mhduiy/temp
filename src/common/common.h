// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdbool.h>
#include <stdio.h>

#define BUFSIZE 1024
#define DEV_NUM_MAX 24

#define UNUSED_VALUE(a) ((void)(a))

bool random_bytes(void *buf, size_t cnt);
bool read_file_line(const char *path, char **val);

bool get_user_id(const char *userName, char **id);
bool is_user_exist(const char *userName);