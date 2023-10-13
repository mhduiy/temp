#include "servicedata.h"

#include "common/common.h"
#include "common/log.h"

#include <fido.h>

#include <stdio.h>

typedef struct
{
    bool needDelete;
    size_t usedCount;
    fido_dev_t *dev;
} SelectDeviceData;

typedef struct
{
    bool needDelete;
    size_t usedCount;
    GList *devList;
} DealDeviceData;

int service_custom_data_get(Service *srv, const gchar *key, gchar **value)
{
    int ret = -1;
    if (srv == NULL || srv->customData == NULL || key == NULL || value == NULL) {
        LOG(LOG_ERR, "param invalid");
        return ret;
    }
    PasskeyServiceData *passkeyData = (PasskeyServiceData *)srv->customData;
    mtx_lock(&(passkeyData->customDataMtx));
    do {
        if (passkeyData->customData == NULL) {
            break;
        }
        gchar *v = (gchar *)g_hash_table_lookup(passkeyData->customData, key);
        *value = g_strdup(v);
        ret = 0;
    } while (0);

    mtx_unlock(&(passkeyData->customDataMtx));

    return ret;
}

int service_custom_data_set(Service *srv, const gchar *key, const gchar *value)
{
    int ret = -1;
    if (srv == NULL || srv->customData == NULL || key == NULL || value == NULL) {
        LOG(LOG_ERR, "param invalid");
        return ret;
    }
    PasskeyServiceData *passkeyData = (PasskeyServiceData *)srv->customData;
    mtx_lock(&(passkeyData->customDataMtx));
    do {
        if (passkeyData->customData == NULL) {
            break;
        }
        g_hash_table_insert(passkeyData->customData, g_strdup(key), g_strdup(value));
        ret = 0;
    } while (0);
    mtx_unlock(&(passkeyData->customDataMtx));

    return ret;
}

int service_custom_data_delete(Service *srv, const gchar *key)
{
    int ret = -1;
    if (srv == NULL || srv->customData == NULL || key == NULL) {
        LOG(LOG_ERR, "param invalid");
        return ret;
    }
    PasskeyServiceData *passkeyData = (PasskeyServiceData *)srv->customData;
    mtx_lock(&(passkeyData->customDataMtx));
    do {
        if (passkeyData->customData == NULL) {
            break;
        }
        g_hash_table_remove(passkeyData->customData, key);
        ret = 0;
    } while (0);

    mtx_unlock(&(passkeyData->customDataMtx));

    return ret;
}

static void free_custom_data(gpointer data)
{
    if (data == NULL) {
        return;
    }
    g_free(data);
}

int service_selected_device_use_start(Service *srv, const gchar *sender, fido_dev_t **dev)
{
    int ret = -1;
    if (srv == NULL || srv->customData == NULL || sender == NULL || dev == NULL) {
        LOG(LOG_ERR, "param invalid");
        return ret;
    }
    PasskeyServiceData *passkeyData = (PasskeyServiceData *)srv->customData;

    mtx_lock(&(passkeyData->selectedDeviceMtx));
    do {
        if (passkeyData->selectedDevice == NULL) {
            LOG(LOG_ERR, "selected device is not exist");
            break;
        }
        SelectDeviceData *v = (SelectDeviceData *)g_hash_table_lookup(passkeyData->selectedDevice, sender);
        if (v == NULL) {
            ret = 0; // 不存在认为是没用此功能，不算错误
            break;
        }
        if (v->needDelete) {
            LOG(LOG_WARNING, "need delete, and can not to use");
            break;
        }
        if (v->dev == NULL) {
            LOG(LOG_WARNING, "dev is not existed");
            break;
        }
        v->usedCount++;
        *dev = v->dev;
        ret = 0;
    } while (0);
    mtx_unlock(&(passkeyData->selectedDeviceMtx));

    return ret;
}

