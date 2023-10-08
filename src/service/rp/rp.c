// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rp.h"

#include "common/common.h"
#include "common/log.h"

#include <fido.h>

#include <stdlib.h>

int dpk_rp_get_rp_id(char **rpId)
{
    char *id = NULL;
    char *rpIdTemp = NULL;
    int callRet = FIDO_ERR_INTERNAL;

    if (!read_file_line("/sys/devices/virtual/dmi/id/product_uuid", &id) || id == NULL) {
        LOG(LOG_WARNING, "can not find product_uuid");
        // 获取不到product_uuid就使用hostname
        if (id != NULL) {
            free(id);
        }
        if (!read_file_line("/etc/hostname", &id) || id == NULL) {
            LOG(LOG_ERR, "can not find product_uuid and hostname");
            goto end;
        }
    }
    rpIdTemp = calloc(BUFSIZE + 1, sizeof(char));
    if (snprintf(rpIdTemp, BUFSIZE, "local://%s", id) < 0) {
        goto end;
    }
    *rpId = rpIdTemp;
    callRet = FIDO_OK;
end:
    if (id != NULL) {
        free(id);
    }
    if (callRet != FIDO_OK && rpIdTemp != NULL) {
        free(rpIdTemp);
    }
    return callRet;
}