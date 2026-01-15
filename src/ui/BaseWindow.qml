pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import Qt5Compat.GraphicalEffects
import base

Window {
    id: control
    flags: control.useOwnDecoration ? (Qt.FramelessWindowHint | Qt.NoDropShadowWindowHint) : Qt.Window
    color: control.useOwnDecoration ? 'transparent' : Theme.backgroundColor

    Material.accent: Theme.accentColor
    Material.theme: Theme.isDarkMode ? Material.Dark : Material.Light

    property bool resizable: false
    property alias windowHeaderComponent: windowHeaderLoader.sourceComponent
    property bool useOwnDecoration: Theme.useOwnDecoration
    property bool showMinimizeButton: true
    property bool showMaximizeButton: true

    property int windowHeaderPadding: windowHeaderLoader.height + 2 * control.shadowMargin

    default property alias content: innerContainer.children

    readonly property bool isMaximized: [ Window.Maximized, Window.FullScreen ].includes(control.visibility)

    readonly property int shadowMargin: 11

    function focusSearchBox() {
        if (typeof windowHeaderLoader.item?.focusSearchBox === "function") {
            windowHeaderLoader.item.focusSearchBox()
        }
    }

    Page {
        id: containerPage
        anchors.fill: parent
        padding: control.useOwnDecoration && !control.isMaximized ? control.shadowMargin : 0

        background: Item {
            visible: control.useOwnDecoration

            Rectangle {
                id: bgRect
                color: Theme.backgroundColor
                radius: 8
                anchors {
                    fill: parent
                    margins: control.isMaximized ? 0 : control.shadowMargin
                }
            }

            DropShadow {
                anchors.fill: bgRect
                horizontalOffset: 0
                verticalOffset: 3
                radius: 8.0
                color: Theme.shadowColor
                visible: !control.isMaximized
                source: bgRect
            }

            Item {
                id: dragHandlerContainer
                enabled: control.useOwnDecoration && control.resizable
                anchors.fill: parent

                readonly property alias borderWidth: control.shadowMargin

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

        contentItem: Item {
            id: outerContainer

            Loader {
                id: windowHeaderLoader
                active: control.useOwnDecoration
                height: 44
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                }
                sourceComponent: Component {
                    WindowHeader {
                        showMinimizeButton: control.showMinimizeButton
                        showMaximizeButton: control.showMaximizeButton
                    }
                }

                Connections {
                    target: windowHeaderLoader.item
                    function onToggleMaximized() {
                        if (!control.resizable) {
                            return
                        }

                        if (control.isMaximized) {
                            control.showNormal()
                        } else {
                            control.showMaximized()
                        }
                    }
                }
            }

            Item {
                id: innerContainer
                clip: true
                anchors {
                    top: control.useOwnDecoration ? windowHeaderLoader.bottom : parent.top
                    bottom: parent.bottom
                    left: parent.left
                    right: parent.right
                }
            }
        }
    }
}