int service_selected_device_use_end(Service *srv, const gchar *sender, fido_dev_t *dev)
{
    int ret = -1;
    if (srv == NULL || srv->customData == NULL || sender == NULL || dev == NULL) {
        LOG(LOG_ERR, "param invalid");
        return ret;
    }
    PasskeyServiceData *passkeyData = (PasskeyServiceData *)srv->customData;

    mtx_lock(&(passkeyData->selectedDeviceMtx));
    do {
        if (passkeyData->selectedDevice == NULL) {
            LOG(LOG_WARNING, "selected device is not exist");
            break;
        }
        SelectDeviceData *v = (SelectDeviceData *)g_hash_table_lookup(passkeyData->selectedDevice, sender);
        if (v == NULL) {
            LOG(LOG_WARNING, "this sender has not device");
            break;
        }
        if (dev != v->dev) {
            LOG(LOG_WARNING, "dev is invalid");
            break;
        }
        v->usedCount--;
        if (v->needDelete && v->usedCount == 0) {
            g_hash_table_remove(passkeyData->selectedDevice, sender);
        }
        ret = 0;
    } while (0);
    mtx_unlock(&(passkeyData->selectedDeviceMtx));

    return ret;
}

int service_selected_device_add(Service *srv, const gchar *sender, fido_dev_t *dev)
{
    int ret = -1;
    if (srv == NULL || srv->customData == NULL || sender == NULL || dev == NULL) {
        LOG(LOG_ERR, "param invalid");
        return ret;
    }
    PasskeyServiceData *passkeyData = (PasskeyServiceData *)srv->customData;
    mtx_lock(&(passkeyData->selectedDeviceMtx));
    do {
        if (passkeyData->selectedDevice == NULL) {
            LOG(LOG_WARNING, "selected device is not exist");
            break;
        }
        SelectDeviceData *v = (SelectDeviceData *)g_hash_table_lookup(passkeyData->selectedDevice, sender);
        if (v != NULL) {
            // 存在则失败，需要先取消选中
            LOG(LOG_WARNING, "selected device is existed, can not to add");
            break;
        }
        SelectDeviceData *sd = (SelectDeviceData *)calloc(1, sizeof(SelectDeviceData));
        sd->dev = dev;
        sd->needDelete = false;
        sd->usedCount = 0;
        g_hash_table_insert(passkeyData->selectedDevice, g_strdup(sender), sd);
        ret = 0;
    } while (0);

    mtx_unlock(&(passkeyData->selectedDeviceMtx));

    return ret;
}

int service_selected_device_delete(Service *srv, const gchar *sender)
{
    int ret = -1;
    if (srv == NULL || srv->customData == NULL || sender == NULL) {
        LOG(LOG_ERR, "param invalid");
        return ret;
    }
    PasskeyServiceData *passkeyData = (PasskeyServiceData *)srv->customData;
    mtx_lock(&(passkeyData->selectedDeviceMtx));
    do {
        if (passkeyData->selectedDevice == NULL) {
            LOG(LOG_WARNING, "selected device is not exist");
            break;
        }

        SelectDeviceData *v = (SelectDeviceData *)g_hash_table_lookup(passkeyData->selectedDevice, sender);
        if (v != NULL) {
            if (v->usedCount == 0) {
                g_hash_table_remove(passkeyData->selectedDevice, sender);
            } else {
                v->needDelete = true;
            }
        }
        ret = 0;
    } while (0);

    mtx_unlock(&(passkeyData->selectedDeviceMtx));

    return ret;
}

static void free_selected_device_key(gpointer data)
{
    if (data == NULL) {
        return;
    }
    g_free(data);
}

static void free_selected_device_value(gpointer data)
{
    if (data == NULL) {
        return;
    }

    SelectDeviceData *sd = (SelectDeviceData *)data;
    if (sd != NULL) {
        if (sd->dev != NULL) {
            fido_dev_cancel(sd->dev);
            fido_dev_close(sd->dev);
            fido_dev_free(&(sd->dev));
        }
        free(sd);
    }
}

