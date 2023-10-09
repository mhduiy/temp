// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "resetkeydialogctrl.h"
#include "../../module/common.h"

#include <QVBoxLayout>
#include <QPixmap>
#include <QObject>
#include <QFontMetrics>
#include <QCloseEvent>
#include <QPushButton>

#include <DFontSizeManager>
#include <DHiDPIHelper>

const QString DialogTitle = QObject::tr("重置密钥");

const QString ResetTip1 = QObject::tr("吊销当前系统所生成证书");
const QString ResetTip2 = QObject::tr("抹除所有用户存储的证书");
const QString ResetTip3 = QObject::tr("删除生物认证的凭据数据");
const QString ResetTip4 = QObject::tr("重设安全密钥的部分特性和设置");

const QString CancelBtnText = QObject::tr("取 消");
const QString ResetBtnText = QObject::tr("重 置");
const QString OkBtnText = QObject::tr("完 成");

const QString FirstTouchTip = QObject::tr("请在10秒内，触摸或轻扫设备两次");
const QString SecondTouchTip = QObject::tr("请再次触摸或轻扫设备");

const QString ResetSuccessTip = QObject::tr("重置密钥成功!");
const QString ResetFailedTip = QObject::tr("重置密钥失败");

const QIcon ResetSuccessIcon = DStyle().standardIcon(DStyle::SP_DialogYesButton);
const QIcon ResetFailedIcon = QIcon::fromTheme("dialog-error");

ResetKeyDialogCtrl::ResetKeyDialogCtrl(QObject *parent)
    : QObject(parent)
    , m_descriptionDialog(nullptr)
    , m_insertDialog(nullptr)
    , m_identifyingDialog(nullptr)
    , m_firstTouchDialog(nullptr)
    , m_secondTouchDialog(nullptr)
    , m_failedDialog(nullptr)
    , m_resultDialog(nullptr)
    , m_resetTimer(new QTimer())
    , m_resetTimerCount(5)
    , m_resultPicLabel(nullptr)
    , m_resultTipLabel(nullptr)
    , m_identifyingSpinner(nullptr)
{
    m_resetTimer->setInterval(1000);
    m_resetTimer->setSingleShot(false);
    m_resetTimer->stop();
}

ResetKeyDialogCtrl::~ResetKeyDialogCtrl()
{
    if (m_descriptionDialog) {
        m_descriptionDialog->deleteLater();
        m_descriptionDialog = nullptr;
    }
    if (m_insertDialog) {
        m_insertDialog->deleteLater();
        m_insertDialog = nullptr;
    }
    if (m_identifyingDialog) {
        m_identifyingDialog->deleteLater();
        m_identifyingDialog = nullptr;
    }
    if (m_firstTouchDialog) {
        m_firstTouchDialog->deleteLater();
        m_firstTouchDialog = nullptr;
    }
    if (m_secondTouchDialog) {
        m_secondTouchDialog->deleteLater();
        m_secondTouchDialog = nullptr;
    }
    if (m_failedDialog) {
        m_failedDialog->deleteLater();
        m_failedDialog = nullptr;
    }
    if (m_resultDialog) {
        m_resultDialog->deleteLater();
        m_resultDialog = nullptr;
    }
    if (m_resetTimer) {
        m_resetTimer->stop();
        m_resetTimer->deleteLater();
        m_resetTimer = nullptr;
    }
}

void ResetKeyDialogCtrl::showDescriptionDialog()
{
    if (!m_descriptionDialog) {
        initDescriptionDialogUI();
    }
    initResetTimer();
    hideAllDialog();
    m_descriptionDialog->getButton(1)->setText(ResetBtnText + "(" + QString::number(m_resetTimerCount) + "s)");
    m_descriptionDialog->getButton(1)->setEnabled(false);

    m_descriptionDialog->setWindowFlags(Qt::WindowStaysOnTopHint);
    m_descriptionDialog->setWindowFlag(Qt::WindowMinimizeButtonHint, false);
    m_descriptionDialog->setAttribute(Qt::WA_ShowModal, true);
    m_descriptionDialog->show();
    m_descriptionDialog->activateWindow();
    m_resetTimer->start();
}

