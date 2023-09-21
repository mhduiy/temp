// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dev.h"

#include "common/common.h"
#include "common/log.h"
#include "servicesignal.h"

#include <stdbool.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>

#define TIMEOUT 15
#define FREQUENCY 1

// 寻找设备，如果没有插入的设备则等待，有超时时间
int dpk_dev_devs_find_wait(fido_dev_info_t **devList, size_t *nDevs)
{
    fido_dev_info_t *devListTemp = NULL;
    size_t nDevsTemp = 0;
    int callRet = FIDO_ERR_INTERNAL;

    if (devList == NULL) {
        LOG(LOG_ERR, "error: fido_dev_info_new failed");
        goto end;
    }

    devListTemp = fido_dev_info_new(DEV_NUM_MAX);
    if (!devListTemp) {
        LOG(LOG_ERR, "error: fido_dev_info_new failed");
        goto end;
    }

    int ret = fido_dev_info_manifest(devListTemp, DEV_NUM_MAX, &nDevsTemp);
    if (ret != FIDO_OK) {
        LOG(LOG_ERR, "Unable to discover device(s), %s (%d)", fido_strerr(ret), ret);
        goto end;
    }

    if (nDevsTemp == 0) {
        for (int i = 0; i < TIMEOUT; i++) {
            LOG(LOG_DEBUG,
                     "\rNo U2F device available, please insert one now,"
                     "you have %2d seconds",
                     TIMEOUT - i);
            thrd_sleep(&(struct timespec){ .tv_sec = FREQUENCY }, NULL);
            ret = fido_dev_info_manifest(devListTemp, DEV_NUM_MAX, &nDevsTemp);
            if (ret != FIDO_OK) {
                LOG(LOG_ERR, "Unable to discover device(s), %s (%d)", fido_strerr(ret), ret);
                break;
            }
            if (nDevsTemp != 0) {
                LOG(LOG_INFO, "Device found!");
                break;
            }
        }
    }
    if (nDevsTemp == 0) {
        callRet = FIDO_ERR_TIMEOUT;
        goto end;
    }

    *nDevs = nDevsTemp;
    *devList = devListTemp;

    callRet = FIDO_OK;
end:
    if (callRet != FIDO_OK && devListTemp != NULL) {
        fido_dev_info_free(&devListTemp, nDevsTemp);
    }
    return callRet;
}

// 寻找当前已插入的设备
int dpk_dev_devs_find_existed(fido_dev_info_t **devList, size_t *nDevs)
{
    fido_dev_info_t *devListTemp = NULL;
    size_t nDevsTemp = 0;
    int callRet = FIDO_ERR_INTERNAL;

    if (devList == NULL) {
        LOG(LOG_ERR, "error: fido_dev_info_new failed");
        goto end;
    }

    devListTemp = fido_dev_info_new(DEV_NUM_MAX);
    if (!devListTemp) {
        LOG(LOG_ERR, "error: fido_dev_info_new failed");
        goto end;
    }

    callRet = fido_dev_info_manifest(devListTemp, DEV_NUM_MAX, &nDevsTemp);
    if (callRet != FIDO_OK) {
        LOG(LOG_ERR, "Unable to discover device(s), %s (%d)", fido_strerr(callRet), callRet);
        goto end;
    }

    if (nDevsTemp == 0) {
        callRet = FIDO_ERR_NOTFOUND;
        goto end;
    }

    *nDevs = nDevsTemp;
    *devList = devListTemp;

    callRet = FIDO_OK;
end:
    if (callRet != FIDO_OK && devListTemp != NULL) {
        fido_dev_info_free(&devListTemp, nDevsTemp);
    }
    return callRet;
}

// 从设备列表中选择一个设备。方式是获取默认设备，目前简单设计，默认是获取第一个设备
// 该方式支持不存在插入的设备时，等待设备插入立即识别
int dpk_dev_get_default_dev(bool isWaitPlugin, fido_dev_t **device)
{
    size_t nDevs = 0;
    fido_dev_info_t *devList = NULL;
    const fido_dev_info_t *di = NULL;
    const char *path = NULL;
    fido_dev_t *dev = NULL;
    int callRet = FIDO_ERR_INTERNAL;

    if (device == NULL) {
        LOG(LOG_ERR, "param invalid");
        goto end;
    }

    if (isWaitPlugin) {
        if ((callRet = dpk_dev_devs_find_wait(&devList, &nDevs)) != FIDO_OK) {
            goto end;
        }
    } else {
        if ((callRet = dpk_dev_devs_find_existed(&devList, &nDevs)) != FIDO_OK) {
            goto end;
        }
    }

    if (nDevs == 0) {
        goto end;
    }
    di = fido_dev_info_ptr(devList, 0); // 默认第一个设备
    if (!di) {
        LOG(LOG_ERR, "error: fido_dev_info_ptr failed");
        goto end;
    }
    if ((path = fido_dev_info_path(di)) == NULL) {
        LOG(LOG_ERR, "error: fido_dev_info_path failed");
        goto end;
    }
    dev = fido_dev_new();
    if (!dev) {
        LOG(LOG_ERR, "fido_dev_new failed");
        goto end;
    }
    callRet = fido_dev_open(dev, path);
    if (callRet != FIDO_OK) {
        LOG(LOG_ERR, "error: fido_dev_open (%d) %s", callRet, fido_strerr(callRet));
        goto end;
    }
    LOG(LOG_INFO, "default device found");
    *device = dev;
    callRet = FIDO_OK;
end:
    if (devList != NULL) {
        fido_dev_info_free(&devList, nDevs);
    }
    if (dev != NULL && callRet != FIDO_OK) {
        fido_dev_close(dev);
        fido_dev_free(&dev);
    }
    return callRet;
}

