// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

import org.deepin.dcc
import org.deepin.dtk as D
import org.deepin.dtk.style as DS

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
        property int pageIndex: 4
        page: Item {
            implicitHeight: root.parentView ? root.parentView.height : 350

            StackLayout {
                anchors.fill: parent
                currentIndex: preparePage.pageIndex

                ColumnLayout {
                    anchors.centerIn: parent

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
                        Layout.alignment: Qt.AlignHCenter
                    }
                }

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
                        text: qsTr("识别到未添加的安全密钥")
                        font.bold: true
                        Layout.alignment: Qt.AlignCenter
                    }

                    Label {
                        text: qsTr("添加后可用于登陆验证，或跳过直接管理安全密钥")
                        Layout.alignment: Qt.AlignCenter
                    }

                    Item {
                        Layout.preferredHeight: 10
                    }

                    Button {
                        text: qsTr("添加密钥")
                        Layout.alignment: Qt.AlignCenter
                    }

                    Button {
                        text: qsTr("跳过")
                        Layout.alignment: Qt.AlignCenter
                        background: null
                    }
                }

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
                        Layout.alignment: Qt.AlignCenter
                    }

                    Item {
                        Layout.preferredHeight: 10
                    }

                    Button {
                        text: qsTr("去注册")
                        Layout.alignment: Qt.AlignCenter
                    }
                }

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
                        Layout.alignment: Qt.AlignCenter
                        spacing: 10
                        
                        D.BusyIndicator {
                            running: visible
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                        }

                        Label {
                            text: qsTr("正在识别安全密钥")
                            font.bold: true
                            Layout.alignment: Qt.AlignCenter
                        }
                    }
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 10

                    Image {
                        fillMode: Image.Pad
                        Layout.alignment: Qt.AlignHCenter
                        clip: true
                        width: 180
                        height: 180
                        source: "qrc:/icons/deepin/builtin/texts/page_touch_180px.svg"
                    }

                    Label {
                        text: qsTr("请在10秒内，触摸或轻扫设备两次")
                        font.bold: true
                        Layout.alignment: Qt.AlignCenter
                    }
                }
            }
        }
    }
}