void ResetKeyDialogCtrl::initDescriptionDialogUI()
{
    m_descriptionDialog = new DDialog();
    m_descriptionDialog->setTitle(DialogTitle);
    m_descriptionDialog->setIcon(DStyle().standardIcon(DStyle::SP_MessageBoxWarning));
    m_descriptionDialog->addButton(CancelBtnText, false, DDialog::ButtonNormal);
    m_descriptionDialog->addButton(ResetBtnText + QString("(5s)"), false, DDialog::ButtonWarning);
    m_descriptionDialog->getButton(1)->setEnabled(false);
    connect(m_descriptionDialog, &DDialog::buttonClicked, this, [this](int index, const QString &text){
        if (index == 1) {
            Q_EMIT resetBtnClicked();
        } else {
            Q_EMIT requestStopReset(false);
            m_resetTimer->stop();
            m_descriptionDialog->hide();
        }
    });
    connect(m_descriptionDialog, &DDialog::closed, this, [this] {
        Q_EMIT requestStopReset(false);
        m_resetTimer->stop();
        m_descriptionDialog->hide();
    });

    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);

    DLabel *picLabel = new DLabel(widget);
    picLabel->setText(tr("重置安全密钥包含但不限以下处理，您确定要重置密钥嘛？"));
    picLabel->setAlignment(Qt::AlignCenter);
    picLabel->setWordWrap(true);
    DFontSizeManager::instance()->bind(picLabel, DFontSizeManager::T6, QFont::Normal);
    layout->addWidget(picLabel, 0, Qt::AlignHCenter);
    layout->addSpacing(50);

    QString info;
    info.append("•" + ResetTip1 + "\n");
    info.append("•" + ResetTip2 + "\n");
    info.append("•" + ResetTip3 + "\n");
    info.append("•" + ResetTip4 + "\n");
    DLabel *tipLabel = new DLabel(widget);
    tipLabel->setText(info);
    tipLabel->setAlignment(Qt::AlignLeft);
    DFontSizeManager::instance()->bind(tipLabel, DFontSizeManager::T6, QFont::Medium);
    layout->addWidget(tipLabel, 1, Qt::AlignHCenter);

    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(40, 20, 40, 0);
    widget->setMinimumHeight(400);

    m_descriptionDialog->addContent(widget);

    m_descriptionDialog->setFixedSize(410, 500);

    connect(m_resetTimer, &QTimer::timeout, this, [this] {
        --m_resetTimerCount;
        if (m_resetTimerCount == 0) {
            initResetTimer();
            m_descriptionDialog->getButton(1)->setText(ResetBtnText);
            m_descriptionDialog->getButton(1)->setEnabled(true);
        } else {
            m_descriptionDialog->getButton(1)->setText(ResetBtnText + "(" + QString::number(m_resetTimerCount) + "s)");
        }
    });
}

void ResetKeyDialogCtrl::showInsertDeviceDialog()
{
    if (!m_insertDialog) {
        initInsertDialogUI();
    }

    initResetTimer();
    hideAllDialog();
    m_insertDialog->setWindowFlags(Qt::WindowStaysOnTopHint);
    m_insertDialog->setWindowFlag(Qt::WindowMinimizeButtonHint, false);
    m_insertDialog->setAttribute(Qt::WA_ShowModal, true);
    m_insertDialog->show();
    m_insertDialog->activateWindow();
}

void ResetKeyDialogCtrl::initInsertDialogUI()
{
    m_insertDialog = new DDialog();
    m_insertDialog->setTitle(DialogTitle);
    m_insertDialog->setIcon(DStyle().standardIcon(DStyle::SP_MessageBoxWarning));
    m_insertDialog->addButton(CancelBtnText, false, DDialog::ButtonNormal);
    connect(m_insertDialog, &DDialog::buttonClicked, this, [this](int index, const QString &text){
        Q_EMIT requestStopReset(false);
        m_insertDialog->hide();
    });
    connect(m_insertDialog, &DDialog::closed, this, [this] {
        Q_EMIT requestStopReset(false);
        m_insertDialog->hide();
    });

    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);

    DLabel *picLabel = new DLabel(widget);
    picLabel->setPixmap(DHiDPIHelper::loadNxPixmap(InsertPixmapPath));
    picLabel->setFixedHeight(180);
    picLabel->setAlignment(Qt::AlignHCenter);
    layout->addWidget(picLabel);
    layout->addSpacing(40);

    DLabel *tipLabel = new DLabel(widget);
    tipLabel->setText(tr("请重新插入安全密钥"));
    tipLabel->setAlignment(Qt::AlignHCenter);
    DFontSizeManager::instance()->bind(tipLabel, DFontSizeManager::T6, QFont::Normal);
    layout->addWidget(tipLabel);

    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(10, 20, 10, 0);
    widget->setMinimumHeight(400);

    m_insertDialog->addContent(widget);
    m_insertDialog->setFixedSize(410, 500);
}