static void free_deal_devices_list_key(gpointer data)
{
    if (data == NULL) {
        return;
    }
    g_free(data);
}

static void free_deal_devices_list_value_func(gpointer data)
{
    if (data == NULL) {
        return;
    }
    fido_dev_t *dev = (fido_dev_t *)data;
    if (dev != NULL) {
        fido_dev_cancel(dev);
        fido_dev_close(dev);
        fido_dev_free(&dev);
    }
}

static void free_deal_devices_list_value(gpointer data)
{
    LOG(LOG_INFO, "data delete: callid->devlist");
    if (data == NULL) {
        return;
    }

    DealDeviceData *dd = (DealDeviceData *)data;
    if (dd != NULL) {
        if (dd->devList) {
            g_list_free_full(dd->devList, free_deal_devices_list_value_func);
        }
        free(dd);
    }
}

int service_deal_devices_list_use_start(Service *srv, const gchar *sender, const gchar *callId, GList **devList)
{
    int ret = -1;
    if (srv == NULL || srv->customData == NULL || sender == NULL || callId == NULL || devList == NULL) {
        LOG(LOG_ERR, "param invalid");
        return ret;
    }
    LOG(LOG_INFO, "start to use deal-devices list, sender:%s, callid:%s", sender, callId);

    PasskeyServiceData *passkeyData = (PasskeyServiceData *)srv->customData;

    mtx_lock(&(passkeyData->dealDevicesMtx));
    do {
        if (passkeyData->dealDevices == NULL) {
            LOG(LOG_WARNING, "passkey service data is invalid");
            break;
        }
        GHashTable *devTable = (GHashTable *)g_hash_table_lookup(passkeyData->dealDevices, sender);
        if (devTable == NULL) {
            LOG(LOG_WARNING, "this sender has not device list");
            break;
        }
        DealDeviceData *dd = (DealDeviceData *)g_hash_table_lookup(devTable, callId);
        if (dd == NULL) {
            LOG(LOG_WARNING, "this callid has not device list");
            break;
        }
        if (dd->needDelete) {
            LOG(LOG_WARNING, "need delete, and can not to use");
            break;
        }
        LOG(LOG_INFO, "start to use deal-devices list sucess, callid:%s", callId);
        dd->usedCount++;
        *devList = dd->devList;
        ret = 0;
    } while (0);
    mtx_unlock(&(passkeyData->dealDevicesMtx));

    return ret;
}

int service_deal_devices_list_use_end(Service *srv, const gchar *sender, const gchar *callId, GList *devList)
{
    int ret = -1;
    if (srv == NULL || srv->customData == NULL || sender == NULL || callId == NULL || devList == NULL) {
        LOG(LOG_ERR, "param invalid");
        return ret;
    }
    LOG(LOG_INFO, "end to use deal-devices list, sender:%s, callid:%s", sender, callId);

    PasskeyServiceData *passkeyData = (PasskeyServiceData *)srv->customData;

    mtx_lock(&(passkeyData->dealDevicesMtx));
    do {
        if (passkeyData->dealDevices == NULL) {
            LOG(LOG_WARNING, "passkey service data is invalid");
            break;
        }
        GHashTable *devTable = (GHashTable *)g_hash_table_lookup(passkeyData->dealDevices, sender);
        if (devTable == NULL) {
            LOG(LOG_WARNING, "this sender has not device list");
            break;
        }
        DealDeviceData *dd = (DealDeviceData *)g_hash_table_lookup(devTable, callId);
        if (dd == NULL) {
            LOG(LOG_WARNING, "this callid has not device list");
            break;
        }
        if (dd->devList != devList) {
            LOG(LOG_WARNING, "devlist is invalid");
            break;
        }
        LOG(LOG_INFO, "end to use deal-devices list sucess, callid:%s", callId);
        dd->usedCount--;
        if (dd->needDelete && dd->usedCount == 0) {
            LOG(LOG_INFO, "end to use deal-devices list, and to delete devlist, callid:%s", callId);
            if (!g_hash_table_remove(devTable, callId)) {
                LOG(LOG_WARNING, "remove map error");
                break;
            }
        }
        ret = 0;
    } while (0);
    mtx_unlock(&(passkeyData->dealDevicesMtx));

    return ret;
}

