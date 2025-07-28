// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "passkeycontroller.h"
#include "dccfactory.h"

PasskeyController::PasskeyController(QObject *parent)
    : QObject(parent)
{
}

PasskeyController::~PasskeyController()
{
}

DCC_FACTORY_CLASS(PasskeyController)

#include "passkeycontroller.moc"
