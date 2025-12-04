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

    property real gridWidth
    property real gridHeight
    property real gridCellWidth
    property real gridCellHeight

    property int xGrid
    property int yGrid
    property int widthGrid: control.minCellWidth
    property int heightGrid: control.minCellHeight

    property int minCellWidth: 8
    property int minCellHeight: 8

    property alias root: resizableRect

    onXGridChanged: () => control.page?.writer.save()
    onYGridChanged: () => control.page?.writer.save()
    onWidthGridChanged: () => control.page?.writer.save()
    onHeightGridChanged: () => control.page?.writer.save()
    onPageChanged: () => control.page?.writer.save()
    Component.onCompleted: () => control.page?.writer.save()

    function makeOpaque(base : color, opacity : double) : color {
        return Qt.rgba(base.r, base.g, base.b, opacity)
    }

    // Basic widget
    Rectangle {
        id: resizableRect
        x: control.xGrid * control.gridCellWidth
        y: control.yGrid * control.gridCellHeight
        width: control.widthGrid * control.gridCellWidth - 24
        height: control.heightGrid * control.gridCellHeight - 24
        radius: resizableRect.widgetRadius
        color: Theme.backgroundColor

        readonly property int widgetRadius: 12

        onXChanged: () => {
            // Round value to grid coordinate and clamp min/max values
            const cellWidth = control.gridCellWidth
            const newVal = Math.round(Util.clamp(Math.round(resizableRect.x / cellWidth) * cellWidth,
                                                 0,
                                                 (ViewHelper.numberOfGridCells() - control.widthGrid) * cellWidth))

            if (newVal !== resizableRect.x) {
                resizableRect.x = newVal
            }
        }
        onYChanged: () => {
            // Round value to grid coordinate and clamp min/max values
            const cellHeight = control.gridCellHeight
            const newVal = Math.round(Util.clamp(Math.round(resizableRect.y / cellHeight) * cellHeight,
                                                 0,
                                                 (ViewHelper.numberOfGridCells() - control.heightGrid) * cellHeight))

            if (newVal !== resizableRect.y) {
                resizableRect.y = newVal
            }
        }

        // Edit mode overlay
        Rectangle {
            id: widgetEdit
            radius: resizableRect.widgetRadius
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
            Item {
                id: dragIndicator
                width: root.width
                height: root.height
                anchors.centerIn: parent

                DragHandler {
                    id: dragControl
                    acceptedButtons: Qt.LeftButton
                    target: root

                    // Show dragged widget on top of all others
                    onGrabChanged: function(grab) {
                        if (grab === PointerDevice.GrabExclusive) {
                            control.page.resetWidgetElevation()
                            control.z = 100
                        }
                    }

                    onActiveChanged: () => {
                        if (!dragControl.active) {
                            // Re-establish bindings after dragging has ended
                            control.xGrid = Math.round(resizableRect.x / control.gridCellWidth)
                            control.yGrid = Math.round(resizableRect.y / control.gridCellHeight)

                            resizableRect.x = Qt.binding(() => control.xGrid * control.gridCellWidth)
                            resizableRect.y = Qt.binding(() => control.yGrid * control.gridCellHeight)
                        }
                    }
                }

                HoverHandler {
                    cursorShape: dragControl.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                }
            }

            // Remove
            Button {
                id: removeButton
                anchors.centerIn: parent
                height: removeButton.width
                icon {
                    source: Icons.editDelete
                    width: 32
                    height: 32
                }
                onClicked: () => {
                    control.page.model.remove(control)
                    control.destroy()
                }
            }

            // Resize
            Item {
                id: resizeHandleOverlay
                anchors.fill: parent
                anchors.centerIn: parent

                property int indicatorSize: 30

                property real startX
                property real startY

                function setNewX(x : real) {
                    const oldX = control.xGrid
                    const newX = Util.clamp(Math.round((root.x + x - resizeHandleOverlay.startX) / control.gridCellWidth),
                                            0,
                                            oldX + control.widthGrid - control.minCellWidth)

                    if (newX !== oldX) {
                        control.widthGrid -= newX - oldX
                        control.xGrid = newX
                    }
                }

                function setNewY(y : real) {
                    const oldY = control.yGrid
                    const newY = Util.clamp(Math.round((root.y + y - resizeHandleOverlay.startY) / control.gridCellHeight),
                                            0,
                                            oldY + control.heightGrid - control.minCellHeight)

                    if (newY !== oldY) {
                        control.heightGrid -= newY - oldY
                        control.yGrid = newY
                    }
                }

                function setNewWidth(x : real) {
                    control.widthGrid = Util.clamp((root.width + x - resizeHandleOverlay.startX) / control.gridCellWidth,
                                                    control.minCellWidth,
                                                    ViewHelper.numberOfGridCells() - control.xGrid)
                }

                function setNewHeight(y : real) {
                    control.heightGrid = Util.clamp((root.height + y - resizeHandleOverlay.startY) / control.gridCellHeight,
                                                    control.minCellHeight,
                                                    ViewHelper.numberOfGridCells() - control.yGrid)
                }

                component ResizeHandle : Item {
                    id: resizeHandle
                    width: resizeHandleOverlay.indicatorSize
                    height: resizeHandleOverlay.indicatorSize

                    property alias cursorShape: resizeMouseArea.cursorShape

                    signal positionChanged(MouseEvent mouse)

                    MouseArea {
                        id: resizeMouseArea
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        drag.target: resizeHandle

                        onPressed: function(mouse) {
                            resizeHandleOverlay.startX = mouse.x
                            resizeHandleOverlay.startY = mouse.y
                        }

                        onPositionChanged: function(mouse) {
                            resizeHandle.positionChanged(mouse)
                        }
                    }
                }

                ResizeHandle {
                    id: resizeTopLeft
                    anchors.left: parent.left
                    anchors.top: parent.top
                    cursorShape: Qt.SizeFDiagCursor

                    onPositionChanged: mouse => {
                        resizeHandleOverlay.setNewX(mouse.x)
                        resizeHandleOverlay.setNewY(mouse.y)
                    }
                }

                ResizeHandle {
                    id: resizeTop
                    cursorShape: Qt.SizeVerCursor
                    anchors {
                        top: resizeTopLeft.top
                        bottom: resizeTopLeft.bottom
                        left: resizeTopLeft.right
                        right: resizeTopRight.left
                    }

                    onPositionChanged: mouse => {
                        resizeHandleOverlay.setNewY(mouse.y)
                    }
                }

                ResizeHandle {
                    id: resizeTopRight
                    anchors.right: parent.right
                    anchors.top: parent.top
                    cursorShape: Qt.SizeBDiagCursor

                    onPositionChanged: mouse => {
                        resizeHandleOverlay.setNewY(mouse.y)
                        resizeHandleOverlay.setNewWidth(mouse.x)
                    }
                }

                ResizeHandle {
                    id: resizeRight
                    cursorShape: Qt.SizeHorCursor
                    anchors {
                        top: resizeTopRight.bottom
                        left: resizeTopRight.left
                        right: resizeTopRight.right
                        bottom: resizeBottomRight.bottom
                    }

                    onPositionChanged: mouse => {
                        resizeHandleOverlay.setNewWidth(mouse.x)
                    }
                }

                ResizeHandle {
                    id: resizeBottomRight
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    cursorShape: Qt.SizeFDiagCursor

                    onPositionChanged: mouse => {
                        resizeHandleOverlay.setNewWidth(mouse.x)
                        resizeHandleOverlay.setNewHeight(mouse.y)
                    }
                }

                ResizeHandle {
                    id: resizeBottom
                    cursorShape: Qt.SizeVerCursor
                    anchors {
                        top: resizeBottomLeft.top
                        bottom: resizeBottomLeft.bottom
                        left: resizeBottomLeft.right
                        right: resizeBottomRight.left
                    }

                    onPositionChanged: mouse => {
                        resizeHandleOverlay.setNewHeight(mouse.y)
                    }
                }

                ResizeHandle {
                    id: resizeBottomLeft
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    cursorShape: Qt.SizeBDiagCursor

                    onPositionChanged: mouse => {
                        resizeHandleOverlay.setNewX(mouse.x)
                        resizeHandleOverlay.setNewHeight(mouse.y)
                    }
                }

                ResizeHandle {
                    id: resizeLeft
                    cursorShape: Qt.SizeHorCursor
                    anchors {
                        top: resizeTopLeft.bottom
                        left: resizeTopLeft.left
                        right: resizeTopLeft.right
                        bottom: resizeBottomLeft.top
                    }

                    onPositionChanged: mouse => {
                        resizeHandleOverlay.setNewX(mouse.x)
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
