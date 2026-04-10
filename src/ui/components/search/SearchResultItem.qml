pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control
    implicitHeight: delegateColumn.implicitHeight

    default property alias content: delegateColumn.children

    readonly property alias mainRowLeftInsetLoader: mainRowLeftInsetLoader
    readonly property alias mainRowRightInsetLoader: mainRowRightInsetLoader
    property alias mainText: mainLabel.text
    property alias secondaryText: secondaryLabel.text
    property bool highlighted
    property bool canBeHighlighted: true

    signal manuallyHovered
    signal triggerPrimaryAction
    signal triggerSecondaryAction

    Column {
        id: delegateColumn
        topPadding: 12
        bottomPadding: 12
        spacing: 4
        anchors {
            left: parent.left
            right: parent.right
        }

        Accessible.role: Accessible.Column
        Accessible.name: qsTr("Search result")
        Accessible.description: qsTr("Currently selected search result")
        Accessible.focusable: true
        Accessible.onPressAction: () => control.triggerPrimaryAction()

        Rectangle {
            id: mainRow
            height: 50
            radius: 2
            color: control.canBeHighlighted && control.highlighted ? Theme.highlightColor : 'transparent'
            anchors {
                left: parent.left
                right: parent.right
            }

            states: [
                State {
                    when: secondaryLabel.text !== ''

                    PropertyChanges {
                        secondaryLabel.visible: true
                    }

                    AnchorChanges {
                        target: mainLabel
                        anchors {
                            verticalCenter: undefined
                            bottom: mainRow.verticalCenter
                        }
                    }
                }
            ]

            Loader {
                id: mainRowLeftInsetLoader
                width: 24
                height: 24
                anchors {
                    left: parent.left
                    leftMargin: 13
                    verticalCenter: parent.verticalCenter
                }
            }

            Label {
                id: mainLabel
                elide: Label.ElideRight
                font.weight: Font.Medium
                anchors {
                    left: mainRowLeftInsetLoader.item ? mainRowLeftInsetLoader.right : parent.left
                    right: mainRowRightInsetLoader.item ? mainRowRightInsetLoader.left : parent.right
                    leftMargin: 10
                    rightMargin: 10
                    verticalCenter: parent.verticalCenter
                }

                Accessible.role: Accessible.StaticText
                Accessible.name: mainLabel.text
            }

            Loader {
                id: mainRowRightInsetLoader
                width: 24
                height: 24
                anchors {
                    right: parent.left
                    rightMargin: 13
                    verticalCenter: parent.verticalCenter
                }
            }

            Label {
                id: secondaryLabel
                elide: Label.ElideRight
                font.pixelSize: mainLabel.font.pixelSize - 2
                color: Theme.secondaryTextColor
                visible: false
                anchors {
                    top: mainLabel.bottom
                    left: mainLabel.left
                    right: mainLabel.right
                }

                Accessible.role: Accessible.StaticText
                Accessible.name: secondaryLabel.text
            }

            HoverHandler {
                id: avatarNameHoverHandler
                cursorShape: Qt.PointingHandCursor
                onHoveredChanged: () => {
                    if (avatarNameHoverHandler.hovered) {
                        control.manuallyHovered()
                    }
                }
            }

            TapHandler {
                grabPermissions: PointerHandler.TakeOverForbidden
                gesturePolicy: TapHandler.WithinBounds
                exclusiveSignals: TapHandler.SingleTap
                acceptedButtons: Qt.LeftButton | Qt.RightButton

                onTapped: (_, mouseButton) => {
                    if (mouseButton === Qt.RightButton) {
                        control.triggerSecondaryAction()
                    } else {
                        control.triggerPrimaryAction()
                    }
                }
            }
        }
    }
}
