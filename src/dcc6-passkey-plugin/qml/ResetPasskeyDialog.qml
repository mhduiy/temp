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
                currentIndex: 0

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

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
                                close()
                            }
                        }

                        D.Button {
                            text: qsTr("重置")
                            Layout.fillWidth: true
                            onClicked: {
                                stackLayout.currentIndex = 1
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

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.bottomMargin: 10
                        D.Button {
                            text: qsTr("取消")
                            Layout.fillWidth: true
                            onClicked: {
                                // close()
                                stackLayout.currentIndex = 2
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
                                stackLayout.currentIndex = 3
                                // close()
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 30

                    D.DciIcon {
                        name: "sp_ok"
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
                            text: qsTr("取消")
                            Layout.fillWidth: true
                            onClicked: {
                                close()
                            }
                        }
                    }
                }
            }
        }
    }
}
