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

    required property double xRelative
    required property double yRelative

    required property double wRelative
    required property double hRelative

    required property double wMin
    required property double hMin

    property double wRelativeMin: control.wMin / control.gridWidth
    property double hRelativeMin: control.hMin / control.gridHeight

    property alias root: resizableRect

    // INFO: Temporarily overrides render/draw order in edit mode. All widgets have
    // a different elevation based on the order they were instantiated in
    z: hoverEdit?.containsMouse ? 100 : 0

    property WidgetModel model: parent.model

    property bool editMode: parent.editMode
    property double gridWidth: parent.gridWidth
    property double gridHeight: parent.gridHeight
    property int density: parent.density

    property var resized: parent

    function setPlacement() {
        control.root.x = Math.round((control.gridWidth * control.xRelative) / control.density) * control.density
        control.root.x = Math.max(0, Math.min(control.root.x, control.gridWidth - control.root.width))

        control.root.y = Math.round((control.gridHeight * control.yRelative) / control.density) * control.density
        control.root.y = Math.max(0, Math.min(control.root.y, control.gridHeight - control.root.height))
    }

    function setDimensions() {
        // Width
        if (control.wRelative < control.wRelativeMin) {
            control.wRelative = control.wRelativeMin
        }

        control.root.width = Math.round((control.gridWidth * control.wRelative) / control.density) * control.density

        // Height
        if (control.hRelative < control.hRelativeMin) {
            control.hRelative = control.hRelativeMin
        }

        control.root.height = Math.round((control.gridHeight * control.hRelative) / control.density) * control.density
    }

    function makeOpaque(base : color, opacity : double) : color {
        return Qt.rgba(base.r, base.g, base.b, opacity)
    }

    Connections {
        target: resized
        function onGridResized() {
            control.setPlacement()
            control.setDimensions()
        }
    }

    Component.onCompleted: {
        control.setPlacement()
        control.setDimensions()
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
            radius: resizableRect.widgetRadius
            visible: control.editMode
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
                width: resizableRect.width
                height: resizableRect.height
                color: "transparent"
                anchors.centerIn: parent

                DragHandler {
                    id: dragControl
                    acceptedButtons: Qt.LeftButton
                    cursorShape: active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                    target: resizableRect

                    onActiveChanged: function(active) {
                        if (!active) {
                            control.xRelative = Number(resizableRect.x / control.gridWidth)
                            control.yRelative = Number(resizableRect.y / control.gridHeight)
                            control.setPlacement()
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
                        control.model.remove(control)
                        control.destroy()
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
                            let rb = control.gridWidth - (resizableRect.x + resizableRect.width)
                            let bb = control.gridHeight - (resizableRect.y + resizableRect.height)
                            let dx = mouse.x - resizeIndicator.startX
                            if (dx > rb) {
                                dx = rb
                            }
                            let dy = mouse.y - resizeIndicator.startY
                            if (dy > bb) {
                                dy = bb
                            }

                            control.wRelative = Number((resizableRect.width + dx) / control.gridWidth)
                            control.hRelative = Number((resizableRect.height + dy) / control.gridHeight)
                            control.setDimensions()
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
                            let lb = resizableRect.x - parent.x
                            let bb = control.gridHeight - (resizableRect.y + resizableRect.height)
                            let dx = resizeIndicator.startX - mouse.x
                            if (dx > lb) {
                                dx = lb
                            }
                            let dy = mouse.y - resizeIndicator.startY
                            if (dy > bb) {
                                dy = bb
                            }

                            let nwr = Number((resizableRect.width + dx) / control.gridWidth)
                            if (nwr >= control.wRelativeMin) {
                                control.wRelative = nwr
                                control.xRelative = Number((resizableRect.x - dx) / control.gridWidth)
                            }
                            let nhr = Number((resizableRect.height + dy) / control.gridHeight)
                            if (nhr >= control.hRelativeMin) {
                                control.hRelative = nhr
                            }

                            control.setDimensions()
                            control.setPlacement()
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
                            let lb = resizableRect.x - parent.x
                            let tb = resizableRect.y - parent.y
                            let dx = resizeIndicator.startX - mouse.x
                            if (dx > lb) {
                                dx = lb
                            }
                            let dy = resizeIndicator.startY - mouse.y
                            if (dy > tb) {
                                dy = tb
                            }

                            let nwr = Number((resizableRect.width + dx) / control.gridWidth)
                            if (nwr >= control.wRelativeMin) {
                                control.wRelative = nwr
                                control.xRelative = Number((resizableRect.x - dx) / control.gridWidth)
                            }
                            let nhr = Number((resizableRect.height + dy) / control.gridHeight)
                            if (nhr >= control.hRelativeMin) {
                                control.hRelative = nhr
                                control.yRelative = Number((resizableRect.y - dy) / control.gridHeight)
                            }

                            control.setDimensions()
                            control.setPlacement()
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
                            let rb = control.gridWidth - (resizableRect.x + resizableRect.width)
                            let tb = resizableRect.y - parent.y
                            let dx = mouse.x - resizeIndicator.startX
                            if (dx > rb) {
                                dx = rb
                            }
                            let dy = resizeIndicator.startY - mouse.y
                            if (dy > tb) {
                                dy = tb
                            }

                            let nwr = Number((resizableRect.width + dx) / control.gridWidth)
                            if (nwr >= control.wRelativeMin) {
                                control.wRelative = nwr
                            }
                            let nhr = Number((resizableRect.height + dy) / control.gridHeight)
                            if (nhr >= control.hRelativeMin) {
                                control.hRelative = nhr
                                control.yRelative = Number((resizableRect.y - dy) / control.gridHeight)
                            }

                            control.setDimensions()
                            control.setPlacement()
                        }
                    }
                }
            }
        }
    }

    DropShadow {
        id: shadowEffect
        anchors.fill: resizableRect
        horizontalOffset: 1
        verticalOffset: 1
        radius: 6.0
        color: Theme.shadowColor
        source: resizableRect
    }
}
