// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "servicebase.h"

#include "common/common.h"
#include "common/log.h"

#include <stdio.h>

#define SERVICE_TIMEOUT 180

const gchar *PASSKEY_SERVICE_DBUS_NAME = "com.deepin.Passkey";
const gchar *PASSKEY_SERVICE_DBUS_PATH = "/com/deepin/Passkey";
const gchar *PASSKEY_SERVICE_DBUS_INTERFACE = "com.deepin.Passkey";
// 源自xml生成的文件
extern const gchar *PASSKEY_SERVICE_DBUS_XML_DATA;

typedef struct _MethodInfo
{
    char *name;
    ServiceMethod cb;
    gboolean isAsync;
} MethodInfo;

static gboolean service_timeout_cb(gpointer userData)
{
    LOG(LOG_INFO, "service timeout and is about to exit...");
    Service *srv = (Service *)userData;
    if (srv == NULL) {
        LOG(LOG_ERR, "service data is invalid");
        return FALSE;
    }
    g_main_loop_quit(srv->loop);
    return TRUE;
}

static void service_timeout_method_called_end(gpointer userData)
{
    gint count = 0;
    guint source = 0;
    Service *srv = (Service *)userData;
    if (srv == NULL) {
        LOG(LOG_ERR, "param invalid");
        return;
    }
    mtx_lock(&(srv->timeoutCallCountMtx));
    srv->timeoutCallCount--;
    count = srv->timeoutCallCount;
    source = srv->timeoutSource;
    mtx_unlock(&(srv->timeoutCallCountMtx));
    LOG(LOG_DEBUG, "service-called end, try to exit? call count: %d", count);
    if (source != 0) {
        return;
    }
    if (count == 0) {
        source = g_timeout_add_seconds(SERVICE_TIMEOUT, (GSourceFunc)service_timeout_cb, userData);
        mtx_lock(&(srv->timeoutCallCountMtx));
        srv->timeoutSource = source;
        mtx_unlock(&(srv->timeoutCallCountMtx));
    }
}

static void service_timeout_method_called(gpointer userData)
{
    gint count = 0;
    guint source = 0;
    Service *srv = (Service *)userData;
    if (srv == NULL) {
        LOG(LOG_ERR, "param invalid");
        return;
    }
    mtx_lock(&(srv->timeoutCallCountMtx));
    srv->timeoutCallCount++;
    count = srv->timeoutCallCount;
    source = srv->timeoutSource;
    mtx_unlock(&(srv->timeoutCallCountMtx));
    LOG(LOG_DEBUG, "service-called start, do not to exit, call count: %d", count);
    if (source != 0 && count > 0) {
        g_source_remove(source);
        mtx_lock(&(srv->timeoutCallCountMtx));
        srv->timeoutSource = 0;
        mtx_unlock(&(srv->timeoutCallCountMtx));
    }
}

static MethodContext *mehtod_context_new(ServiceMethod cb, GVariant *param, GDBusMethodInvocation *invocation, gpointer userData)
{
    MethodContext *method = (MethodContext *)calloc(1, sizeof(MethodContext));
    method->cb = cb;

    method->parameters = g_variant_get_normal_form(param);
    method->invocation = invocation;
    method->userData = userData;
    method->callId = g_uuid_string_random();

    return method;
}

static void mehtod_context_delete(MethodContext *mc)
{
    if (mc == NULL) {
        return;
    }
    if (mc->callId != NULL) {
        g_free(mc->callId);
        mc->callId = NULL;
    }

    if (mc->parameters != NULL) {
        g_variant_unref(mc->parameters);
    }

    free(mc);
}

static int method_call(void *data)
{
    MethodContext *mc = (MethodContext *)data;
    if (mc == NULL || mc->cb == NULL) {
        return -1;
    }
    service_timeout_method_called(mc->userData);
    (*(mc->cb))(mc);
    service_timeout_method_called_end(mc->userData);

    mehtod_context_delete(mc);
    return 0;
}

static int method_call_async(void *data)
{
    MethodContext *method = (MethodContext *)data;
    if (method == NULL || method->cb == NULL || method->invocation == NULL || method->callId == NULL) {
        return -1;
    }
    g_dbus_method_invocation_return_value(method->invocation, g_variant_new("()"));

    thrd_t thr;
    thrd_create(&thr, method_call, method);
    thrd_detach(thr);

    return 0;
}

static void on_name_acquired(GDBusConnection *connection, const gchar *name, gpointer userData)
{
    UNUSED_VALUE(connection);
    UNUSED_VALUE(name);
    UNUSED_VALUE(userData);
    LOG(LOG_DEBUG, "on_name_acquired has been invoked");
}

static void on_name_lost(GDBusConnection *connection, const gchar *name, gpointer userData)
{
    UNUSED_VALUE(connection);
    UNUSED_VALUE(name);
    UNUSED_VALUE(userData);
    LOG(LOG_DEBUG, "on_name_lost has been invoked");
}