int service_deal_devices_add_list(Service *srv, const gchar *sender, const gchar *callId, GList *devList)
{
    int ret = -1;
    if (srv == NULL || srv->customData == NULL || sender == NULL || callId == NULL || devList == NULL) {
        LOG(LOG_ERR, "param invalid");
        return ret;
    }
    LOG(LOG_INFO, "to add deal-devices list, sender:%s, callid:%s", sender, callId);

    PasskeyServiceData *passkeyData = (PasskeyServiceData *)srv->customData;
    mtx_lock(&(passkeyData->dealDevicesMtx));
    do {
        if (passkeyData->dealDevices == NULL) {
            LOG(LOG_WARNING, "passkey service data is invalid");
            break;
        }

        GHashTable *devTable = (GHashTable *)g_hash_table_lookup(passkeyData->dealDevices, sender);
        if (devTable == NULL) {
            LOG(LOG_INFO, "sender->callmap is not exist, and to create:%s", sender);
            devTable = g_hash_table_new_full(g_str_hash, g_str_equal, free_deal_devices_list_key, free_deal_devices_list_value);
            if (devTable == NULL) {
                LOG(LOG_ERR, "create dev map failed");
                break;
            }
            if (!g_hash_table_insert(passkeyData->dealDevices, g_strdup(sender), devTable)) {
                LOG(LOG_ERR, "insert dev map failed");
                break;
            }
        }

        DealDeviceData *ddExist = (DealDeviceData *)g_hash_table_lookup(devTable, callId);
        if (ddExist != NULL) {
            LOG(LOG_WARNING, "deal devices is existed, can not to add");
            break;
        }

        DealDeviceData *dd = (DealDeviceData *)calloc(1, sizeof(DealDeviceData));
        dd->needDelete = false;
        dd->usedCount = 0;
        dd->devList = devList;

        LOG(LOG_INFO, "add deal-devices list success, callid:%s", callId);
        g_hash_table_insert(devTable, g_strdup(callId), dd);
        ret = 0;
    } while (0);

    mtx_unlock(&(passkeyData->dealDevicesMtx));

    return ret;
}

static gboolean service_deal_devices_delete_func(gpointer key, gpointer value, gpointer user_data)
{
    UNUSED_VALUE(key);
    UNUSED_VALUE(user_data);
    if (value == NULL) {
        return TRUE;
    }
    DealDeviceData *dd = (DealDeviceData *)value;
    if (dd->usedCount == 0) {
        return TRUE;
    } else {
        LOG(LOG_WARNING, "can not to delete, the data is using");
        dd->needDelete = true;
    }
    return FALSE; // FALSE: 暂不删除
}

int service_deal_devices_delete(Service *srv, const gchar *sender)
{
    int ret = -1;
    if (srv == NULL || srv->customData == NULL || sender == NULL) {
        LOG(LOG_ERR, "param invalid");
        return ret;
    }
    PasskeyServiceData *passkeyData = (PasskeyServiceData *)srv->customData;
    mtx_lock(&(passkeyData->dealDevicesMtx));
    do {
        if (passkeyData->dealDevices == NULL) {
            LOG(LOG_WARNING, "passkey service data is invalid");
            break;
        }
        GHashTable *devTable = (GHashTable *)g_hash_table_lookup(passkeyData->dealDevices, sender);
        if (devTable != NULL) {
            g_hash_table_foreach_remove(devTable, service_deal_devices_delete_func, NULL);
            if (g_hash_table_size(devTable) == 0) {
                if (!g_hash_table_remove(passkeyData->dealDevices, sender)) {
                    LOG(LOG_ERR, "remove map error");
                    break;
                }
            }
        }
        ret = 0;
    } while (0);

    mtx_unlock(&(passkeyData->dealDevicesMtx));

    return ret;
}

