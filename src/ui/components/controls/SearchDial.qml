import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

/// Combined widget with contact search box, dial button and outgoing call type selector (inkognito, etc.)
Item {
    id: control
    height: 60

    property int radius: 6

    signal numberSelected(string number, string contactId, string preferredIdentity)
    signal escapePressed()

    function activate() {
        searchBox.activate()
    }

    Rectangle {
        id: leftWidgetBackground
        width: 140
        color: leftHoverHandler.hovered ? Theme.backgroundOffsetHoveredColor : Theme.backgroundOffsetColor
        topLeftRadius: control.radius
        bottomLeftRadius: control.radius
        anchors {
            top: parent.top
            bottom: parent.bottom
            left: parent.left
        }

        Behavior on color { ColorAnimation {} }
    }

    Rectangle {
        id: rightWidgetBackground
        width: control.height
        color: rightHoverHandler.hovered ? Theme.backgroundOffsetHoveredColor : Theme.backgroundOffsetColor
        topRightRadius: control.radius
        bottomRightRadius: control.radius
        anchors {
            top: parent.top
            bottom: parent.bottom
            right: parent.right
        }

        Behavior on color { ColorAnimation {} }

        HoverHandler {
            id: rightHoverHandler
        }

        TapHandler {
            onTapped: () => control.numberSelected(searchBox.text, "", modeSelector.currentValue)
        }
    }

    Rectangle {
        id: background
        color: 'transparent'
        radius: control.radius
        border.width: 1
        border.color: Theme.borderColor
        anchors.fill: parent
    }

    IconLabel {
        id: searchIcon
        icon {
            source: Icons.systemSearch
            width: 20
            height: 20
        }
        anchors {
            verticalCenter: parent.verticalCenter
            left: leftWidgetBackground.right
            leftMargin: 20
        }
    }

    IconLabel {
        id: phoneIcon
        anchors.centerIn: rightWidgetBackground
        icon {
            source: Icons.callStart
            width: 25
            height: 25
        }
    }

    Item {
        id: dummyItemForSearchBoxTapHandler
        anchors {
            top: parent.top
            bottom: parent.bottom
            left: leftWidgetBackground.right
            right: rightWidgetBackground.left
        }

        TapHandler {
            onTapped: () => searchBox.activate()
        }
    }

    Label {
        id: placeholderLabel
        text: qsTr("Number or contact")
        color: Theme.secondaryTextColor
        visible: !searchBox.text
        font.pixelSize: 18
        anchors {
            left: searchBox.left
            right: searchBox.left
            verticalCenter: searchBox.verticalCenter
        }
    }

    SearchBox {
        id: searchBox
        anchors {
            verticalCenter: parent.verticalCenter
            left: searchIcon.right
            leftMargin: 20
            right: clearButtonContainer.left
        }

        onNumberSelected: (number, contactId) => control.numberSelected(number, contactId, modeSelector.currentValue)
        onEscapePressed: () => control.escapePressed()
    }

    Item {
        id: clearButtonContainer
        width: 60
        anchors {
            top: parent.top
            bottom: parent.bottom
            right: phoneIcon.left
            rightMargin: 20
        }

        IconLabel {
            id: clearButton
            visible: searchBox.text !== ""
            icon {
                source: Icons.editClear
                width: 20
                height: 20
                color: clearButtonHoveredHandler.hovered ? Theme.primaryTextColor : Theme.secondaryTextColor
            }
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
                rightMargin: 20
            }

            Behavior on color { ColorAnimation {} }
        }

        HoverHandler {
            id: clearButtonHoveredHandler
        }

        TapHandler {
            onTapped: () => searchBox.clear()
        }
    }

    ComboBox {
        id: modeSelector
        flat: true
        editable: false
        anchors.fill: leftWidgetBackground
        background: Item {}
        textRole: 'displayName'
        valueRole: 'id'
        popup.width: control.width
        model: [
            {
                displayName: qsTr("Default"),
                id: "default"
            }, {
                displayName: qsTr("Auto"),
                id: "auto"
            }
        ].concat(SIPManager.preferredIdentities)

        contentItem: Label {
            text: modeSelector.displayText
            wrapMode: Label.WordWrap
            font.pixelSize: 14
            maximumLineCount: 2
            elide: Label.ElideRight
            verticalAlignment: Label.AlignVCenter
            leftPadding: 10
        }

        function setDefaultIdentity() {
            const selectedId = SIPManager.defaultPreferredIdentity
            const model = modeSelector.model

            for (let i = 0; i < model.length; ++i) {
                if (model[i].id === selectedId) {
                    modeSelector.currentIndex = i
                    return
                }
            }

            modeSelector.currentIndex = -1
        }

        Component.onCompleted: () => modeSelector.setDefaultIdentity()

        Connections {
            target: SIPManager
            function onPreferredIdentitiesChanged() {
                modeSelector.setDefaultIdentity()
            }
        }

        HoverHandler {
            id: leftHoverHandler
        }
    }
}
