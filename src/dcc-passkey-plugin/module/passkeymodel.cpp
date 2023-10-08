// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "passkeymodel.h"

PasskeyModel::PasskeyModel(QObject* parent)
    : QObject(parent)
{
    m_promptPageInfo.first = PromptType::Count;
}

PasskeyModel::~PasskeyModel()
{
}

void PasskeyModel::setCurrentPage(const StackedPageIndex &page)
{
    if (page == m_currentPage) {
        return;
    }

    m_currentPage = page;
    Q_EMIT refreshUI();
}

void PasskeyModel::setPromptPageInfo(const PromptType &type, const PromptInfo &info)
{
    if (type == m_promptPageInfo.first) {
        return;
    }

    m_promptPageInfo.first = type;
    m_promptPageInfo.second = info;
    Q_EMIT refreshUI();
}

void PasskeyModel::setManagePageInfo(const ManageInfo &info)
{
    m_managePageInfo = info;
    Q_EMIT refreshUI();
}

void PasskeyModel::setResetDialogStyle(const ResetDialogStyle &style, bool status)
{
    m_resetDialogStyle.first = style;
    m_resetDialogStyle.second = status;
    Q_EMIT refreshResetDialogStyle();
}

void PasskeyModel::setSetPinDialogStyle(const SetPinDialogStyle &style)
{
    m_setPinDialogStyle = style;
}
