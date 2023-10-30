// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QWidget>
#include <QPixmap>
#include <QVBoxLayout>

#include <DLabel>
#include <DSpinner>
#include <DPushButton>
#include <DTipLabel>
#include <DCommandLinkButton>

DWIDGET_USE_NAMESPACE

class PromptWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PromptWidget(QWidget *parent = nullptr);
    ~PromptWidget() override;

    void setPixmap(const QString &path);
    void setPromptInfo(const QString &text, const QString &tip, bool needSpinner = false);
    void setOperateBtn(const QString &text, bool needBtn = false);

Q_SIGNALS:
    void operateBtnClicked(const QString &text);
    void jumpLinkBtnClicked();

private:
    QVBoxLayout *m_mainLayout;
    DLabel *m_picLabel;
    QHBoxLayout *m_promptStretchLayout;
    DSpinner *m_spinner;
    DLabel *m_promptLabel;
    DLabel *m_tipLabel;
    DPushButton *m_operateBtn;
    DCommandLinkButton *m_jumpLinkBtn;
};

