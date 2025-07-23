// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "servicesignal.h"

#include "common/log.h"
#include "err.h"

#include <gio/gio.h>
#include <glib.h>

void emit_reset_status(MethodContext *mc, int finish, int status)
{
    GError *error = NULL;
    char *jsonData = NULL;
    if (mc == NULL || mc->serviceData == NULL || mc->callId == NULL) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: service-data is invalid.");
        goto end;
    }
    Service *srv = (Service *)mc->serviceData;
    if (srv->connection == NULL) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: service-data is invalid.");
        goto end;
    }

    if (dp_err_code_to_json(status, &jsonData) < 0) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: can not get json data.");
        goto end;
    }

    g_dbus_connection_emit_signal(srv->connection, NULL, "/org/deepin/Passkey1", "org.deepin.Passkey1", "ResetStatus", g_variant_new("(sis)", mc->callId, finish, jsonData), &error);

end:
    if (error != NULL) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: %s", error->message);
        g_error_free(error);
    }
    if (jsonData != NULL) {
        free(jsonData);
    }
}

void emit_device_detect_status(MethodContext *mc, int finish, int status)
{
    GError *error = NULL;
    char *jsonData = NULL;
    if (mc == NULL || mc->serviceData == NULL || mc->callId == NULL) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: service-data is invalid.");
        goto end;
    }
    Service *srv = (Service *)mc->serviceData;
    if (srv->connection == NULL) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: service-data is invalid.");
        goto end;
    }

    if (dp_err_code_to_json(status, &jsonData) < 0) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: can not get json data.");
        goto end;
    }

    g_dbus_connection_emit_signal(srv->connection, NULL, "/org/deepin/Passkey1", "org.deepin.Passkey1", "DeviceDetectStatus", g_variant_new("(sis)", mc->callId, finish, jsonData), &error);
end:
    if (error != NULL) {
        LOG(LOG_WARNING, "emit signal error: %s", error->message);
        g_error_free(error);
    }
    if (jsonData != NULL) {
        free(jsonData);
    }
}

void emit_make_cred_status(MethodContext *mc, const char *user, int finish, int status)
{
    GError *error = NULL;
    char *jsonData = NULL;
    if (mc == NULL || mc->serviceData == NULL || mc->callId == NULL) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: service-data is invalid.");
        goto end;
    }
    Service *srv = (Service *)mc->serviceData;
    if (srv->connection == NULL) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: service-data is invalid.");
        goto end;
    }

    if (dp_err_code_to_json(status, &jsonData) < 0) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: can not get json data.");
        goto end;
    }

    g_dbus_connection_emit_signal(srv->connection, NULL, "/org/deepin/Passkey1", "org.deepin.Passkey1", "MakeCredStatus", g_variant_new("(ssis)", mc->callId, user, finish, jsonData), &error);
end:
    if (error != NULL) {
        LOG(LOG_WARNING, "emit signal error: %s", error->message);
        g_error_free(error);
    }
    if (jsonData != NULL) {
        free(jsonData);
    }
}

void emit_get_assert_status(MethodContext *mc, const char *user, int finish, int status)
{
    GError *error = NULL;
    char *jsonData = NULL;
    if (mc == NULL || mc->serviceData == NULL || mc->callId == NULL) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: service-data is invalid.");
        goto end;
    }
    Service *srv = (Service *)mc->serviceData;
    if (srv->connection == NULL) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: service-data is invalid.");
        goto end;
    }

    if (dp_err_code_to_json(status, &jsonData) < 0) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: can not get json data.");
        goto end;
    }

    g_dbus_connection_emit_signal(srv->connection, NULL, "/org/deepin/Passkey1", "org.deepin.Passkey1", "GetAssertStatus", g_variant_new("(ssis)", mc->callId, user, finish, jsonData), &error);
end:
    if (error != NULL) {
        LOG(LOG_WARNING, "emit signal error: %s", error->message);
        g_error_free(error);
    }
    if (jsonData != NULL) {
        free(jsonData);
    }
}

void emit_device_select_status(MethodContext *mc, int finish, int status)
{
    GError *error = NULL;
    char *jsonData = NULL;
    if (mc == NULL || mc->serviceData == NULL || mc->callId == NULL) {
        LOG(LOG_WARNING, "emit 'DeviceSelect' signal error: service-data is invalid.");
        goto end;
    }
    Service *srv = (Service *)mc->serviceData;
    if (srv->connection == NULL) {
        LOG(LOG_WARNING, "emit 'DeviceSelect' signal error: service-data is invalid.");
        goto end;
    }

    if (dp_err_code_to_json(status, &jsonData) < 0) {
        LOG(LOG_WARNING, "emit 'ResetStatus' signal error: can not get json data.");
        goto end;
    }

    g_dbus_connection_emit_signal(srv->connection, NULL, "/org/deepin/Passkey1", "org.deepin.Passkey1", "DeviceSelect", g_variant_new("(sis)", mc->callId, finish, jsonData), &error);
end:
    if (error != NULL) {
        LOG(LOG_WARNING, "emit signal error: %s", error->message);
        g_error_free(error);
    }
    if (jsonData != NULL) {
        free(jsonData);
    }
}