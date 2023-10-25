// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "keypindialogctrl.h"
#include "../../module/common.h"

#include <QVBoxLayout>
#include <QPixmap>

#include <DFontSizeManager>
#include <DLabel>
#include <DHiDPIHelper>

const QString ChangeBtnText = QObject::tr("更 改");
const QString SetBtnText = QObject::tr("设 置");

KeyPinDialogCtrl::KeyPinDialogCtrl(QObject *parent)
    : QObject(parent)
    , m_setPinDialog(nullptr)
    , m_changePinDialog(nullptr)
    , m_failedDialog(nullptr)
{

}

KeyPinDialogCtrl::~KeyPinDialogCtrl()
{
    if (m_setPinDialog) {
        m_setPinDialog->deleteLater();
        m_setPinDialog = nullptr;
    }
    if (m_changePinDialog) {
        m_changePinDialog->deleteLater();
        m_changePinDialog = nullptr;
    }
    if (m_failedDialog) {
        m_failedDialog->deleteLater();
        m_failedDialog = nullptr;
    }
}

void KeyPinDialogCtrl::showSetPinDialog()
{
    if (!m_setPinDialog) {
        initSetPinDialogUI();
    }
    m_setPinDialog->setWindowFlags(Qt::WindowStaysOnTopHint);
    m_setPinDialog->setWindowFlag(Qt::WindowMinimizeButtonHint, false);
    m_setPinDialog->setAttribute(Qt::WA_ShowModal, true);
    m_setPinDialog->show();
    m_setPinDialog->activateWindow();
}

void KeyPinDialogCtrl::initSetPinDialogUI()
{
    m_setPinDialog = new DDialog();
    m_setPinDialog->setTitle(tr("设置安全密钥PIN"));
    m_setPinDialog->setIcon(DStyle().standardIcon(DStyle::SP_MessageBoxWarning));
    m_setPinDialog->addButton(tr("Cancel"), false, DDialog::ButtonNormal);
    m_setPinDialog->addButton(tr("Set"), false, DDialog::ButtonRecommend);

    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->setAlignment(Qt::AlignTop);

    DLabel *picLabel = new DLabel(widget);
    picLabel->setPixmap(DHiDPIHelper::loadNxPixmap(SetPasswordPixmapPath));
    picLabel->setFixedHeight(180);
    picLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    picLabel->setAlignment(Qt::AlignHCenter);
    layout->addWidget(picLabel);

    DLabel *newPwdLabel = new DLabel(widget);
    newPwdLabel->setText(tr("新的PIN："));
    newPwdLabel->setContentsMargins(10, 0, 0, 0);
    DFontSizeManager::instance()->bind(newPwdLabel, DFontSizeManager::T6, QFont::Normal);
    DPasswordEdit *newPwdEdit = new DPasswordEdit(widget);
    newPwdEdit->setPlaceholderText(tr("必填"));
    setPasswordEditAttribute(newPwdEdit);
    layout->addWidget(newPwdLabel);
    layout->addSpacing(4);
    layout->addWidget(newPwdEdit);
    layout->addSpacing(10);

    DLabel *repeatPwdLabel = new DLabel(widget);
    repeatPwdLabel->setText(tr("重复PIN："));
    repeatPwdLabel->setContentsMargins(10, 0, 0, 0);
    DFontSizeManager::instance()->bind(repeatPwdLabel, DFontSizeManager::T6, QFont::Normal);
    DPasswordEdit *repeatPwdEdit = new DPasswordEdit(widget);
    repeatPwdEdit->setPlaceholderText(tr("必填"));
    setPasswordEditAttribute(repeatPwdEdit);
    layout->addWidget(repeatPwdLabel);
    layout->addSpacing(4);
    layout->addWidget(repeatPwdEdit);

    layout->setContentsMargins(10, 10, 10, 0);
    widget->setLayout(layout);
    widget->setMinimumHeight(400);

    m_setPinDialog->addContent(widget);
    m_setPinDialog->setOnButtonClickedClose(false);
    m_setPinDialog->setFixedSize(400, 534);

    connect(m_setPinDialog, &DDialog::buttonClicked, this, [this, newPwdEdit, repeatPwdEdit](int index, const QString &text) {
        if (index == 0) {
            m_setPinDialog->hide();
            newPwdEdit->clear();
            repeatPwdEdit->clear();
        } else {
            if (!judgePinSize(newPwdEdit)) {
                return;
            }
            if (!judgePinSize(repeatPwdEdit)) {
                return;
            }
            if (!judgePinConsistent(newPwdEdit, repeatPwdEdit)) {
                return;
            }
            Q_EMIT requestSetPin("", newPwdEdit->text());
            m_setPinDialog->hide();
            newPwdEdit->clear();
            repeatPwdEdit->clear();
        }
    });
    connect(m_setPinDialog, &DDialog::closed, this, [this, newPwdEdit, repeatPwdEdit] {
        m_setPinDialog->hide();
        newPwdEdit->clear();
        repeatPwdEdit->clear();
    });
    connect(newPwdEdit, &DPasswordEdit::textChanged, this, [newPwdEdit](const QString &text){
        if (newPwdEdit->isAlert()) {
            newPwdEdit->hideAlertMessage();
            newPwdEdit->setAlert(false);
        }
    });
    connect(repeatPwdEdit, &DPasswordEdit::textChanged, this, [repeatPwdEdit](const QString &text){
        if (repeatPwdEdit->isAlert()) {
            repeatPwdEdit->hideAlertMessage();
            repeatPwdEdit->setAlert(false);
        }
    });
}

