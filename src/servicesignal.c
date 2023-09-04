// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "servicesignal.h"

#include <gio/gio.h>
#include <glib.h>

void emit_reset_status(int finish, int status)
{
    GError *error = NULL;
    GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    g_dbus_connection_emit_signal(connection, NULL, "/com/deepin/Passkey", "com.deepin.Passkey", "ResetStatus", g_variant_new("(ii)", finish, status), &error);
}

void emit_device_detect_status(int finish, int status)
{
    GError *error = NULL;
    GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    g_dbus_connection_emit_signal(connection, NULL, "/com/deepin/Passkey", "com.deepin.Passkey", "DeviceDetectStatus", g_variant_new("(ii)", finish, status), &error);
}

void emit_make_cred_status(const char *user, int finish, int status)
{
    GError *error = NULL;
    GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    g_dbus_connection_emit_signal(connection, NULL, "/com/deepin/Passkey", "com.deepin.Passkey", "MakeCredStatus", g_variant_new("(sii)", user, finish, status), &error);
}

void emit_get_assert_status(const char *user, int finish, int status)
{
    GError *error = NULL;
    GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    g_dbus_connection_emit_signal(connection, NULL, "/com/deepin/Passkey", "com.deepin.Passkey", "GetAssertStatus", g_variant_new("(sii)", user, finish, status), &error);
}