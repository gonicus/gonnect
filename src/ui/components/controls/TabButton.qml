pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls
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
    icon.color: !enabled ? Theme.secondaryInactiveTextColor : down || checked ? Theme.accentColor : Theme.primaryTextColor

    font.family: "Noto Sans"
    font.pixelSize: 16
    font.weight: Font.Medium

    property real topLeftRadius: 0
    property real topRightRadius: 0
    property real bottomLeftRadius: 0
    property real bottomRightRadius: 0

    contentItem: Label {
        text: control.text
        color: !control.enabled ? Theme.secondaryInactiveTextColor : control.down || control.checked ? Theme.accentColor : Theme.secondaryTextColor
        horizontalAlignment: Label.AlignHCenter
        elide: Label.ElideRight

        font.family: "Noto Sans"
        font.pixelSize: 16
        font.weight: Font.Medium
    }

    background: Rectangle {
        color: control.hovered ? Theme.backgroundOffsetHoveredColor : 'transparent'

        topLeftRadius: control.topLeftRadius
        topRightRadius: control.topRightRadius
        bottomLeftRadius: control.bottomLeftRadius
        bottomRightRadius: control.bottomRightRadius
    }
}
