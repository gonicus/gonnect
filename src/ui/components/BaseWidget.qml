pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import Qt5Compat.GraphicalEffects
import base

Item {
    id: control

    required property string widgetId
    required property string name
    required property int type
    required property var page

    required property double wMin
    required property double hMin

    property real gridWidth
    property real gridHeight
    property real gridCellWidth
    property real gridCellHeight

    property double xRelative: 0
    property double yRelative: 0

    property double wRelative: 0
    property double hRelative: 0

    property double wRelativeMin: 0
    property double hRelativeMin: 0

    property alias root: resizableRect
    property alias posX: resizableRect.x
    property alias posY: resizableRect.y
    property alias sizeW: resizableRect.width
    property alias sizeH: resizableRect.height

    function setWidth() {
        control.root.width = Math.round((control.gridWidth * control.wRelative) / control.gridCellWidth) * control.gridCellWidth
    }

    function setHeight() {
        control.root.height = Math.round((control.gridHeight * control.hRelative) / control.gridCellHeight) * control.gridCellHeight
    }

    function setX() {
        control.root.x = Math.round((control.gridWidth * control.xRelative) / control.gridCellWidth) * control.gridCellWidth
        control.root.x = Math.max(0, Math.min(control.root.x, control.gridWidth - control.root.width))
    }

    function setY() {
        control.root.y = Math.round((control.gridHeight * control.yRelative) / control.gridCellHeight) * control.gridCellHeight
        control.root.y = Math.max(0, Math.min(control.root.y, control.gridHeight - control.root.height))
    }

    function setMinSize() {
        control.wRelativeMin = Number(control.wMin / control.gridWidth)
        if (control.wRelative < control.wRelativeMin) {
            control.wRelative = control.wRelativeMin
        }

        control.hRelativeMin = Number(control.hMin / control.gridHeight)
        if (control.hRelative < control.hRelativeMin) {
            control.hRelative = control.hRelativeMin
        }
    }

    // INFO: This function, unlike the others, does not depend on relative float scaling
    // Old implementation that "mostly" worked: 487a071fb743c3e0275867c8db4723d33445026e
    function resize() {
        if (control.gridWidth <= 0 || control.gridHeight <= 0) {
            return
        }

        console.error(name, "X,Y:", posX, posY, "W,H:", sizeW, sizeH, "(g) W,H:", control.page.oldGridWidth, control.page.oldGridHeight)

        let minWidth = control.wMin
        let newWidth = Math.round(((control.root.width / control.page.oldGridWidth) * control.gridWidth) / control.gridCellWidth) * control.gridCellWidth
        if (newWidth < minWidth) {
            control.root.width = minWidth
        } else {
            control.root.width = newWidth
        }

        let minHeight = control.hMin
        let newHeight = Math.round(((control.root.height / control.page.oldGridHeight) * control.gridHeight) / control.gridCellHeight) * control.gridCellHeight
        if (newHeight < minHeight) {
            control.root.height = minHeight
        } else {
            control.root.height = newHeight
        }

        control.root.x = Math.round(((control.root.x / control.page.oldGridWidth) * control.gridWidth) / control.gridCellWidth) * control.gridCellWidth
        control.root.x = Math.max(0, Math.min(control.root.x, control.gridWidth - control.root.width))

        control.root.y = Math.round(((control.root.y / control.page.oldGridHeight) * control.gridHeight) / control.gridCellHeight) * control.gridCellHeight
        control.root.y = Math.max(0, Math.min(control.root.y, control.gridHeight - control.root.height))

        console.error(name, "X,Y:", posX, posY, "W,H:", sizeW, sizeH, "(g) W,H:", control.gridWidth, control.gridHeight, "(min) W,H:", minWidth, minHeight)
    }

    function makeOpaque(base : color, opacity : double) : color {
        return Qt.rgba(base.r, base.g, base.b, opacity)
    }

    Component.onCompleted: {
        if (sizeW < wMin) {
            sizeW = wMin
        }

        if (sizeH < hMin) {
            sizeH = hMin
        }
    }

    Connections {
        target: page
        function onGridResized() {
            control.resize()
        }
    }

    // Basic widget
    Rectangle {
        id: resizableRect
        radius: widgetRadius
        color: Theme.backgroundColor

        property int widgetRadius: 12

        // Edit mode overlay
        Rectangle {
            id: widgetEdit
            radius: root.widgetRadius
            visible: control.page.editMode
            color: control.makeOpaque(Theme.backgroundColor, 0.5)
            z: 1
            anchors.fill: parent

            // INFO: Inhibit all lower widget hover, scroll and tap actions
            MouseArea {
                id: hoverEdit
                enabled: true
                hoverEnabled: true
                anchors.fill: parent

                preventStealing: true
                acceptedButtons: Qt.AllButtons
                propagateComposedEvents: false

                onClicked: {}
                onWheel: {}
            }

            TapHandler {
                id: tapEdit
                enabled: true
                exclusiveSignals: TapHandler.SingleTap | TapHandler.DoubleTap
                acceptedButtons: Qt.AllButtons
                grabPermissions: PointerHandler.ApprovesTakeOverByAnything
                gesturePolicy: TapHandler.WithinBounds

                onTapped: {}
                onDoubleTapped: {}
            }

            // Drag
            Rectangle {
                id: dragIndicator
                width: root.width
                height: root.height
                color: "transparent"
                anchors.centerIn: parent

                DragHandler {
                    id: dragControl
                    acceptedButtons: Qt.LeftButton
                    cursorShape: active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                    target: root

                    // INFO: Temporarily override render/draw order of a widget
                    // in edit mode.
                    onGrabChanged: function(grab) {
                        if (grab === PointerDevice.GrabExclusive) {
                            control.page.resetWidgetElevation()
                            control.z = 100
                        }
                    }

                    onActiveChanged: function(active) {
                        if (!active) {
                            control.xRelative = Number(root.x / control.gridWidth)
                            control.yRelative = Number(root.y / control.gridHeight)

                            control.setX()
                            control.setY()

                            SM.setUiDirtyState(true)
                        }
                    }
                }
            }

            // Remove
            Rectangle {
                id: removeIndicator
                width: 35
                height: 35
                color: Theme.backgroundSecondaryColor
                radius: 6
                anchors.centerIn: parent

                IconLabel {
                    id: removeIcon
                    anchors.centerIn: parent
                    icon {
                        source: Icons.editDelete
                        width: parent.width * 0.75
                        height: parent.height * 0.75
                    }
                }

                MouseArea {
                    id: removeControl
                    parent: removeIndicator
                    anchors.fill: parent
                    cursorShape: Qt.ForbiddenCursor
                    drag.target: removeIndicator

                    onClicked: {
                        control.control.page.model.remove(control)
                        control.destroy()

                        SM.setUiDirtyState(true)
                    }
                }
            }

            // Resize
            Rectangle {
                id: resizeIndicator
                color: "transparent"
                anchors.fill: parent
                anchors.centerIn: parent

                property int indicatorSize: 30

                property real startX
                property real startY

                Rectangle {
                    id: resizeBottomRight
                    width: resizeIndicator.indicatorSize
                    height: resizeIndicator.indicatorSize
                    color: "transparent"
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom

                    MouseArea {
                        parent: resizeBottomRight
                        anchors.fill: parent
                        cursorShape: Qt.SizeFDiagCursor
                        drag.target: resizeBottomRight
                        acceptedButtons: Qt.LeftButton

                        onPressed: function(mouse) {
                            resizeIndicator.startX = mouse.x
                            resizeIndicator.startY = mouse.y
                        }

                        onPositionChanged: function(mouse) {
                            // Resize within bounds
                            let rb = control.gridWidth - (root.x + root.width)
                            let bb = control.gridHeight - (root.y + root.height)
                            let dx = mouse.x - resizeIndicator.startX
                            if (dx > rb) {
                                dx = rb
                            }
                            let dy = mouse.y - resizeIndicator.startY
                            if (dy > bb) {
                                dy = bb
                            }

                            // Width, X
                            let nwr = Number((root.width + dx) / control.gridWidth)
                            if (nwr >= control.wRelativeMin) {
                                control.wRelative = nwr
                            } else {
                                control.wRelative = control.wRelativeMin
                            }

                            control.setWidth()

                            // Height, Y
                            let nhr = Number((root.height + dy) / control.gridHeight)
                            if (nhr >= control.hRelativeMin) {
                                control.hRelative = nhr
                            } else {
                                control.hRelative = control.hRelativeMin
                            }

                            control.setHeight()

                            SM.setUiDirtyState(true)
                        }
                    }
                }

                Rectangle {
                    id: resizeBottomLeft
                    width: resizeIndicator.indicatorSize
                    height: resizeIndicator.indicatorSize
                    color: "transparent"
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom

                    MouseArea {
                        parent: resizeBottomLeft
                        anchors.fill: parent
                        cursorShape: Qt.SizeBDiagCursor
                        drag.target: resizeBottomLeft
                        acceptedButtons: Qt.LeftButton

                        onPressed: function(mouse) {
                            resizeIndicator.startX = mouse.x
                            resizeIndicator.startY = mouse.y
                        }

                        onPositionChanged: function(mouse) {
                            // Resize within bounds
                            let lb = root.x - parent.x
                            let bb = control.gridHeight - (root.y + root.height)
                            let dx = resizeIndicator.startX - mouse.x
                            if (dx > lb) {
                                dx = lb
                            }
                            let dy = mouse.y - resizeIndicator.startY
                            if (dy > bb) {
                                dy = bb
                            }

                            // Width, X
                            let nwr = Number((root.width + dx) / control.gridWidth)
                            if (nwr >= control.wRelativeMin) {
                                control.wRelative = nwr
                                control.xRelative = Number((root.x - dx) / control.gridWidth)

                                control.setWidth()
                                control.setX()
                            } else {
                                control.wRelative = control.wRelativeMin
                                let oldWidth = root.width
                                control.setWidth()

                                control.xRelative = Number((root.x + (oldWidth - root.width)) / control.gridWidth)
                                control.setX()
                            }

                            // Height, Y
                            let nhr = Number((root.height + dy) / control.gridHeight)
                            if (nhr >= control.hRelativeMin) {
                                control.hRelative = nhr
                            } else {
                                control.hRelative = control.hRelativeMin
                            }

                            control.setHeight()

                            SM.setUiDirtyState(true)
                        }
                    }
                }

                Rectangle {
                    id: resizeTopLeft
                    width: resizeIndicator.indicatorSize
                    height: resizeIndicator.indicatorSize
                    color: "transparent"
                    anchors.left: parent.left
                    anchors.top: parent.top

                    MouseArea {
                        parent: resizeTopLeft
                        anchors.fill: parent
                        cursorShape: Qt.SizeFDiagCursor
                        drag.target: resizeTopLeft
                        acceptedButtons: Qt.LeftButton

                        onPressed: function(mouse) {
                            resizeIndicator.startX = mouse.x
                            resizeIndicator.startY = mouse.y
                        }

                        onPositionChanged: function(mouse) {
                            // Resize within bounds
                            let lb = root.x - parent.x
                            let tb = root.y - parent.y
                            let dx = resizeIndicator.startX - mouse.x
                            if (dx > lb) {
                                dx = lb
                            }
                            let dy = resizeIndicator.startY - mouse.y
                            if (dy > tb) {
                                dy = tb
                            }

                            // Width, X
                            let nwr = Number((root.width + dx) / control.gridWidth)
                            if (nwr >= control.wRelativeMin) {
                                control.wRelative = nwr
                                control.xRelative = Number((root.x - dx) / control.gridWidth)

                                control.setWidth()
                                control.setX()
                            } else {
                                control.wRelative = control.wRelativeMin
                                let oldWidth = root.width
                                control.setWidth()

                                control.xRelative = Number((root.x + (oldWidth - root.width)) / control.gridWidth)
                                control.setX()
                            }

                            // Height, Y
                            let nhr = Number((root.height + dy) / control.gridHeight)
                            if (nhr >= control.hRelativeMin) {
                                control.hRelative = nhr
                                control.yRelative = Number((root.y - dy) / control.gridHeight)

                                control.setHeight()
                                control.setY()
                            } else {
                                control.hRelative = control.hRelativeMin
                                let oldHeight = root.height
                                control.setHeight()

                                control.yRelative = Number((root.y + (oldHeight - root.height)) / control.gridHeight)
                                control.setY()
                            }

                            SM.setUiDirtyState(true)
                        }
                    }
                }

                Rectangle {
                    id: resizeTopRight
                    width: resizeIndicator.indicatorSize
                    height: resizeIndicator.indicatorSize
                    color: "transparent"
                    anchors.right: parent.right
                    anchors.top: parent.top

                    MouseArea {
                        parent: resizeTopRight
                        anchors.fill: parent
                        cursorShape: Qt.SizeBDiagCursor
                        drag.target: resizeTopRight
                        acceptedButtons: Qt.LeftButton

                        onPressed: function(mouse) {
                            resizeIndicator.startX = mouse.x
                            resizeIndicator.startY = mouse.y
                        }

                        onPositionChanged: function(mouse) {
                            // Resize within bounds
                            let rb = control.gridWidth - (root.x + root.width)
                            let tb = root.y - parent.y
                            let dx = mouse.x - resizeIndicator.startX
                            if (dx > rb) {
                                dx = rb
                            }
                            let dy = resizeIndicator.startY - mouse.y
                            if (dy > tb) {
                                dy = tb
                            }

                            // Width, X
                            let nwr = Number((root.width + dx) / control.gridWidth)
                            if (nwr >= control.wRelativeMin) {
                                control.wRelative = nwr
                            } else {
                                control.wRelative = control.wRelativeMin
                            }

                            control.setWidth()

                            // Height, Y
                            let nhr = Number((root.height + dy) / control.gridHeight)
                            if (nhr >= control.hRelativeMin) {
                                control.hRelative = nhr
                                control.yRelative = Number((root.y - dy) / control.gridHeight)

                                control.setHeight()
                                control.setY()
                            } else {
                                control.hRelative = control.hRelativeMin
                                let oldHeight = root.height
                                control.setHeight()

                                control.yRelative = Number((root.y + (oldHeight - root.height)) / control.gridHeight)
                                control.setY()
                            }

                            SM.setUiDirtyState(true)
                        }
                    }
                }
            }
        }
    }

    DropShadow {
        id: shadowEffect
        anchors.fill: root
        horizontalOffset: 1
        verticalOffset: 1
        radius: 6.0
        color: Theme.shadowColor
        source: root
    }
}