void ResetKeyDialogCtrl::showIdentifyingDeviceDialog()
{
    if (!m_identifyingDialog) {
        initIdentifyingDeviceUI();
    }

    initResetTimer();
    hideAllDialog();
    m_identifyingDialog->setWindowFlags(Qt::WindowStaysOnTopHint);
    m_identifyingDialog->setWindowFlag(Qt::WindowMinimizeButtonHint, false);
    m_identifyingDialog->setAttribute(Qt::WA_ShowModal, true);
    m_identifyingDialog->show();
    m_identifyingDialog->activateWindow();
}

void ResetKeyDialogCtrl::initIdentifyingDeviceUI()
{
    m_identifyingDialog = new DDialog();
    m_identifyingDialog->setTitle(DialogTitle);
    m_identifyingDialog->setIcon(DStyle().standardIcon(DStyle::SP_MessageBoxWarning));
    m_identifyingDialog->addButton(CancelBtnText, false, DDialog::ButtonNormal);
    connect(m_identifyingDialog, &DDialog::buttonClicked, this, [this](int index, const QString &text){
        Q_EMIT requestStopReset(false);
        m_identifyingDialog->hide();
    });
    connect(m_identifyingDialog, &DDialog::closed, this, [this] {
        Q_EMIT requestStopReset(false);
        m_identifyingDialog->hide();
    });

    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);

    DLabel *picLabel = new DLabel(widget);
    picLabel->setPixmap(DHiDPIHelper::loadNxPixmap(InsertPixmapPath));
    picLabel->setFixedHeight(180);
    picLabel->setAlignment(Qt::AlignHCenter);
    layout->addWidget(picLabel);
    layout->addSpacing(40);

    DLabel *tipLabel = new DLabel(widget);
    tipLabel->setText(tr("正在识别安全密钥"));
    tipLabel->setAlignment(Qt::AlignHCenter);
    tipLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    DFontSizeManager::instance()->bind(tipLabel, DFontSizeManager::T6, QFont::Normal);
    DSpinner *spinner = new DSpinner(widget);
    spinner->setVisible(true);
    spinner->setMinimumSize(22, 22);
    spinner->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(m_identifyingDialog, &DDialog::visibleChanged, this, [spinner](bool visible) {
        visible ? spinner->start() : spinner->stop();
    });
    QHBoxLayout *tipLayout = new QHBoxLayout();
    tipLayout->setMargin(0);
    tipLayout->setSpacing(10);
    tipLayout->addStretch(1);
    tipLayout->addWidget(spinner, 0);
    tipLayout->addWidget(tipLabel, 0);
    tipLayout->addStretch(1);
    layout->addLayout(tipLayout);
    layout->addStretch(1);

    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(10, 20, 10, 0);
    widget->setMinimumHeight(400);

    m_identifyingDialog->addContent(widget);
    m_identifyingDialog->setFixedSize(410, 500);
}

void ResetKeyDialogCtrl::showFirstTouchDeviceDialog()
{
    if (!m_firstTouchDialog) {
        initFirstTouchDialogUI();
    }

    initResetTimer();
    hideAllDialog();
    m_firstTouchDialog->setWindowFlags(Qt::WindowStaysOnTopHint);
    m_firstTouchDialog->setWindowFlag(Qt::WindowMinimizeButtonHint, false);
    m_firstTouchDialog->setAttribute(Qt::WA_ShowModal, true);
    m_firstTouchDialog->show();
    m_firstTouchDialog->activateWindow();
}

