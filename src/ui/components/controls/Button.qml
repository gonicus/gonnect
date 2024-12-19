import QtQuick
import QtQuick.Controls.Material as M

M.Button {
    id: control
    icon.width: 16
    icon.height: 16

    M.Material.elevation: 0
    M.Material.roundedScale: control.materialRadius

    readonly property int materialRadius: 6
}
