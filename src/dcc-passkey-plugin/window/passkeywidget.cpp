// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "passkeywidget.h"
#include "widgets/keypindialogctrl.h"
#include "../module/passkeymodel.h"
#include "../module/passkeyworker.h"

#include <QPixmap>

#include <DLabel>

PasskeyWidget::PasskeyWidget(QWidget *parent)
    : DStackedWidget(parent)
    , m_model(nullptr)
    , m_worker(nullptr)
    , m_promptPage(nullptr)
    , m_managePage(nullptr)
    , m_setPinDialogCtrl(nullptr)
    , m_resetKeyDialogCtrl(nullptr)
    , m_promptMonitorTimer(new QTimer(this))
    , m_resetPasskeyMonitorTimer(new QTimer(this))
{
    initUI();
}

PasskeyWidget::~PasskeyWidget()
{
    if (m_setPinDialogCtrl) {
        m_setPinDialogCtrl->deleteLater();
        m_setPinDialogCtrl = nullptr;
    }
    if (m_resetKeyDialogCtrl) {
        m_resetKeyDialogCtrl->deleteLater();
        m_resetKeyDialogCtrl = nullptr;
    }
    if (m_promptMonitorTimer) {
        m_promptMonitorTimer->stop();
        m_promptMonitorTimer->deleteLater();
        m_promptMonitorTimer = nullptr;
    }
    if (m_resetPasskeyMonitorTimer) {
        m_resetPasskeyMonitorTimer->stop();
        m_resetPasskeyMonitorTimer->deleteLater();
        m_resetPasskeyMonitorTimer = nullptr;
    }
}

void PasskeyWidget::setModel(const PasskeyModel *model, const PasskeyWorker *work)
{
    m_model = const_cast<PasskeyModel *>(model);
    connect (m_model, &PasskeyModel::refreshUI, this, &PasskeyWidget::refreshUI);
    connect (m_model, &PasskeyModel::refreshResetDialogStyle, this, &PasskeyWidget::refreshResetDialogStyle);
    connect (m_model, &PasskeyModel::refreshSetPinDialogStyle, this, &PasskeyWidget::refreshSetPinDialogStyle);

    m_worker = const_cast<PasskeyWorker *>(work);
    connect(this, &PasskeyWidget::requestPromptOperate, m_worker, &PasskeyWorker::handlePromptOperate);
    connect(this, &PasskeyWidget::requestJump, m_worker, &PasskeyWorker::handleJump);
    connect(m_worker, &PasskeyWorker::resetPasskeyMonitorCompleted, this, [this] {
        m_resetPasskeyMonitorTimer->stop();
    });
    connect(this, &PasskeyWidget::requestSetKeyPin, m_worker, &PasskeyWorker::handleSetPasskeyPin);
    connect(this, &PasskeyWidget::destroyed, m_worker, &PasskeyWorker::deactivate);

    m_promptMonitorTimer->setInterval(1000);
    m_promptMonitorTimer->setSingleShot(false);
    connect(m_promptMonitorTimer, &QTimer::timeout, m_worker, &PasskeyWorker::handlePromptMonitor);
    m_promptMonitorTimer->start();

    m_resetPasskeyMonitorTimer->setInterval(1000);
    m_resetPasskeyMonitorTimer->setSingleShot(false);
    connect(m_resetPasskeyMonitorTimer, &QTimer::timeout, m_worker, &PasskeyWorker::handleResetPasskeyMonitor);
    m_resetPasskeyMonitorTimer->stop();
}

void PasskeyWidget::refreshUI()
{
    // 设置对应页面
    const StackedPageIndex &pageIndex = m_model->currentPage();
    if (pageIndex < 0 || pageIndex >= count()) {
        return;
    }
    setCurrentIndex(pageIndex);

    if (m_resetPasskeyMonitorTimer->isActive()) {
        m_resetPasskeyMonitorTimer->stop();
    }

    // 更新页面信息
    if (pageIndex == StackedPageIndex::Prompt) {
        if (!m_promptMonitorTimer->isActive()) {
            m_promptMonitorTimer->start();
        }
        const QPair<PromptType, PromptInfo> &info = m_model->promptPageInfo();
        m_promptPage->setPixmap(info.second.iconPath);
        m_promptPage->setPromptInfo(info.second.promptMsg, info.second.tipMsg, info.second.needSpinner);
        m_promptPage->setOperateBtn(info.second.operateBtnText, info.second.needBtn, info.second.showLink);
    } else if (pageIndex == StackedPageIndex::Manage) {
        // 停止提示监测
        if (m_promptMonitorTimer->isActive()) {
            m_promptMonitorTimer->stop();
        }
        const ManageInfo &info = m_model->managePageInfo();
        m_managePage->setPinStatus(info.supportPin, info.existPin);
    }
}

