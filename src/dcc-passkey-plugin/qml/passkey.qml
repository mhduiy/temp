// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls

import org.deepin.dcc
import org.deepin.dtk as D

DccObject {
    id: passkey
    name: "passkey"
    parentName: "accountsloginMethod"
    displayName: qsTr("Security Key")
    weight: 50
    visible: !config.dccPasskeyPluginHideStatus

    page: Control {
        id: control
        width: parent ? parent.width : 10
        height: parent ? parent.height : 10
        contentItem: dccObj.children.length > 0 ? dccObj.children[0].getSectionItem(control) : null
    }

    D.Config {
        id: config
        name: "org.deepin.dde.control-center.passkey"
        property bool dccPasskeyPluginHideStatus : true
    }
}
