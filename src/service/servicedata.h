// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "serviceframe/servicebase.h"

#include <fido.h>
#include <gio/gio.h>
#include <glib.h>

#include <stdbool.h>
#include <threads.h>

#ifdef __cplusplus
extern "C" {
#endif

// 服务中，有一些数据是非单个调用的生命周期独有的，会多个调用使用，这类数据定义为服务数据
// 服务数据通过service.customData来共享
// 服务数据需要考虑线程安全

typedef struct _PasskeyServiceData
{
    // customData 提供一个string->string的kv结构
    // individual data structure instances are not automatically locked for performance reasons.
    // So, for example you must coordinate accesses to the same GHashTable from multiple threads.
    GHashTable *customData;
    mtx_t customDataMtx;

    // CTAP设备选择功能
    // 释放场景， 1-设备错误， 2-主动取消设备选择， 3-调用方退出(TODO)
    GHashTable *selectedDevice;
    mtx_t selectedDeviceMtx;

    // 调用独立设备的列表
    GHashTable *dealDevices;
    mtx_t dealDevicesMtx;

    // 服务非对称密钥, type->key
    GHashTable *serviceAsymKey;
    mtx_t serviceAsymKeyMtx;

    // 客户端对称密钥, sender->key, 调用方退出(TODO)
    GHashTable *clientSymKey;
    mtx_t clientSymKeyMtx;
} PasskeyServiceData;

// 使用PasskeyServiceData前必须先init
int service_passkey_data_init(PasskeyServiceData *passkeyData);
int service_passkey_data_free(PasskeyServiceData *passkeyData);

// PasskeyServiceData.customData
int service_custom_data_get(Service *srv, const gchar *key, gchar **value);
int service_custom_data_set(Service *srv, const gchar *key, const gchar *value);
int service_custom_data_delete(Service *srv, const gchar *key);

// PasskeyServiceData.selectedDevice
// sender->dev
int service_selected_device_use_start(Service *srv, const gchar *sender, fido_dev_t **dev);
// 使用start了，一定得对应使用end !!!!
int service_selected_device_use_end(Service *srv, const gchar *sender, fido_dev_t *dev);
int service_selected_device_add(Service *srv, const gchar *sender, fido_dev_t *dev);
int service_selected_device_delete(Service *srv, const gchar *sender);

// PasskeyServiceData.dealDevices
// sender->calllist->devlist
// 使用start了，一定得对应使用end !!!!
int service_deal_devices_list_use_start(Service *srv, const gchar *sender, const gchar *callId, GList **devList);
int service_deal_devices_list_use_end(Service *srv, const gchar *sender, const gchar *callId, GList *devList);
int service_deal_devices_add_list(Service *srv, const gchar *sender, const gchar *callId, GList *devList);
int service_deal_devices_delete(Service *srv, const gchar *sender);
int service_deal_devices_delete_list(Service *srv, const gchar *sender, const gchar *callId);

// PasskeyServiceData.serviceKey
int service_service_asym_key_get(Service *srv, int type, unsigned char **asymKey);
int service_service_asym_key_set(Service *srv, int type, const unsigned char *asymKey);
int service_service_asym_key_delete(Service *srv, int type);
// PasskeyServiceData.clientKey
int service_client_sym_key_get(Service *srv, const gchar *sender, int *type, unsigned char **symKey);
int service_client_sym_key_set(Service *srv, const gchar *sender, int type, const unsigned char *symKey);
int service_client_sym_key_delete(Service *srv, const gchar *sender);

#ifdef __cplusplus
}
#endif