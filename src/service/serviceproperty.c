// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "serviceproperty.h"

int service_property_version_init(Service *srv)
{
    GVariant *var = NULL;
    var = g_variant_new("s", "1.0");
    if (var == NULL) {
        goto end;
    }
    if (g_variant_is_floating(var)) {
        g_variant_ref_sink(var);
    }
    service_register_property(srv, "Version", var);
end:
    if (var != NULL) {
        g_variant_unref(var);
    }
    return 0;
}

int service_property_version_get(Service *srv, gchar **version)
{
    GVariant *var = NULL;
    gchar *ver = NULL;
    int ret = -1;
    if (service_get_property(srv, "Version", &var) != 0) {
        goto end;
    }
    g_variant_get(var, "s", &ver);
    *version = ver;
    ret = 0;
end:
    if (var != NULL) {
        g_variant_unref(var);
    }
    return ret;
}

int service_property_version_set(Service *srv, const gchar *version)
{
    int ret = -1;
    GVariant *var = NULL;
    var = g_variant_new("s", version);
    if (var == NULL) {
        goto end;
    }
    if (g_variant_is_floating(var)) {
        g_variant_ref_sink(var);
    }
    if (service_set_property(srv, "Version", var) != 0) {
        goto end;
    }
    ret = 0;
end:
    if (var != NULL) {
        g_variant_unref(var);
    }
    return ret;
}