static void handle_method(GDBusConnection *connection,
                          const gchar *sender,
                          const gchar *objectPath,
                          const gchar *interfaceName,
                          const gchar *methodName,
                          GVariant *parameters,
                          GDBusMethodInvocation *invocation,
                          gpointer userData)
{
    UNUSED_VALUE(connection);
    UNUSED_VALUE(objectPath);
    UNUSED_VALUE(interfaceName);
    LOG(LOG_INFO, "Received call '%s' from %s", methodName, sender);

    Service *srv = (Service *)userData;
    if (srv == NULL || srv->methods == NULL) {
        LOG(LOG_ERR, "param invalid");
        return;
    }

    MethodInfo *info = (MethodInfo *)g_hash_table_lookup(srv->methods, methodName);
    if (info == NULL) {
        LOG(LOG_ERR, "method invalid");
        return;
    }

    if (info->isAsync) {
        method_call_async(mehtod_context_new(info->cb, parameters, invocation, userData));
    } else {
        method_call(mehtod_context_new(info->cb, parameters, invocation, userData));
    }

    return;
}

static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer userData)
{
    UNUSED_VALUE(name);
    GError *error = NULL;
    GDBusNodeInfo *introspection_data;
    GDBusInterfaceInfo *interface_info;

    GDBusInterfaceVTable interface_vtable = {
        .method_call = handle_method,
    };

    introspection_data = g_dbus_node_info_new_for_xml(PASSKEY_SERVICE_DBUS_XML_DATA, &error);
    if (error != NULL) {
        LOG(LOG_ERR, "Unable to create introspection data: %s", error->message);
        return;
    }

    interface_info = g_dbus_node_info_lookup_interface(introspection_data, PASSKEY_SERVICE_DBUS_INTERFACE);
    if (interface_info == NULL) {
        LOG(LOG_ERR, "Unable to lookup interface info");
        return;
    }

    guint registration_id = g_dbus_connection_register_object(connection, PASSKEY_SERVICE_DBUS_PATH, interface_info, &interface_vtable, userData, NULL, NULL);
    if (registration_id == 0) {
        LOG(LOG_ERR, "Unable to register object");
        return;
    }
}

static void free_methods_key(gpointer data)
{
    if (data == NULL) {
        return;
    }
    g_free(data);
}

static void free_methods_value(gpointer data)
{
    if (data == NULL) {
        return;
    }

    MethodInfo *info = (MethodInfo *)data;
    if (info->name != NULL) {
        g_free(info->name);
    }
    free(info);
}

void service_init(Service *srv)
{
    if (srv == NULL) {
        return;
    }
    srv->budOwnId = 0;
    srv->loop = NULL;
    srv->timeoutSource = 0;
    srv->timeoutCallCount = 0;

    // srv->vtable = vtable;
    srv->methods = g_hash_table_new_full(g_str_hash, g_str_equal, free_methods_key, free_methods_value);

    mtx_init(&(srv->timeoutCallCountMtx), mtx_plain);
    srv->timeoutSource = g_timeout_add_seconds(SERVICE_TIMEOUT, (GSourceFunc)service_timeout_cb, (gpointer)srv);
    srv->budOwnId = g_bus_own_name(G_BUS_TYPE_SYSTEM, PASSKEY_SERVICE_DBUS_NAME, G_BUS_NAME_OWNER_FLAGS_NONE, on_bus_acquired, on_name_acquired, on_name_lost, (gpointer)srv, NULL);
    srv->loop = g_main_loop_new(NULL, FALSE);
}

void service_run(Service *srv)
{
    g_main_loop_run(srv->loop);
}

void service_unref(Service *srv)
{
    if (srv == NULL) {
        LOG(LOG_WARNING, "service is not exist?");
        return;
    }
    mtx_destroy(&(srv->timeoutCallCountMtx));

    if (srv->methods != NULL) {
        g_hash_table_destroy(srv->methods);
    }

    if (srv->budOwnId > 0) {
        g_bus_unown_name(srv->budOwnId);
    }

    if (srv->loop != NULL) {
        g_main_loop_unref(srv->loop);
    }

    if (srv->timeoutSource != 0) {
        g_source_remove(srv->timeoutSource);
    }
}

void service_register_method(Service *srv, const gchar *methodName, ServiceMethod cb, gboolean isAsync)
{
    if (srv == NULL) {
        LOG(LOG_WARNING, "service is not exist?");
        return;
    }
    MethodInfo *info = (MethodInfo *)calloc(1, sizeof(MethodInfo));
    info->name = g_strdup(methodName);
    info->cb = cb;
    info->isAsync = isAsync;
    g_hash_table_insert(srv->methods, g_strdup(methodName), info);
}