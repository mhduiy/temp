// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "promptwidget.h"

#include <QBoxLayout>

#include <DFontSizeManager>
#include <DHiDPIHelper>

PromptWidget::PromptWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(new QVBoxLayout())
    , m_picLabel(new DLabel(this))
    , m_promptStretchLayout(new QHBoxLayout())
    , m_spinner(new DSpinner(this))
    , m_promptLabel(new DLabel(this))
    , m_tipLabel(new DLabel(this))
    , m_operateBtn(new DPushButton(this))
    , m_jumpLinkBtn(new DCommandLinkButton(tr("跳过"), this))
{
    m_picLabel->setAlignment(Qt::AlignHCenter);
    m_picLabel->setFixedHeight(180);
    m_picLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_picLabel->setContentsMargins(0, 0, 0, 0);

    m_spinner->setVisible(false);
    m_spinner->setMinimumSize(22, 22);
    m_spinner->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_promptLabel->setAlignment(Qt::AlignCenter);
    m_promptLabel->setContentsMargins(0, 0, 0, 0);
    m_promptLabel->setMinimumHeight(22);
    m_promptLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    DFontSizeManager::instance()->bind(m_promptLabel, DFontSizeManager::T6, QFont::Medium);

    m_promptStretchLayout->setMargin(0);
    m_promptStretchLayout->setSpacing(10);
    m_promptStretchLayout->addStretch(1);
    m_promptStretchLayout->addWidget(m_spinner, 0);
    m_promptStretchLayout->addWidget(m_promptLabel, 0);
    m_promptStretchLayout->addStretch(1);

    m_tipLabel->setAlignment(Qt::AlignCenter);
    m_tipLabel->setContentsMargins(0, 0, 0, 0);
    m_tipLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    DFontSizeManager::instance()->bind(m_tipLabel, DFontSizeManager::T8, QFont::Medium);

    m_operateBtn->setVisible(false);
    m_operateBtn->setFixedWidth(200);
    connect(m_operateBtn, &DPushButton::clicked, this, [this] {
        Q_EMIT operateBtnClicked(m_operateBtn->text());
    });
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setMargin(0);
    btnLayout->setSpacing(0);
    btnLayout->addWidget(m_operateBtn);
    btnLayout->setAlignment(Qt::AlignHCenter);

    DFontSizeManager::instance()->bind(m_jumpLinkBtn, DFontSizeManager::T7, QFont::Medium);
    QFontMetrics fontMetrics(m_jumpLinkBtn->font());
    m_jumpLinkBtn->setFixedWidth(fontMetrics.boundingRect(m_jumpLinkBtn->text()).width() + 20);
    connect(m_jumpLinkBtn, &DCommandLinkButton::clicked, this, &PromptWidget::jumpLinkBtnClicked);
    QHBoxLayout *jumpLinkLayout = new QHBoxLayout();
    jumpLinkLayout->setMargin(0);
    jumpLinkLayout->setSpacing(0);
    jumpLinkLayout->addWidget(m_jumpLinkBtn);
    jumpLinkLayout->setAlignment(Qt::AlignHCenter);

    m_mainLayout->setMargin(0);
    m_mainLayout->setSpacing(0);

    m_mainLayout->addStretch(1);
    m_mainLayout->addWidget(m_picLabel, 0);
    m_mainLayout->addSpacing(20);
    m_mainLayout->addLayout(m_promptStretchLayout, 0);
    m_mainLayout->addSpacing(5);
    m_mainLayout->addWidget(m_tipLabel, 0);
    m_mainLayout->addSpacing(20);
    m_mainLayout->addLayout(btnLayout, 0);
    m_mainLayout->addSpacing(10);
    m_mainLayout->addLayout(jumpLinkLayout, 0);
    m_mainLayout->addStretch(1);

    setLayout(m_mainLayout);
}

PromptWidget::~PromptWidget()
{
}

void PromptWidget::setPixmap(const QString &path)
{
    m_picLabel->setPixmap(DHiDPIHelper::loadNxPixmap(path));
}

void PromptWidget::setPromptInfo(const QString &text, const QString &tip, bool needSpinner)
{
    if (needSpinner) {
        m_spinner->start();
        m_spinner->setVisible(true);
        m_promptStretchLayout->setSpacing(10);
    } else {
        m_spinner->stop();
        m_spinner->setVisible(false);
        m_promptStretchLayout->setSpacing(0);
    }
    m_promptLabel->setText(text);

    if (tip.isEmpty()) {
        m_tipLabel->hide();
    } else {
        m_tipLabel->show();
        m_tipLabel->setText(tip);
    }
}

void PromptWidget::setOperateBtn(const QString &text, bool needBtn, bool showLink)
{
    m_operateBtn->setText(text);
    needBtn ? m_operateBtn->setVisible(true) : m_operateBtn->setVisible(false);
    showLink ? m_jumpLinkBtn->setVisible(true) : m_jumpLinkBtn->setVisible(false);
}