void PasskeyWidget::refreshResetDialogStyle()
{
    if (!m_resetKeyDialogCtrl) {
        return;
    }

    const QPair<ResetDialogStyle, bool> &pair = m_model->resetDialogStyle();
    showResetKeyDialog(pair.first, pair.second);
}

void PasskeyWidget::refreshSetPinDialogStyle()
{
    if (!m_setPinDialogCtrl) {
        return;
    }

    showKeyPinDialog(m_model->setPinDialogStyle());
}

void PasskeyWidget::initUI()
{
    m_promptPage = new PromptWidget(this);
    connect(m_promptPage, &PromptWidget::operateBtnClicked, this, &PasskeyWidget::requestPromptOperate);
    connect(m_promptPage, &PromptWidget::jumpLinkBtnClicked, this, &PasskeyWidget::requestJump);
    addWidget(m_promptPage);

    m_managePage = new ManageWidget(this);
    connect(m_managePage, &ManageWidget::pinBtnClicked, this, [this] {
        if (!m_worker->existDevice()) {
            return;
        }
        const ManageInfo &info = m_model->managePageInfo();
        m_model->setSetPinDialogStyle(info.existPin ? SetPinDialogStyle::ChangePinStyle : SetPinDialogStyle::SetPinStyle);
        showKeyPinDialog(m_model->setPinDialogStyle());
    });
    connect(m_managePage, &ManageWidget::resetBtnClicked, this, [this] {
        if (!m_worker->existDevice()) {
            m_resetPasskeyMonitorTimer->stop();
            return;
        }
        showResetKeyDialog(ResetDialogStyle::DescriptionStyle, false);
    });
    addWidget(m_managePage);
}

void PasskeyWidget::showKeyPinDialog(const SetPinDialogStyle &style)
{
    if (!m_setPinDialogCtrl) {
        m_setPinDialogCtrl = new KeyPinDialogCtrl();
        connect(m_setPinDialogCtrl, &KeyPinDialogCtrl::requestSetPin, this, &PasskeyWidget::requestSetKeyPin);
    }

    switch (style)
    {
    case SetPinDialogStyle::SetPinStyle:
        m_setPinDialogCtrl->showSetPinDialog();
        break;
    case SetPinDialogStyle::ChangePinStyle:
        m_setPinDialogCtrl->showChangePinDialog();
        break;
    case SetPinDialogStyle::SetFailedStyle:
        m_setPinDialogCtrl->showFailedDialog();
        break;
    default:
        break;
    }
}

void PasskeyWidget::showResetKeyDialog(const ResetDialogStyle &style, bool status)
{
    if (!m_resetKeyDialogCtrl) {
        m_resetKeyDialogCtrl = new ResetKeyDialogCtrl();
        connect(m_resetKeyDialogCtrl, &ResetKeyDialogCtrl::resetBtnClicked, this, [this] {
            if (!m_worker->existDevice()) {
                m_resetPasskeyMonitorTimer->stop();
                m_resetKeyDialogCtrl->hideAllDialog();
                return;
            }
            m_resetPasskeyMonitorTimer->start();
            showResetKeyDialog(ResetDialogStyle::InsertStyle, false);
        });
        connect(m_resetKeyDialogCtrl, &ResetKeyDialogCtrl::requestStopReset, this, [this] (bool success) {
            m_resetPasskeyMonitorTimer->stop();
            success ? Q_EMIT m_worker->requestActive() : Q_EMIT m_worker->requestDeactivate();
        });
    }

    switch (style)
    {
    case ResetDialogStyle::DescriptionStyle:
        m_resetKeyDialogCtrl->showDescriptionDialog();
        break;
    case ResetDialogStyle::InsertStyle:
        m_resetKeyDialogCtrl->showInsertDeviceDialog();
        break;
    case ResetDialogStyle::IdentifyingStyle:
        m_resetKeyDialogCtrl->showIdentifyingDeviceDialog();
        break;
    case ResetDialogStyle::FirstTouchStyle:
        m_resetKeyDialogCtrl->showFirstTouchDeviceDialog();
        break;
    case ResetDialogStyle::SecondTouchStyle:
        m_resetKeyDialogCtrl->showSecondTouchDeviceDialog();
        break;
    case ResetDialogStyle::FailedStyle:
        m_resetKeyDialogCtrl->showFailedDialog();
        break;
    case ResetDialogStyle::ResultStyle:
        m_resetKeyDialogCtrl->showResultDialog(status);
        break;
    default:
        break;
    }
}

