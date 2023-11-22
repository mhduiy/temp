// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "serviceframe/servicebase.h"

#include "common/common.h"
#include "common/log.h"

#include <stdio.h>

#define SERVICE_TIMEOUT 180

typedef struct _MethodInfo
{
    char *name;
    ServiceMethod cb;
    gboolean isAsync;
} MethodInfo;

static gboolean service_timeout_cb(gpointer serviceData)
{
    LOG(LOG_INFO, "service timeout and is about to exit...");
    Service *srv = (Service *)serviceData;
    if (srv == NULL) {
        LOG(LOG_ERR, "service data is invalid");
        return FALSE;
    }
    g_main_loop_quit(srv->loop);
    return TRUE;
}

static void service_timeout_unref(gpointer serviceData)
{
    gint timeoutSecond = -1;
    gint count = 0;
    guint source = 0;
    Service *srv = (Service *)serviceData;
    if (srv == NULL) {
        LOG(LOG_ERR, "param invalid");
        return;
    }
    mtx_lock(&(srv->timeoutCallCountMtx));
    srv->timeoutCallCount--;
    count = srv->timeoutCallCount;
    source = srv->timeoutSource;
    timeoutSecond = srv->timeoutSecond;
    mtx_unlock(&(srv->timeoutCallCountMtx));
    LOG(LOG_DEBUG, "service-called end, try to exit? call count: %d", count);
    if (source != 0) {
        return;
    }
    if (timeoutSecond == -1) {
        return;
    }
    if (count == 0) {
        source = g_timeout_add_seconds(timeoutSecond, (GSourceFunc)service_timeout_cb, serviceData);
        mtx_lock(&(srv->timeoutCallCountMtx));
        srv->timeoutSource = source;
        mtx_unlock(&(srv->timeoutCallCountMtx));
    }
}

