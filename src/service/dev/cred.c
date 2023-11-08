// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cred.h"

#include "common/common.h"
#include "common/log.h"
#include "decode/b64.h"
#include "decode/hex.h"
#include "dev.h"
#include "info.h"
#include "servicesignal.h"

#include <fido.h>
#include <fido/eddsa.h>
#include <fido/es256.h>
#include <fido/rs256.h>

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

const char *PASSKEY_CRED_DIR = "/usr/share/deepin-passkey/cred/";

struct opts
{
    fido_opt_t up;
    fido_opt_t uv;
    fido_opt_t pin;
};

struct pk
{
    void *ptr;
    int type;
};

static const char *cose_type_to_string(int type)
{
    switch (type) {
    case COSE_ES256:
        return "es256";
    case COSE_RS256:
        return "rs256";
    case COSE_EDDSA:
        return "eddsa";
    default:
        return "unknown";
    }
}

static bool cose_string_to_type(const char *str, int *type)
{
    if (strcasecmp(str, "es256") == 0) {
        *type = COSE_ES256;
    } else if (strcasecmp(str, "rs256") == 0) {
        *type = COSE_RS256;
    } else if (strcasecmp(str, "eddsa") == 0) {
        *type = COSE_EDDSA;
    } else {
        *type = 0;
        return false;
    }
    return true;
}

// 生成证书参数准备
int dpk_dev_prepare_cred(const CredArgs *const args, fido_cred_t **credRes)
{
    fido_cred_t *cred = NULL;
    const char *appid = NULL;
    const char *user = NULL;
    unsigned char userid[32];
    unsigned char cdh[32];
    char origin[BUFSIZE];
    int callRet = FIDO_ERR_INTERNAL;

    if ((cred = fido_cred_new()) == NULL) {
        LOG(LOG_ERR, "fido_cred_new failed");
        goto end;
    }

    if ((callRet = fido_cred_set_type(cred, args->type)) != FIDO_OK) {
        LOG(LOG_ERR, "fido_cred_set_type (%d): %s", callRet, fido_strerr(callRet));
        goto end;
    }

    if (!random_bytes(cdh, sizeof(cdh))) {
        LOG(LOG_ERR, "random_bytes failed");
        goto end;
    }

    if ((callRet = fido_cred_set_clientdata_hash(cred, cdh, sizeof(cdh))) != FIDO_OK) {
        LOG(LOG_ERR, "fido_cred_set_clientdata_hash (%d): %s", callRet, fido_strerr(callRet));
        goto end;
    }

    if (args->origin == NULL) {
        LOG(LOG_ERR, "origin not existed");
        goto end;
    }
    strncpy(origin, args->origin, sizeof(origin));

    if (args->appid) {
        appid = args->appid;
    } else {
        appid = origin;
    }

    if ((callRet = fido_cred_set_rp(cred, origin, appid)) != FIDO_OK) {
        LOG(LOG_ERR, "error: fido_cred_set_rp (%d) %s", callRet, fido_strerr(callRet));
        goto end;
    }

    if (args->userName == NULL) {
        LOG(LOG_ERR, "user name is not existed");
        goto end;
    }
    user = args->userName;

    if (!random_bytes(userid, sizeof(userid))) {
        LOG(LOG_ERR, "random_bytes failed");
        goto end;
    }

    if ((callRet = fido_cred_set_user(cred, userid, sizeof(userid), user, user, NULL)) != FIDO_OK) {
        LOG(LOG_ERR, "error: fido_cred_set_user (%d) %s", callRet, fido_strerr(callRet));
        goto end;
    }

    // 凭据保护策略，目前使用默认即可
    if ((callRet = fido_cred_set_rk(cred, args->resident ? FIDO_OPT_TRUE : FIDO_OPT_OMIT)) != FIDO_OK) {
        LOG(LOG_ERR, "error: fido_cred_set_rk (%d) %s", callRet, fido_strerr(callRet));
        goto end;
    }
    // 指定认证时，是否需要pin和内置认证（指纹等）
    // 注意：CTAP2.1中不赞成使用此“uv”选项键。应该使用pinUvAuthParam，后续规划支持
    // 如果认证器不支持内置用户验证，则不得包含“uv”选项密钥
    if ((callRet = fido_cred_set_uv(cred, args->userVerification ? FIDO_OPT_TRUE : FIDO_OPT_OMIT)) != FIDO_OK) {
        LOG(LOG_ERR, "fido_cred_set_uv: %s (%d)", fido_strerr(callRet), callRet);
        goto end;
    }

    LOG(LOG_INFO, "prepare cred success, rp:%s, user:%s.", origin, user);
    *credRes = cred;
    callRet = FIDO_OK;

end:
    if (callRet != FIDO_OK && cred != NULL) {
        fido_cred_free(&cred);
    }

    return callRet;
}

