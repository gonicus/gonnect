pragma ComponentBehavior: Bound

import QtQuick
import base


Item {
    id: control
    height: 20
    width: 20

    property bool isFavorite
    property alias iconVisible: favoriteIcon.visible

    signal toggled

    Rectangle {
        anchors.fill: parent
        radius: 4
        color: favHoverHandler.hovered ? Theme.backgroundOffsetHoveredColor : 'transparent'
    }

    Label {
        id: favoriteIcon
        anchors.centerIn: parent
        text: control.isFavorite ? "★" : "☆"
        font.pixelSize: 20
    }

    TapHandler {
        grabPermissions: PointerHandler.TakeOverForbidden
        gesturePolicy: TapHandler.WithinBounds
        onTapped: () => control.toggled()
    }

    HoverHandler {
        id: favHoverHandler
        enabled: !delg.isAnonymous
        blocking: true
    }
}