void KeyPinDialogCtrl::showChangePinDialog()
{
    if (!m_changePinDialog) {
        initChangePinDialogUI();
    }
    m_changePinDialog->setWindowFlags(Qt::WindowStaysOnTopHint);
    m_changePinDialog->setWindowFlag(Qt::WindowMinimizeButtonHint, false);
    m_changePinDialog->setAttribute(Qt::WA_ShowModal, true);
    m_changePinDialog->show();
    m_changePinDialog->activateWindow();
}

void KeyPinDialogCtrl::initChangePinDialogUI()
{
    m_changePinDialog = new DDialog();
    m_changePinDialog->setTitle(tr("更改安全密钥PIN"));
    m_changePinDialog->setIcon(DStyle().standardIcon(DStyle::SP_MessageBoxWarning));
    m_changePinDialog->addButton(tr("Cancel"), false, DDialog::ButtonNormal);
    m_changePinDialog->addButton(tr("Change"), false, DDialog::ButtonRecommend);

    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->setAlignment(Qt::AlignTop);

    DLabel *picLabel = new DLabel(widget);
    picLabel->setPixmap(DHiDPIHelper::loadNxPixmap(SetPasswordPixmapPath));
    picLabel->setFixedHeight(180);
    picLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    picLabel->setAlignment(Qt::AlignHCenter);
    layout->addWidget(picLabel);

    DLabel *oldPwdLabel = new DLabel(widget);
    oldPwdLabel->setText(tr("旧的PIN："));
    oldPwdLabel->setContentsMargins(10, 0, 0, 0);
    DFontSizeManager::instance()->bind(oldPwdLabel, DFontSizeManager::T6, QFont::Normal);
    DPasswordEdit *oldPwdEdit = new DPasswordEdit(widget);
    oldPwdEdit->setPlaceholderText(tr("必填"));
    setPasswordEditAttribute(oldPwdEdit);
    layout->addWidget(oldPwdLabel);
    layout->addSpacing(4);
    layout->addWidget(oldPwdEdit);
    layout->addSpacing(10);

    DLabel *newPwdLabel = new DLabel(widget);
    newPwdLabel->setText(tr("新的PIN："));
    newPwdLabel->setContentsMargins(10, 0, 0, 0);
    DFontSizeManager::instance()->bind(newPwdLabel, DFontSizeManager::T6, QFont::Normal);
    DPasswordEdit *newPwdEdit = new DPasswordEdit(widget);
    newPwdEdit->setPlaceholderText(tr("必填"));
    setPasswordEditAttribute(newPwdEdit);
    layout->addWidget(newPwdLabel);
    layout->addSpacing(4);
    layout->addWidget(newPwdEdit);
    layout->addSpacing(10);

    DLabel *repeatPwdLabel = new DLabel(widget);
    repeatPwdLabel->setText(tr("重复PIN："));
    repeatPwdLabel->setContentsMargins(10, 0, 0, 0);
    DFontSizeManager::instance()->bind(repeatPwdLabel, DFontSizeManager::T6, QFont::Normal);
    DPasswordEdit *repeatPwdEdit = new DPasswordEdit(widget);
    repeatPwdEdit->setPlaceholderText(tr("必填"));
    setPasswordEditAttribute(repeatPwdEdit);
    layout->addWidget(repeatPwdLabel);
    layout->addSpacing(4);
    layout->addWidget(repeatPwdEdit);

    layout->setContentsMargins(10, 10, 10, 0);
    widget->setLayout(layout);
    widget->setMinimumHeight(600);

    m_changePinDialog->addContent(widget);
    m_changePinDialog->setOnButtonClickedClose(false);
    m_changePinDialog->setFixedSize(400, 604);

    connect(m_changePinDialog, &DDialog::buttonClicked, this, [this, oldPwdEdit, newPwdEdit, repeatPwdEdit](int index, const QString &text) {
        if (index == 0) {
            m_changePinDialog->hide();
            oldPwdEdit->clear();
            newPwdEdit->clear();
            repeatPwdEdit->clear();
        } else {
            if (!judgePinSize(oldPwdEdit) || !judgePinSize(newPwdEdit) || !judgePinSize(repeatPwdEdit) 
                || !judgePinConsistent(newPwdEdit, repeatPwdEdit)) {
                return;
            }
            Q_EMIT requestSetPin(oldPwdEdit->text(), newPwdEdit->text());
            m_changePinDialog->hide();
            oldPwdEdit->clear();
            newPwdEdit->clear();
            repeatPwdEdit->clear();
        }
    });
    connect(m_changePinDialog, &DDialog::closed, this, [this, oldPwdEdit, newPwdEdit, repeatPwdEdit] {
        m_changePinDialog->hide();
        oldPwdEdit->clear();
        newPwdEdit->clear();
        repeatPwdEdit->clear();
    });
    connect(oldPwdEdit, &DPasswordEdit::textChanged, this, [oldPwdEdit](const QString &text){
        if (oldPwdEdit->isAlert()) {
            oldPwdEdit->hideAlertMessage();
            oldPwdEdit->setAlert(false);
        }
    });
    connect(newPwdEdit, &DPasswordEdit::textChanged, this, [newPwdEdit](const QString &text){
        if (newPwdEdit->isAlert()) {
            newPwdEdit->hideAlertMessage();
            newPwdEdit->setAlert(false);
        }
    });
    connect(repeatPwdEdit, &DPasswordEdit::textChanged, this, [repeatPwdEdit](const QString &text){
        if (repeatPwdEdit->isAlert()) {
            repeatPwdEdit->hideAlertMessage();
            repeatPwdEdit->setAlert(false);
        }
    });
}

