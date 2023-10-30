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
        strncpy(apiResult.msg, gettext("Success"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_INVALID_COMMAND) {
        strncpy(apiResult.msg, gettext("Invalid command"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_INVALID_PARAMETER) {
        strncpy(apiResult.msg, gettext("Invalid parameter"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_INVALID_LENGTH) {
        strncpy(apiResult.msg, gettext("Invalid length"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_TIMEOUT) {
        strncpy(apiResult.msg, gettext("Timeout"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_CHANNEL_BUSY) {
        strncpy(apiResult.msg, gettext("Channel busy"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_LOCK_REQUIRED) {
        strncpy(apiResult.msg, gettext("Lock required"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_INVALID_CHANNEL) {
        strncpy(apiResult.msg, gettext("Invalid channel"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_UNSUPPORTED_EXTENSION) {
        strncpy(apiResult.msg, gettext("Unsupported"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_INVALID_CREDENTIAL) {
        strncpy(apiResult.msg, gettext("Invalid credential"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_USER_ACTION_PENDING) {
        strncpy(apiResult.msg, gettext("Touch or swipe the security key"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_UNSUPPORTED_ALGORITHM) {
        strncpy(apiResult.msg, gettext("Unsupported algorithm"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_OPERATION_DENIED) {
        strncpy(apiResult.msg, gettext("Operation denied"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_UNSUPPORTED_OPTION) {
        strncpy(apiResult.msg, gettext("Unsupported option"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_INVALID_OPTION) {
        strncpy(apiResult.msg, gettext("Invalid option"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_NO_CREDENTIALS) {
        strncpy(apiResult.msg, gettext("No credentials"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_USER_ACTION_TIMEOUT) {
        strncpy(apiResult.msg, gettext("Validation timed out"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_PIN_INVALID) {
        strncpy(apiResult.msg, gettext("Pin invalid"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_PIN_AUTH_INVALID) {
        strncpy(apiResult.msg, gettext("Pin auth invalid"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_PIN_NOT_SET) {
        strncpy(apiResult.msg, gettext("Pin not set"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_PIN_REQUIRED) {
        strncpy(apiResult.msg, gettext("Pin required"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_TX) {
        strncpy(apiResult.msg, gettext("tx error"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_RX) {
        strncpy(apiResult.msg, gettext("rx error"), DP_API_RESULT_MSG_LEN);
    } else if (code == FIDO_ERR_USER_PRESENCE_REQUIRED) {
        strncpy(apiResult.msg, gettext("User persence required"), DP_API_RESULT_MSG_LEN);
    } else if (code == DEEPIN_ERR_DEVICE_CLAIMED) {
        strncpy(apiResult.msg, gettext("Device claimed"), DP_API_RESULT_MSG_LEN);
    } else if (code == DEEPIN_ERR_DEVICE_NOT_FOUND) {
        strncpy(apiResult.msg, gettext("Device not fuound"), DP_API_RESULT_MSG_LEN);
    } else if (code == DEEPIN_ERR_DEVICE_OPEN) {
        strncpy(apiResult.msg, gettext("Device open"), DP_API_RESULT_MSG_LEN);
    } else {
        strncpy(apiResult.msg, gettext("Unknow error"), DP_API_RESULT_MSG_LEN);
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