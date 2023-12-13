// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <fido.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 设备OPTIONS特性有3个状态：不支持、支持且已有设置、支持但未设置
#define INFO_OPTIONS_NOT_SUPPORT 0
#define INFO_OPTIONS_SET 1
#define INFO_OPTIONS_UNSET 2

// versions
#define INFO_VERSION_MAX_LEN 128
#define INFO_VERSION_U2F_V2 "U2F_V2"
#define INFO_VERSION_FIDO_2_0 "FIDO_2_0"
#define INFO_VERSION_FIDO_2_1_PRE "FIDO_2_1_PRE"
#define INFO_VERSION_FIDO_2_1 "FIDO_2_1"

// authenticatorGetInfo (0x04)
int dpk_dev_get_info(fido_dev_t *dev, fido_cbor_info_t **info);
// versions (0x04.0x01)
int dpk_dev_get_version(fido_cbor_info_t *info, char ***versions, int *count);
// options (0x04.0x04)
int dpk_dev_get_options_support_pin(fido_cbor_info_t *info, int *support);
int dpk_dev_get_options_support_rk(fido_cbor_info_t *info, int *support);
int dpk_dev_get_options_support_uv(fido_cbor_info_t *info, int *support);
int dpk_dev_get_options_support_up(fido_cbor_info_t *info, int *support);
int dpk_dev_get_options_support_bio(fido_cbor_info_t *info, int *support);
int dpk_dev_get_options_support_mcuvnr(fido_cbor_info_t *info, int *support);
int dpk_dev_get_options_support_pin_uv_auth_token(fido_cbor_info_t *info, int *support);
int dpk_dev_get_options_support_always_uv(fido_cbor_info_t *info, int *support);
int dpk_dev_get_options_support_ep(fido_cbor_info_t *info, int *support);
// algorithms (0x04.0x0A)
int dpk_dev_get_fido_default_support_algorithm(fido_cbor_info_t *info, int *algorithm);
int dpk_dev_get_u2f_default_support_algorithm(fido_cbor_info_t *info, int *algorithm);

// 检查当前设备是否允许通过2.0以上协议操作凭证时，无需pin验证
int dpk_dev_check_fido2_support_no_pin_req(fido_cbor_info_t *info, int *support);
// 检查version是否存在versions列表
bool dpk_dev_check_version_exist(const char **versions, int versionsCount, const char *version);

// 生物认证
int dpk_dev_get_uv_modality(fido_cbor_info_t *info, uint64_t *modality);

#ifdef __cplusplus
}
#endif