int dpk_dev_make_cred(const CredArgs *args, fido_dev_t *dev, fido_cred_t *cred, fido_cbor_info_t *info, const char *pin)
{
    int callRet = FIDO_ERR_INTERNAL;
    if (dev == NULL || cred == NULL) {
        LOG(LOG_ERR, "param is invalid");
        goto end;
    }

    if (strcmp(args->version, INFO_VERSION_U2F_V2) == 0) {
        // catp1.0
        LOG(LOG_INFO, "to create 1.0 cred.");
        fido_dev_force_u2f(dev);
        callRet = fido_dev_make_cred(dev, cred, NULL);
        if (callRet != FIDO_OK) {
            LOG(LOG_WARNING, "error: fido_dev_make_cred (%d) %s", callRet, fido_strerr(callRet));
            goto end;
        }
    } else {
        // catp2.0
        LOG(LOG_INFO, "to create 2.0 cred.");
        /* Let built-in UV have precedence over PIN. No UV also handled here. */
        if (args->userVerification || !args->pinVerification) {
            LOG(LOG_INFO, "fido_dev_make_cred without pin.");
            callRet = fido_dev_make_cred(dev, cred, NULL);
        } else {
            callRet = FIDO_ERR_PIN_REQUIRED;
        }

        /* Some form of UV required; built-in UV failed or is not available. */
        if ((callRet == FIDO_ERR_PIN_REQUIRED || callRet == FIDO_ERR_UV_BLOCKED || callRet == FIDO_ERR_PIN_BLOCKED) && pin != NULL && strlen(pin) > 0) {
            int pinSupport = INFO_OPTIONS_NOT_SUPPORT;
            if ((callRet = dpk_dev_get_options_support_pin(info, &pinSupport)) != FIDO_OK) {
                LOG(LOG_WARNING, "need pin, but device not support pin.");
                goto end;
            }
            if (pinSupport == INFO_OPTIONS_SET) {
                LOG(LOG_INFO, "fido_dev_make_cred with pin.");
                callRet = fido_dev_make_cred(dev, cred, pin);
            }
        }
        if (callRet != FIDO_OK) {
            LOG(LOG_WARNING, "error: fido_dev_make_cred (%d) %s", callRet, fido_strerr(callRet));
            goto end;
        }
    }

    LOG(LOG_INFO, "success to create cred.");
    callRet = FIDO_OK;
end:
    return callRet;
}

int dpk_dev_verify_cred(const fido_cred_t *const cred)
{
    int callRet = FIDO_ERR_INTERNAL;

    if (cred == NULL) {
        LOG(LOG_ERR, "param is invalid");
        goto end;
    }

    if (fido_cred_x5c_ptr(cred) == NULL) {
        if ((callRet = fido_cred_verify_self(cred)) != FIDO_OK) {
            LOG(LOG_ERR, "error: fido_cred_verify_self (%d) %s", callRet, fido_strerr(callRet));
            goto end;
        }
    } else {
        if ((callRet = fido_cred_verify(cred)) != FIDO_OK) {
            LOG(LOG_ERR, "error: fido_cred_verify (%d) %s", callRet, fido_strerr(callRet));
            goto end;
        }
    }
    LOG(LOG_INFO, "verify cred success.");
    callRet = FIDO_OK;
end:
    return callRet;
}

int dk_dev_get_cred_file_path(const char *userName, char **path)
{
    char *userId = NULL;
    char *buf = NULL;
    int callRet = FIDO_ERR_INTERNAL;

    if (userName == NULL || path == NULL) {
        LOG(LOG_ERR, "param is invalid");
        goto end;
    }

    if (!get_user_id(userName, &userId) || userId == NULL) {
        LOG(LOG_ERR, "failed to get user-id");
        goto end;
    }

    buf = calloc(BUFSIZE + 1, sizeof(char));
    if (snprintf(buf, BUFSIZE, "%s%s", PASSKEY_CRED_DIR, userId) < 0) {
        LOG(LOG_ERR, "snprintf error");
        goto end;
    }

    *path = buf;
    callRet = FIDO_OK;
end:
    if (callRet != FIDO_OK && buf != NULL) {
        free(buf);
    }
    if (userId != NULL) {
        free(userId);
    }
    return callRet;
}

