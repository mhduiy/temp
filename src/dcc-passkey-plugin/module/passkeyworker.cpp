// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "passkeyworker.h"
#include "common.h"
#include "decode/decode.h"
#include "decode/evp.h"
#include "decode/sm2.h"
#include "decode/b64.h"
#include "decode/sm4.h"
#include "common/errcode.h"
#include <polkit-qt5-1/PolkitQt1/Authority>

#include <QDBusPendingReply>
#include <QDebug>
#include <QDBusConnection>

using namespace PolkitQt1;

// FIDO协议约定的错误码(>=0)
#define FIDO_ERR_USER_ACTION_PENDING    0x23    // 提示用户要操作设备了，比如触碰
#define FIDO_ERR_USER_ACTION_TIMEOUT    0x2f    // 操作设备超时
#define FIDO_ERR_PIN_REQUIRED           0x36    // 认证过程中，触摸超时，协议会让再用PIN试一次，此处直接当超时处理

const QString DisplayManagerService("org.freedesktop.DisplayManager");
const QString IdErrorFlag("ID_ERR");
const QString PublicKeyErrorFlag("KEY_ERR");

const QMap<PromptType, PromptInfo> AllPromptInfo {
    { PromptType::Insert, {InsertPixmapPath, false, QObject::tr("Please plug in the security key"), "", "", false} },
    { PromptType::Identifying, {IdentifyPixmapPath, true, QObject::tr("Identifying the security key"), "", "", false} },
    { PromptType::Touch, {TouchPixmapPath, false, QObject::tr("Touch or swipe the security key"), "", "", false} },
    { PromptType::Timeout, {UnknownPixmapPath, false, QObject::tr("Validation timed out"), "", QObject::tr("Retry"), true} },
    { PromptType::Unregistered, {UnregisteredPixmapPath, false, QObject::tr("The security key is recognized, you can register it for login authentication."), "", QObject::tr("Register"), true} },
    { PromptType::Unknown, {UnknownPixmapPath, false, QObject::tr("Unknown error"), "", QObject::tr("Retry"), true} }
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
    , m_needCloseDevice(false)
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

void PasskeyWorker::deactivate()
{
    if (m_needCloseDevice) {
        qCDebug(DCC_PASSKEY) << "Close passkey device with id " << m_currentId;
        deviceClose(m_currentId);
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
    QString publicKey;
    unsigned char *genKey = nullptr;
    EVP_PKEY *pubEvpPKey = nullptr;
    unsigned char *encData = nullptr;
    size_t encDataLen = 0;
    char *pkB64 = nullptr;

    unsigned char *oldPinEcbData = nullptr;
    int oldPinEcbDataLen = 0;
    char *oldPinB64 = nullptr;
    QString oldPinB64Str;

    unsigned char *newPinEcbData = nullptr;
    int newPinEcbDataLen = 0;
    char *newPinB64 = nullptr;
    QString newPinB64Str;

    // PIN加密
    bool success = false;

#ifndef NO_USE_ENCRYPT
    do {
        // 1. 获取服务器公钥
        publicKey = encryptKey(DP_SUPPORT_SM2_SM3);
        if (publicKey == PublicKeyErrorFlag) {
            qCWarning(DCC_PASSKEY) << "Set passkey pin failed, error code: 1";
            break;
        }

        // 2. 生成DP_SUPPORT_SM4_ECB对称密钥
        genKey = dp_gen_symmetric_key_16();
        if (genKey == nullptr) {
            qCWarning(DCC_PASSKEY) << "Set passkey pin failed, error code: 1a";
            break;
        }

        if (dp_sm2_public_key_create_by_string((unsigned char *)publicKey.toStdString().c_str(), &pubEvpPKey) < 0) {
            qCWarning(DCC_PASSKEY) << "Set passkey pin failed, error code: 2";
            break;
        }

        // 3. 用服务器公钥加密对称密钥，并转成base64格式
        if (dp_sm2_encrypt(pubEvpPKey, genKey, strlen((char *)genKey), &encData, &encDataLen) < 0) {
            qCWarning(DCC_PASSKEY) << "Set passkey pin failed, error code: 3";
            break;
        }
        if (b64_encode(encData, encDataLen, &pkB64) < 0) {
            qCWarning(DCC_PASSKEY) << "Set passkey pin failed, error code: 4";
            break;
        }

        // 4. 发送对称密钥到服务
        if (!setSymmetricKey(DP_SUPPORT_SM2_SM3, DP_SUPPORT_SM4_ECB, pkB64)) {
            qCWarning(DCC_PASSKEY) << "Set passkey pin failed, error code: 5";
            break;
        }

        // 5. 通过对称密钥加密旧PIN码，并转为base64格式
        if (!oldPin.isEmpty()) {
            if (dp_sm4_ecb_encrypt(genKey, (unsigned char *)oldPin.toStdString().c_str(), oldPin.toStdString().length(),
                                &oldPinEcbData, &oldPinEcbDataLen) < 0) {
                qCWarning(DCC_PASSKEY) << "Set passkey pin failed, error code: 6";
                break;
            }
            if (b64_encode(oldPinEcbData, oldPinEcbDataLen, &oldPinB64) < 0) {
                qCWarning(DCC_PASSKEY) << "Set passkey pin failed, error code: 7";
                break;
            }
        }

        // 6. 通过对称密钥加密新PIN码，并转为base64格式
        if (dp_sm4_ecb_encrypt(genKey, (unsigned char *)newPin.toStdString().c_str(), strlen(newPin.toStdString().c_str()),
                            &newPinEcbData, &newPinEcbDataLen) < 0) {
            qCWarning(DCC_PASSKEY) << "Set passkey pin failed, error code: 8";
            break;
        }
        if (b64_encode(newPinEcbData, newPinEcbDataLen, &newPinB64) < 0) {
            qCWarning(DCC_PASSKEY) << "Set passkey pin failed, error code: 9";
            break;
        }
        oldPinB64Str = QString(oldPinB64);
        newPinB64Str = QString(newPinB64);
        success = true;

    } while(0);

#else
    success = true;
    oldPinB64Str = oldPin;
    newPinB64Str = newPin;
#endif
    // 加密成功则设置PIN，失败则提示
    if (success) {
        setPin(oldPinB64Str, newPinB64Str);
    } else {
        m_model->setSetPinDialogStyle(SetPinDialogStyle::SetFailedStyle);
        Q_EMIT m_model->refreshSetPinDialogStyle();
    }

    if (genKey != nullptr) {
        free(genKey);
    }
    if (pubEvpPKey != nullptr) {
        EVP_PKEY_free(pubEvpPKey);
    }
    if (encData != nullptr) {
        free(encData);
    }
    if (pkB64 != nullptr) {
        free(pkB64);
    }
    if (oldPinEcbData != nullptr) {
        free(oldPinEcbData);
    }
    if (oldPinB64 != nullptr) {
        free(oldPinB64);
    }
    if (newPinEcbData != nullptr) {
        free(newPinEcbData);
    }
    if (newPinB64 != nullptr) {
        free(newPinB64);
    }
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
void PasskeyWorker::requestMakeCredStatus(const QString &id, const QString &user, int finish, const QString &result)
{
    qCDebug(DCC_PASSKEY) << "Request make cred status signal : id = " << id << ", user = " << user
                         << ", finish = " << finish << ", result = " << result;

    Q_UNUSED(user);

    if (m_currentId == IdErrorFlag || m_currentId != id) {
        return;
    }

    QJsonObject obj = QJsonDocument::fromJson(result.toUtf8()).object();
    if (obj.isEmpty()) {
        return;
    }
    const int code = obj["code"].toInt();

    // finish = 0，表示正在注册设备的流程中，比如需要触摸设备
    // finish = 1，表示注册设备流程结束
    if (finish == 0) {
        if (code == FIDO_ERR_USER_ACTION_PENDING) {
            // 触碰设备
            updatePromptInfo(PromptType::Touch);
        } else if (code == DEEPIN_ERR_DEVICE_OPEN) {
            // 设备开启，开始闪烁，此时如果切到控制中心其它模块或关闭控制中心，需要通知后端关闭设备
            m_needCloseDevice = true;
        } else if (code == FIDO_ERR_USER_ACTION_TIMEOUT) {
            // 操作设备超时
            updatePromptInfo(PromptType::Timeout);
        } else {
            // 其余错误都按未知错误处理
            updatePromptInfo(PromptType::Unknown);
        }
    } else {
        m_needCloseDevice = false;
        if (code == FIDO_OK) {
            // 成功
            updateManageInfo();
        } else if (code == FIDO_ERR_PIN_REQUIRED) {
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
        qCWarning(DCC_PASSKEY) << "Call method 'GetPinStatus' failed, error message: " << status.error().message();
        m_model->setManagePageInfo({false , false});
        return;
    }

    ManageInfo info;
    info.supportPin = (status.argumentAt(0).toInt() == 1);
    info.existPin = (status.argumentAt(1).toInt() == 1);
    m_model->setManagePageInfo(info);
    m_model->setSetPinDialogStyle(info.existPin ? SetPinDialogStyle::ChangePinStyle : SetPinDialogStyle::SetPinStyle);
}

QString PasskeyWorker::getCurrentUser()
{
    return m_login1Inter->property("Name").toString();
}

int PasskeyWorker::getDeviceCount()
{
    m_needCloseDevice = false;
    QDBusPendingReply<int> reply = m_passkeyInter->GetDeviceCount();
    reply.waitForFinished();
    if (!reply.isValid()) {
        qCWarning(DCC_PASSKEY) << "Call method 'GetDeviceCount' failed, error message: " << reply.error().message();
        return -1;
    }
    return reply;
}

int PasskeyWorker::getUserValidCredentialCount()
{
    m_needCloseDevice = false;
    QDBusPendingReply<int> reply = m_passkeyInter->GetValidCredCount(getCurrentUser());
    reply.waitForFinished();
    if (!reply.isValid()) {
        qCWarning(DCC_PASSKEY) << "Call method 'GetValidCredCount' failed, error message: " << reply.error().message();
        return -1;
    }
    return reply;
}

QDBusPendingReply<int, int> PasskeyWorker::getPinStatus()
{
    m_needCloseDevice = false;
    // reply.argumentAt(0): support - 1表示支持pin；0不支持pin。
    // reply.argumentAt(1): exist - 1表示pin已设置；0表示pin未设置，如果要使用pin需要先设置pin。
    QDBusPendingReply<int, int> reply = m_passkeyInter->GetPinStatus();
    reply.waitForFinished();
    return reply;
}

void PasskeyWorker::getAssertion()
{
    m_needCloseDevice = false;
    m_needPromptMonitor = false;

    // 认证设备
    QDBusPendingReply<QString> reply = m_passkeyInter->GetAssertion(getCurrentUser(), "", "");
    reply.waitForFinished();
    if (reply.isValid()) {
        m_currentId = reply;
        qCDebug(DCC_PASSKEY) << "GetAssertion: id = " << m_currentId;
    } else {
        m_currentId = IdErrorFlag;
        qCWarning(DCC_PASSKEY) << "Call method 'GetAssertion' failed, error message: " << reply.error().message();
    }
}

// 认证设备信号处理
void PasskeyWorker::requestGetAssertStatus(const QString &id, const QString &user, int finish, const QString &result)
{
    qCDebug(DCC_PASSKEY) << "Request get assert status signal : id = " << id << ", user = " << user
                         << ", finish = " << finish << ", result = " << result;

    Q_UNUSED(user);

    if (m_currentId == IdErrorFlag || m_currentId != id) {
        return;
    }

    QJsonObject obj = QJsonDocument::fromJson(result.toUtf8()).object();
    if (obj.isEmpty()) {
        return;
    }
    const int code = obj["code"].toInt();

    // finish = 0，表示正在认证设备的流程中，比如需要触摸设备
    // finish = 1，表示认证设备流程结束
    if (finish == 0) {
        if (code == FIDO_ERR_USER_ACTION_PENDING) {
            // 触碰设备
            m_resetAssertion ? m_model->setResetDialogStyle(ResetDialogStyle::FirstTouchStyle) : updatePromptInfo(PromptType::Touch);
        } else if (code == FIDO_ERR_USER_ACTION_TIMEOUT) {
            // 操作设备超时
            m_resetAssertion ? m_model->setResetDialogStyle(ResetDialogStyle::FailedStyle, false) : updatePromptInfo(PromptType::Timeout);
        } else if (code == DEEPIN_ERR_DEVICE_OPEN) {
            // 设备开启，开始闪烁，此时如果切到控制中心其它模块或关闭控制中心，需要通知后端关闭设备
            m_needCloseDevice = true;
        } else {
            // 其余错误都按未知错误处理
            m_resetAssertion ? m_model->setResetDialogStyle(ResetDialogStyle::FailedStyle, false) : updatePromptInfo(PromptType::Unknown);
        }
    } else {
        m_needCloseDevice = false;
        if (code == FIDO_OK) {
            // 成功
            m_resetAssertion ? reset() : updateManageInfo();
        } else if (code == FIDO_ERR_PIN_REQUIRED) {
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
    m_needCloseDevice = false;
    m_needPromptMonitor = false;

    QDBusPendingReply<QString> reply = m_passkeyInter->Reset();
    reply.waitForFinished();
    if (reply.isValid()) {
        m_currentId = reply;
        qCDebug(DCC_PASSKEY) << "Reset: id = " << m_currentId;
    } else {
        m_currentId = IdErrorFlag;
        qCWarning(DCC_PASSKEY) << "Call method 'Reset' failed, error message: " << reply.error().message();
    }
}

void PasskeyWorker::requestResetStatus(const QString &id, int finish, const QString &result)
{
    qCDebug(DCC_PASSKEY) << "Request reset status signal : id = " << id << ", finish = " << finish << ", result = " << result;

    if (m_currentId == IdErrorFlag || m_currentId != id) {
        return;
    }

    QJsonObject obj = QJsonDocument::fromJson(result.toUtf8()).object();
    if (obj.isEmpty()) {
        return;
    }
    const int code = obj["code"].toInt();

    // finish = 0，表示正在重置设备的流程中，比如需要触摸设备
    // finish = 1，表示重置设备流程结束
    if (finish == 0) {
        if (code == FIDO_ERR_USER_ACTION_PENDING) {
            // 触碰设备
            m_model->setResetDialogStyle(ResetDialogStyle::SecondTouchStyle);
        } else if (code == DEEPIN_ERR_DEVICE_OPEN) {
            // 设备开启，开始闪烁，此时如果切到控制中心其它模块或关闭控制中心，需要通知后端关闭设备
            m_needCloseDevice = true;
        } else {
            // 其余错误都按未知错误处理
            m_model->setResetDialogStyle(ResetDialogStyle::FailedStyle, false);
        }
    } else {
        m_needCloseDevice = false;
        if (code == FIDO_OK) {
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
    m_needCloseDevice = false;
    QDBusPendingReply<> reply = m_passkeyInter->SetPin(oldPin, newPin);
    reply.waitForFinished();
    if (reply.isError()) {
        qCWarning(DCC_PASSKEY) << "Call method 'SetPin' failed, error message: " << reply.error().name() << reply.error().message();

        // 后端服务返回错误
        if (reply.error().name().contains("com.deepin.Passkey")) {
            QJsonObject obj = QJsonDocument::fromJson(reply.error().message().toUtf8()).object();
            if (obj.isEmpty()) {
                Q_EMIT setPinCompleted(false, false, QString());
                return;
            }
            const int code = obj["code"].toInt();
            switch (code) {
            // PIN错误
            case FIDO_ERR_PIN_INVALID:
                Q_EMIT setPinCompleted(false, true, tr("PIN error"));
                break;
            // 3次PIN错误后，PIN认证锁定，需要重新拔插（上电）才可以再次使用PIN
            case FIDO_ERR_PIN_AUTH_BLOCKED:
                Q_EMIT setPinCompleted(false, true, tr("PIN locked, please replug"));
                break;
            // 错误超过8次之后就会彻底锁定，需要重置才可以使用PIN
            case FIDO_ERR_PIN_BLOCKED:
                Q_EMIT setPinCompleted(false, true, tr("PIN Blocked"));
                break;
            default:
                Q_EMIT setPinCompleted(false, false, QString());
                break;
            }
        } else {
            Q_EMIT setPinCompleted(false, false, QString());
        }
    } else {
        Q_EMIT setPinCompleted(true, false, QString());
        updateManageInfo();
        qCDebug(DCC_PASSKEY) << "Passkey set pin completed .";
    }
}

QString PasskeyWorker::encryptKey(int keyType)
{
    m_needPromptMonitor = false;

    QDBusPendingReply<QString> reply = m_passkeyInter->EncryptKey(keyType);
    reply.waitForFinished();
    if (reply.isValid()) {
        return reply;
    } else {
        qCWarning(DCC_PASSKEY) << "Call method 'EncryptKey' failed, error message: " << reply.error().message();
        return PublicKeyErrorFlag;
    }
}

bool PasskeyWorker::setSymmetricKey(int encryptType, int keyType, const QString &key)
{
    QDBusPendingReply<> reply = m_passkeyInter->SetSymmetricKey(encryptType, keyType, key);
    reply.waitForFinished();
    if (reply.isError()) {
        qCWarning(DCC_PASSKEY) << "Call method 'SetSymmetricKey' failed, error message: " << reply.error().message();
        return false;
    }
    return true;
}

void PasskeyWorker::deviceClose(const QString &id)
{
    m_needCloseDevice = false;
    if (m_currentId == IdErrorFlag) {
        return;
    }

    // getAssertion认证，接收到DEEPIN_ERR_DEVICE_OPEN信号，提示用户触摸设备时，可以调用该接口结束流程
    QDBusPendingReply<> reply = m_passkeyInter->DeviceClose(id);
    reply.waitForFinished();
    if (reply.isError()) {
        qCWarning(DCC_PASSKEY) << "Call method 'DeviceClose' failed, error message: " << reply.error().message();
    }
}
