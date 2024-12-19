pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import Qt5Compat.GraphicalEffects
import base

Window {
    id: control
    flags: Theme.useOwnDecoration ? Qt.FramelessWindowHint : Qt.Window
    color: Theme.useOwnDecoration ? 'transparent' : Theme.backgroundColor
    Material.accent: Theme.accentColor
    Material.theme: Theme.isDarkMode ? Material.Dark : Material.Light

    property bool resizable: false

    default property alias content: innerContainer.children

    Page {
        id: containerPage
        anchors.fill: parent
        padding: Theme.useOwnDecoration ? 55 : 0

        background: Item {
            visible: Theme.useOwnDecoration

            Rectangle {
                id: bgRect
                color: Theme.backgroundColor
                radius: 8
                anchors {
                    fill: parent
                    margins: 55
                }
            }

            DropShadow {
                anchors.fill: bgRect
                horizontalOffset: 0
                verticalOffset: 3
                radius: 8.0
                color: Theme.shadowColor
                source: bgRect
            }
        }

        contentItem: Item {
            id: outerContainer

            WindowHeader {
                id: windowHeader
                visible: Theme.useOwnDecoration
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                }
            }

            Item {
                id: innerContainer
                clip: true
                anchors {
                    top: Theme.useOwnDecoration ? windowHeader.bottom : parent.top
                    bottom: parent.bottom
                    left: parent.left
                    right: parent.right
                }
            }
        }

        Item {
            id: dragHandlerContainer
            enabled: Theme.useOwnDecoration && control.resizable
            anchors.fill: parent

            readonly property int borderWidth: 5

            /*
             * This group of items map the border of the window into 8 different items. Each has a HoverHandler for
             * the specific cursor shape and a DragHandler to start the resize action on the window.
             *
             * top left    |  top   |    top right
             * ------------+--------+-------------
             * left        |        |        right
             * ------------+--------+-------------
             * bottom left | bottom | bottom right
             */

            Item {
                id: topLeftBorder
                width: dragHandlerContainer.borderWidth
                height: dragHandlerContainer.borderWidth
                anchors {
                    top: parent.top
                    left: parent.left
                }

                HoverHandler {
                    cursorShape: Qt.SizeFDiagCursor
                }
                DragHandler {
                    target: null
                    dragThreshold: 0
                    onActiveChanged: () => {
                        if (active) {
                            control.startSystemResize(Qt.LeftEdge | Qt.TopEdge)
                        }
                    }
                }
            }

            Item {
                id: topBorder
                height: dragHandlerContainer.borderWidth
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                    leftMargin: dragHandlerContainer.borderWidth
                    rightMargin: dragHandlerContainer.borderWidth
                }

                HoverHandler {
                    cursorShape: Qt.SizeVerCursor
                }
                DragHandler {
                    target: null
                    dragThreshold: 0
                    onActiveChanged: () => {
                        if (active) {
                            control.startSystemResize(Qt.TopEdge)
                        }
                    }
                }
            }

            Item {
                id: topRightBorder
                width: dragHandlerContainer.borderWidth
                height: dragHandlerContainer.borderWidth
                anchors {
                    top: parent.top
                    right: parent.right
                }

                HoverHandler {
                    cursorShape: Qt.SizeBDiagCursor
                }
                DragHandler {
                    target: null
                    dragThreshold: 0
                    onActiveChanged: () => {
                        if (active) {
                            control.startSystemResize(Qt.TopEdge | Qt.RightEdge)
                        }
                    }
                }
            }

            Item {
                id: rightBorder
                width: dragHandlerContainer.borderWidth
                anchors {
                    right: parent.right
                    top: parent.top
                    bottom: parent.bottom
                    topMargin: dragHandlerContainer.borderWidth
                    bottomMargin: dragHandlerContainer.borderWidth
                }

                HoverHandler {
                    cursorShape: Qt.SizeHorCursor
                }
                DragHandler {
                    target: null
                    dragThreshold: 0
                    onActiveChanged: () => {
                        if (active) {
                            control.startSystemResize(Qt.RightEdge)
                        }
                    }
                }
            }

            Item {
                id: bottomRightBorder
                width: dragHandlerContainer.borderWidth
                height: dragHandlerContainer.borderWidth
                anchors {
                    bottom: parent.bottom
                    right: parent.right
                }

                HoverHandler {
                    cursorShape: Qt.SizeFDiagCursor
                }
                DragHandler {
                    target: null
                    dragThreshold: 0
                    onActiveChanged: () => {
                        if (active) {
                            control.startSystemResize(Qt.BottomEdge | Qt.RightEdge)
                        }
                    }
                }
            }

            Item {
                id: bottomBorder
                height: dragHandlerContainer.borderWidth
                anchors {
                    bottom: parent.bottom
                    left: parent.left
                    right: parent.right
                    leftMargin: dragHandlerContainer.borderWidth
                    rightMargin: dragHandlerContainer.borderWidth
                }

                HoverHandler {
                    cursorShape: Qt.SizeVerCursor
                }
                DragHandler {
                    target: null
                    dragThreshold: 0
                    onActiveChanged: () => {
                        if (active) {
                            control.startSystemResize(Qt.BottomEdge)
                        }
                    }
                }
            }

            Item {
                id: bottomLeftBorder
                width: dragHandlerContainer.borderWidth
                height: dragHandlerContainer.borderWidth
                anchors {
                    bottom: parent.bottom
                    left: parent.left
                }

                HoverHandler {
                    cursorShape: Qt.SizeBDiagCursor
                }
                DragHandler {
                    target: null
                    dragThreshold: 0
                    onActiveChanged: () => {
                        if (active) {
                            control.startSystemResize(Qt.LeftEdge | Qt.BottomEdge)
                        }
                    }
                }
            }

            Item {
                id: leftBorder
                width: dragHandlerContainer.borderWidth
                anchors {
                    left: parent.left
                    top: parent.top
                    bottom: parent.bottom
                    topMargin: dragHandlerContainer.borderWidth
                    bottomMargin: dragHandlerContainer.borderWidth
                }

                HoverHandler {
                    cursorShape: Qt.SizeHorCursor
                }
                DragHandler {
                    target: null
                    dragThreshold: 0
                    onActiveChanged: () => {
                        if (active) {
                            control.startSystemResize(Qt.LeftEdge)
                        }
                    }
                }
            }
        }
    }
}
