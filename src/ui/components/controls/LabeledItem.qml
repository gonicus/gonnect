pragma ComponentBehavior: Bound

import QtQuick
import base

Column {
    id: control
    spacing: Theme.d / 2

    Accessible.role: Accessible.Grouping
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

        Accessible.labelFor: control.children.length > 1 ? control.children[1] : null
        Accessible.ignored: true
    }
}
