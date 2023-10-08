// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <fido.h>

#ifdef __cplusplus
extern "C" {
#endif

// 设备OPTIONS特性有3个状态：不支持、支持但未设置、支持且已有设置
#define INFO_OPTIONS_NOT_SUPPORT 0
#define INFO_OPTIONS_SET 1
#define INFO_OPTIONS_UNSET 2

// authenticatorGetInfo (0x04)
int dpk_dev_get_info(fido_dev_t *dev, fido_cbor_info_t **info);
// options (0x04.0x04)
int dpk_dev_get_options_support_pin(fido_cbor_info_t *info, int *support);
int dpk_dev_get_options_support_uv(fido_cbor_info_t *info, int *support);
int dpk_dev_get_options_support_bio(fido_cbor_info_t *info, int *support);
int dpk_dev_get_options_support_mcuvnr(fido_cbor_info_t *info, int *support);
// algorithms (0x04.0x0A)
int dpk_dev_get_support_algorithm(fido_cbor_info_t *info, int *algorithm);

#ifdef __cplusplus
}
#endif