int dk_dev_save_cred(const CredArgs *const args, const fido_cred_t *const cred)
{
    char *khB64 = NULL;
    char *pkB64 = NULL;
    char *userId = NULL;
    FILE *credFile = NULL;
    int callRet = FIDO_ERR_INTERNAL;

    const char *user = NULL;
    const unsigned char *kh = NULL;
    size_t kh_len;
    if ((kh = fido_cred_id_ptr(cred)) == NULL) {
        LOG(LOG_ERR, "error: fido_cred_id_ptr returned NULL");
        goto end;
    }

    if ((kh_len = fido_cred_id_len(cred)) == 0) {
        LOG(LOG_ERR, "error: fido_cred_id_len returned 0");
        goto end;
    }

    const unsigned char *pk = NULL;
    size_t pk_len;
    if ((pk = fido_cred_pubkey_ptr(cred)) == NULL) {
        LOG(LOG_ERR, "error: fido_cred_pubkey_ptr returned NULL");
        goto end;
    }

    if ((pk_len = fido_cred_pubkey_len(cred)) == 0) {
        LOG(LOG_ERR, "error: fido_cred_pubkey_len returned 0");
        goto end;
    }
    if (b64_encode(kh, kh_len, &khB64) < 0) {
        LOG(LOG_ERR, "error: failed to encode key handle");
        goto end;
    }
    if (b64_encode(pk, pk_len, &pkB64) < 0) {
        LOG(LOG_ERR, "error: failed to encode public key");
        goto end;
    }

    if (!args->nouser) {
        if ((user = fido_cred_user_name(cred)) == NULL) {
            LOG(LOG_ERR, "error: fido_cred_user_name returned NULL");
            goto end;
        }
    }

    char credVersion[INFO_VERSION_MAX_LEN + 1] = { 0 };
    if (strlen(args->version) > 0) {
        snprintf(credVersion, INFO_VERSION_MAX_LEN + 1, "+%s", args->version);
    }

    char buf[BUFSIZE] = { 0 };
    if (snprintf(buf,
                 BUFSIZE,
                 "%s:%s:%s,%s,%s,%s%s%s%s",
                 args->userName,
                 args->credName,
                 args->resident ? "*" : khB64,
                 pkB64,
                 cose_type_to_string(fido_cred_type(cred)),
                 credVersion,
                 !args->noUserPresence ? "+presence" : "",
                 args->userVerification ? "+verification" : "",
                 args->pinVerification ? "+pin" : "")
        < 0) {
        LOG(LOG_WARNING, "error");
        goto end;
    }
    LOG(LOG_INFO, "cred:%s", buf);

    if (0 != access(PASSKEY_CRED_DIR, F_OK)) {
        LOG(LOG_ERR, "failed to access");
        goto end;
    }
    if (!get_user_id(args->userName, &userId) || userId == NULL) {
        LOG(LOG_ERR, "failed to get user-id");
        goto end;
    }
    char credFilePath[BUFSIZE] = { 0 };
    if (snprintf(credFilePath, BUFSIZE, "%s%s", PASSKEY_CRED_DIR, userId) < 0) {
        LOG(LOG_ERR, "snprintf error");
        goto end;
    }

    credFile = fopen(credFilePath, "a+");
    if (credFile == NULL) {
        LOG(LOG_ERR, "fopen error");
        goto end;
    }

    if (fprintf(credFile, "%s\n", buf) < 0) {
        LOG(LOG_ERR, "fprintf");
        goto end;
    }

    callRet = FIDO_OK;

end:
    if (userId != NULL) {
        free(userId);
    }
    if (credFile != NULL) {
        fclose(credFile);
    }
    if (khB64 != NULL) {
        free(khB64);
    }
    if (pkB64 != NULL) {
        free(pkB64);
    }
    return callRet;
}

void dk_dev_reset_cred(CredInfo *cred)
{
    if (cred == NULL) {
        return;
    }
    if (cred->name != NULL) {
        free(cred->name);
    }
    if (cred->keyHandle != NULL) {
        free(cred->keyHandle);
    }
    if (cred->publicKey != NULL) {
        free(cred->publicKey);
    }
    if (cred->coseType != NULL) {
        free(cred->coseType);
    }
    if (cred->attributes != NULL) {
        free(cred->attributes);
    }
    memset(cred, 0, sizeof(*cred));
}

static int parse_native_credential(char *s, CredInfo *cred)
{
    const char *delim = ",";
    const char *kh, *pk, *type, *attr;
    char *saveptr = NULL;

    memset(cred, 0, sizeof(*cred));

    if ((kh = strtok_r(s, delim, &saveptr)) == NULL) {
        LOG(LOG_ERR, "Missing key handle");
        goto fail;
    }

    if ((pk = strtok_r(NULL, delim, &saveptr)) == NULL) {
        LOG(LOG_ERR, "Missing public key");
        goto fail;
    }

    if ((type = strtok_r(NULL, delim, &saveptr)) == NULL) {
        // debug_dbg(cfg, "Old format, assume es256 and +presence");
        cred->oldFormat = 1;
        type = "es256";
        attr = "+presence";
    } else if ((attr = strtok_r(NULL, delim, &saveptr)) == NULL) {
        // debug_dbg(cfg, "Empty attributes");
        attr = "";
    }

    cred->keyHandle = cred->oldFormat ? normal_b64(kh) : strdup(kh);
    if (cred->keyHandle == NULL || (cred->publicKey = strdup(pk)) == NULL || (cred->coseType = strdup(type)) == NULL || (cred->attributes = strdup(attr)) == NULL) {
        LOG(LOG_ERR, "Unable to allocate memory for credential components");
        goto fail;
    }

    return 1;

fail:
    dk_dev_reset_cred(cred);
    return 0;
}

