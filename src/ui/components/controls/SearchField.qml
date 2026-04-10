pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

Item {
    id: control
    implicitWidth: 500
    implicitHeight: 30

    property alias text: searchInputField.text

    function giveFocus() {
        searchInputField.forceActiveFocus()
    }

    Accessible.role: Accessible.Form
    Accessible.name: placeholderLabel.text

    states: [
        State {
            when: !control.activeFocus && searchInputField.text.trim() === ""
            PropertyChanges {
                placeholderLabel.visible: true
            }
        },
        State {
            when: searchInputField.text.trim() !== ""
            PropertyChanges {
                clearButton.visible: true
            }
        }
    ]

    Rectangle {
        id: background
        color: Theme.backgroundSecondaryColor
        radius: 6
        border.width: 1
        border.color: Theme.borderColor
        anchors.fill: parent
    }

    IconLabel {
        id: searchIcon
        icon {
            source: Icons.systemSearch
            width: 16
            height: 16
        }
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            leftMargin: 10
        }
    }

    Label {
        id: placeholderLabel
        text: qsTr("Search for contacts or room names...")
        color: Theme.secondaryTextColor
        visible: false
        elide: Text.ElideRight
        anchors {
            verticalCenter: parent.verticalCenter
            left: searchInputField.left
            right: searchInputField.right
            rightMargin: 10
        }

        Accessible.ignored: true
    }

    TextInput {
        id: searchInputField
        font.pixelSize: 14
        color: Theme.primaryTextColor
        anchors {
            verticalCenter: parent.verticalCenter
            left: searchIcon.right
            right: clearButton.visible ? clearButton.left : parent.right
            leftMargin: 10
            rightMargin: 10
        }

        Accessible.role: Accessible.EditableText
        Accessible.name: placeholderLabel.text
        Accessible.searchEdit: true
        Accessible.focusable: true
    }

    Item {
        id: clearButton
        visible: false
        width: 16 + 20
        anchors {
            top: parent.top
            bottom: parent.bottom
            right: parent.right
        }

        Accessible.role: Accessible.Button
        Accessible.name: qsTr("Clear search field")
        Accessible.focusable: true
        Accessible.onPressAction: () => searchInputField.clear()

        IconLabel {
            id: clearIcon
            anchors.centerIn: parent
            rotation: LayoutMirroring.enabled ? 180 : 0
            icon {
                source: Icons.editClear
                color: clearButtonHoverHandler.hovered ? Theme.primaryTextColor : Theme.secondaryInactiveTextColor
                width: 16
                height: 16
            }

            Accessible.ignored: true
        }

        HoverHandler {
            id: clearButtonHoverHandler
        }

        TapHandler {
            onTapped: () => searchInputField.clear()
        }
    }
}
