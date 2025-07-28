// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QObject>
#include <QPair>
#include <QDebug>

#include "common.h"

using namespace dcc::passkey::common;

class PasskeyModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(PasskeyStage currentStage READ currentStage WRITE setCurrentStage NOTIFY currentStageChanged)
    Q_PROPERTY(PromptType promptType READ promptType WRITE setPromptType NOTIFY promptTypeChanged)
    Q_PROPERTY(bool existPin READ getExistPin NOTIFY managePageInfoChanged)
    Q_PROPERTY(bool supportPin READ getSupportPin NOTIFY managePageInfoChanged)

    Q_PROPERTY(ResetDialogStyle resetDialogStyle READ resetDialogStyle WRITE setResetDialogStyle NOTIFY refreshResetDialogStyle)
    Q_PROPERTY(SetPinDialogStyle setPinDialogStyle READ setPinDialogStyle WRITE setSetPinDialogStyle NOTIFY refreshSetPinDialogStyle)

public:
    explicit PasskeyModel(QObject* parent = nullptr);
    ~PasskeyModel();

    void setCurrentStage(const PasskeyStage &stage);
    PasskeyStage currentStage() const { return m_currentStage; }

    void setPromptType(const PromptType &type);
    PromptType promptType() const { return m_promptType; }

    void setManagePageInfo(const ManageInfo &info);
    ManageInfo managePageInfo() const { return m_managePageInfo; }

    bool getExistPin() const { return m_managePageInfo.existPin; }
    bool getSupportPin() const { return m_managePageInfo.supportPin; }

    void setResetDialogStyle(const ResetDialogStyle &style);
    ResetDialogStyle resetDialogStyle() const { return m_resetDialogStyle; }

    void setSetPinDialogStyle(const SetPinDialogStyle &style);
    SetPinDialogStyle setPinDialogStyle() const { return m_setPinDialogStyle; }

Q_SIGNALS:
    void refreshUI();
    void refreshResetDialogStyle();
    void refreshSetPinDialogStyle();

    void currentStageChanged(const PasskeyStage &stage);
    void promptTypeChanged(const PromptType &type);
    void managePageInfoChanged();

private:
    PasskeyStage m_currentStage;
    PromptType m_promptType = PromptType::Insert;
    ManageInfo m_managePageInfo;

    ResetDialogStyle m_resetDialogStyle = ResetDialogStyle::DescriptionStyle;
    SetPinDialogStyle m_setPinDialogStyle = SetPinDialogStyle::SetPinStyle;
};
