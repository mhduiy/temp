// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#define DP_API_RESULT_MSG_LEN 1024

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int code;
    char msg[DP_API_RESULT_MSG_LEN];
} DpApiResult;

int dp_result_to_json(const DpApiResult *apiResult, char **jsonData);
int dp_json_to_result(const char *jsonData, DpApiResult *apiResult);

#ifdef __cplusplus
}
#endif
