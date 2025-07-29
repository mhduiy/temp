// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "passkeymodel.h"
#include <qlogging.h>

PasskeyModel::PasskeyModel(QObject* parent)
    : QObject(parent)
{
    m_currentStage = PasskeyStage::Prompt;


}

PasskeyModel::~PasskeyModel()
{
}

void PasskeyModel::setCurrentStage(const PasskeyStage &stage)
{
    if (stage == m_currentStage) {
        return;
    }

    m_currentStage = stage;
    Q_EMIT currentStageChanged(stage);
}

void PasskeyModel::setPromptType(const PromptType &type)
{
    qWarning() << "---setPromptType---" << type;
    if (type == m_promptType) {
        return;
    }

    m_promptType = type;
    Q_EMIT promptTypeChanged(type);
}


void PasskeyModel::setResetDialogStyle(const ResetDialogStyle &style)
{
    if (style == m_resetDialogStyle) {
        return;
    }

    m_resetDialogStyle = style;
    Q_EMIT refreshResetDialogStyle();
}

void PasskeyModel::setSetPinDialogStyle(const SetPinDialogStyle &style)
{
    if (style == m_setPinDialogStyle) {
        return;
    }

    m_setPinDialogStyle = style;
    Q_EMIT refreshSetPinDialogStyle();
}

void PasskeyModel::setManagePageInfo(const ManageInfo &info)
{
    m_managePageInfo = info;
    Q_EMIT managePageInfoChanged();
}
