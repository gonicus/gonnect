pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window
import QtQuick.Controls.Material
import base

BaseDialog {
    id: control

    signal accepted

    property alias text: contentLabel.text

    Label {
        id: contentLabel
        elide: Label.ElideRight
        wrapMode: Label.WordWrap
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: okButton.top
            margins: 20
        }
    }

    Button {
        id: okButton
        text: qsTr("Ok")
        highlighted: true
        anchors {
            bottom: parent.bottom
            right: parent.right
            bottomMargin: 5
            rightMargin: 10
        }

        onClicked: () => {
            control.accepted()
            control.close()
        }
    }
}