static bool parse_native_format(const char *userName, FILE *credFile, CredInfo *creds, unsigned int *credsCount)
{

    char *buf = NULL;
    size_t bufSize = 0;
    ssize_t len;
    bool success = false;

    *credsCount = 0;
    while (1) {
        char *user = NULL;
        char *credName = NULL;
        char *credential = NULL;
        if (buf != NULL) {
            free(buf);
            buf = NULL;
        }
        if ((len = getline(&buf, &bufSize, credFile)) < 0) {
            break;
        }
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
        }

        LOG(LOG_DEBUG, "Read %zu bytes(%s)", len, buf);
        user = strtok_r(buf, ":", &credential);
        if (credential[0] == ':') {
            credName = strtok_r(NULL, ":", &credential);
            char *temp = credential;
            credential = credName;
            credName = temp;
        } else {
            credName = strtok_r(NULL, ":", &credential);
            if (strlen(credential) == 0) {
                LOG(LOG_WARNING, "cred data is invalid");
                continue;
            }
        }
        if (user == NULL || credName == NULL || credential == NULL) {
            LOG(LOG_WARNING, "cred data is invalid");
            continue;
        }
        // credName暂时不使用，根据后续需求规划，需要证书命名，先预留
        if (strcmp(userName, user) == 0) {
            LOG(LOG_DEBUG, "cred matched user: %s", user);
            if (!parse_native_credential(credential, &creds[*credsCount])) {
                LOG(LOG_WARNING, "Failed to parse credential");
                continue;
            }
            (&creds[*credsCount])->name = strdup(credName);
            (*credsCount)++;
            if ((*credsCount) >= CRED_NUM_MAX) {
                LOG(LOG_ERR, "Found more than %d creds, ignoring the remaining ones", DEV_NUM_MAX);
                break;
            }
        }
    }

    if (!feof(credFile)) {
        LOG(LOG_ERR, "authfile parsing ended before eof (%d)", errno);
        goto end;
    }

    success = true;
end:
    if (buf != NULL) {
        free(buf);
    }

    return success;
}

int dk_dev_get_creds_from_file(const char *file, const char *userName, CredInfo *creds, unsigned int *credsCount)
{
    int fd = -1;
    struct stat st;
    FILE *credfile = NULL;
    unsigned int i;
    int callRet = FIDO_ERR_INTERNAL;

    if (file == NULL || userName == NULL) {
        LOG(LOG_ERR, "param error.");
        goto end;
    }

    *credsCount = 0;

    fd = open(file, O_RDONLY | O_CLOEXEC | O_NOCTTY);
    if (fd < 0) {
        LOG(LOG_WARNING, "Cannot open authentication file %s: %s", file, strerror(errno));
        goto end;
    }

    if (fstat(fd, &st) < 0) {
        LOG(LOG_WARNING, "Cannot stat authentication file: %s", strerror(errno));
        goto end;
    }

    if (!S_ISREG(st.st_mode)) {
        LOG(LOG_WARNING, "Authentication file is not a regular file");
        goto end;
    }

    if (st.st_size == 0) {
        LOG(LOG_WARNING, "Authentication file is empty");
        goto end;
    }

    credfile = fdopen(fd, "r");
    if (credfile == NULL) {
        LOG(LOG_WARNING, "fdopen: %s", strerror(errno));
        goto end;
    } else {
        fd = -1;
    }

    if (!parse_native_format(userName, credfile, creds, credsCount)) {
        goto end;
    }

    callRet = FIDO_OK;

end:
    if (callRet != FIDO_OK) {
        for (i = 0; i < *credsCount; i++) {
            dk_dev_reset_cred(&creds[i]);
        }
        *credsCount = 0;
    }
    if (credfile) {
        fclose(credfile);
    }
    if (fd != -1) {
        close(fd);
    }

    return callRet;
}

static int is_resident(const char *kh)
{
    return strcmp(kh, "*") == 0;
}

static void init_opts(struct opts *opts)
{
    opts->up = FIDO_OPT_FALSE;
    opts->uv = FIDO_OPT_OMIT;
    opts->pin = FIDO_OPT_FALSE;
}

static bool set_opts(const struct opts *opts, fido_assert_t *assert)
{
    bool success = false;
    if (fido_assert_set_up(assert, opts->up) != FIDO_OK) {
        LOG(LOG_ERR, "Failed to set UP");
        goto end;
    }
    if (fido_assert_set_uv(assert, opts->uv) != FIDO_OK) {
        LOG(LOG_ERR, "Failed to set UV");
        goto end;
    }
    success = true;
end:
    return success;
}