// authenticatorSelection (0x0B)的支持
// 从设备列表中选择一个设备。选择方式是交互式的，通过用户去通过在场证明的方式选择设备，存在阻塞
// 该方式只支持已插入的设备
int dpk_dev_dev_select(fido_dev_t **dev)
{
    size_t nDevs = 0;
    fido_dev_info_t *devList = NULL;
    fido_dev_t **devTab; // 识别到的设备都创建对象
    size_t nOpen = 0;
    struct timespec tsStart;
    struct timespec tsNow;
    struct timespec tsDelta;
    long msRemain;
    int callRet = FIDO_ERR_INTERNAL;

    const fido_dev_info_t *di;

    LOG(LOG_DEBUG, "authenticator select start.");
    if (dev == NULL) {
        LOG(LOG_ERR, "param invalid");
        goto end;
    }
    *dev = NULL;

    if ((callRet = dpk_dev_devs_find_existed(&devList, &nDevs)) != FIDO_OK) {
        LOG(LOG_ERR, "dev find error.");
        goto end;
    }
    LOG(LOG_DEBUG, "found %ld authenticators.", nDevs);
    if (devList == NULL || nDevs == 0) {
        LOG(LOG_ERR, "can not find authenticators.");
        callRet = FIDO_ERR_NOTFOUND;
        goto end;
    }

    if ((devTab = calloc(nDevs, sizeof(*devTab))) == NULL) {
        LOG(LOG_ERR, "devTab init failed.");
        goto end;
    }

    for (size_t i = 0; i < nDevs; i++) {
        devTab[i] = NULL;
        fido_dev_t *devTemp = fido_dev_new();
        if (devTemp == NULL) {
            continue;
        }
        di = fido_dev_info_ptr(devList, i);
        if (!di) {
            fido_dev_free(&devTemp);
            continue;
        }
        if (fido_dev_open(devTemp, fido_dev_info_path(di)) != FIDO_OK) {
            fido_dev_free(&devTemp);
            continue;
        }
        if (fido_dev_set_timeout(devTemp, 15 * 1000) != FIDO_OK) {
            LOG(LOG_ERR, "set timeout failed.");
        }
        devTab[i] = devTemp;
        nOpen++;
    }
    LOG(LOG_DEBUG, "open %ld authenticators", nOpen);

    if (nOpen == 0) {
        LOG(LOG_ERR, "No authenticators are opened.");
        goto end;
    }

    for (size_t i = 0; i < nDevs; i++) {
        di = fido_dev_info_ptr(devList, i);
        if (devTab[i] == NULL) {
            continue;
        }
        LOG(LOG_DEBUG, "authenticator %s touch begin...", fido_dev_info_path(di));
        if (fido_dev_get_touch_begin(devTab[i]) != FIDO_OK) {
            LOG(LOG_WARNING, "authenticator touch begin failed.");
            continue;
        }
    }

    // 等待触摸，超时30s
    // 此为接口超时，非设备超时。 如果设备全部低于30秒已全部超时，这里会提前结束
    int secTimeout = 30;
    if (clock_gettime(CLOCK_MONOTONIC, &tsStart) != 0) {
        LOG(LOG_ERR, "clock_gettime error.");
        goto end;
    }

    int touched;
    const int pollMs = 50;
    LOG(LOG_DEBUG, "touch wait...");
    do {
        thrd_sleep(&(struct timespec){ .tv_sec = 0, .tv_nsec = 200000000 }, NULL); // 间隔200ms

        if (nOpen == 0) {
            goto end;
        }

        for (size_t i = 0; i < nDevs; i++) {
            di = fido_dev_info_ptr(devList, i);
            if (devTab[i] == NULL) {
                continue;
            }
            int ret = 0;
            if ((ret = fido_dev_get_touch_status(devTab[i], &touched, pollMs)) != FIDO_OK) {
                // 该设备已经失败，从等待的设备列表中去掉
                LOG(LOG_INFO, "authenticator %s error:%s", fido_dev_info_path(di), fido_strerr(ret));
                fido_dev_close(devTab[i]);
                fido_dev_free(&devTab[i]);
                devTab[i] = NULL;
                nOpen--;
                continue;
            }
            if (touched) {
                *dev = devTab[i];
                callRet = FIDO_OK;
                LOG(LOG_DEBUG, "authenticator %s touch success", fido_dev_info_path(di));
                goto end;
            }
        }

        if (clock_gettime(CLOCK_MONOTONIC, &tsNow) != 0) {
            LOG(LOG_ERR, "clock_gettime error.");
            goto end;
        }

        tsDelta.tv_sec = tsNow.tv_sec - tsStart.tv_sec;
        tsDelta.tv_nsec = tsNow.tv_nsec - tsStart.tv_nsec;
        if (tsDelta.tv_nsec < 0) {
            tsDelta.tv_sec--;
            tsDelta.tv_nsec += 1000000000L;
        }
        msRemain = (secTimeout * 1000) - ((long)tsDelta.tv_sec * 1000) + ((long)tsDelta.tv_nsec / 1000000);
    } while (msRemain > pollMs);

    if (msRemain <= pollMs) {
        LOG(LOG_ERR, "error:select timeout.");
        goto end;
    }

    callRet = FIDO_OK;
end:
    LOG(LOG_DEBUG, "dev select finish, ret code:%d", callRet);
    if (devTab != NULL) {
        for (size_t i = 0; i < nDevs; i++) {
            if (devTab[i] && devTab[i] != *dev) {
                fido_dev_cancel(devTab[i]);
                fido_dev_close(devTab[i]);
                fido_dev_free(&devTab[i]);
            }
        }
        free(devTab);
    }
    if (devList != NULL) {
        fido_dev_info_free(&devList, nDevs);
    }
    return callRet;
}