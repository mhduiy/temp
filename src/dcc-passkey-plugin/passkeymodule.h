// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QObject>

#include "interface/namespace.h"
#include "interface/moduleinterface.h"
#include "interface/frameproxyinterface.h"
#include "window/passkeywidget.h"
#include "module/passkeyworker.h"

using namespace DCC_NAMESPACE;

class PasskeyWidget;
class PasskeyModule : public QObject, public DCC_NAMESPACE::ModuleInterface
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID ModuleInterface_iid FILE "passkey.json")
    Q_INTERFACES(DCC_NAMESPACE::ModuleInterface)
    Q_PROPERTY(QString description READ description)

public:
    explicit PasskeyModule(QObject *parent = nullptr);
    ~PasskeyModule() override;

    void initialize() override;
    void preInitialize(bool sync = false, FrameProxyInterface::PushType = FrameProxyInterface::PushType::Normal) override;
    const QString name() const override;
    const QString displayName() const override;
    QIcon icon() const override;
    void active() override;
    void deactive() override;
    QString path() const override { return MAINWINDOW; }
    QString follow() const override { return "authentication"; }
    int load(const QString &path) override;
    void addChildPageTrans() const override;
    QString description() const { return tr("Sign in with a physical security key"); }

private:
    void initSearchData() override;

private:
    QPointer<PasskeyWidget> m_widget;
    PasskeyModel *m_model;
    QSharedPointer<PasskeyWorker> m_worker;
    QSharedPointer<QThread> m_workerThread;
};
