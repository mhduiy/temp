// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QObject>
#include <QPair>
#include <QDebug>

#include "common.h"

class PasskeyModel : public QObject {
    Q_OBJECT
public:
    explicit PasskeyModel(QObject* parent = nullptr);
    ~PasskeyModel();

    void setCurrentPage(const StackedPageIndex &page);
    StackedPageIndex currentPage() const { return m_currentPage; }

    void setPromptPageInfo(const PromptType &type, const PromptInfo &info);
    QPair<PromptType, PromptInfo> promptPageInfo() const { return m_promptPageInfo; }

    void setManagePageInfo(const ManageInfo &info);
    ManageInfo managePageInfo() const { return m_managePageInfo; }

    void setResetDialogStyle(const ResetDialogStyle &style, bool status = false);
    QPair<ResetDialogStyle, bool> resetDialogStyle() const { return m_resetDialogStyle; }

    void setSetPinDialogStyle(const SetPinDialogStyle &style);
    SetPinDialogStyle setPinDialogStyle() const { return m_setPinDialogStyle; }

Q_SIGNALS:
    void refreshUI();
    void refreshResetDialogStyle();
    void refreshSetPinDialogStyle();

private:
    StackedPageIndex m_currentPage;
    QPair<PromptType, PromptInfo> m_promptPageInfo;
    ManageInfo m_managePageInfo;
    QPair<ResetDialogStyle, bool> m_resetDialogStyle;
    SetPinDialogStyle m_setPinDialogStyle;
};
