// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QObject>
#include <QDBusInterface>

#include "passkeymodel.h"
#include "dbus/dbuspasskey.h"

#include <org_freedesktop_login1.h>
#include <org_freedesktop_login1_session_self.h>

using Login1SessionInter = org::freedesktop::login1::Session;
using PasskeyInter = com::deepin::Passkey;

class PasskeyWorker : public QObject {
    Q_OBJECT
public:
    explicit PasskeyWorker(PasskeyModel* model, QObject* parent = nullptr);
    ~PasskeyWorker();

    bool existDevice();
    void updatePromptInfo(const PromptType &type);
    void updateManageInfo();

Q_SIGNALS:
    void requestInit();
    void requestActive();
    void resetPasskeyMonitorCompleted();

public Q_SLOTS:
    void init();
    void activate();
    void deactivate();
    void handlePromptOperate(const QString &operate);
    void handleJump();
    void handlePromptMonitor();
    void handleResetPasskeyMonitor();
    void handleSetPasskeyPin(const QString &oldPin, const QString &newPin);

private Q_SLOTS:
    void requestMakeCredStatus(const QString &id, const QString &user, int finish, int result);
    void requestGetAssertStatus(const QString &id, const QString &user, int finish, int result);
    void requestResetStatus(const QString &id, int finish, int result);

private:
    QString getCurrentUser();
    int getDeviceCount();
    int getUserValidCredentialCount();
    QDBusPendingReply<int, int> getPinStatus();
    void makeCredential();
    void getAssertion();
    void reset();
    void setPin(const QString &oldPin, const QString &newPin);

private:
    PasskeyModel *m_model;

    PasskeyInter *m_passkeyInter;
    Login1SessionInter *m_login1Inter;

    bool m_needPromptMonitor; // 在注册和认证过程中不需要监控设备
    PromptType m_currentType;

    bool m_resetAssertion; // 区分正常认证和重置认证
    QString m_currentId; // 异步通信中确定id
};
