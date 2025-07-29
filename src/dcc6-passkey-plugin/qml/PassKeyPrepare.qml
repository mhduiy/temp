// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

import org.deepin.dcc
import org.deepin.dtk as D
import org.deepin.dtk.style as DS
import org.deepin.dcc.passkey 1.0

DccObject {
    id: root

    property var parentView

    name: "prepare"
    parentName: "passkey"
    pageType: DccObject.Item
    page: Flickable {
        contentHeight: groupView.height + 10
        ScrollBar.vertical: ScrollBar {
            width: 10
        }

        DccGroupView {
            id: groupView
            isGroup: false
            height: implicitHeight + 10
            anchors {
                left: parent.left
                right: parent.right
            }
        }

        Component.onCompleted: {
            root.parentView = this
        }
    }

    DccObject {
        id: preparePage
        parentName: "passkey/prepare"
        weight: 10
        pageType: DccObject.Item
        /**
         * 0: 插入设备
         * 1: 未添加
         * 2: 未注册
         * 3: 正在识别
         * 4: 触摸验证
         */

         /*
            Insert = 0,     // 插入设备
            Identifying,    // 正在识别，目前暂不使用，因为识别很快，保留
            Touch,          // 触摸验证设备
            Timeout,        // 验证超时
            Unregistered,   // 未注册
            Unknown,        // 未知错误
         */
        property int pageIndex: {
            switch (dccData.model.promptType) {
                case Common.Insert:
                    return 0;
                case Common.Unregistered:
                    return 1;
                case Common.Unknown:
                    return 3;
                case Common.Identifying:
                    return 3;
                case Common.Touch:
                    return 4;
                default:
                    return 2;
            }
        }
        page: Item {
            implicitHeight: root.parentView ? root.parentView.height : 350
            StackLayout {
                anchors.fill: parent
                currentIndex: preparePage.pageIndex

                onWidthChanged: {
                    console.log("width", width)
                }

                Item {
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 10

                        Image {
                            fillMode: Image.Pad
                            Layout.alignment: Qt.AlignHCenter
                            clip: true
                            width: 180
                            height: 180
                            source: "qrc:/icons/deepin/builtin/texts/page_insert_180px.svg"
                        }

                        Label {
                            text: qsTr("请插入安全密钥")
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }

                Item {
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 10

                        Image {
                            fillMode: Image.Pad
                            Layout.alignment: Qt.AlignHCenter
                            clip: true
                            width: 180
                            height: 180
                            source: "qrc:/icons/deepin/builtin/texts/page_unregistered_180px.svg"
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("识别到未添加的安全密钥")
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Label {
                            text: qsTr("添加后可用于登陆验证，或跳过直接管理安全密钥")
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Item {
                            Layout.preferredHeight: 10
                        }

                        Button {
                            text: qsTr("添加密钥")
                            Layout.alignment: Qt.AlignHCenter
                            onClicked: {
                                dccData.worker.handlePromptOperate(Common.Unregistered);
                            }
                        }

                        Button {
                            text: qsTr("跳过")
                            Layout.alignment: Qt.AlignHCenter
                            background: null
                        }
                    }
                }

                Item {
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 10

                        Image {
                            fillMode: Image.Pad
                            Layout.alignment: Qt.AlignHCenter
                            clip: true
                            width: 180
                            height: 180
                            source: "qrc:/icons/deepin/builtin/texts/page_unknown_180px.svg"
                        }

                        Label {
                            text: qsTr("识别到未注册的安全密钥")
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Item {
                            Layout.preferredHeight: 10
                        }

                        Button {
                            text: qsTr("去注册")
                            Layout.alignment: Qt.AlignHCenter
                            onClicked: {
                                dccData.worker.handlePromptOperate(Common.Unregistered);
                            }
                        }
                    }   
                }

                Item {
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 10

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
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }   
                }

                Item {
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 10

                        Image {
                            fillMode: Image.Pad
                            Layout.alignment: Qt.AlignCenter
                            clip: true
                            width: 180
                            height: 180
                            source: "qrc:/icons/deepin/builtin/texts/page_touch_180px.svg"
                        }

                        Label {
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("请验证安全密钥，触摸或轻扫设备")
                            font.bold: true
                        }
                    }
                }
            }
        }
    }
}