// 判断证书是否支持当前选项
static bool check_cred_version(const char *attr, const char *version)
{
    bool success = false;
    if (!strstr(attr, version)) {
        goto end;
    }
    success = true;
end:
    return success;
}

// 判断证书是否支持当前选项
static bool check_cred_opts(const char *attr, const struct opts *opts)
{
    bool success = false;
    // 限制证书的使用，安全性考虑，下面规则根据目前需求而定：
    // 请求不需要+证书没有，证书可用
    // 请求不需要+证书有，证书可用
    // 请求需要+证书没有，证书不可用
    // 请求需要+证书有，证书可用
    if (opts->up == FIDO_OPT_TRUE && !strstr(attr, "+presence")) {
        goto end;
    }
    if (opts->uv == FIDO_OPT_TRUE && !strstr(attr, "+verification")) {
        goto end;
    }
    if (opts->pin == FIDO_OPT_TRUE && !strstr(attr, "+pin")) {
        goto end;
    }
    success = true;
end:
    return success;
}

// 根据参数，填充选项
static void parse_opts(const AssertArgs *args, struct opts *opts)
{
    if (args->userPresence == 1) {
        opts->up = FIDO_OPT_TRUE;
    } else if (args->userPresence == 0) {
        opts->up = FIDO_OPT_FALSE;
    } else {
        opts->up = FIDO_OPT_OMIT;
    }

    if (args->userVerification == 1) {
        opts->uv = FIDO_OPT_TRUE;
    } else if (args->userVerification == 0)
        opts->uv = FIDO_OPT_FALSE;
    else {
        opts->uv = FIDO_OPT_OMIT;
    }

    if (args->pinVerification == 1) {
        opts->pin = FIDO_OPT_TRUE;
    } else if (args->pinVerification == 0) {
        opts->pin = FIDO_OPT_FALSE;
    } else {
        opts->pin = FIDO_OPT_OMIT;
    }
}

static int match_device_opts(fido_dev_t *dev, struct opts *opts)
{
    int pin, uv;
    int callRet = FIDO_ERR_INTERNAL;

    /* FIXME: fido_dev_{supports,has}_{pin,uv} (1.7.0) */
    fido_cbor_info_t *info = NULL;
    if ((callRet = dpk_dev_get_info(dev, &info)) != FIDO_OK) {
        goto end;
    }
    if ((callRet = dpk_dev_get_options_support_pin(info, &pin)) != FIDO_OK) {
        goto end;
    }
    if ((callRet = dpk_dev_get_options_support_uv(info, &uv)) != FIDO_OK) {
        goto end;
    }

    if (opts->uv == FIDO_OPT_FALSE && uv < 0) {
        opts->uv = FIDO_OPT_OMIT;
    }

    if ((opts->pin == FIDO_OPT_TRUE && pin != 1) || (opts->uv == FIDO_OPT_TRUE && uv != 1)) {
        callRet = FIDO_ERR_UNSUPPORTED_OPTION;
        goto end;
    }
    callRet = FIDO_OK;
end:
    if (info != NULL) {
        fido_cbor_info_free(&info);
    }
    return callRet;
}

static bool set_cdh(fido_assert_t *assert)
{
    unsigned char cdh[32];
    int ret = 0;
    bool success = false;
    if (!random_bytes(cdh, sizeof(cdh))) {
        LOG(LOG_ERR, "Failed to generate challenge");
        goto end;
    }

    ret = fido_assert_set_clientdata_hash(assert, cdh, sizeof(cdh));
    if (ret != FIDO_OK) {
        LOG(LOG_ERR, "Unable to set challenge: %s (%d)", fido_strerr(ret), ret);
        goto end;
    }
    success = true;
end:
    return success;
}

static int prepare_assert(const AssertArgs *args, const CredInfo *cred, const struct opts *opts, fido_assert_t **assertOut)
{
    fido_assert_t *assert = NULL;
    unsigned char *buf = NULL;
    size_t buf_len;
    int callRet = FIDO_ERR_INTERNAL;

    if ((assert = fido_assert_new()) == NULL) {
        LOG(LOG_ERR, "Unable to allocate assertion");
        goto end;
    }

    if (cred->oldFormat) {
        callRet = fido_assert_set_rp(assert, args->appid);
    } else {
        callRet = fido_assert_set_rp(assert, args->origin);
    }

    if (callRet != FIDO_OK) {
        LOG(LOG_ERR, "Unable to set origin: %s (%d)", fido_strerr(callRet), callRet);
        goto end;
    }

    if (is_resident(cred->keyHandle)) {
        LOG(LOG_INFO, "Credential is resident");
    } else {
        LOG(LOG_INFO, "Key handle: %s", cred->keyHandle);
        if (b64_decode(cred->keyHandle, (void **)&buf, &buf_len) < 0) {
            LOG(LOG_ERR, "Failed to decode key handle");
            goto end;
        }

        callRet = fido_assert_allow_cred(assert, buf, buf_len);
        if (callRet != FIDO_OK) {
            LOG(LOG_ERR, "Unable to set keyHandle: %s (%d)", fido_strerr(callRet), callRet);
            goto end;
        }
    }

    if (!set_opts(opts, assert)) {
        LOG(LOG_ERR, "Failed to set assert options");
        goto end;
    }

    if (!set_cdh(assert)) {
        LOG(LOG_ERR, "Failed to set client data hash");
        goto end;
    }

    *assertOut = assert;
    callRet = FIDO_OK;

end:

    if (callRet != FIDO_OK && assert != NULL) {
        fido_assert_free(&assert);
    }

    if (buf != NULL) {
        free(buf);
    }

    return callRet;
}

