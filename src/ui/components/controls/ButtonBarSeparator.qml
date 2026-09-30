pragma ComponentBehavior: Bound

import QtQuick
import base

Rectangle {
    id: control
    width: 1
    color: Theme.borderColor
    anchors {
        top: parent?.top
        bottom: parent?.bottom
        topMargin: 3
        bottomMargin: 3
    }
}
