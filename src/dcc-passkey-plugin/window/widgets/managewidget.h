// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <widgets/contentwidget.h>

#include <QWidget>

DWIDGET_USE_NAMESPACE

using namespace dcc;

class ManageWidget : public ContentWidget
{
    Q_OBJECT

public:
    explicit ManageWidget(QWidget *parent = nullptr);
    ~ManageWidget() override;

    void setPinStatus(bool support, bool exist);

public Q_SLOTS:

Q_SIGNALS:
    void pinStatusChanged(bool support, bool exist);
    void pinBtnClicked();
    void resetBtnClicked();

private Q_SLOTS:

private:

};