static void service_timeout_ref(gpointer serviceData)
{
    gint count = 0;
    guint source = 0;
    Service *srv = (Service *)serviceData;
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

static MethodContext *mehtod_context_new(const gchar *sender, ServiceMethod cb, GVariant *param, GDBusMethodInvocation *invocation, gpointer serviceData)
{
    MethodContext *method = (MethodContext *)calloc(1, sizeof(MethodContext));

    method->sender = g_strdup(sender);
    method->cb = cb;

    if (param != NULL) {
        method->parameters = g_variant_get_normal_form(param);
    }
    method->invocation = invocation;
    method->serviceData = serviceData;
    method->callId = g_uuid_string_random();

    return method;
}

static void mehtod_context_delete(MethodContext *mc)
{
    if (mc == NULL) {
        return;
    }

    if (mc->sender != NULL) {
        g_free(mc->sender);
        mc->sender = NULL;
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
    service_timeout_ref(mc->serviceData);
    (*(mc->cb))(mc);
    service_timeout_unref(mc->serviceData);

    mehtod_context_delete(mc);
    return 0;
}

static int method_call_async(void *data)
{
    MethodContext *method = (MethodContext *)data;
    if (method == NULL || method->cb == NULL || method->invocation == NULL || method->callId == NULL) {
        return -1;
    }
    g_dbus_method_invocation_return_value(method->invocation, g_variant_new("(s)", method->callId));

    thrd_t thr;
    thrd_create(&thr, method_call, method);
    thrd_detach(thr);

    return 0;
}

static void on_name_acquired(GDBusConnection *connection, const gchar *name, gpointer serviceData)
{
    UNUSED_VALUE(connection);
    UNUSED_VALUE(name);
    UNUSED_VALUE(serviceData);
    LOG(LOG_DEBUG, "on_name_acquired has been invoked");
}

static void on_name_lost(GDBusConnection *connection, const gchar *name, gpointer serviceData)
{
    UNUSED_VALUE(connection);
    UNUSED_VALUE(name);
    UNUSED_VALUE(serviceData);
    LOG(LOG_DEBUG, "on_name_lost has been invoked");
}

static void handle_method(GDBusConnection *connection,
                          const gchar *sender,
                          const gchar *objectPath,
                          const gchar *interfaceName,
                          const gchar *methodName,
                          GVariant *parameters,
                          GDBusMethodInvocation *invocation,
                          gpointer serviceData)
{
    UNUSED_VALUE(connection);
    UNUSED_VALUE(objectPath);
    UNUSED_VALUE(interfaceName);
    LOG(LOG_INFO, "Received call '%s' from %s", methodName, sender);

    Service *srv = (Service *)serviceData;
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
        method_call_async(mehtod_context_new(sender, info->cb, parameters, invocation, serviceData));
    } else {
        method_call(mehtod_context_new(sender, info->cb, parameters, invocation, serviceData));
    }

    return;
}

static GVariant *handle_get_property(
                    GDBusConnection *connection,
                    const gchar *sender,
                    const gchar *objectPath,
                    const gchar *interfaceName,
                    const gchar *propertyName,
                    GError **error,
                    gpointer serviceData)
{
    UNUSED_VALUE(connection);
    UNUSED_VALUE(sender);
    UNUSED_VALUE(objectPath);
    UNUSED_VALUE(interfaceName);
    UNUSED_VALUE(error);
    LOG(LOG_INFO, "Received call get '%s' from %s", propertyName, sender);

    Service *srv = (Service *)serviceData;
    if (srv == NULL || srv->propertys == NULL) {
        LOG(LOG_ERR, "param invalid");
        return NULL;
    }
    service_timeout_ref(srv);
    GVariant *val = NULL;
    do {
        if (service_get_property(srv, propertyName, &val) != 0) {
            LOG(LOG_ERR, "insert prop map failed");
            break;
        }
    } while (0);
    service_timeout_unref(srv);

    return val;
}

static gboolean handle_set_property(
                GDBusConnection *connection,
                const gchar *sender,
                const gchar *objectPath,
                const gchar *interfaceName,
                const gchar *propertyName,
                GVariant *value,
                GError **error,
                gpointer serviceData)
{
    UNUSED_VALUE(connection);
    UNUSED_VALUE(sender);
    UNUSED_VALUE(objectPath);
    UNUSED_VALUE(interfaceName);
    UNUSED_VALUE(error);
    Service *srv = (Service *)serviceData;
    if (srv == NULL || srv->propertys == NULL) {
        LOG(LOG_ERR, "param invalid");
        return TRUE;
    }

    service_timeout_ref(srv);
    do {
        if (service_set_property(srv, propertyName, value) != 0) {
            LOG(LOG_ERR, "insert prop map failed");
            break;
        }
    } while (0);
    service_timeout_unref(srv);

    return TRUE;
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

static void free_propertys_key(gpointer data)
{
    if (data == NULL) {
        return;
    }
    g_free(data);
}

static void free_propertys_value(gpointer data)
{
    if (data == NULL) {
        return;
    }
    g_variant_unref(data);
}

int service_register_interface(Service *srv, const gchar *path, const gchar *interface, const gchar *interfaceXml)
{
    GError *error = NULL;
    GDBusNodeInfo *introspectionData;
    GDBusInterfaceInfo *interfaceInfo;
    int ret = -1;

    GDBusInterfaceVTable interfaceVtable = {
        .method_call = handle_method,
        .get_property = handle_get_property,
        .set_property = handle_set_property,
    };

    introspectionData = g_dbus_node_info_new_for_xml(interfaceXml, &error);
    if (error != NULL) {
        LOG(LOG_ERR, "Unable to create introspection data: %s", error->message);
        goto end;
    }

    interfaceInfo = g_dbus_node_info_lookup_interface(introspectionData, interface);
    if (interfaceInfo == NULL) {
        LOG(LOG_ERR, "Unable to lookup interface info");
        goto end;
    }

    guint registration_id = g_dbus_connection_register_object(srv->connection, path, interfaceInfo, &interfaceVtable, (gpointer)srv, NULL, NULL);
    if (registration_id == 0) {
        LOG(LOG_ERR, "Unable to register object");
        goto end;
    }
    ret = 0;

end:
    if (error != NULL) {
        g_error_free(error);
        if (introspectionData != NULL) {
            g_dbus_node_info_unref(introspectionData);
        }
    }
    return ret;
}

int service_init(Service *srv, const gchar *busName, gpointer customData)
{
    GError *error = NULL;
    int ret = -1;

    if (srv == NULL) {
        goto end;
    }
    srv->busOwnId = 0;
    srv->loop = NULL;
    srv->timeoutSource = 0;
    srv->timeoutCallCount = 0;
    srv->customData = customData;
    srv->timeoutSecond = SERVICE_TIMEOUT;

    srv->methods = g_hash_table_new_full(g_str_hash, g_str_equal, free_methods_key, free_methods_value);
    srv->propertys = g_hash_table_new_full(g_str_hash, g_str_equal, free_propertys_key, free_propertys_value);

    mtx_init(&(srv->timeoutCallCountMtx), mtx_plain);
    if (srv->timeoutSecond > 0) {
        srv->timeoutSource = g_timeout_add_seconds(srv->timeoutSecond, (GSourceFunc)service_timeout_cb, (gpointer)srv);
    }

    srv->connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (error != NULL) {
        LOG(LOG_ERR, "Unable to get bus connection: %s", error->message);
        goto end;
    }
    srv->busOwnId = g_bus_own_name_on_connection(srv->connection, busName, G_BUS_NAME_OWNER_FLAGS_NONE, on_name_acquired, on_name_lost, (gpointer)srv, NULL);
    if (srv->busOwnId == 0) {
        goto end;
    }
    srv->loop = g_main_loop_new(NULL, FALSE);

    ret = 0;
end:
    if (error != NULL) {
        g_error_free(error);
    }
    return ret;
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
    if (srv->propertys != NULL) {
        g_hash_table_destroy(srv->propertys);
    }

    if (srv->busOwnId > 0) {
        g_bus_unown_name(srv->busOwnId);
    }

    if (srv->connection != NULL) {
        g_object_unref(srv->connection);
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

void service_register_property(Service *srv, const gchar *propName, GVariant *value)
{
    if (srv == NULL || srv->propertys == NULL || propName == NULL || value == NULL) {
        LOG(LOG_WARNING, "param is invalid.");
        return;
    }

    g_hash_table_insert(srv->propertys, g_strdup(propName), g_variant_ref(value));
}

int service_get_property(Service *srv, const gchar *propName, GVariant **value)
{
    int ret = -1;
    GVariant *val = NULL;

    if (srv == NULL || srv->propertys == NULL || propName == NULL || value == NULL) {
        LOG(LOG_WARNING, "param is invalid.");
        goto end;
    }

    val = (GVariant *)g_hash_table_lookup(srv->propertys, propName);
    if (val == NULL) {
        LOG(LOG_ERR, "property is not registered");
        goto end;
    }
    *value = g_variant_ref(val);
    ret = 0;
end:
    return ret;
}

int service_set_property(Service *srv, const gchar *propName, GVariant *value)
{
    int ret = -1;
    if (srv == NULL || srv->propertys == NULL || propName == NULL || value == NULL) {
        LOG(LOG_WARNING, "param is invalid.");
        goto end;
    }

    GVariant *val = NULL;
    val = (GVariant *)g_hash_table_lookup(srv->propertys, propName);
    if (val == NULL) {
        LOG(LOG_ERR, "property is not registered");
        goto end;
    }
    if (!g_hash_table_insert(srv->propertys, g_strdup(propName), g_variant_ref(value))) {
        LOG(LOG_ERR, "insert prop map failed");
        goto end;
    }

    // if need, to send signal property change notifications

    ret = 0;
end:
    return ret;
}
