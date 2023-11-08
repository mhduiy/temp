// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "info.h"
#include "serviceframe/servicebase.h"

#include <fido.h>

#ifdef __cplusplus
extern "C" {
#endif

// 请求证书参数
typedef struct _CredArgs
{
    const char *appid;  // Relying party name, defaults to the value of origin
    const char *origin; // Relying party ID
    int type;           //  COSE type, algo
    const char *userName;
    const char *credName;
    int resident;
    int noUserPresence;
    int pinVerification;
    int userVerification;
    int debug;
    int verbose;
    int nouser;
    char version[INFO_VERSION_MAX_LEN];
} CredArgs;

typedef struct _CredInfo
{
    char *name;
    char *publicKey;
    char *keyHandle;
    char *coseType;
    char *attributes;
    int oldFormat;
} CredInfo;

// 请求认证参数
typedef struct _AssertArgs
{
    unsigned int maxDevs;
    int manual;
    int debug;
    int noUserOk;
    int openAsUser;
    int alwaysOk;
    int interactive;
    int cue;
    int noDetect;
    int userPresence;
    int userVerification;
    int pinVerification;
    int sshFormat;
    int expand;
    const char *authFile;
    const char *authpendingFile;
    const char *origin;
    const char *appid;
    const char *prompt;
    const char *cuePrompt;
    const char *pin;
    const char *userName;
    const char *credName;
    FILE *debugFile;
} AssertArgs;

// 生成证书的参数
int dpk_dev_prepare_cred(const CredArgs *const args, fido_cred_t **credRes);
// 生成证书
int dpk_dev_make_cred(const CredArgs *args, fido_dev_t *dev, fido_cred_t *cred, fido_cbor_info_t *info, const char *pin);
int dpk_dev_verify_cred(const fido_cred_t *const cred);
// 保存证书到本地
int dk_dev_save_cred(const CredArgs *const args, const fido_cred_t *const cred);
// 本地证书路径
int dk_dev_get_cred_file_path(const char *userName, char **path);
// 读取本地证书
int dk_dev_get_creds_from_file(const char *file, const char *userName, CredInfo *creds, unsigned int *credsCount);
void dk_dev_reset_cred(CredInfo *cred);

// 认证
int dk_dev_has_valid_cred_count(const AssertArgs *args, const CredInfo *creds, const unsigned int credsCount, fido_dev_t *dev, unsigned int *validCredsCount);
int dk_dev_do_authentication(MethodContext *mc, const AssertArgs *args, const CredInfo *devices, const unsigned int credsCount, fido_dev_t *dev);

#ifdef __cplusplus
}
#endif
