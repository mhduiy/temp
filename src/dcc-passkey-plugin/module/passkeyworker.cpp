// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "passkeyworker.h"
#include "common.h"
#include <qdbusconnection.h>
#include <qdbusinterface.h>
#include <qloggingcategory.h>
#include <polkit-qt5-1/PolkitQt1/Authority>

#include <QDBusPendingReply>
#include <QDebug>
#include <QDBusConnection>

using namespace PolkitQt1;

// libfido2上游错误码(<=0)
#define FIDO_OK             0       // 成功，在异步调用的信号中，收到该码意味着异步结束
#define FIDO_ERR_RX         -2      // 过程中设备拔掉
#define FIDO_ERR_NOTFOUND   -10     // 未找到设备

// FIDO协议约定的错误码(>=0)
#define FIDO_ERR_USER_ACTION_PENDING    0x23    // 提示用户要操作设备了，比如触碰
#define FIDO_ERR_USER_ACTION_TIMEOUT    0x2f    // 操作设备超时
#define FIDO_ERR_PIN_REQUIRED           0x36    // 认证过程中，触摸超时，协议会让再用PIN试一次，此处直接当超时处理

const QString DisplayManagerService("org.freedesktop.DisplayManager");
const QString IdErrorFlag("ID_ERR");

const QMap<PromptType, PromptInfo> AllPromptInfo {
    { PromptType::Insert, {InsertPixmapPath, false, QObject::tr("请插入安全密钥"), "", "", false, false} },
    { PromptType::Identifying, {IdentifyPixmapPath, true, QObject::tr("正在识别安全密钥"), "", "", false, false} },
    { PromptType::Touch, {TouchPixmapPath, false, QObject::tr("请验证安全密钥，触摸或轻扫设备"), "", "", false, false} },
    { PromptType::Timeout, {UnknownPixmapPath, false, QObject::tr("触摸超时"), "", QObject::tr("重试"), true, false} },
    { PromptType::Unregistered, {UnregisteredPixmapPath, false, QObject::tr("识别到未添加的安全密钥，添加后可用于登录认证"), "", QObject::tr("添加密钥"), true, false} },
    { PromptType::Uncertified, {UnknownPixmapPath, false, QObject::tr("设备未认证"), "", QObject::tr("去认证"), true, false} },
    { PromptType::Unknown, {UnknownPixmapPath, false, QObject::tr("未知错误"), "", QObject::tr("重试"), true, false} }
};

PasskeyWorker::PasskeyWorker(PasskeyModel* model, QObject* parent)
    : QObject(parent)
    , m_model(model)
    , m_passkeyInter(nullptr)
    , m_login1Inter(nullptr)
    , m_needPromptMonitor(true)
    , m_currentType(PromptType::Count)
    , m_resetAssertion(false)
    , m_currentId(IdErrorFlag)
{
}

PasskeyWorker::~PasskeyWorker()
{
    if (m_login1Inter) {
        m_login1Inter->deleteLater();
        m_login1Inter = nullptr;
    }

    if (m_passkeyInter) {
        m_passkeyInter->deleteLater();
        m_passkeyInter = nullptr;
    }
}

void PasskeyWorker::init()
{
    m_login1Inter = new Login1SessionInter("org.freedesktop.login1",
                                           "/org/freedesktop/login1/session/self",
                                           QDBusConnection::systemBus(),
                                           this);

    m_passkeyInter = new PasskeyInter("com.deepin.Passkey", "/com/deepin/Passkey", QDBusConnection::systemBus(), this);
    connect(m_passkeyInter, &PasskeyInter::MakeCredStatus, this, &PasskeyWorker::requestMakeCredStatus);
    connect(m_passkeyInter, &PasskeyInter::GetAssertStatus, this, &PasskeyWorker::requestGetAssertStatus);
    connect(m_passkeyInter, &PasskeyInter::ResetStatus, this, &PasskeyWorker::requestResetStatus);

    updatePromptInfo(PromptType::Insert);
}

void PasskeyWorker::handlePromptMonitor()
{
    static bool checkAdd = false;

    // 在注册、认证过程中不监测
    if (!m_needPromptMonitor) {
        return;
    }

    // 监测拔出
    if (!checkAdd && (getDeviceCount() <= 0)) {
        if (m_currentType != PromptType::Insert) {
            updatePromptInfo(PromptType::Insert);
        }
        checkAdd = true;
    }

    // 监测插入
    if (checkAdd) {
        if (getDeviceCount() > 0) {
            checkAdd = false;
            activate();
        }
    }
}

