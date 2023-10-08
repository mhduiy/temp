// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QObject>
#include <QTimer>
#include <QIcon>

#include <DDialog>
#include <DLabel>
#include <DSpinner>

DWIDGET_USE_NAMESPACE

class ResetKeyDialogCtrl : public QObject
{
    Q_OBJECT
public:
    explicit ResetKeyDialogCtrl(QObject *parent = nullptr);
    ~ResetKeyDialogCtrl() override;

    void hideAllDialog();
    void showDescriptionDialog();
    void showInsertDeviceDialog();
    void showIdentifyingDeviceDialog();
    void showFirstTouchDeviceDialog();
    void showSecondTouchDeviceDialog();
    void showFailedDialog();
    void showResultDialog(bool success = false);

Q_SIGNALS:
    void resetBtnClicked();
    void requestStopReset(bool success);

private:
    void initResetTimer();
    void initDescriptionDialogUI();
    void initInsertDialogUI();
    void initIdentifyingDeviceUI();
    void initFirstTouchDialogUI();
    void initSecondTouchDialogUI();
    void initFailedDialogUI();
    void initResultDialogUI();

private:
    DDialog *m_descriptionDialog;
    DDialog *m_insertDialog;
    DDialog *m_identifyingDialog;
    DDialog *m_firstTouchDialog;
    DDialog *m_secondTouchDialog;
    DDialog *m_failedDialog;
    DDialog *m_resultDialog;

    QTimer *m_resetTimer;
    int m_resetTimerCount;

    DLabel *m_resultPicLabel;
    DLabel *m_resultTipLabel;
    DSpinner *m_identifyingSpinner;
};
