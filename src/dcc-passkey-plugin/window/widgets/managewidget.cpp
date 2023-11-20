// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "managewidget.h"
#include <widgets/translucentframe.h>
#include <widgets/settingsgroup.h>
#include <widgets/titlevalueitem.h>

#include <QBoxLayout>
#include <QObject>
#include <QPushButton>

#include <DLabel>
#include <DFontSizeManager>

using namespace dcc::widgets;

ManageWidget::ManageWidget(QWidget *parent)
    : ContentWidget(parent)
{
    TranslucentFrame* contentWidget = new TranslucentFrame(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);

    //~ contents_path /passkey/Manage Security Key
    DLabel *keyManageLabel = new DLabel(tr("Manage Security Key"), this);
    DFontSizeManager::instance()->bind(keyManageLabel, DFontSizeManager::T5, QFont::DemiBold);
    keyManageLabel->setContentsMargins(10, 0, 0, 0);
    contentLayout->addWidget(keyManageLabel);
    contentLayout->addSpacing(5);

    SettingsGroup *keyManageGroup = new SettingsGroup(contentWidget);

    TitleAuthorizedItem *pin = new TitleAuthorizedItem(keyManageGroup);
    //~ contents_path /passkey/PIN
    pin->setTitle(tr("PIN"));
    pin->setValueForegroundRole(QColor(255, 0, 0));
    pin->setButtonText(tr("Change"));
    connect(pin, &TitleAuthorizedItem::clicked, this, &ManageWidget::pinBtnClicked);
    connect(this, &ManageWidget::pinStatusChanged, pin, [pin] (bool support, bool exist) {
        const auto btn = qobject_cast<QPushButton *>(pin->getActivatorBtn());
        if (support) {
            btn->setEnabled(true);
            exist ? btn->setText(tr("Change")) : btn->setText(tr("Setting"));
        } else {
            btn->setEnabled(false);
            btn->setText(tr("Setting"));
        }
    });
    keyManageGroup->appendItem(pin);

    TitleAuthorizedItem *resetKey = new TitleAuthorizedItem(keyManageGroup);
    //~ contents_path /passkey/Reset Security Key
    resetKey->setTitle(tr("Reset Security Key"));
    resetKey->setValueForegroundRole(QColor(255, 0, 0));
    resetKey->setButtonText(tr("Reset"));
    connect(resetKey, &TitleAuthorizedItem::clicked, this, &ManageWidget::resetBtnClicked);
    keyManageGroup->appendItem(resetKey);

    contentLayout->addWidget(keyManageGroup);
    contentLayout->setAlignment(Qt::AlignTop);

    setContentsMargins(36, 10, 36, 10);
    setContent(contentWidget);
}

ManageWidget::~ManageWidget()
{

}

void ManageWidget::setPinStatus(bool support, bool exist)
{
    Q_EMIT pinStatusChanged(support, exist);
}