bool PasskeyWorker::existDevice()
{
    // 当前设备数大于0即为存在设备
    if (getDeviceCount() > 0) {
        return true;
    }

    // 显示插入设备界面
    updatePromptInfo(PromptType::Insert);
    m_needPromptMonitor = true;
    m_resetAssertion = false;
    return false;
}

void PasskeyWorker::activate()
{
    m_needPromptMonitor = true;
    m_currentId = IdErrorFlag;

    // 不存在设备
    if (!existDevice()) {
        return;
    }

    updatePromptInfo(PromptType::Identifying);

    // 当前用户的有效凭证小于等于0，即设备未注册
    if (getUserValidCredentialCount() <= 0) {
        updatePromptInfo(PromptType::Unregistered);
    } else {
        // 有有效凭证，则认证证书
        m_resetAssertion = false;
        getAssertion();
    }
}

void PasskeyWorker::handlePromptOperate(const QString &operate)
{
    if (operate != m_model->promptPageInfo().second.operateBtnText) {
        return;
    }

    const PromptType &type = m_model->promptPageInfo().first;
    switch (type)
    {
    // 未注册，注册设备，生成证书
    case PromptType::Unregistered:
        makeCredential();
        break;
    // 验证超时，重新走流程
    case PromptType::Timeout:
        activate();
        break;
    // 未知错误，重新走流程
    case PromptType::Unknown:
        activate();
        break;
    default:
        break;
    }
}

void PasskeyWorker::handleJump()
{
    updateManageInfo();
}

void PasskeyWorker::handleResetPasskeyMonitor()
{
    // 定时器启动，检测重新拔插
    // 重新拔插后，显示正在识别，并调认证设备接口
    static bool remove = false;
    static bool insert = false;

    // 先移除设备
    if (!remove) {
        (getDeviceCount() <= 0) ? remove = true : remove = false;
        if (getDeviceCount() <= 0) {
            remove = true;
            qCDebug(DCC_PASSKEY) << "Remove and insert - remove device completed .";
        } else {
            remove = false;
        }
    }

    // 已经移除，但是未插入
    if (remove && !insert) {
        (getDeviceCount() > 0) ? insert = true : insert = false;
        if (getDeviceCount() > 0) {
            insert = true;
            qCDebug(DCC_PASSKEY) << "Remove and insert - insert device completed .";
        } else {
            insert = false;
        }
    }

    // 完成拔插流程，关闭定时器，提示用户正在识别，并开启认证流程
    if (remove && insert) {
        m_model->setResetDialogStyle(ResetDialogStyle::IdentifyingStyle, false);
        remove = false;
        insert = false;
        Q_EMIT resetPasskeyMonitorCompleted();
        m_resetAssertion = true;
        getAssertion();
    }
}

void PasskeyWorker::handleSetPasskeyPin(const QString &oldPin, const QString &newPin)
{
    setPin(oldPin, newPin);
}

void PasskeyWorker::makeCredential()
{
    // 鉴权后才允许注册设备
    connect(Authority::instance(), &Authority::checkAuthorizationFinished, this, [this](Authority::Result authenticationResult) {
        disconnect(Authority::instance(), nullptr, this, nullptr);
        if (Authority::Result::Yes != authenticationResult) {
            return;
        }

        m_needPromptMonitor = false;

        // 注册设备，生成证书
        QDBusPendingReply<QString> reply = m_passkeyInter->MakeCredential(getCurrentUser(), "", "");
        reply.waitForFinished();
        if (reply.isValid()) {
            m_currentId = reply;
            qCDebug(DCC_PASSKEY) << "MakeCredential: id = " << m_currentId;
        } else {
            m_currentId = IdErrorFlag;
            qCWarning(DCC_PASSKEY) << "Call method 'MakeCredential' failed, error message: " << reply.error().message();
        }
    });
    Authority::instance()->checkAuthorization("com.deepin.dde.passkey.dcc-plugin.register", UnixProcessSubject(getpid()), Authority::AllowUserInteraction);
}

