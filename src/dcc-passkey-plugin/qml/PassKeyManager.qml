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
    id: root

    name: "manager"
    parentName: "passkey"
    pageType: DccObject.Item
    page: DccRightView {}

    DccTitleObject {
        name: "passkeyManagerTitle"
        parentName: "passkey/manager"
        displayName: qsTr("Key Management")
        weight: 10
    }

    DccObject {
        name: "passkeyManagerPin"
        parentName: "passkey/manager"
        displayName: qsTr("PIN")
        weight: 20
        pageType: DccObject.Editor
        backgroundType: DccObject.Normal
        page: D.Button {
            text: dccData.model.existPin ? qsTr("Change") : qsTr("Setting")
            onClicked: {
                setPinDialogLoader.active = true
            }
        }
    }

    Loader {
        id: setPinDialogLoader
        active: false
        sourceComponent: SetPinDialog {
            onClosing: {
                console.warn("SetPinDialog ===== onClosing", oldPin, newPin);
                if (oldPin.length > 0 && newPin.length > 0) {
                    dccData.worker.handleSetPasskeyPin(oldPin, newPin)
                }
                setPinDialogLoader.active = false
            }
        }
        onLoaded: {
            item.show()
        }
    }

    DccObject {
        name: "passkeyManagerChange"
        parentName: "passkey/manager"
        displayName: qsTr("Reset Security Key")
        weight: 30
        pageType: DccObject.Editor
        backgroundType: DccObject.Normal
        page: D.Button {
            text: qsTr("Reset")
            onClicked: {
                dccData.model.resetDialogStyle = Common.DescriptionStyle
                resetPasskeyDialogLoader.active = true
            }
        }
    }

    Loader {
        id: resetPasskeyDialogLoader
        active: false
        sourceComponent: ResetPasskeyDialog {
            onClosing: {
                resetPasskeyDialogLoader.active = false
            }
        }
        onLoaded: {
            item.show()
        }
    }

/*
    DccTitleObject {
        name: "passkeyManagerUseForTitle"
        parentName: "passkey/manager"
        displayName: qsTr("密钥使用于")
        weight: 40
    }

    DccObject {
        name: "passkeyManagerUseFor"
        parentName: "passkey/manager"
        displayName: qsTr("登录认证")
        weight: 50
        pageType: DccObject.Editor
        backgroundType: DccObject.Normal
        page: D.Switch {
            checked: true
        }
    }

    DccObject {
        name: "passkeyManagerReset"
        parentName: "passkey/manager"
        displayName: qsTr("鉴权认证")
        weight: 60
        pageType: DccObject.Editor
        backgroundType: DccObject.Normal
        page: D.Switch {
            checked: true
        }
    }
*/
}
