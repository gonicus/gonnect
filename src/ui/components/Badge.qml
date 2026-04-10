import QtQuick
import base

Item {
    id: control
    implicitWidth: lbl.implicitWidth + 2 * 8
    implicitHeight: lbl.implicitHeight + 2 * 4

    property alias color: lbl.color
    property alias text: lbl.text

    Accessible.role: Accessible.Indicator
    Accessible.name: control.text

    Rectangle {
        anchors.fill: parent
        color: 'transparent'
        antialiasing: true
        radius: 4
        border.width: 1
        border.color: control.color

        Accessible.ignored: true
    }

    Label {
        id: lbl
        anchors.centerIn: parent
        font.capitalization: Font.AllUppercase
        font.pixelSize: 9

        Accessible.ignored: true
    }
}
