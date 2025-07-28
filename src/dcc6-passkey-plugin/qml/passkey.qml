import QtQuick
import QtQuick.Controls
import org.deepin.dcc 1.0

DccObject {
    id: passkey
    name: "passkey"
    // parentName: "accountsloginMethod"
    parentName: "root"
    displayName: qsTr("安全密钥")
    weight: 50

    page: Control {
        id: control
        width: parent ? parent.width : 10
        height: parent ? parent.height : 10
        contentItem: dccObj.children.length > 0 ? dccObj.children[0].getSectionItem(control) : null
    }
}