static void reset_pk(struct pk *pk)
{
    if (pk == NULL) {
        return;
    }
    if (pk->ptr != NULL) {
        if (pk->type == COSE_ES256) {
            es256_pk_free((es256_pk_t **)&pk->ptr);
        } else if (pk->type == COSE_RS256) {
            rs256_pk_free((rs256_pk_t **)&pk->ptr);
        } else if (pk->type == COSE_EDDSA) {
            eddsa_pk_free((eddsa_pk_t **)&pk->ptr);
        }
    }
    memset(pk, 0, sizeof(*pk));
}

static int translate_old_format_pubkey(es256_pk_t *es256_pk, const unsigned char *pk, size_t pk_len)
{
    EC_KEY *ec = NULL;
    EC_POINT *q = NULL;
    const EC_GROUP *g = NULL;
    int ret = FIDO_ERR_INTERNAL;

    if (es256_pk == NULL)
        goto end;

    if ((ec = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1)) == NULL || (g = EC_KEY_get0_group(ec)) == NULL)
        goto end;

    if ((q = EC_POINT_new(g)) == NULL || !EC_POINT_oct2point(g, q, pk, pk_len, NULL) || !EC_KEY_set_public_key(ec, q))
        goto end;

    ret = es256_pk_from_EC_KEY(es256_pk, ec);

end:
    if (ec != NULL)
        EC_KEY_free(ec);
    if (q != NULL)
        EC_POINT_free(q);

    return ret;
}

static bool parse_pk(int old, const char *type, const char *pk, struct pk *out)
{
    unsigned char *buf = NULL;
    size_t buf_len;
    int ret = 0;
    bool success = false;

    reset_pk(out);

    if (old) {
        if (!hex_decode(pk, &buf, &buf_len)) {
            LOG(LOG_ERR, "Failed to decode public key");
            goto end;
        }
    } else {
        if (b64_decode(pk, (void **)&buf, &buf_len) < 0) {
            LOG(LOG_ERR, "Failed to decode public key");
            goto end;
        }
    }

    if (!cose_string_to_type(type, &out->type)) {
        LOG(LOG_ERR, "Unknown cose type '%s'", type);
        goto end;
    }

    // For backwards compatibility, failure to pack the public key is not
    // returned as an error.  Instead, it is handled by fido_verify_assert().
    if (out->type == COSE_ES256) {
        if ((out->ptr = es256_pk_new()) == NULL) {
            LOG(LOG_ERR, "Failed to allocate ES256 public key");
            goto end;
        }
        if (old) {
            ret = translate_old_format_pubkey(out->ptr, buf, buf_len);
        } else {
            ret = es256_pk_from_ptr(out->ptr, buf, buf_len);
        }
        if (ret != FIDO_OK) {
            LOG(LOG_ERR, "Failed to convert ES256 public key");
        }
    } else if (out->type == COSE_RS256) {
        if ((out->ptr = rs256_pk_new()) == NULL) {
            LOG(LOG_ERR, "Failed to allocate RS256 public key");
            goto end;
        }
        ret = rs256_pk_from_ptr(out->ptr, buf, buf_len);
        if (ret != FIDO_OK) {
            LOG(LOG_ERR, "Failed to convert RS256 public key");
        }
    } else if (out->type == COSE_EDDSA) {
        if ((out->ptr = eddsa_pk_new()) == NULL) {
            LOG(LOG_ERR, "Failed to allocate EDDSA public key");
            goto end;
        }
        ret = eddsa_pk_from_ptr(out->ptr, buf, buf_len);
        if (ret != FIDO_OK) {
            LOG(LOG_ERR, "Failed to convert EDDSA public key");
        }
    } else {
        LOG(LOG_ERR, "COSE type '%s' not handled", type);
        goto end;
    }

    success = true;
end:
    if (buf != NULL) {
        free(buf);
    }

    return success;
}

