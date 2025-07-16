pragma ComponentBehavior: Bound

import QtQuick
import base

BaseWindow {
    id: control
    width: 510
    height: 354
    visible: true
    title: qsTr("Dialog")
    resizable: false

    minimumWidth: control.width
    minimumHeight: control.height
    maximumWidth: control.width
    maximumHeight: control.height
}
