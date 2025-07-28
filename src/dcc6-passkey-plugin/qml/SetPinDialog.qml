// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import org.deepin.dtk 1.0 as D
import org.deepin.dtk.style 1.0 as DS
import org.deepin.dcc 1.0

D.DialogWindow {
    id: root

    width: 360
    icon: "preferences-system"
    modality: Qt.WindowModal

    ColumnLayout {
        width: parent.width

        D.Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: qsTr("设置安全密钥PIN")
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
            text: qsTr("旧的PIN：")
        }

        D.PasswordEdit {
            Layout.fillWidth: true
        }

        D.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("新的PIN：")
        }

        D.PasswordEdit {
            Layout.fillWidth: true
        }

        D.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("重复PIN：")
        }

        D.PasswordEdit {
            Layout.fillWidth: true
        }

        Item {
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: 10

            D.Button {
                text: qsTr("取消")
                Layout.fillWidth: true
                onClicked: {
                    close()
                }
            }

            D.Button {
                text: qsTr("更改")
                Layout.fillWidth: true
                onClicked: {
                    close()
                }
            }
        }
    }
}
