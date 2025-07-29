// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

import org.deepin.dcc 1.0
import org.deepin.dtk as D
import org.deepin.dtk.style as DS
import org.deepin.dcc.passkey 1.0

DccObject {
    id: root

    name: "manager"
    parentName: "passkey"
    pageType: DccSObject.Item
    page: DccRightView {}

    DccTitleObject {
        name: "passkeyManagerTitle"
        parentName: "passkey/manager"
        displayName: qsTr("密钥管理")
        weight: 10
    }

    DccObject {
        name: "passkeyManagerPin"
        parentName: "passkey/manager"
        displayName: qsTr("安全密钥PIN")
        weight: 20
        pageType: DccObject.Editor
        backgroundType: DccObject.Normal
        page: D.Button {
            text: dccData.model.existPin ? qsTr("更改") : qsTr("设置")
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
        displayName: qsTr("更改密钥")
        weight: 30
        pageType: DccObject.Editor
        backgroundType: DccObject.Normal
        page: D.Button {
            text: qsTr("重置")
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
}
