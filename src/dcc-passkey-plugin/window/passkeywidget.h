// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QObject>
#include <QWidget>
#include <QTimer>

#include <interface/namespace.h>
#include "widgets/managewidget.h"
#include "widgets/promptwidget.h"
#include "widgets/keypindialogctrl.h"
#include "widgets/resetkeydialogctrl.h"
#include "../module/common.h"

#include <DStackedWidget>

DWIDGET_USE_NAMESPACE

class PasskeyModel;
class PasskeyWorker;
class PromptWidget;
class ManageWidget;
class PasskeyWidget : public DStackedWidget
{
    Q_OBJECT

public:
    explicit PasskeyWidget(QWidget *parent = nullptr);
    ~PasskeyWidget() override;

    void setModel(const PasskeyModel *model, const PasskeyWorker *work);

public Q_SLOTS:
    void refreshUI();
    void refreshResetDialogStyle();
    void refreshSetPinDialogStyle();

Q_SIGNALS:
    void requestPromptOperate(const QString &operate);
    void requestJump();
    void requestSetKeyPin(const QString &oldPin, const QString &newPin);

private Q_SLOTS:
    void showKeyPinDialog(const SetPinDialogStyle &style);
    void showResetKeyDialog(const ResetDialogStyle &style, bool status);

private:
    void initUI();

private:
    PasskeyModel *m_model;
    PasskeyWorker *m_worker;

    PromptWidget *m_promptPage; // 提示引导页面
    ManageWidget *m_managePage; // 管理页面

    KeyPinDialogCtrl *m_setPinDialogCtrl; // 设置密钥Pin对话框管理
    ResetKeyDialogCtrl *m_resetKeyDialogCtrl; // 重置密钥对话框管理

    QTimer *m_promptMonitorTimer; // 提示引导流程监测
    QTimer *m_resetPasskeyMonitorTimer; // 重置安全密钥流程监测
};


