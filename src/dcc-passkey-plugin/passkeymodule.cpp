// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "passkeymodule.h"
#include "passkeywidget.h"
#include "passkeymodel.h"
#include "passkeyworker.h"
#include "module/common.h"
#include "gsettingwatcher.h"

#include <DConfig>

DCORE_USE_NAMESPACE

Q_LOGGING_CATEGORY(DCC_PASSKEY, "dcc.passkey")

PasskeyModule::PasskeyModule(QObject *parent)
    : QObject(parent)
    , m_widget(nullptr)
    , m_model(nullptr)
    , m_worker(nullptr)
    , m_workerThread(nullptr)
{
    QTranslator *translator = new QTranslator(this);
    translator->load(QString("/usr/share/dcc-passkey-plugin/translations/dcc-passkey-plugin_%1.qm").arg(QLocale::system().name()));
    QCoreApplication::installTranslator(translator);
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
    bool hide = true;
    DConfig *config = DConfig::create("org.deepin.dde.passkey", "org.deepin.dde.passkey.dcc-plugin");
    if ((config && config->isValid())) {
        hide = config->value("dccPasskeyPluginHideStatus", true).toBool();
    }
    if (hide) {
        qCInfo(DCC_PASSKEY) << displayName() << " is disabled";
        setAvailable(false);
        return;
    }

    Q_UNUSED(sync);
    Q_UNUSED(pushtype);

    m_model = new PasskeyModel(this);

    m_worker = QSharedPointer<PasskeyWorker>(new PasskeyWorker(m_model));
    m_workerThread = QSharedPointer<QThread>(new QThread);
    m_worker->moveToThread(m_workerThread.get());
    m_workerThread->start(QThread::LowPriority);
    connect(m_worker.get(), &PasskeyWorker::requestInit, m_worker.get(), &PasskeyWorker::init);
    connect(m_worker.get(), &PasskeyWorker::requestActive, m_worker.get(), &PasskeyWorker::activate);
    connect(m_worker.get(), &PasskeyWorker::requestDeactivate, m_worker.get(), &PasskeyWorker::deactivate);
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
    return tr("Security Key");
}

QIcon PasskeyModule::icon() const
{
    return QIcon(IconPixmapPath);
}

void PasskeyModule::active()
{
    m_worker->updatePromptInfo(PromptType::Insert);
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
        m_frameProxy->addChildPageTrans("Manage Security Key", tr("Manage Security Key"));
    }
}

void PasskeyModule::initSearchData()
{
    const QString &module = displayName();
    const QString &passkeyManage = tr("Manage Security Key");
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
            m_frameProxy->setDetailVisible(module, passkeyManage, tr("Manage Security Key"), visible);
            m_frameProxy->updateSearchData(module);
        }
    });

    auto func_process_all = [ = ] {
        visible = func_is_visible(hideModuleFlag);
        m_frameProxy->setModuleVisible(module, visible);
        m_frameProxy->setWidgetVisible(module, passkeyManage, visible);
        m_frameProxy->setDetailVisible(module, passkeyManage, tr("Manage Security Key"), visible);
    };

    func_process_all();
}

QString PasskeyModule::description() const
{
    return tr("Sign in with a physical security key");
}
