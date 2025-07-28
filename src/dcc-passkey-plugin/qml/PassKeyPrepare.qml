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

        // 0. 插入设备 1. 未添加 2. 未注册 3. 正在识别 4. 触摸验证
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
                            text: qsTr("Please plug in the security key")
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
                            text: qsTr("Security keys that have not been added")
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                        }
    
                        Label {
                            text: qsTr("After adding, it can be used for login verification or skip directly managing security keys")
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Item {
                            Layout.preferredHeight: 10
                        }

                        Button {
                            text: qsTr("Register")
                            Layout.alignment: Qt.AlignHCenter
                            onClicked: {
                                dccData.worker.handlePromptOperate(Common.Unregistered);
                            }
                        }

                        // TODO: 跳过按钮，暂时不使用
                        // Button {
                        //     text: qsTr("Skip")
                        //     Layout.alignment: Qt.AlignHCenter
                        //     background: null
                        // }
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
                            text: qsTr("Unregistered security key identified")
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Item {
                            Layout.preferredHeight: 10
                        }

                        Button {
                            text: qsTr("Register")
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
                                text: qsTr("Identifying the security key")
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
                            text: qsTr("Touch or swipe the security key")
                            font.bold: true
                        }
                    }
                }
            }
        }
    }
}
