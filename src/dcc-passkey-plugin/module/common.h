// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(DCC_PASSKEY)

enum StackedPageIndex {
    Prompt = 0,    // 提示页面
    Manage         // 管理设备页面
};

enum PromptType {
    Insert = 0,     // 插入设备提示
    Identifying,    // 正在识别提示，目前暂不使用，因为识别很快，保留
    Touch,          // 触摸验证设备提示
    Timeout,        // 验证超时提示
    Unregistered,   // 未注册提示
    Unknown,        // 未知错误提示
    Count
};

struct PromptInfo {
    QString iconPath;
    bool needSpinner;
    QString promptMsg;
    QString tipMsg;
    QString operateBtnText;
    bool needBtn;
};

struct ManageInfo {
    bool supportPin;
    bool existPin;
};

enum ResetDialogStyle {
    DescriptionStyle = 0,
    InsertStyle,
    IdentifyingStyle,
    FirstTouchStyle,
    SecondTouchStyle,
    ResultStyle
};

enum SetPinDialogStyle {
    SetPinStyle = 0,
    ChangePinStyle,
    SetFailedStyle
};

const QString IconPixmapPath = ":/passkey/themes/light/icons/icon_passkey.svg";
const QString InsertPixmapPath = ":/passkey/themes/light/icons/page_insert.svg";
const QString IdentifyPixmapPath = ":/passkey/themes/light/icons/page_identify.svg";
const QString TouchPixmapPath = ":/passkey/themes/light/icons/page_touch.svg";
const QString UnregisteredPixmapPath = ":/passkey/themes/light/icons/page_unregistered.svg";
const QString UnknownPixmapPath = ":/passkey/themes/light/icons/page_unknown.svg";
const QString SetPasswordPixmapPath = ":/passkey/themes/light/icons/set_password.svg";

#endif // COMMON_H