void ResetKeyDialogCtrl::initFirstTouchDialogUI()
{
    m_firstTouchDialog = new DDialog();
    m_firstTouchDialog->setTitle(DialogTitle);
    m_firstTouchDialog->setIcon(DStyle().standardIcon(DStyle::SP_MessageBoxWarning));
    m_firstTouchDialog->addButton(CancelBtnText, false, DDialog::ButtonNormal);
    connect(m_firstTouchDialog, &DDialog::buttonClicked, this, [this](int index, const QString &text){
        Q_EMIT requestStopReset(false);
        m_firstTouchDialog->hide();
    });
    connect(m_firstTouchDialog, &DDialog::closed, this, [this] {
        Q_EMIT requestStopReset(false);
        m_firstTouchDialog->hide();
    });

    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);

    DLabel *picLabel = new DLabel(widget);
    picLabel->setPixmap(DHiDPIHelper::loadNxPixmap(TouchPixmapPath));
    picLabel->setFixedHeight(180);
    picLabel->setAlignment(Qt::AlignHCenter);
    layout->addWidget(picLabel);
    layout->addSpacing(40);

    DLabel *tipLabel = new DLabel(widget);
    tipLabel->setText(FirstTouchTip);
    tipLabel->setAlignment(Qt::AlignHCenter);
    DFontSizeManager::instance()->bind(tipLabel, DFontSizeManager::T6, QFont::Normal);
    layout->addWidget(tipLabel);

    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(10, 20, 10, 0);
    widget->setMinimumHeight(400);

    m_firstTouchDialog->addContent(widget);
    m_firstTouchDialog->setFixedSize(410, 500);
}

void ResetKeyDialogCtrl::showSecondTouchDeviceDialog()
{
    if (!m_secondTouchDialog) {
        initSecondTouchDialogUI();
    }

    initResetTimer();
    hideAllDialog();
    m_secondTouchDialog->setWindowFlags(Qt::WindowStaysOnTopHint);
    m_secondTouchDialog->setWindowFlag(Qt::WindowMinimizeButtonHint, false);
    m_secondTouchDialog->setAttribute(Qt::WA_ShowModal, true);
    m_secondTouchDialog->show();
    m_secondTouchDialog->activateWindow();
}

void ResetKeyDialogCtrl::initSecondTouchDialogUI()
{
    m_secondTouchDialog = new DDialog();
    m_secondTouchDialog->setTitle(DialogTitle);
    m_secondTouchDialog->setIcon(DStyle().standardIcon(DStyle::SP_MessageBoxWarning));
    m_secondTouchDialog->addButton(CancelBtnText, false, DDialog::ButtonNormal);
    connect(m_secondTouchDialog, &DDialog::buttonClicked, this, [this](int index, const QString &text){
        Q_EMIT requestStopReset(false);
        m_secondTouchDialog->hide();
    });
    connect(m_secondTouchDialog, &DDialog::closed, this, [this] {
        Q_EMIT requestStopReset(false);
        m_secondTouchDialog->hide();
    });

    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);

    DLabel *picLabel = new DLabel(widget);
    picLabel->setPixmap(DHiDPIHelper::loadNxPixmap(TouchPixmapPath));
    picLabel->setFixedHeight(180);
    picLabel->setAlignment(Qt::AlignHCenter);
    layout->addWidget(picLabel);
    layout->addSpacing(40);

    DLabel *tipLabel = new DLabel(widget);
    tipLabel->setText(SecondTouchTip);
    tipLabel->setAlignment(Qt::AlignHCenter);
    DFontSizeManager::instance()->bind(tipLabel, DFontSizeManager::T6, QFont::Normal);
    layout->addWidget(tipLabel);

    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(10, 20, 10, 0);
    widget->setMinimumHeight(400);

    m_secondTouchDialog->addContent(widget);
    m_secondTouchDialog->setFixedSize(410, 500);
}

void ResetKeyDialogCtrl::showFailedDialog()
{
    if (!m_failedDialog) {
        initFailedDialogUI();
    }

    initResetTimer();
    hideAllDialog();
    m_failedDialog->setAttribute(Qt::WA_ShowModal, true);
    m_failedDialog->setWindowFlag(Qt::WindowMinimizeButtonHint, false);
    m_failedDialog->show();
    m_failedDialog->activateWindow();
}