static int do_get_assert_check_fido2_cred_is_valid(fido_dev_t *dev, fido_assert_t *assert)
{
    int callRet = FIDO_ERR_INTERNAL;
    if (dev == NULL || assert == NULL) {
        return callRet;
    }

    callRet = fido_dev_get_assert(dev, assert, NULL);
    if ((!fido_dev_is_fido2(dev) && callRet == FIDO_ERR_USER_PRESENCE_REQUIRED) || (fido_dev_is_fido2(dev) && callRet == FIDO_OK)) {
        LOG(LOG_DEBUG, "dev is valid by check cred");
        callRet = FIDO_OK;
    }

    return callRet;
}

static int do_get_assert_check_u2f_cred_is_valid(fido_dev_t *dev, fido_assert_t *assert)
{
    int callRet = FIDO_ERR_INTERNAL;
    if (dev == NULL || assert == NULL) {
        return callRet;
    }

    callRet = fido_dev_get_assert(dev, assert, NULL);
    if ((!fido_dev_is_fido2(dev) && callRet == FIDO_ERR_USER_PRESENCE_REQUIRED) || (fido_dev_is_fido2(dev) && callRet == FIDO_OK)) {
        LOG(LOG_DEBUG, "dev is valid by check cred");
        callRet = FIDO_OK;
    }

    return callRet;
}

int dk_dev_do_authentication(MethodContext *mc, const AssertArgs *args, const CredInfo *creds, const unsigned int credsCount, fido_dev_t *dev)
{
    fido_assert_t *assert = NULL;
    int cued = 0;
    struct opts opts;
    struct pk pk;
    const char *pin = NULL;
    int callRet = FIDO_ERR_INTERNAL;

    init_opts(&opts);

    memset(&pk, 0, sizeof(pk));

    if (args->noDetect) {
        LOG(LOG_WARNING, "noDetect option specified, suitable key detection will be skipped");
    }

    // 逐个证书进行认证
    for (unsigned int i = 0; i < credsCount; i++) {
        callRet = FIDO_ERR_INTERNAL;
        LOG(LOG_DEBUG, "Attempting authentication with device number %d", i + 1);

        init_opts(&opts); /* used during authenticator discovery */
        callRet = prepare_assert(args, &creds[i], &opts, &assert);
        if (callRet != FIDO_OK) {
            LOG(LOG_INFO, "cred error and try another, failed to prepare assert.");
            continue;
        }

        if (!parse_pk(creds[i].oldFormat, creds[i].coseType, creds[i].publicKey, &pk)) {
            LOG(LOG_INFO, "cred error and try another, failed to parse pk.");
            continue;
        }

        /* options used during authentication */
        parse_opts(args, &opts);

        if (!check_cred_opts(creds[i].attributes, &opts)) {
            LOG(LOG_INFO, "cred error and try another, cred does not meet the requirements.");
            continue;
        }

        // 设备认证
        if (check_cred_version(creds[i].attributes, INFO_VERSION_U2F_V2)) {
            // catp1.0
            LOG(LOG_INFO, "to get-assert 1.0 cred.");
            do {
                if (is_resident(creds[i].keyHandle) || args->noDetect) {
                    LOG(LOG_INFO, "resident credential or noDetect: try the device without check.");
                    break;
                }

                if (do_get_assert_check_u2f_cred_is_valid(dev, assert) != FIDO_OK) {
                    LOG(LOG_INFO, "check device by get-assert, cred is invalid and skip this device.");
                    break;
                }

                if (fido_assert_set_up(assert, opts.up) != FIDO_OK) {
                    LOG(LOG_ERR, "Failed to set UP");
                    goto end;
                }

                if (!set_cdh(assert)) {
                    LOG(LOG_INFO, "authenticator error and try another, failed to set cdh.");
                    break;
                }

                emit_get_assert_status(mc, args->userName, SIGNAL_NOT_FINISH, FIDO_ERR_USER_ACTION_PENDING);
                callRet = fido_dev_get_assert(dev, assert, NULL);
                if (callRet != FIDO_OK) {
                    LOG(LOG_INFO, "authenticator error and try another, failed to get assert:%s", fido_strerr(callRet));
                    break;
                }

                callRet = fido_assert_verify(assert, 0, pk.type, pk.ptr);
                if (callRet == FIDO_OK) {
                    // 只要有一个成功，跳过其他证书和认证器的认证，结束流程
                    goto end;
                }
            } while (0);
        } else {
            // catp2.0
            LOG(LOG_INFO, "to get-assert 2.0 cred.");
            do {
                if (is_resident(creds[i].keyHandle) || args->noDetect) {
                    LOG(LOG_INFO, "resident credential or noDetect: try the device without check.");
                    break;
                } else {
                    if (do_get_assert_check_fido2_cred_is_valid(dev, assert) != FIDO_OK) {
                        LOG(LOG_INFO, "check device by get-assert, cred is invalid and skip this device.");
                        break;
                    }
                }

                callRet = match_device_opts(dev, &opts);
                if (callRet != FIDO_OK) {
                    LOG(LOG_INFO, "skipping authenticator, %s", fido_strerr(callRet));
                    break;
                }

                if (!set_opts(&opts, assert)) {
                    LOG(LOG_INFO, "authenticator error and try another, failed to set opts");
                    break;
                }

                if (!set_cdh(assert)) {
                    LOG(LOG_INFO, "authenticator error and try another, failed to set cdh.");
                    break;
                }

                if (opts.pin == FIDO_OPT_TRUE) {
                    pin = args->pin;
                    if (pin == NULL) {
                        LOG(LOG_INFO, "cred error and try another, need pin, but input pin is null.");
                        break;
                    }
                    if (strlen(pin) == 0) {
                        LOG(LOG_INFO, "cred error and try another, need pin, but input pin is emtpy");
                        break;
                    }
                }
                if (opts.up == FIDO_OPT_TRUE || opts.uv == FIDO_OPT_TRUE) {
                    if (args->manual == 0 && args->cue && !cued) {
                        cued = 1;
                        // converse(pamh, PAM_TEXT_INFO, args->cuePrompt != NULL ? args->cuePrompt : "Please touch the device.");
                    }
                }

                emit_get_assert_status(mc, args->userName, SIGNAL_NOT_FINISH, FIDO_ERR_USER_ACTION_PENDING);
                callRet = fido_dev_get_assert(dev, assert, pin);

                if (pin) {
                    pin = NULL;
                }
                if (callRet != FIDO_OK) {
                    LOG(LOG_INFO, "authenticator error and try another, failed to get assert:%s", fido_strerr(callRet));
                    break;
                }
                if (opts.pin == FIDO_OPT_TRUE || opts.uv == FIDO_OPT_TRUE) {
                    callRet = fido_assert_set_uv(assert, FIDO_OPT_TRUE);
                    if (callRet != FIDO_OK) {
                        LOG(LOG_INFO, "authenticator error and try another, failed to set uv:%s", fido_strerr(callRet));
                        break;
                    }
                }
                callRet = fido_assert_verify(assert, 0, pk.type, pk.ptr);
                if (callRet == FIDO_OK) {
                    // 只要有一个成功，跳过其他证书和认证器的认证，结束流程
                    goto end;
                }

            } while (0);
        }

        if (assert != NULL) {
            fido_assert_free(&assert);
            assert = NULL;
        }
    }
    if (callRet == FIDO_OK) {
        callRet = FIDO_ERR_INTERNAL;
    }
    LOG(LOG_INFO, "All creds can not to authenticate, %s.", fido_strerr(callRet));

end:
    reset_pk(&pk);
    if (assert != NULL) {
        fido_assert_free(&assert);
    }

    return callRet;
}

