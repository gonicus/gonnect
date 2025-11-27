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

    property int xGrid
    property int yGrid
    property int widthGrid: control.minCellWidth
    property int heightGrid: control.minCellHeight

    property int minCellWidth: 8
    property int minCellHeight: 8

    property alias root: resizableRect

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
            const newVal = Util.clamp(Math.round(resizableRect.x / cellWidth) * cellWidth,
                                      0,
                                      (ViewHelper.numberOfGridCells() - control.widthGrid) * cellWidth)

            if (newVal !== resizableRect.x) {
                resizableRect.x = newVal
            }
        }
        onYChanged: () => {
            // Round value to grid coordinate and clamp min/max values
            const cellHeight = control.gridCellHeight
            const newVal = Util.clamp(Math.round(resizableRect.y / cellHeight) * cellHeight,
                                      0,
                                      (ViewHelper.numberOfGridCells() - control.heightGrid) * cellHeight)

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

                            // Re-establish bindings after dragging has ended
                            control.xGrid = Math.round(resizableRect.x / control.gridCellWidth)
                            control.yGrid = Math.round(resizableRect.y / control.gridCellHeight)

                            resizableRect.x = Qt.binding(() => control.xGrid * control.gridCellWidth)
                            resizableRect.y = Qt.binding(() => control.yGrid * control.gridCellHeight)

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
                            control.widthGrid = Util.clamp((root.width + mouse.x - resizeIndicator.startX) / control.gridCellWidth,
                                                           control.minCellWidth,
                                                           ViewHelper.numberOfGridCells() - control.xGrid)
                            control.heightGrid = Util.clamp((root.height + mouse.y - resizeIndicator.startY) / control.gridCellHeight,
                                                            control.minCellHeight,
                                                            ViewHelper.numberOfGridCells() - control.yGrid)

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
                            const oldX = control.xGrid
                            control.xGrid = Util.clamp((root.x + mouse.x - resizeIndicator.startX) / control.gridCellWidth,
                                                       0,
                                                       oldX + control.widthGrid - control.minCellWidth)
                            control.widthGrid -= control.xGrid - oldX
                            control.heightGrid = Util.clamp((root.height + mouse.y - resizeIndicator.startY) / control.gridCellHeight,
                                                            control.minCellHeight,
                                                            ViewHelper.numberOfGridCells() - control.yGrid)

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
                            const oldX = control.xGrid
                            control.xGrid = Util.clamp((root.x + mouse.x - resizeIndicator.startX) / control.gridCellWidth,
                                                       0,
                                                       oldX + control.widthGrid - control.minCellWidth)
                            control.widthGrid -= control.xGrid - oldX

                            const oldY = control.yGrid
                            control.yGrid = Util.clamp((root.y + mouse.y - resizeIndicator.startY) / control.gridCellHeight,
                                                       0,
                                                       oldY + control.heightGrid - control.minCellHeight)
                            control.heightGrid -= control.yGrid - oldY

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
                            const oldY = control.yGrid
                            control.widthGrid = Util.clamp((root.width + mouse.x - resizeIndicator.startX) / control.gridCellWidth,
                                                            control.minCellWidth,
                                                            ViewHelper.numberOfGridCells() - control.xGrid)
                            control.yGrid = Util.clamp((root.y + mouse.y - resizeIndicator.startY) / control.gridCellHeight,
                                                       0,
                                                       oldY + control.heightGrid - control.minCellHeight)
                            control.heightGrid -= control.yGrid - oldY

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
