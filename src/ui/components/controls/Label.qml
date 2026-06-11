import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.Material
import base

T.Label {
    id: control
    font.pixelSize: Theme.fontPixelSize
    color: control.enabled ? Theme.primaryTextColor : Theme.secondaryTextColor
    linkColor: Material.accentColor
}
