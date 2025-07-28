// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import org.deepin.dtk as D
import org.deepin.dtk.style as DS
import org.deepin.dcc
import org.deepin.dcc.passkey

D.DialogWindow {
    id: root

    width: 360
    icon: "preferences-system"
    modality: Qt.WindowModal

    property string oldPin: ""
    property string newPin: ""

    ColumnLayout {
        width: parent.width

        D.Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: dccData.model.existPin ? qsTr("Change PIN") : qsTr("Set PIN")
        }

        Image {
            Layout.alignment: Qt.AlignCenter
            width: 180
            height: 180
            source: "qrc:/icons/deepin/builtin/texts/set_password_180px.svg"
        }

        D.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Old PIN")
            visible: dccData.model.existPin
        }

        D.PasswordEdit {
            id: oldPinEdit
            Layout.fillWidth: true
            visible: dccData.model.existPin
            alertDuration: 3000
            onEditingFinished: {
                oldPinEdit.showAlert = false
                oldPinEdit.alertText = ""
                if (text.length === 0) {
                    oldPinEdit.showAlert = true
                    oldPinEdit.alertText = qsTr("PIN cannot be empty")
                } else if (text.length < 4) {
                    oldPinEdit.showAlert = true
                    oldPinEdit.alertText = qsTr("PIN length must not be less than 4 characters")
                } else if (text.length > 63) {
                    oldPinEdit.showAlert = true
                    oldPinEdit.alertText = qsTr("PIN length cannot be more than 63 characters")
                }
            }
        }

        D.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("New PIN")
        }

        D.PasswordEdit {
            id: newPinEdit
            Layout.fillWidth: true
            alertDuration: 3000

            onEditingFinished: {
                newPinEdit.showAlert = false
                newPinEdit.alertText = ""
                if (text.length === 0) {
                    newPinEdit.showAlert = true
                    newPinEdit.alertText = qsTr("PIN cannot be empty")
                } else if (text.length < 4) {
                    newPinEdit.showAlert = true
                    newPinEdit.alertText = qsTr("PIN length must not be less than 4 characters")
                } else if (text.length > 63) {
                    newPinEdit.showAlert = true
                    newPinEdit.alertText = qsTr("PIN length cannot be more than 63 characters")
                }
            }
        }

        D.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Repeat PIN")
        }

        D.PasswordEdit {
            id: repeatPinEdit
            Layout.fillWidth: true
            alertDuration: 3000
            onTextChanged: {
                if (newPinEdit.text !== repeatPinEdit.text) {
                    repeatPinEdit.showAlert = true
                    repeatPinEdit.alertText = qsTr("PIN inconsistency")
                } else {
                    repeatPinEdit.showAlert = false
                    repeatPinEdit.alertText = ""
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: 10

            D.Button {
                text: qsTr("Cancel")
                Layout.fillWidth: true
                onClicked: {
                    root.oldPin = ""
                    root.newPin = ""
                    close()
                }
            }

            D.Button {
                text: dccData.model.existPin ? qsTr("Change") : qsTr("Set")
                Layout.fillWidth: true
                onClicked: {
                    if (dccData.model.existPin && oldPinEdit.text.length === 0) {
                        return;
                    }

                    if (newPinEdit.text.length === 0 || repeatPinEdit.text.length === 0) {
                        return;
                    }

                    if (newPinEdit.text !== repeatPinEdit.text) {
                        return;
                    }

                    root.oldPin = oldPinEdit.text
                    root.newPin = newPinEdit.text
                    close()
                }
            }
        }
    }
}
