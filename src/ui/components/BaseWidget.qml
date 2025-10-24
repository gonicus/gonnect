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

    required property double wMin
    required property double hMin

    property double xRelative: 0
    property double yRelative: 0

    property double wRelative: 0
    property double hRelative: 0

    property double wRelativeMin: 0
    property double hRelativeMin: 0

    property alias page: control.parent
    property alias widget: resizableRect

    function setPlacement() {
        control.widget.x = Math.round((page.gridWidth * control.xRelative) / page.gridDensity) * page.gridDensity
        control.widget.x = Math.max(0, Math.min(control.widget.x, page.gridWidth - control.widget.width))

        control.widget.y = Math.round((page.gridHeight * control.yRelative) / page.gridDensity) * page.gridDensity
        control.widget.y = Math.max(0, Math.min(control.widget.y, page.gridHeight - control.widget.height))
    }

    function setDimensions() {
        // Width
        if (control.wRelative < control.wRelativeMin) {
            control.wRelative = control.wRelativeMin
        }

        control.widget.width = Math.round((page.gridWidth * control.wRelative) / page.gridDensity) * page.gridDensity

        // Height
        if (control.hRelative < control.hRelativeMin) {
            control.hRelative = control.hRelativeMin
        }

        control.widget.height = Math.round((page.gridHeight * control.hRelative) / page.gridDensity) * page.gridDensity
    }

    function makeOpaque(base : color, opacity : double) : color {
        return Qt.rgba(base.r, base.g, base.b, opacity)
    }

    Connections {
        target: page
        function onGridResized() {
            control.wRelativeMin = Number(control.wMin / page.gridWidth)
            control.hRelativeMin = Number(control.hMin / page.gridHeight)

            control.setDimensions()
            control.setPlacement()
        }
    }

    Component.onCompleted: {
        control.wRelativeMin = Number(control.wMin / page.gridWidth)
        control.hRelativeMin = Number(control.hMin / page.gridHeight)

        control.setDimensions()
        control.setPlacement()
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
            radius: widget.widgetRadius
            visible: page.editMode
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
                width: widget.width
                height: widget.height
                color: "transparent"
                anchors.centerIn: parent

                DragHandler {
                    id: dragControl
                    acceptedButtons: Qt.LeftButton
                    cursorShape: active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                    target: widget

                    // INFO: Temporarily override render/draw order of a widget
                    // in edit mode.
                    onGrabChanged: function(grab) {
                        if (grab === PointerDevice.GrabExclusive) {
                            page.resetWidgetElevation()
                            control.z = 100
                        }
                    }

                    onActiveChanged: function(active) {
                        if (!active) {
                            control.xRelative = Number(widget.x / page.gridWidth)
                            control.yRelative = Number(widget.y / page.gridHeight)

                            control.setPlacement()

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
                        control.page.model.remove(control)
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

                // TODO: Fix resize stopping on reverse mouse (drag beyond limits of ither side)

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
                            let rb = page.gridWidth - (widget.x + widget.width)
                            let bb = page.gridHeight - (widget.y + widget.height)
                            let dx = mouse.x - resizeIndicator.startX
                            if (dx > rb) {
                                dx = rb
                            }
                            let dy = mouse.y - resizeIndicator.startY
                            if (dy > bb) {
                                dy = bb
                            }

                            control.wRelative = Number((widget.width + dx) / page.gridWidth)
                            control.hRelative = Number((widget.height + dy) / page.gridHeight)

                            control.setDimensions()

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
                            let lb = widget.x - parent.x
                            let bb = page.gridHeight - (widget.y + widget.height)
                            let dx = resizeIndicator.startX - mouse.x
                            if (dx > lb) {
                                dx = lb
                            }
                            let dy = mouse.y - resizeIndicator.startY
                            if (dy > bb) {
                                dy = bb
                            }

                            let nwr = Number((widget.width + dx) / page.gridWidth)
                            if (nwr >= control.wRelativeMin) {
                                control.wRelative = nwr
                                control.xRelative = Number((widget.x - dx) / page.gridWidth)
                            }
                            let nhr = Number((widget.height + dy) / page.gridHeight)
                            if (nhr >= control.hRelativeMin) {
                                control.hRelative = nhr
                            }

                            control.setDimensions()
                            control.setPlacement()

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
                            let lb = widget.x - parent.x
                            let tb = widget.y - parent.y
                            let dx = resizeIndicator.startX - mouse.x
                            if (dx > lb) {
                                dx = lb
                            }
                            let dy = resizeIndicator.startY - mouse.y
                            if (dy > tb) {
                                dy = tb
                            }

                            let nwr = Number((widget.width + dx) / page.gridWidth)
                            if (nwr >= control.wRelativeMin) {
                                control.wRelative = nwr
                                control.xRelative = Number((widget.x - dx) / page.gridWidth)
                            }
                            let nhr = Number((widget.height + dy) / page.gridHeight)
                            if (nhr >= control.hRelativeMin) {
                                control.hRelative = nhr
                                control.yRelative = Number((widget.y - dy) / page.gridHeight)
                            }

                            control.setDimensions()
                            control.setPlacement()

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
                            let rb = page.gridWidth - (widget.x + widget.width)
                            let tb = widget.y - parent.y
                            let dx = mouse.x - resizeIndicator.startX
                            if (dx > rb) {
                                dx = rb
                            }
                            let dy = resizeIndicator.startY - mouse.y
                            if (dy > tb) {
                                dy = tb
                            }

                            let nwr = Number((widget.width + dx) / page.gridWidth)
                            if (nwr >= control.wRelativeMin) {
                                control.wRelative = nwr
                            }
                            let nhr = Number((widget.height + dy) / page.gridHeight)
                            if (nhr >= control.hRelativeMin) {
                                control.hRelative = nhr
                                control.yRelative = Number((widget.y - dy) / page.gridHeight)
                            }

                            control.setDimensions()
                            control.setPlacement()

                            SM.setUiDirtyState(true)
                        }
                    }
                }
            }
        }
    }

    DropShadow {
        id: shadowEffect
        anchors.fill: widget
        horizontalOffset: 1
        verticalOffset: 1
        radius: 6.0
        color: Theme.shadowColor
        source: widget
    }
}
