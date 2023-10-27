// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/json.h"

#include <json-c/json.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int dp_result_to_json(const DpApiResult *apiResult, char **jsonData)
{
    struct json_object *jsonObject = NULL;
    struct json_object *jsonCode = NULL;
    struct json_object *jsonMsg = NULL;
    const char *jsonStr = NULL;
    int dataLen = 0;
    int ret = -1;

    if (jsonData == NULL || apiResult == NULL) {
        goto end;
    }

    jsonObject = json_object_new_object();

    jsonCode = json_object_new_int(apiResult->code);
    json_object_object_add(jsonObject, "code", jsonCode);

    jsonMsg = json_object_new_string(apiResult->msg);
    json_object_object_add(jsonObject, "msg", jsonMsg);

    jsonStr = json_object_to_json_string(jsonObject);

    dataLen = strlen(jsonStr);
    *jsonData = calloc(dataLen + 1, sizeof(char));
    strncpy(*jsonData, jsonStr, dataLen);
    (*jsonData)[dataLen] = 0;
    ret = 0;
end:
    if (jsonObject != NULL) {
        json_object_put(jsonObject);
    }
    return ret;
}

int dp_json_to_result(const char *jsonData, DpApiResult *apiResult)
{
    struct json_tokener *jsonTokener = NULL;
    struct json_object *parsedJson = NULL;
    struct json_object *jsonCode = NULL;
    struct json_object *jsonMsg = NULL;
    enum json_tokener_error jsonErr;
    int ret = -1;

    if (jsonData == NULL || apiResult == NULL) {
        goto end;
    }

    jsonTokener = json_tokener_new();
    if (jsonTokener == NULL) {
        goto end;
    }

    parsedJson = json_tokener_parse_ex(jsonTokener, jsonData, (int)strlen(jsonData));
    if ((jsonErr = json_tokener_get_error(jsonTokener)) != json_tokener_success) {
        // json_tokener_error_desc(jerr)
        goto end;
    }
    if (!json_object_object_get_ex(parsedJson, "code", &jsonCode)) {
        goto end;
    }
    if (!json_object_object_get_ex(parsedJson, "msg", &jsonMsg)) {
        goto end;
    }

    apiResult->code = json_object_get_int(jsonCode);
    snprintf(apiResult->msg, DP_API_RESULT_MSG_LEN, "%s", json_object_get_string(jsonMsg));

    ret = 0;
end:
    if (jsonTokener != NULL) {
        json_tokener_free(jsonTokener);
    }
    return ret;
}
