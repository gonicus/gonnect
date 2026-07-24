pragma ComponentBehavior: Bound

import QtQuick
import base

Column {
    id: control
    spacing: Theme.d / 2

    Accessible.role: Accessible.Column
    Accessible.name: itemLabel.text

    property alias text: itemLabel.text

    Label {
        id: itemLabel
        text: qsTr('Color scheme')
        anchors {
            left: parent.left
            right: parent.right
        }

        Accessible.ignored: true
    }
}
