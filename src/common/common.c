// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common.h"

#include "log.h"

#include <fcntl.h>
#include <pwd.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool random_bytes(void *buf, size_t cnt)
{
    int fd = -1;
    ssize_t n;
    bool success = false;

    fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        LOG(LOG_ERR, "can not to open /dev/urandom");
        goto end;
    }

    n = read(fd, buf, cnt);
    if (n < 0 || (size_t)n != cnt) {
        LOG(LOG_ERR, "can not to read /dev/urandom");
        goto end;
    }
    success = true;
end:
    if (fd >= 0) {
        close(fd);
    }

    return success;
}

bool read_file_line(const char *path, char **val)
{
    FILE *file = NULL;
    char *buf = NULL;
    size_t bufSize = 0;
    bool success = false;

    if (path == NULL) {
        LOG(LOG_WARNING, "path is empty");
        goto end;
    }

    file = fopen(path, "r");
    if (file == NULL) {
        LOG(LOG_WARNING, "error to open %s", path);
        goto end;
    }

    if (getline(&buf, &bufSize, file) == -1 || buf == NULL) {
        LOG(LOG_WARNING, "error to read %s", path);
        goto end;
    }
    if (buf[strlen(buf) - 1] == '\n') {
        buf[strlen(buf) - 1] = '\0';
    }
    *val = buf;
    success = true;
end:
    if (file != NULL) {
        fclose(file);
    }
    if (!success && buf != NULL) {
        free(buf);
    }
    return success;
}

bool get_user_id(const char *userName, char **id)
{
    char path[BUFSIZE] = { 0 };
    ssize_t len;
    char *buf = NULL;
    size_t bufSize = 0;
    FILE *file = NULL;
    const char *accountPath = "/var/lib/AccountsService/deepin/users";
    bool success = false;

    if (userName == NULL) {
        LOG(LOG_ERR, "user name is empty");
        goto end;
    }

    if (snprintf(path, BUFSIZE, "%s/%s", accountPath, userName) < 0) {
        LOG(LOG_ERR, "snprintf error");
        goto end;
    }

    bool find = false;
    if (0 == access(path, R_OK)) {
        file = fopen(path, "r");
        if (file == NULL) {
            LOG(LOG_WARNING, "error to open %s", path);
            goto end;
        }

        while ((len = getline(&buf, &bufSize, file)) != -1) {
            char *key = NULL;
            char *value = NULL;
            if (len > 0 && buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
            }
            key = strtok_r(buf, "=", &value);
            if (key != NULL && value != NULL) {
                if (strcmp(key, "UUID") == 0) {
                    *id = calloc(strlen(value) + 1, sizeof(char));
                    strcpy(*id, value);
                    (*id)[strlen(value)] = '\0';
                    find = true;
                    break;
                }
            }
            if (buf != NULL) {
                free(buf);
                buf = NULL;
            }
        }
    }
    if (!find) {
        LOG(LOG_WARNING, "account %s file is not exist. use user name.", userName);
        *id = calloc(strlen(userName) + 1, sizeof(char));
        strcpy(*id, userName);
        (*id)[strlen(userName)] = '\0';
    }

    success = true;
end:
    if (file != NULL) {
        fclose(file);
    }
    if (buf != NULL) {
        free(buf);
        buf = NULL;
    }
    return success;
}

bool is_user_exist(const char *userName)
{
    bool isExist = true;
    struct passwd *pw = getpwnam(userName);
    if (pw == NULL || pw->pw_dir == NULL || pw->pw_dir[0] != '/') {
        isExist = false;
    }
    return isExist;
}