void ResetKeyDialogCtrl::initFailedDialogUI()
{
    m_failedDialog = new DDialog();
    m_failedDialog->setTitle(DialogTitle);
    m_failedDialog->setIcon(DStyle().standardIcon(DStyle::SP_MessageBoxWarning));
    m_failedDialog->addButton(CancelBtnText, false, DDialog::ButtonNormal);
    connect(m_failedDialog, &DDialog::buttonClicked, this, [this](int index, const QString &text){
        Q_EMIT requestStopReset(false);
        m_failedDialog->hide();
    });
    connect(m_failedDialog, &DDialog::closed, this, [this] {
        Q_EMIT requestStopReset(false);
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
    tipLabel->setText(tr("发生错误，重置失败！"));
    tipLabel->setAlignment(Qt::AlignHCenter);
    DFontSizeManager::instance()->bind(tipLabel, DFontSizeManager::T6, QFont::Normal);
    layout->addWidget(tipLabel);

    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(10, 20, 10, 0);
    widget->setMinimumHeight(400);

    m_failedDialog->addContent(widget);
    m_failedDialog->setFixedSize(410, 500);
}

void ResetKeyDialogCtrl::showResultDialog(bool success)
{
    if (!m_resultDialog) {
        initResultDialogUI();
    }

    initResetTimer();
    hideAllDialog();

    if (success) {
        m_resultPicLabel->setPixmap(ResetSuccessIcon.pixmap(128, 128));
        m_resultTipLabel->setText(ResetSuccessTip);
    } else {
        m_resultPicLabel->setPixmap(ResetFailedIcon.pixmap(128, 128));
        m_resultTipLabel->setText(ResetFailedTip);
    }
    m_resultDialog->setWindowFlags(Qt::WindowStaysOnTopHint);
    m_resultDialog->setWindowFlag(Qt::WindowMinimizeButtonHint, false);
    m_resultDialog->setAttribute(Qt::WA_ShowModal, true);
    m_resultDialog->show();
    m_resultDialog->activateWindow();
}

void ResetKeyDialogCtrl::initResultDialogUI()
{
    m_resultDialog = new DDialog();
    m_resultDialog->setTitle(DialogTitle);
    m_resultDialog->setIcon(DStyle().standardIcon(DStyle::SP_MessageBoxWarning));
    m_resultDialog->addButton(OkBtnText, false, DDialog::ButtonNormal);
    connect(m_resultDialog, &DDialog::buttonClicked, this, [this](int index, const QString &text){
        m_resultDialog->hide();
        if (m_resultTipLabel && m_resultTipLabel->text().contains(ResetSuccessTip)) {
            Q_EMIT requestStopReset(true);
        } else {
            Q_EMIT requestStopReset(false);
        }
    });
    connect(m_resultDialog, &DDialog::closed, this, [this] {
        m_resultDialog->hide();
        if (m_resultTipLabel && m_resultTipLabel->text().contains(ResetSuccessTip)) {
            Q_EMIT requestStopReset(true);
        } else {
            Q_EMIT requestStopReset(false);
        }
    });

    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);

    DLabel *picLabel = new DLabel(widget);
    m_resultPicLabel = picLabel;
    picLabel->setPixmap(ResetFailedIcon.pixmap(128, 128));
    picLabel->setFixedHeight(128);
    picLabel->setAlignment(Qt::AlignHCenter);
    layout->addWidget(picLabel);
    layout->addSpacing(30);

    DLabel *tipLabel = new DLabel(widget);
    m_resultTipLabel = tipLabel;
    tipLabel->setAlignment(Qt::AlignHCenter);
    DFontSizeManager::instance()->bind(tipLabel, DFontSizeManager::T6, QFont::Normal);
    layout->addWidget(tipLabel);

    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(10, 45, 10, 0);
    widget->setMinimumHeight(400);

    m_resultDialog->addContent(widget);
    m_resultDialog->setFixedSize(410, 500);
}

void ResetKeyDialogCtrl::hideAllDialog()
{
    if (m_descriptionDialog) {
        m_descriptionDialog->hide();
    }
    if (m_insertDialog) {
        m_insertDialog->hide();
    }
    if (m_identifyingDialog) {
        m_identifyingDialog->hide();
    }
    if (m_firstTouchDialog) {
        m_firstTouchDialog->hide();
    }
    if (m_secondTouchDialog) {
        m_secondTouchDialog->hide();
    }
    if (m_failedDialog) {
        m_failedDialog->hide();
    }
    if (m_resultDialog) {
        m_resultDialog->hide();
    }
}

void ResetKeyDialogCtrl::initResetTimer()
{
    m_resetTimer->stop();
    m_resetTimerCount = 5;
}
