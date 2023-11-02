// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QObject>

#include <DDialog>
#include <DPasswordEdit>
#include <DLabel>

DWIDGET_USE_NAMESPACE

class KeyPinDialogCtrl : public QObject
{
    Q_OBJECT
public:
    explicit KeyPinDialogCtrl(QObject *parent = nullptr);
    ~KeyPinDialogCtrl() override;

    void hideAllDialog();
    void showSetPinDialog();
    void showChangePinDialog();
    void showFailedDialog();

    void showChangePinDialogAlertMessage(const QString &msg);

Q_SIGNALS:
    void requestSetPin(const QString &oldPin, const QString &newPin);

private:
    void initSetPinDialogUI();
    void initChangePinDialogUI();
    void initFailedDialogUI();

    void setPasswordEditAttribute(DPasswordEdit *edit);
    bool judgePinSize(DPasswordEdit *edit);
    bool judgePinConsistent(DPasswordEdit *targetEdit, DPasswordEdit *sourceEdit);

private:
    DDialog *m_setPinDialog;
    DDialog *m_changePinDialog;
    DDialog *m_failedDialog;
    bool m_setPinState;
    DLabel *m_failedTipLabel;
    DPasswordEdit *m_oldPwdEdit;
};

