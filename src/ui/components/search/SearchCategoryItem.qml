import QtQuick
import QtQuick.Controls.impl
import base

Item {
    id: control
    height: buttonLabel.implicitHeight + Theme.d
    anchors {
        left: parent?.left
        right: parent?.right
    }

    signal clicked

    property alias name: buttonLabel.text
    property bool highlighted

    Accessible.role: Accessible.ColumnHeader
    Accessible.name: qsTr("Search result category filter %1").arg(control.name)
    Accessible.description: qsTr("Filter for the individual search result items by category")
    Accessible.focusable: true
    Accessible.onPressAction: () => control.selectCategory()

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

        Accessible.ignored: true
    }

    IconLabel {
        id: checkIconLabel
        width: 16
        icon.source: control.highlighted ? Icons.dataSuccess : Icons.dataError
        anchors {
            left: parent.left
            leftMargin: Theme.d
            verticalCenter: parent.verticalCenter
        }
    }

    Label {
        id: buttonLabel
        font.weight: Font.Medium
        maximumLineCount: 2
        elide: Label.ElideRight
        wrapMode: Label.WordWrap
        anchors {
            verticalCenter: parent.verticalCenter
            left: checkIconLabel.right
            right: parent.right
            leftMargin: 6
            rightMargin: Theme.d
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
