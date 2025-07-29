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
import org.deepin.dcc.passkey 1.0

D.DialogWindow {
    id: dialog
    icon: "preferences-system"
    modality: Qt.WindowModal
    width: 360

    Item {
        implicitWidth: parent.width
        implicitHeight: 450 - DS.Style.dialogWindow.titleBarHeight - DS.Style.dialogWindow.contentHMargin

        ColumnLayout {
            anchors.fill: parent

            D.Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: qsTr("重置安全密钥")
            }

            StackLayout {
                id: stackLayout
                Layout.fillWidth: true
                Layout.fillHeight: true

                /*
                0. 提示
                1. 重新插入安全密钥
                2. 正在识别
                3. 触摸两次提示
                4. 触摸第二次
                5. 成功
                6. 失败
                */

                /*
                    DescriptionStyle = 0,
                    InsertStyle,
                    IdentifyingStyle,
                    FirstTouchStyle,
                    SecondTouchStyle,
                    ResultSuccessStyle,
                    ResultFailedStyle
                */

                currentIndex: {
                    switch (dccData.model.resetDialogStyle) {
                        case Common.DescriptionStyle:
                            return 0
                        case Common.InsertStyle:
                            return 1
                        case Common.IdentifyingStyle:
                            return 2
                        case Common.FirstTouchStyle:
                            return 3
                        case Common.SecondTouchStyle:
                            return 4
                        case Common.ResultSuccessStyle:
                            return 5
                        case Common.ResultFailedStyle:
                            return 6
                        default:
                            return 6
                    }
                }

                ColumnLayout {

                    D.Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: qsTr("重置安全密钥包含但不仅限以下处理，您确认要重置密钥吗？")
                    }

                    Item {
                        Layout.preferredHeight: 40
                    }

                    D.Label {
                        Layout.alignment: Qt.AlignHCenter
                        wrapMode: Text.WordWrap
                        font.bold: true
                        text: "•吊销当前系统所生成证书\n•抹除所有户存储的证书\n•删除生物认证的凭证数据\n•重设安全密钥的部分特性和设置"
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
                                dccData.worker.deactivate()
                                close()
                            }
                        }

                        D.Button {
                            id: resetButton
                            // enabled: false
                            Layout.fillWidth: true
                            text: qsTr("重置") + "(5)"
                            onClicked: {
                                dccData.worker.requestReset()
                            }

                            Timer {
                                interval: 1000
                                property int count: 4
                                repeat: true
                                running: true
                                onTriggered: {
                                    if (count <= 0) {
                                        resetButton.enabled = true
                                        resetButton.text = qsTr("重置")
                                        running = false
                                    } else {
                                        resetButton.text = qsTr("重置(%1)").arg(count)
                                        count--
                                    }
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 30

                    Image {
                        source: "qrc:/icons/deepin/builtin/texts/page_insert_180px.svg"
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 180
                        Layout.preferredHeight: 180
                    }

                    D.Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: qsTr("请重新插入安全密钥")
                    }

                    Item {
                        Layout.fillHeight: true
                    }

                        
                    D.Button {
                        text: qsTr("取消")
                        Layout.fillWidth: true
                        Layout.bottomMargin: 10
                        onClicked: {
                            dccData.worker.requestStopReset()
                            close()
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 30

                    Image {
                        fillMode: Image.Pad
                        Layout.alignment: Qt.AlignHCenter
                        clip: true
                        width: 180
                        height: 180
                        source: "qrc:/icons/deepin/builtin/texts/page_identify_180px.svg"
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 10
                        
                        D.BusyIndicator {
                            running: visible
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                        }

                        Label {
                            text: qsTr("正在识别安全密钥")
                            font.bold: true
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }

                    D.Button {
                        text: qsTr("取消")
                        Layout.fillWidth: true
                        Layout.bottomMargin: 10
                        onClicked: {
                            dccData.worker.requestStopReset()
                            close()
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 30

                    Image {
                        source: "qrc:/icons/deepin/builtin/texts/page_touch_180px.svg"
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 180
                        Layout.preferredHeight: 180
                    }

                    D.Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: qsTr("请在10秒内，触摸或轻扫设备两次")
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
                                dccData.worker.requestStopReset()
                                close()
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 30

                    Image {
                        source: "qrc:/icons/deepin/builtin/texts/page_touch_180px.svg"
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 180
                        Layout.preferredHeight: 180
                    }

                    D.Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: qsTr("请再次触摸或轻扫设备")
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
                                dccData.worker.requestStopReset()
                                close()
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 30

                    Item {
                        Layout.preferredHeight: 50
                    }

                    D.DciIcon {
                        name: "icon_success"
                        sourceSize: Qt.size(96, 96)
                        Layout.alignment: Qt.AlignHCenter
                    }

                    D.Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: qsTr("重置密钥成功")
                    }

                    Item {
                        Layout.fillHeight: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.bottomMargin: 10
                        D.Button {
                            text: qsTr("完成")
                            Layout.fillWidth: true
                            onClicked: {
                                dccData.worker.activate()
                                close()
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 30

                    Item {
                        Layout.preferredHeight: 50
                    }

                    D.DciIcon {
                        name: "icon_fail"
                        sourceSize: Qt.size(96, 96)
                        Layout.alignment: Qt.AlignHCenter
                    }

                    D.Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: qsTr("无法完成安全密钥重置")
                    }

                    Item {
                        Layout.fillHeight: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.bottomMargin: 10
                        D.Button {
                            text: qsTr("完成")
                            Layout.fillWidth: true
                            onClicked: {
                                dccData.worker.activate()
                                close()
                            }
                        }
                    }
                }
            }
        }
    }
}
