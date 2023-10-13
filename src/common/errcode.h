#pragma once

#include <fido.h>

#ifdef __cplusplus
extern "C" {
#endif

// 错误码由三部分组成：
// 1 FIDO协议的约定的错误码(>=0)，见<fido/err.h>
// 2 libfido2上游错误码(<=0)，见<fido/err.h>
// 3 deepin自定义错误码，优先使用1和2

#define DEEPIN_ERR_DEVICE_CLAIMED -10011
#define DEEPIN_ERR_DEVICE_NOT_FOUND -10021
#define DEEPIN_ERR_DEVICE_OPEN -10022

#ifdef __cplusplus
}
#endif