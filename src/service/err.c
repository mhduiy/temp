// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "err.h"

#include "common/json.h"

#include <libintl.h>

#include <stdio.h>
#include <string.h>

int dp_err_code_to_json(int code, char **json)
{
    DpApiResult apiResult;
    char *jsonData = NULL;
    int ret = -1;

    if (json == NULL) {
        goto end;
    }

    if (code == FIDO_OK) {
        strncpy(apiResult.msg, gettext("成功"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_INVALID_COMMAND) {
        strncpy(apiResult.msg, gettext("非法命令"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_INVALID_PARAMETER) {
        strncpy(apiResult.msg, gettext("非法参数"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_INVALID_LENGTH) {
        strncpy(apiResult.msg, gettext("非法长度"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_TIMEOUT) {
        strncpy(apiResult.msg, gettext("超时"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_CHANNEL_BUSY) {
        strncpy(apiResult.msg, gettext("设备通道忙"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_LOCK_REQUIRED) {
        strncpy(apiResult.msg, gettext("请求锁"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_INVALID_CHANNEL) {
        strncpy(apiResult.msg, gettext("非法设备通道"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_UNSUPPORTED_EXTENSION) {
        strncpy(apiResult.msg, gettext("不支持的拓展"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_INVALID_CREDENTIAL) {
        strncpy(apiResult.msg, gettext("证书错误"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_USER_ACTION_PENDING) {
        strncpy(apiResult.msg, gettext("请验证安全密钥，触摸或轻扫设备"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_UNSUPPORTED_ALGORITHM) {
        strncpy(apiResult.msg, gettext("不支持的算法"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_OPERATION_DENIED) {
        strncpy(apiResult.msg, gettext("操作拒绝"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_UNSUPPORTED_OPTION) {
        strncpy(apiResult.msg, gettext("不支持的操作"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_INVALID_OPTION) {
        strncpy(apiResult.msg, gettext("非法操作"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_NO_CREDENTIALS) {
        strncpy(apiResult.msg, gettext("证书不存在"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_USER_ACTION_TIMEOUT) {
        strncpy(apiResult.msg, gettext("验证安全密钥超时"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_PIN_INVALID) {
        strncpy(apiResult.msg, gettext("PIN错误"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_PIN_AUTH_INVALID) {
        strncpy(apiResult.msg, gettext("PIN校验失败"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_PIN_NOT_SET) {
        strncpy(apiResult.msg, gettext("PIN未设置"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_PIN_REQUIRED) {
        strncpy(apiResult.msg, gettext("请求PIN"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_TX) {
        strncpy(apiResult.msg, gettext("设备传输错误"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_RX) {
        strncpy(apiResult.msg, gettext("设备传输错误"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_USER_PRESENCE_REQUIRED) {
        strncpy(apiResult.msg, gettext("请求在场证明"), DP_API_RESULT_MSG_LEN);
    } else if (code == DEEPIN_ERR_DEVICE_CLAIMED) {
        strncpy(apiResult.msg, gettext("设备被占用"), DP_API_RESULT_MSG_LEN);
    } else if (code == DEEPIN_ERR_DEVICE_NOT_FOUND) {
        strncpy(apiResult.msg, gettext("设备不存在"), DP_API_RESULT_MSG_LEN);
    } else if (code == DEEPIN_ERR_DEVICE_OPEN) {
        strncpy(apiResult.msg, gettext("设备已连接"), DP_API_RESULT_MSG_LEN);
    } else {
        strncpy(apiResult.msg, gettext("未知错误"), DP_API_RESULT_MSG_LEN);
    }

    apiResult.code = code;
    if (dp_result_to_json(&apiResult, &jsonData) < 0) {
        goto end;
    }

    *json = jsonData;
    ret = 0;
end:
    if (ret != 0 && jsonData != NULL) {
        free(jsonData);
    }

    return ret;
}