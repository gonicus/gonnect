import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control

    readonly property BaseWindow baseWindow: control.Window.window as BaseWindow
    readonly property real inset: control.baseWindow
                                  && control.baseWindow.useOwnDecoration
                                  && !control.baseWindow.isMaximized
                                  ? control.baseWindow.shadowMargin
                                  : 0

    Behavior on opacity { NumberAnimation { duration: 150 } }

    Rectangle {
        color: control.Material.backgroundDimColor
        radius: control.inset > 0 ? control.baseWindow.windowRadius : 0
        anchors {
            fill: parent
            margins: control.inset
        }
    }
}
