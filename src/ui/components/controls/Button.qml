import QtQuick
import QtQuick.Controls.Material as M
import base

M.Button {
    id: control
    icon.width: 16
    icon.height: 16
    font.pixelSize: Theme.fontSizeNormal

    M.Material.elevation: 0
    M.Material.roundedScale: control.materialRadius

    readonly property int materialRadius: 6
}
