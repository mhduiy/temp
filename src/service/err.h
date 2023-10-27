#pragma once

#include <common/errcode.h>

#ifdef __cplusplus
extern "C" {
#endif

int dp_err_code_to_json(int code, char **json);

#ifdef __cplusplus
}
#endif