// 注册设备信号处理
void PasskeyWorker::requestMakeCredStatus(const QString &id, const QString &user, int finish, int result)
{
    qCDebug(DCC_PASSKEY) << "Request make cred status signal : id = " << id << ", user = " << user
                         << ", finish = " << finish << ", result = " << result;

    Q_UNUSED(user);

    if (m_currentId == IdErrorFlag || m_currentId != id) {
        return;
    }

    // finish = 0，表示正在注册设备的流程中，比如需要触摸设备
    // finish = 1，表示注册设备流程结束
    if (finish == 0) {
        if (result == FIDO_ERR_USER_ACTION_PENDING) {
            // 触碰设备
            updatePromptInfo(PromptType::Touch);
        } else if (result == FIDO_ERR_USER_ACTION_TIMEOUT) {
            // 操作设备超时
            updatePromptInfo(PromptType::Timeout);
        } else {
            // 其余错误都按未知错误处理
            updatePromptInfo(PromptType::Unknown);
        }
    } else {
        if (result == FIDO_OK) {
            // 成功
            updateManageInfo();
        } else if (result == FIDO_ERR_PIN_REQUIRED) {
            // 认证过程中，触摸超时，协议会让再用PIN试一次，此处直接当超时处理
            updatePromptInfo(PromptType::Timeout);
        } else {
            // 其余错误都按未知错误处理
            updatePromptInfo(PromptType::Unknown);
        }
    }
}

void PasskeyWorker::updatePromptInfo(const PromptType &type)
{
    m_currentType = type;
    m_model->setCurrentPage(StackedPageIndex::Prompt);
    m_model->setPromptPageInfo(type, AllPromptInfo.value(type));
}

void PasskeyWorker::updateManageInfo()
{
    m_needPromptMonitor = false;

    m_model->setCurrentPage(StackedPageIndex::Manage);
    const QDBusPendingReply<int, int> &status = getPinStatus();
    if (!status.isValid()) {
        qCWarning(DCC_PASSKEY) << "Passkey dbus service reply error ! Method is GetPinStatus.";
        m_model->setManagePageInfo({false , false});
        return;
    }

    ManageInfo info;
    info.supportPin = (status.argumentAt(0).toInt() == 1);
    info.existPin = (status.argumentAt(1).toInt() == 1);
    m_model->setManagePageInfo(info);
    m_model->setSetPinDialogStyle(info.existPin ? SetPinDialogStyle::ChangePinStyle : SetPinDialogStyle::SetPinStyle);
}

void PasskeyWorker::deactivate()
{
}

QString PasskeyWorker::getCurrentUser()
{
    return m_login1Inter->property("Name").toString();
}

int PasskeyWorker::getDeviceCount()
{
    QDBusPendingReply<int> reply = m_passkeyInter->GetDeviceCount();
    reply.waitForFinished();
    if (!reply.isValid()) {
        qCWarning(DCC_PASSKEY) << "Passkey dbus service reply error ! Method is GetDeviceCount.";
        return -1;
    }
    return reply;
}

int PasskeyWorker::getUserValidCredentialCount()
{
    QDBusPendingReply<int> reply = m_passkeyInter->GetValidCredCount(getCurrentUser());
    reply.waitForFinished();
    if (!reply.isValid()) {
        qCWarning(DCC_PASSKEY) << "Passkey dbus service reply error ! Method is GetValidCredCount.";
        return -1;
    }
    return reply;
}

QDBusPendingReply<int, int> PasskeyWorker::getPinStatus()
{
    // reply.argumentAt(0): support - 1表示支持pin；0不支持pin。
    // reply.argumentAt(1): exist - 1表示pin已设置；0表示pin未设置，如果要使用pin需要先设置pin。
    QDBusPendingReply<int, int> reply = m_passkeyInter->GetPinStatus();
    reply.waitForFinished();
    return reply;
}

void PasskeyWorker::getAssertion()
{
    m_needPromptMonitor = false;

    // 认证设备
    QDBusPendingReply<QString> reply = m_passkeyInter->GetAssertion(getCurrentUser(), "", "");
    reply.waitForFinished();
    if (reply.isValid()) {
        m_currentId = reply;
        qCDebug(DCC_PASSKEY) << "GetAssertion: id = " << m_currentId;
    } else {
        m_currentId = IdErrorFlag;
        qCWarning(DCC_PASSKEY) << "Passkey dbus service reply error ! Method is GetAssertion.";
    }
}

