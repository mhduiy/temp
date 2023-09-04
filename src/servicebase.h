// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gio/gio.h>
#include <glib.h>

#include <threads.h>

typedef struct _Service
{
    guint budOwnId;
    GMainLoop *loop; // 线程安全
    // const GDBusInterfaceVTable *vtable;
    GHashTable *methods;
    guint timeoutSource;
    gint timeoutCallCount;
    mtx_t timeoutCallCountMtx;
} Service;

struct _MethodContext;
typedef int (*ServiceMethod)(struct _MethodContext *mc);

typedef struct _MethodContext
{
    ServiceMethod cb;
    GVariant *parameters;
    GDBusMethodInvocation *invocation;
    gpointer userData;
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
