import QtQuick
import base

Item {
    id: control
    height: 30
    anchors {
        left: parent?.left
        right: parent?.right
    }

    signal clicked

    property alias name: buttonLabel.text
    property alias highlighted: highlightedBackground.visible

    Accessible.role: Accessible.ColumnHeader
    Accessible.name: qsTr("Search result category filter: ") + control.name
    Accessible.description: qsTr("Filter for the individual search result items by category")
    Accessible.focusable: true
    Accessible.onPressAction: () => control.selectCategory()

    Rectangle {
        id: highlightedBackground
        anchors.fill: parent
        visible: false
        color: Theme.backgroundSecondaryColor
        radius: 5
        border {
            width: 1
            color: Theme.borderColor
        }
    }

    Rectangle {
        id: hoveredBackground
        anchors.fill: parent
        visible: control.enabled && hoverHandler.hovered
        color: "transparent"
        radius: 5
        border {
            width: 1
            color: Theme.secondaryTextColor
        }
    }

    Label {
        id: buttonLabel
        font.weight: Font.Medium
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            right: parent.right
            leftMargin: 12
            rightMargin: 12
        }

        Accessible.ignored: true
    }

    HoverHandler {
        id: hoverHandler
    }

    TapHandler {
        onTapped: () => control.selectCategory()
    }

    function selectCategory() {
        if (control.enabled) {
            control.clicked()
        }
    }
}
