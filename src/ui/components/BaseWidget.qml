pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import Qt5Compat.GraphicalEffects
import base

Item {
    id: control

    required property string name
    required property int type

    required property double xRelative
    required property double yRelative

    required property double wRelative
    required property double wRelativeMin
    required property double wRelativeMax

    required property double hRelative
    required property double hRelativeMin
    required property double hRelativeMax

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
        //TODO: Enforce absolute min sizes again?

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
            color: Theme.backgroundColor
            opacity: 0.6
            z: 1
            anchors.fill: parent

            // TODO: Taps still register on the items below this layer
            MouseArea {
                id: hoverEdit
                hoverEnabled: true
                anchors.fill: parent

                onClicked: function(mouse) {
                    mouse.accepted = true
                }
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
                        source: Icons.mobileCloseApp
                        width: parent.width * 0.5
                        height: parent.height * 0.5
                        color: Theme.redColor
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

                // TODO: Fix weird reposition effect on min size resize
                // and max size / bounds swap-over

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

                        onPressed: function(mouse) {
                            resizeIndicator.startX = mouse.x
                            resizeIndicator.startY = mouse.y
                        }

                        onPositionChanged: function(mouse) {
                            if (mouse.buttons === Qt.LeftButton) {
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

                        onPressed: function(mouse) {
                            resizeIndicator.startX = mouse.x
                            resizeIndicator.startY = mouse.y
                        }

                        onPositionChanged: function(mouse) {
                            if (mouse.buttons === Qt.LeftButton) {
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

                                control.wRelative = Number((resizableRect.width + dx) / control.gridWidth)
                                control.hRelative = Number((resizableRect.height + dy) / control.gridHeight)
                                control.setDimensions()

                                // Reposition
                                if (control.wRelative > control.wRelativeMin) {
                                    control.xRelative = Number((resizableRect.x - dx) / control.gridWidth)
                                }

                                control.setPlacement()
                            }
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

                        onPressed: function(mouse) {
                            resizeIndicator.startX = mouse.x
                            resizeIndicator.startY = mouse.y
                        }

                        onPositionChanged: function(mouse) {
                            if (mouse.buttons === Qt.LeftButton) {
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

                                control.wRelative = Number((resizableRect.width + dx) / control.gridWidth)
                                control.hRelative = Number((resizableRect.height + dy) / control.gridHeight)
                                control.setDimensions()

                                // Reposition
                                if (control.wRelative > control.wRelativeMin) {
                                    control.xRelative = Number((resizableRect.x - dx) / control.gridWidth)
                                }

                                if (control.hRelative > control.hRelativeMin) {
                                    control.yRelative = Number((resizableRect.y - dy) / control.gridHeight)
                                }

                                control.setPlacement()
                            }
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

                        onPressed: function(mouse) {
                            resizeIndicator.startX = mouse.x
                            resizeIndicator.startY = mouse.y
                        }

                        onPositionChanged: function(mouse) {
                            if (mouse.buttons === Qt.LeftButton) {
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

                                control.wRelative = Number((resizableRect.width + dx) / control.gridWidth)
                                control.hRelative = Number((resizableRect.height + dy) / control.gridHeight)
                                control.setDimensions()

                                // Reposition
                                if (control.hRelative > control.hRelativeMin) {
                                    control.yRelative = Number((resizableRect.y - dy) / control.gridHeight)
                                }

                                control.setPlacement()
                            }
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
