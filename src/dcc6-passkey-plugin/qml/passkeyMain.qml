// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Window 2.15
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.15

import org.deepin.dcc 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.dtk.style 1.0 as DS

import org.deepin.dcc.passkey 1.0

DccObject {
    PassKeyPrepare {
        visible: dccData.model.currentStage === Common.Prompt
    }

    PassKeyManager {
        visible: dccData.model.currentStage === Common.Manage
    }
}