void KeyPinDialogCtrl::showFailedDialog()
{
    if (!m_failedDialog) {
        initFailedDialogUI();
    }
    m_failedDialog->setWindowFlags(Qt::WindowStaysOnTopHint);
    m_failedDialog->setWindowFlag(Qt::WindowMinimizeButtonHint, false);
    m_failedDialog->setAttribute(Qt::WA_ShowModal, true);
    m_failedDialog->show();
    m_failedDialog->activateWindow();
}

void KeyPinDialogCtrl::initFailedDialogUI()
{
    m_failedDialog = new DDialog();
    m_failedDialog->setIcon(DStyle().standardIcon(DStyle::SP_MessageBoxWarning));
    m_failedDialog->addButton(tr("Cancel"), false, DDialog::ButtonNormal);
    connect(m_failedDialog, &DDialog::buttonClicked, this, [this](int index, const QString &text){
        m_failedDialog->hide();
    });
    connect(m_failedDialog, &DDialog::closed, this, [this] {
        m_failedDialog->hide();
    });

    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);

    DLabel *picLabel = new DLabel(widget);
    picLabel->setPixmap(DHiDPIHelper::loadNxPixmap(UnknownPixmapPath));
    picLabel->setFixedHeight(180);
    picLabel->setAlignment(Qt::AlignHCenter);
    layout->addWidget(picLabel);
    layout->addSpacing(40);

    DLabel *tipLabel = new DLabel(widget);
    tipLabel->setText(tr("操作失败！"));
    tipLabel->setAlignment(Qt::AlignHCenter);
    DFontSizeManager::instance()->bind(tipLabel, DFontSizeManager::T6, QFont::Normal);
    layout->addWidget(tipLabel);

    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(10, 20, 10, 0);
    widget->setMinimumHeight(400);

    m_failedDialog->addContent(widget);
    m_failedDialog->setOnButtonClickedClose(false);
    m_failedDialog->setFixedSize(400, 500);
}

void KeyPinDialogCtrl::setPasswordEditAttribute(DPasswordEdit *edit)
{
    edit->setAttribute(Qt::WA_InputMethodEnabled, false);
    edit->lineEdit()->setValidator(new QRegExpValidator(QRegExp("[^\\x4e00-\\x9fa5]+")));
    edit->setCopyEnabled(false);
    edit->setCutEnabled(false);
}

bool KeyPinDialogCtrl::judgePinSize(DPasswordEdit *edit)
{
    bool result = false;
    const QString &pwd = edit->text();
    if (pwd.isEmpty()) {
        edit->setAlert(true);
        edit->showAlertMessage(tr("PIN 不能为空"), edit, 2000);
    } else if (pwd.size() < 4) {
        edit->setAlert(true);
        edit->showAlertMessage(tr("PIN 长度不能少于4个字符"), edit, 2000);
    } else if (pwd.size() > 63) {
        edit->setAlert(true);
        edit->showAlertMessage(tr("PIN 长度不能超过63个字符"), edit, 2000);
    } else {
        result = true;
    }
    return result;
}

bool KeyPinDialogCtrl::judgePinConsistent(DPasswordEdit *targetEdit, DPasswordEdit *sourceEdit)
{
    bool result = false;
    const QString &target = targetEdit->text();
    const QString &source = sourceEdit->text();
    if (target.compare(source) == 0) {
        result = true;
    } else {
        sourceEdit->setAlert(true);
        sourceEdit->showAlertMessage(tr("PIN 不一致"), sourceEdit, 2000);
    }
    return result;
}

void KeyPinDialogCtrl::hideAllDialog()
{
    if (m_setPinDialog) {
        m_setPinDialog->hide();
    }
    if (m_changePinDialog) {
        m_changePinDialog->hide();
    }
    if (m_failedDialog) {
        m_failedDialog->hide();
    }
}

