// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "passkeycontroller.h"
#include "dccfactory.h"

PasskeyController::PasskeyController(QObject *parent)
    : QObject(parent)
    , m_model(new PasskeyModel(this))
    , m_worker(new PasskeyWorker(m_model, this))
{
    m_worker->init();
    qmlRegisterUncreatableMetaObject(dcc::passkey::common::staticMetaObject, "org.deepin.dcc.passkey", 1, 0, "Common", "Cannot create namespace");
    m_worker->activate();
}

PasskeyController::~PasskeyController()
{
    m_model->deleteLater();
    m_worker->deactivate();
    m_worker->deleteLater();
}

DCC_FACTORY_CLASS(PasskeyController)

#include "passkeycontroller.moc"
