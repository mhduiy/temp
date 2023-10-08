// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <fido.h>

#ifdef __cplusplus
extern "C" {
#endif

int dpk_dev_devs_find_wait(fido_dev_info_t **devList, size_t *nDevs);
int dpk_dev_devs_find_existed(fido_dev_info_t **devList, size_t *nDevs);
int dpk_dev_get_default_dev(bool isWaitPlugin, fido_dev_t **device);
int dpk_dev_dev_select(fido_dev_t **dev);

#ifdef __cplusplus
}
#endif
