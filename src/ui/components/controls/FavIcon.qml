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

    Accessible.role: Accessible.Button
    Accessible.name: control.isFavorite ? qsTr("Set favorite") : ("Unset favorite")
    Accessible.focusable: true
    Accessible.onPressAction: () => control.toggled()

    Rectangle {
        anchors.fill: parent
        radius: 4
        color: favHoverHandler.hovered ? Theme.backgroundOffsetHoveredColor : 'transparent'

        Accessible.ignored: true
    }

    Label {
        id: favoriteIcon
        anchors.centerIn: parent
        text: control.isFavorite ? "★" : "☆"
        font.pixelSize: 20

        Accessible.ignored: true
    }

    TapHandler {
        grabPermissions: PointerHandler.TakeOverForbidden
        gesturePolicy: TapHandler.WithinBounds
        onTapped: () => control.toggled()
    }

    HoverHandler {
        id: favHoverHandler
        blocking: true
    }
}
