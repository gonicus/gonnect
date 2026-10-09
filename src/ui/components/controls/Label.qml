pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.Material
import base

T.Label {
    id: control
    font.pixelSize: Theme.fontSizeNormal
    color: control.enabled ? Theme.primaryTextColor : Theme.secondaryTextColor
    linkColor: Material.accentColor

    ToolTip.text: control.text
    ToolTip.visible: hoverHandlerLoader.item?.hovered ?? false

    property bool tooltipsEnabled: true

    Loader {
        id: hoverHandlerLoader
        active: control.truncated && control.tooltipsEnabled
        sourceComponent: HoverHandler {
            parent: control
        }
    }
}
