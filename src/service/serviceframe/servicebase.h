// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gio/gio.h>
#include <glib.h>

#include <threads.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Service
{
    // bus信息
    guint busOwnId;
    GDBusConnection *connection;
    gchar *busName;
    gchar *busInterface;
    gchar *busPath;
    // loop线程安全
    GMainLoop *loop;
    // 注册的方法
    GHashTable *methods;
    GHashTable *propertys;
    // 空闲退出
    gint timeoutSecond; // -1 不空闲退出
    guint timeoutSource;
    gint timeoutCallCount;
    mtx_t timeoutCallCountMtx;
    // ServiceBase不维护customData，由服务实例本身维护。该数据是可选的。
    gpointer customData;
} Service;

struct _MethodContext;
typedef int (*ServiceMethod)(struct _MethodContext *mc);

typedef struct _MethodContext
{
    gchar *sender;
    ServiceMethod cb;
    GVariant *parameters;
    GDBusMethodInvocation *invocation;
    gpointer serviceData;
    gchar *callId;
} MethodContext;

// dbus服务初始化，vtable是dbus接口的处理表
int service_init(Service *srv, const gchar *busName, gpointer customData);
int service_register_interface(Service *srv, const gchar *path, const gchar *interface, const gchar *interfaceXml);
// 注册method处理函数, 需要实现ServiceMethod
// 同步方法，使用GDBusMethodInvocation直接应答
// 异步方法，无需应答，如果需要通过信号通知结果
void service_register_method(Service *srv, const gchar *methodName, ServiceMethod cb, gboolean isAsync);
// 注册属性, 参数propName、value外部自行free和unref
void service_register_property(Service *srv, const gchar *propName, GVariant *value);
int service_set_property(Service *srv, const gchar *propName, GVariant *value);
int service_get_property(Service *srv, const gchar *propName, GVariant **value);
// 启动服务，会阻塞直接结束
void service_run(Service *srv);
// 服务资源释放
void service_unref(Service *srv);

#ifdef __cplusplus
}
#endif