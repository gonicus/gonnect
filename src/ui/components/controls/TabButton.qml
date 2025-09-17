pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl
import base


T.TabButton {
    id: control
    height: 46

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 12
    spacing: 6

    icon.width: 24
    icon.height: 24
    icon.color: !enabled ? Material.hintTextColor : down || checked ? Material.accentColor : Material.foreground

    font.family: "Noto Sans"
    font.pixelSize: 16
    font.weight: Font.Medium

    property real topLeftRadius: 0
    property real topRightRadius: 0
    property real bottomLeftRadius: 0
    property real bottomRightRadius: 0

    contentItem: Label {
        text: control.text
        color: !control.enabled ? control.Material.hintTextColor : control.down || control.checked ? control.Material.accentColor : Theme.secondaryTextColor
        horizontalAlignment: Label.AlignHCenter
        elide: Label.ElideRight

        font.family: "Noto Sans"
        font.pixelSize: 16
        font.weight: Font.Medium
    }

    background: Rectangle {
        color: control.hovered ? control.Material.rippleColor : 'transparent'

        topLeftRadius: control.topLeftRadius
        topRightRadius: control.topRightRadius
        bottomLeftRadius: control.bottomLeftRadius
        bottomRightRadius: control.bottomRightRadius
    }
}
