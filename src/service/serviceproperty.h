// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "serviceframe/servicebase.h"

#ifdef __cplusplus
extern "C" {
#endif

// 基于servicebase，封装统一处理类型转换
int service_property_version_init(Service *srv);
int service_property_version_get(Service *srv, gchar **version);
// int service_property_version_set(Service *srv, const gchar *version); // read-only

#ifdef __cplusplus
}
#endif
