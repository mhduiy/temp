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
    guint busOwnId;
    // loop线程安全
    GMainLoop *loop;
    GHashTable *methods;
    // individual data structure instances are not automatically locked for performance reasons.
    // So, for example you must coordinate accesses to the same GHashTable from multiple threads.
    GHashTable *customData;
    mtx_t customDataMtx;
    guint timeoutSource;
    gint timeoutCallCount;
    mtx_t timeoutCallCountMtx;
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
void service_init(Service *srv);
// 注册method处理函数, 需要实现ServiceMethod
// 同步方法，使用GDBusMethodInvocation直接应答
// 异步方法，无需应答，如果需要通过信号通知结果
void service_register_method(Service *srv, const gchar *methodName, ServiceMethod cb, gboolean isAsync);
// 启动服务，会阻塞直接结束
void service_run(Service *srv);
// 服务资源释放
void service_unref(Service *srv);

// 自定义数据函数
int service_custom_data_get(Service *srv, const gchar *key, gchar **value);
int service_custom_data_set(Service *srv, const gchar *key, const gchar *value);
int service_custom_data_delete(Service *srv, const gchar *key);

#ifdef __cplusplus
}
#endif