int dk_dev_has_valid_cred_count(const AssertArgs *args, const CredInfo *creds, const unsigned int credsCount, fido_dev_t *dev, unsigned int *validCredsCount)
{
    fido_assert_t *assert = NULL;
    int callRet = FIDO_ERR_INTERNAL;
    struct opts opts;
    struct pk pk;

    init_opts(&opts);
    memset(&pk, 0, sizeof(pk));

    if (args == NULL || creds == NULL || validCredsCount == NULL) {
        goto end;
    }
    *validCredsCount = 0;

    unsigned int validCred = 0;

    for (unsigned int i = 0; i < credsCount; i++) {
        callRet = FIDO_ERR_INTERNAL;
        LOG(LOG_DEBUG, "Attempting authentication with device number %d", i + 1);

        init_opts(&opts); /* used during authenticator discovery */
        callRet = prepare_assert(args, &creds[i], &opts, &assert);

        if (callRet != FIDO_OK) {
            LOG(LOG_ERR, "Failed to prepare assert");
            goto end;
        }

        if (!parse_pk(creds[i].oldFormat, creds[i].coseType, creds[i].publicKey, &pk)) {
            LOG(LOG_ERR, "Failed to parse public key");
            goto end;
        }

        if (check_cred_version(creds[i].attributes, INFO_VERSION_U2F_V2)) {
            if (do_get_assert_check_u2f_cred_is_valid(dev, assert) == FIDO_OK) {
                validCred++;
                LOG(LOG_INFO, "check device by get-assert, cred is invalid and skip this device.");
            }
        } else {
            if (do_get_assert_check_fido2_cred_is_valid(dev, assert) == FIDO_OK) {
                validCred++;
                LOG(LOG_INFO, "check device by get-assert, cred is invalid and skip this device.");
            }
        }

        if (assert != NULL) {
            fido_assert_free(&assert);
            assert = NULL;
        }
    }
    LOG(LOG_DEBUG, "valid cred count:%d", validCred);
    *validCredsCount = validCred;
    callRet = FIDO_OK;
end:
    reset_pk(&pk);

    if (assert != NULL) {
        fido_assert_free(&assert);
    }

    return callRet;
}
