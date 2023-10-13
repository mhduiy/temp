// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <fido.h>

#ifdef __cplusplus
extern "C" {
#endif

// 寻找设备，但不打开设备
int dpk_dev_info_find_wait(fido_dev_info_t **devList, size_t *nDevs);
// 寻找设备，但不打开设备
int dpk_dev_info_find_existed(fido_dev_info_t **devList, size_t *nDevs);
// 在寻找到的设备中，打开其中一个设备
int dpk_dev_open_default_dev(fido_dev_info_t *devList, size_t nDevs, fido_dev_t **device);
// 在寻找到的设备中，打开所有设备
int dpk_dev_open_all_dev(fido_dev_info_t *devInfoList, size_t nDevInfos, fido_dev_t **devices, size_t devSize);
// 在寻找到的设备中，通过在场证明进行设备选择，选择其中一个设备
int dpk_dev_select_dev(fido_dev_info_t *devList, size_t nDevs, fido_dev_t **dev);

#ifdef __cplusplus
}
#endif
