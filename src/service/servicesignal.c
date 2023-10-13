// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "servicesignal.h"

#include "common/log.h"

#include <gio/gio.h>
#include <glib.h>

void emit_reset_status(MethodContext *mc, int finish, int status)
{
    GError *error = NULL;
    if (mc == NULL || mc->serviceData == NULL || mc->callId == NULL) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: service-data is invalid.");
        goto end;
    }
    Service *srv = (Service *)mc->serviceData;
    if (srv->connection == NULL) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: service-data is invalid.");
        goto end;
    }

    g_dbus_connection_emit_signal(srv->connection, NULL, "/com/deepin/Passkey", "com.deepin.Passkey", "ResetStatus", g_variant_new("(sii)", mc->callId, finish, status), &error);

end:
    if (error != NULL) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: %s", error->message);
        g_error_free(error);
    }
}

void emit_device_detect_status(MethodContext *mc, int finish, int status)
{
    GError *error = NULL;
    if (mc == NULL || mc->serviceData == NULL || mc->callId == NULL) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: service-data is invalid.");
        goto end;
    }
    Service *srv = (Service *)mc->serviceData;
    if (srv->connection == NULL) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: service-data is invalid.");
        goto end;
    }

    g_dbus_connection_emit_signal(srv->connection, NULL, "/com/deepin/Passkey", "com.deepin.Passkey", "DeviceDetectStatus", g_variant_new("(sii)", mc->callId, finish, status), &error);
end:
    if (error != NULL) {
        LOG(LOG_WARNING, "emit signal error: %s", error->message);
        g_error_free(error);
    }
}

void emit_make_cred_status(MethodContext *mc, const char *user, int finish, int status)
{
    GError *error = NULL;
    if (mc == NULL || mc->serviceData == NULL || mc->callId == NULL) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: service-data is invalid.");
        goto end;
    }
    Service *srv = (Service *)mc->serviceData;
    if (srv->connection == NULL) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: service-data is invalid.");
        goto end;
    }

    g_dbus_connection_emit_signal(srv->connection, NULL, "/com/deepin/Passkey", "com.deepin.Passkey", "MakeCredStatus", g_variant_new("(ssii)", mc->callId, user, finish, status), &error);
end:
    if (error != NULL) {
        LOG(LOG_WARNING, "emit signal error: %s", error->message);
        g_error_free(error);
    }
}

void emit_get_assert_status(MethodContext *mc, const char *user, int finish, int status)
{
    GError *error = NULL;
    if (mc == NULL || mc->serviceData == NULL || mc->callId == NULL) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: service-data is invalid.");
        goto end;
    }
    Service *srv = (Service *)mc->serviceData;
    if (srv->connection == NULL) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: service-data is invalid.");
        goto end;
    }

    g_dbus_connection_emit_signal(srv->connection, NULL, "/com/deepin/Passkey", "com.deepin.Passkey", "GetAssertStatus", g_variant_new("(ssii)", mc->callId, user, finish, status), &error);
end:
    if (error != NULL) {
        LOG(LOG_WARNING, "emit signal error: %s", error->message);
        g_error_free(error);
    }
}

void emit_device_select_status(MethodContext *mc, int finish, int status)
{
    GError *error = NULL;
    if (mc == NULL || mc->serviceData == NULL || mc->callId == NULL) {
        LOG(LOG_WARNING, "emit 'DeviceSelect' signal error: service-data is invalid.");
        goto end;
    }
    Service *srv = (Service *)mc->serviceData;
    if (srv->connection == NULL) {
        LOG(LOG_WARNING, "emit 'DeviceSelect' signal error: service-data is invalid.");
        goto end;
    }

    g_dbus_connection_emit_signal(srv->connection, NULL, "/com/deepin/Passkey", "com.deepin.Passkey", "DeviceSelect", g_variant_new("(sii)", mc->callId, finish, status), &error);
end:
    if (error != NULL) {
        LOG(LOG_WARNING, "emit signal error: %s", error->message);
        g_error_free(error);
    }
}