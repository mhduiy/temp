// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "passkeymodule.h"
#include "passkeywidget.h"
#include "passkeymodel.h"
#include "passkeyworker.h"
#include "module/common.h"
#include "gsettingwatcher.h"

DCORE_USE_NAMESPACE

Q_LOGGING_CATEGORY(DCC_PASSKEY, "dcc.passkey")

PasskeyModule::PasskeyModule(QObject *parent)
    : QObject(parent)
    , m_widget(nullptr)
    , m_model(nullptr)
    , m_worker(nullptr)
    , m_workerThread(nullptr)
{

}

PasskeyModule::~PasskeyModule()
{
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

void PasskeyModule::preInitialize(bool sync, FrameProxyInterface::PushType pushtype)
{
    Q_UNUSED(sync);
    Q_UNUSED(pushtype);

    m_model = new PasskeyModel(this);

    m_worker  = QSharedPointer<PasskeyWorker>(new PasskeyWorker(m_model));
    m_workerThread = QSharedPointer<QThread>(new QThread);
    m_worker->moveToThread(m_workerThread.get());
    m_workerThread->start(QThread::LowPriority);
    connect(m_worker.get(), &PasskeyWorker::requestInit, m_worker.get(), &PasskeyWorker::init);
    connect(m_worker.get(), &PasskeyWorker::requestActive, m_worker.get(), &PasskeyWorker::activate);
    Q_EMIT m_worker->requestInit();

    addChildPageTrans();
    initSearchData();
}

void PasskeyModule::initialize()
{

}

const QString PasskeyModule::name() const
{
    return QStringLiteral("passkey");
}

const QString PasskeyModule::displayName() const
{
    return tr("Passkey");
}

QIcon PasskeyModule::icon() const
{
    return QIcon(IconPixmapPath);
}

void PasskeyModule::active()
{
    Q_EMIT m_worker->requestActive();

    m_widget = new PasskeyWidget();
    m_widget->setModel(m_model, m_worker.get());
    m_widget->refreshUI();

    m_frameProxy->pushWidget(this, m_widget);
}

void PasskeyModule::deactive()
{
    if (m_model) {
        m_model->deleteLater();
        m_model = nullptr;
    }

    if (m_worker) {
        m_worker->deleteLater();
        m_worker = nullptr;
    }
}

int PasskeyModule::load(const QString &path)
{
    if (m_widget && m_worker) {
        Q_EMIT m_worker->requestActive();
        m_widget->refreshUI();
        return 0;
    }

    return -1;
}

void PasskeyModule::addChildPageTrans() const
{
    if (m_frameProxy != nullptr) {
        m_frameProxy->addChildPageTrans("密钥管理", tr("密钥管理"));
        m_frameProxy->addChildPageTrans("安全密钥PIN", tr("安全密钥PIN"));
        m_frameProxy->addChildPageTrans("重置密钥", tr("重置密钥"));
    }
}

void PasskeyModule::initSearchData()
{
    const QString &module = displayName();
    const QString &passkeyManage = tr("密钥管理");
    const QString &hideModuleFlag = "hideModule";
    static bool visible = false;

    auto func_is_visible = [=](const QString &gsetting) {
        if (gsetting != hideModuleFlag) {
            return false;
        }
        return !GSettingWatcher::instance()->get(gsetting).toString().contains("passkey");
    };

    connect(GSettingWatcher::instance(), &GSettingWatcher::notifyGSettingsChanged, this, [=](const QString &gsetting, const QString &state) {
        if (gsetting != hideModuleFlag) {
            return;
        }

        bool show = !GSettingWatcher::instance()->get(gsetting).toString().contains("passkey");
        if (show != visible) {
            visible = show;
            m_frameProxy->setModuleVisible(module, visible);
            m_frameProxy->setWidgetVisible(module, passkeyManage, visible);
            m_frameProxy->setDetailVisible(module, passkeyManage, tr("密钥管理"), visible);
            m_frameProxy->setDetailVisible(module, passkeyManage, tr("安全密钥PIN"), visible);
            m_frameProxy->setDetailVisible(module, passkeyManage, tr("重置密钥"), visible);
            m_frameProxy->updateSearchData(module);
        }
    });

    auto func_process_all = [ = ] {
        visible = func_is_visible(hideModuleFlag);
        m_frameProxy->setModuleVisible(module, visible);
        m_frameProxy->setWidgetVisible(module, passkeyManage, visible);
        m_frameProxy->setDetailVisible(module, passkeyManage, tr("密钥管理"), visible);
        m_frameProxy->setDetailVisible(module, passkeyManage, tr("安全密钥PIN"), visible);
        m_frameProxy->setDetailVisible(module, passkeyManage, tr("重置密钥"), visible);
     };

    func_process_all();
}