// 认证设备信号处理
void PasskeyWorker::requestGetAssertStatus(const QString &id, const QString &user, int finish, int result)
{
    qCDebug(DCC_PASSKEY) << "Request get assert status signal : id = " << id << ", user = " << user
                         << ", finish = " << finish << ", result = " << result;

    Q_UNUSED(user);

    if (m_currentId == IdErrorFlag || m_currentId != id) {
        return;
    }

    // finish = 0，表示正在认证设备的流程中，比如需要触摸设备
    // finish = 1，表示认证设备流程结束
    if (finish == 0) {
        if (result == FIDO_ERR_USER_ACTION_PENDING) {
            // 触碰设备
            m_resetAssertion ? m_model->setResetDialogStyle(ResetDialogStyle::FirstTouchStyle) : updatePromptInfo(PromptType::Touch);
        } else if (result == FIDO_ERR_USER_ACTION_TIMEOUT) {
            // 操作设备超时
            m_resetAssertion ? m_model->setResetDialogStyle(ResetDialogStyle::FailedStyle, false) : updatePromptInfo(PromptType::Timeout);
        } else {
            // 其余错误都按未知错误处理
            m_resetAssertion ? m_model->setResetDialogStyle(ResetDialogStyle::FailedStyle, false) : updatePromptInfo(PromptType::Unknown);
        }
    } else {
        if (result == FIDO_OK) {
            // 成功
            m_resetAssertion ? reset() : updateManageInfo();
        } else if (result == FIDO_ERR_PIN_REQUIRED) {
            // 认证过程中，触摸超时，协议会让再用PIN试一次，此处直接当超时处理
            m_resetAssertion ? m_model->setResetDialogStyle(ResetDialogStyle::FailedStyle, false) : updatePromptInfo(PromptType::Timeout);
        } else {
            // 其余错误都按未知错误处理
            m_resetAssertion ? m_model->setResetDialogStyle(ResetDialogStyle::FailedStyle, false) : updatePromptInfo(PromptType::Unknown);
        }
    }
}

void PasskeyWorker::reset()
{
    m_needPromptMonitor = false;

    QDBusPendingReply<QString> reply = m_passkeyInter->Reset();
    reply.waitForFinished();
    if (reply.isValid()) {
        m_currentId = reply;
        qCDebug(DCC_PASSKEY) << "Reset: id = " << m_currentId;
    } else {
        m_currentId = IdErrorFlag;
        qCWarning(DCC_PASSKEY) << "Passkey dbus service reply error ! Method is Reset.";
    }
}

void PasskeyWorker::requestResetStatus(const QString &id, int finish, int result)
{
    qCDebug(DCC_PASSKEY) << "Request reset status signal : id = " << id << ", finish = " << finish << ", result = " << result;

    if (m_currentId == IdErrorFlag || m_currentId != id) {
        return;
    }

    // finish = 0，表示正在重置设备的流程中，比如需要触摸设备
    // finish = 1，表示重置设备流程结束
    if (finish == 0) {
        if (result == FIDO_ERR_USER_ACTION_PENDING) {
            // 触碰设备
            m_model->setResetDialogStyle(ResetDialogStyle::SecondTouchStyle);
        } else {
            // 其余错误都按未知错误处理
            m_model->setResetDialogStyle(ResetDialogStyle::FailedStyle, false);
        }
    } else {
        if (result == FIDO_OK) {
            // 成功
            m_model->setResetDialogStyle(ResetDialogStyle::ResultStyle, true);
        } else {
            // 其余错误都按未知错误处理
            m_model->setResetDialogStyle(ResetDialogStyle::ResultStyle, false);
        }
    }
}

void PasskeyWorker::setPin(const QString &oldPin, const QString &newPin)
{
    QDBusPendingReply<> reply = m_passkeyInter->SetPin(oldPin, newPin);
    reply.waitForFinished();
    if (reply.isError()) {
        m_model->setSetPinDialogStyle(SetPinDialogStyle::SetFailedStyle);
        Q_EMIT m_model->refreshSetPinDialogStyle();
        qCWarning(DCC_PASSKEY) << "Passkey dbus service reply error ! Method is setPin.";
    } else {
        updateManageInfo();
    }
}
