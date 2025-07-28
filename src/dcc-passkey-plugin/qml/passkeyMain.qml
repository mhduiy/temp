// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

import org.deepin.dcc
import org.deepin.dtk as D
import org.deepin.dtk.style as DS
import org.deepin.dcc.passkey

DccObject {
    PassKeyPrepare {
        visible: dccData.model.currentStage === Common.Prompt
    }

    PassKeyManager {
        visible: dccData.model.currentStage === Common.Manage
    }
}
