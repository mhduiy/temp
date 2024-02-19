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

ResetKeyDialogCtrl::ResetKeyDialogCtrl(QWidget *parent, QObject *obj)
    : QObject(obj)
    , m_parentWidget(parent)
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
    m_descriptionDialog->getButton(1)->setText(tr("Reset") + "(" + QString::number(m_resetTimerCount) + "s)");
    m_descriptionDialog->getButton(1)->setEnabled(false);

    m_descriptionDialog->setModal(true);
    m_descriptionDialog->show();

    m_resetTimer->start();
}

void ResetKeyDialogCtrl::initDescriptionDialogUI()
{
    m_descriptionDialog = new DDialog(m_parentWidget);
    m_descriptionDialog->setTitle(tr("Reset Security Key"));
    m_descriptionDialog->setIcon(DStyle().standardIcon(DStyle::SP_MessageBoxWarning));
    m_descriptionDialog->addButton(tr("Cancel"), false, DDialog::ButtonNormal);
    m_descriptionDialog->addButton(tr("Reset") + QString("(5s)"), false, DDialog::ButtonWarning);
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

    DLabel *tipLabel = new DLabel(widget);
    QString tip = tr("Resetting the security key includes, but is not limited to, the following processes: Revoking the current system-generated certificate, erasing all stored certificates, deleting biometric authentication credentials record data, and resetting some of the features and settings of the security key.");
    tipLabel->setText(tip);
    tipLabel->setAlignment(Qt::AlignCenter);
    tipLabel->setWordWrap(true);
    DFontSizeManager::instance()->bind(tipLabel, DFontSizeManager::T6, QFont::Medium);
 
    layout->addStretch();
    layout->addWidget(tipLabel, 0, Qt::AlignCenter);
    layout->addStretch();
    layout->setContentsMargins(20, 0, 20, 0);
    widget->setMinimumHeight(150);

    m_descriptionDialog->addContent(widget);
    m_descriptionDialog->setFixedWidth(410);

    connect(m_resetTimer, &QTimer::timeout, this, [this] {
        --m_resetTimerCount;
        if (m_resetTimerCount == 0) {
            initResetTimer();
            m_descriptionDialog->getButton(1)->setText(tr("Reset"));
            m_descriptionDialog->getButton(1)->setEnabled(true);
        } else {
            m_descriptionDialog->getButton(1)->setText(tr("Reset") + "(" + QString::number(m_resetTimerCount) + "s)");
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

    m_insertDialog->setModal(true);
    m_insertDialog->show();
}

void ResetKeyDialogCtrl::initInsertDialogUI()
{
    m_insertDialog = new DDialog(m_parentWidget);
    m_insertDialog->setTitle(tr("Reset Security Key"));
    m_insertDialog->setIcon(DStyle().standardIcon(DStyle::SP_MessageBoxWarning));
    m_insertDialog->addButton(tr("Cancel"), false, DDialog::ButtonNormal);
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
    tipLabel->setText(tr("Please plug in the security key again"));
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

    m_identifyingDialog->setModal(true);
    m_identifyingDialog->show();
}

void ResetKeyDialogCtrl::initIdentifyingDeviceUI()
{
    m_identifyingDialog = new DDialog(m_parentWidget);
    m_identifyingDialog->setTitle(tr("Reset Security Key"));
    m_identifyingDialog->setIcon(DStyle().standardIcon(DStyle::SP_MessageBoxWarning));
    m_identifyingDialog->addButton(tr("Cancel"), false, DDialog::ButtonNormal);
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
    tipLabel->setText(tr("Identifying the security key"));
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

    m_firstTouchDialog->setModal(true);
    m_firstTouchDialog->show();
}

void ResetKeyDialogCtrl::initFirstTouchDialogUI()
{
    m_firstTouchDialog = new DDialog(m_parentWidget);
    m_firstTouchDialog->setTitle(tr("Reset Security Key"));
    m_firstTouchDialog->setIcon(DStyle().standardIcon(DStyle::SP_MessageBoxWarning));
    m_firstTouchDialog->addButton(tr("Cancel"), false, DDialog::ButtonNormal);
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
    tipLabel->setText(tr("Please touch or swipe the security key twice within 10 seconds"));
    tipLabel->setAlignment(Qt::AlignHCenter);
    tipLabel->setWordWrap(true);
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

    m_secondTouchDialog->setModal(true);
    m_secondTouchDialog->show();
}

void ResetKeyDialogCtrl::initSecondTouchDialogUI()
{
    m_secondTouchDialog = new DDialog(m_parentWidget);
    m_secondTouchDialog->setTitle(tr("Reset Security Key"));
    m_secondTouchDialog->setIcon(DStyle().standardIcon(DStyle::SP_MessageBoxWarning));
    m_secondTouchDialog->addButton(tr("Cancel"), false, DDialog::ButtonNormal);
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
    tipLabel->setText(tr("Please touch or swipe the security key again"));
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

    m_failedDialog->setModal(true);
    m_failedDialog->show();
}

void ResetKeyDialogCtrl::initFailedDialogUI()
{
    m_failedDialog = new DDialog(m_parentWidget);
    m_failedDialog->setTitle(tr("Reset Security Key"));
    m_failedDialog->setIcon(DStyle().standardIcon(DStyle::SP_MessageBoxWarning));
    m_failedDialog->addButton(tr("Cancel"), false, DDialog::ButtonNormal);
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
    tipLabel->setText(tr("Unable to complete the security key reset"));
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
        m_resultPicLabel->setPixmap(QIcon::fromTheme("icon_success").pixmap(128, 128));
        m_resultTipLabel->setText(tr("Reset complete"));
    } else {
        m_resultPicLabel->setPixmap(QIcon::fromTheme("icon_fail").pixmap(128, 128));
        m_resultTipLabel->setText(tr("Unable to complete the security key reset"));
    }

    m_resultDialog->setModal(true);
    m_resultDialog->show();
}

void ResetKeyDialogCtrl::initResultDialogUI()
{
    m_resultDialog = new DDialog(m_parentWidget);
    m_resultDialog->setTitle(tr("Reset Security Key"));
    m_resultDialog->setIcon(DStyle().standardIcon(DStyle::SP_MessageBoxWarning));
    m_resultDialog->addButton(tr("Done"), false, DDialog::ButtonNormal);
    connect(m_resultDialog, &DDialog::buttonClicked, this, [this](int index, const QString &text){
        m_resultDialog->hide();
        if (m_resultTipLabel && m_resultTipLabel->text().contains(tr("Reset complete"))) {
            Q_EMIT requestStopReset(true);
        } else {
            Q_EMIT requestStopReset(false);
        }
    });
    connect(m_resultDialog, &DDialog::closed, this, [this] {
        m_resultDialog->hide();
        if (m_resultTipLabel && m_resultTipLabel->text().contains(tr("Reset complete"))) {
            Q_EMIT requestStopReset(true);
        } else {
            Q_EMIT requestStopReset(false);
        }
    });

    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);

    DLabel *picLabel = new DLabel(widget);
    m_resultPicLabel = picLabel;
    picLabel->setPixmap(QIcon::fromTheme("icon_fail").pixmap(128, 128));
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
