// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QLoggingCategory>

#include <QtQml/qqml.h>

Q_DECLARE_LOGGING_CATEGORY(DCC_PASSKEY)

namespace dcc {
namespace passkey {
namespace common {

Q_NAMESPACE
QML_NAMED_ELEMENT(Common)

enum PasskeyStage {
    Prompt = 0,    // 提示引导阶段
    Manage         // 管理设备阶段
};

Q_ENUM_NS(PasskeyStage)

enum PromptType {
    Insert = 0,     // 插入设备
    Identifying,    // 正在识别，目前暂不使用，因为识别很快，保留
    Touch,          // 触摸验证设备
    Timeout,        // 验证超时
    Unregistered,   // 未注册
    Unknown,        // 未知错误
    Count
};

Q_ENUM_NS(PromptType)

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
    ResultSuccessStyle,
    ResultFailedStyle
};

Q_ENUM_NS(ResetDialogStyle)


enum SetPinDialogStyle {
    SetPinStyle = 0,
    ChangePinStyle,
    SetFailedStyle
};

Q_ENUM_NS(SetPinDialogStyle)

} // end namespace common
} // end namespace update
} // end namespace dcc

#endif // COMMON_H
