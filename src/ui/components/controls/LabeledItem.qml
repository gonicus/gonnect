pragma ComponentBehavior: Bound

import QtQuick
import base

Column {
    id: control
    spacing: Theme.d / 2

    Accessible.role: Accessible.Column
    Accessible.name: itemLabel.text
    Accessible.description: control.description

    property alias text: itemLabel.text
    property string description

    Label {
        id: itemLabel
        anchors {
            left: parent.left
            right: parent.right
        }

        Accessible.ignored: true
    }
}