int service_deal_devices_delete_list(Service *srv, const gchar *sender, const gchar *callId)
{
    int ret = -1;
    if (srv == NULL || srv->customData == NULL || sender == NULL || callId == NULL) {
        LOG(LOG_ERR, "param invalid");
        return ret;
    }
    LOG(LOG_INFO, "to delete deal-devices list, sender:%s, call id:%s", sender, callId);

    PasskeyServiceData *passkeyData = (PasskeyServiceData *)srv->customData;
    mtx_lock(&(passkeyData->dealDevicesMtx));
    do {
        if (passkeyData->dealDevices == NULL) {
            LOG(LOG_WARNING, "passkey service data is invalid");
            break;
        }

        GHashTable *devTable = (GHashTable *)g_hash_table_lookup(passkeyData->dealDevices, sender);
        if (devTable != NULL) {
            DealDeviceData *dd = (DealDeviceData *)g_hash_table_lookup(devTable, callId);
            if (dd != NULL) {
                if (dd->usedCount == 0) {
                    if (!g_hash_table_remove(devTable, callId)) {
                        LOG(LOG_ERR, "remove map error");
                        break;
                    }
                    LOG(LOG_INFO, "delete deal-devices list success, call id:%s", callId);
                    if (g_hash_table_size(devTable) == 0) {
                        if (!g_hash_table_remove(passkeyData->dealDevices, sender)) {
                            LOG(LOG_ERR, "remove map error");
                            break;
                        }
                    }
                } else {
                    LOG(LOG_INFO, "delete deal-devices list failed, wait to delete again when is not used, call id:%s", callId);
                    dd->needDelete = true;
                }
            }
        }
        ret = 0;
    } while (0);

    mtx_unlock(&(passkeyData->dealDevicesMtx));

    return ret;
}

static void free_deal_devices_key(gpointer data)
{
    if (data == NULL) {
        return;
    }
    g_free(data);
}

static void free_deal_devices_value(gpointer data)
{
    LOG(LOG_INFO, "data delete: sender->callidmap");
    if (data == NULL) {
        return;
    }

    GHashTable *list = (GHashTable *)data;
    if (list != NULL) {
        g_hash_table_destroy(list);
    }
}

int service_passkey_data_init(PasskeyServiceData *passkeyData)
{
    if (passkeyData == NULL) {
        return -1;
    }
    passkeyData->customData = g_hash_table_new_full(g_str_hash, g_str_equal, free_custom_data, free_custom_data);
    mtx_init(&(passkeyData->customDataMtx), mtx_plain);

    passkeyData->selectedDevice = g_hash_table_new_full(g_str_hash, g_str_equal, free_selected_device_key, free_selected_device_value);
    mtx_init(&(passkeyData->selectedDeviceMtx), mtx_plain);

    passkeyData->dealDevices = g_hash_table_new_full(g_str_hash, g_str_equal, free_deal_devices_key, free_deal_devices_value);
    mtx_init(&(passkeyData->dealDevicesMtx), mtx_plain);

    return 0;
}

int service_passkey_data_free(PasskeyServiceData *passkeyData)
{
    if (passkeyData == NULL) {
        return -1;
    }
    mtx_destroy(&(passkeyData->customDataMtx));
    if (passkeyData->customData != NULL) {
        g_hash_table_destroy(passkeyData->customData);
    }

    mtx_destroy(&(passkeyData->selectedDeviceMtx));
    if (passkeyData->selectedDevice != NULL) {
        g_hash_table_destroy(passkeyData->selectedDevice);
    }

    mtx_destroy(&(passkeyData->dealDevicesMtx));
    if (passkeyData->dealDevices != NULL) {
        g_hash_table_destroy(passkeyData->dealDevices);
    }

    return 0;
}
