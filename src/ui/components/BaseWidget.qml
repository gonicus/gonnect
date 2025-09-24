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
        // Width
        //TODO: Enforce absolute min sizes again?
        if (control.wRelative < wRelativeMin) {
            control.wRelative = wRelativeMin
        }

        let absWidth = Math.round((control.gridWidth * control.wRelative) / control.density) * control.density

        let wBound = control.root.x + absWidth
        if (wBound > control.gridWidth) {
            let wDiff = wBound - control.gridWidth
            absWidth -= wDiff
        }

        control.root.width = absWidth

        // Height
        if (control.hRelative < hRelativeMin) {
            control.hRelative = hRelativeMin
        }

        let absHeight = Math.round((control.gridHeight * control.hRelative) / control.density) * control.density

        let hBound = control.root.y + absHeight
        if (hBound > control.gridHeight) {
            let hDiff = hBound - control.gridHeight
            absHeight -= hDiff
        }

        control.root.height = absHeight
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
        property int indicatorPadding: 15
        property int indicatorSize: 20

        // Edit mode overlay
        Rectangle {
            id: widgetEdit
            radius: resizableRect.widgetRadius
            visible: control.editMode
            color: Theme.backgroundColor
            opacity: 0.6
            z: 1
            anchors.fill: parent

            MouseArea {
                id: hoverEdit
                enabled: control.editMode
                hoverEnabled: true
                anchors.fill: parent
            }

            // Centered options
            Row {
                anchors.centerIn: parent
                spacing: resizableRect.indicatorPadding

                // Remove
                Rectangle {
                    id: removeIndicator
                    width: resizableRect.indicatorSize
                    height: resizableRect.indicatorSize
                    color: "transparent"

                    IconLabel {
                        id: removeIcon
                        anchors.centerIn: parent
                        icon {
                            source: Icons.mobileCloseApp
                            width: parent.width
                            height: parent.height
                            color: Theme.redColor
                        }
                    }

                    MouseArea {
                        id: removeControl
                        parent: removeIndicator
                        anchors.fill: parent
                        cursorShape: Qt.CrossCursor

                        onClicked: {
                            control.model.remove(control)
                            control.destroy()
                        }
                    }
                }
            }

            // Drag
            Rectangle {
                id: dragIndicator
                width: resizableRect.width - resizableRect.indicatorSize
                height: resizableRect.height - resizableRect.indicatorSize
                //color: "transparent"
                anchors.centerIn: parent

                MouseArea {
                    enabled: control.editMode
                    hoverEnabled: true
                    anchors.fill: parent
                    cursorShape: Qt.SizeAllCursor
                }

                DragHandler {
                    id: dragControl
                    acceptedButtons: Qt.LeftButton
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

            // Resize
            Rectangle {
                id: resizeIndicator
                color: "transparent"
                anchors.fill: parent
                anchors.centerIn: parent

                property real startX
                property real startY

                Rectangle {
                    id: resizeBottomRight
                    width: resizableRect.indicatorSize
                    height: resizableRect.indicatorSize
                    color: "transparent"
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    //anchors.margins: resizableRect.indicatorPadding

                    MouseArea {
                        parent: resizeBottomRight
                        anchors.fill: parent
                        cursorShape: Qt.SizeFDiagCursor

                        onPressed: function(mouse) {
                            resizeIndicator.startX = mouse.x
                            resizeIndicator.startY = mouse.y
                        }

                        onPositionChanged: function(mouse) {
                            if (mouse.buttons === Qt.LeftButton) {
                                // Deltas
                                let dx = mouse.x - resizeIndicator.startX
                                let dy = mouse.y - resizeIndicator.startY

                                control.wRelative = Number((resizableRect.width + dx) / control.gridWidth)
                                control.hRelative = Number((resizableRect.height + dy) / control.gridHeight)
                                control.setDimensions()
                            }
                        }
                    }
                }

                Rectangle {
                    id: resizeBottomLeft
                    width: resizableRect.indicatorSize
                    height: resizableRect.indicatorSize
                    color: "transparent"
                    anchors.right: parent.left
                    anchors.bottom: parent.bottom
                    //anchors.margins: resizableRect.indicatorPadding

                    MouseArea {
                        parent: resizeBottomLeft
                        anchors.fill: parent
                        cursorShape: Qt.SizeBDiagCursor

                        onPressed: function(mouse) {
                            resizeIndicator.startX = mouse.x
                            resizeIndicator.startY = mouse.y
                        }

                        onPositionChanged: function(mouse) {
                            if (mouse.buttons === Qt.LeftButton) {
                                // Deltas
                                let dx = 0 // resizeIndicator.startX - mouse.x
                                let dy = 0 // resizeIndicator.startY - mouse.y

                                control.xRelative = Number(mouse.x / control.gridWidth)
                                control.yRelative = Number(mouse.y / control.gridWidth)
                                control.setPlacement()

                                control.wRelative = Number((resizableRect.width + dx) / control.gridWidth)
                                control.hRelative = Number((resizableRect.height + dy) / control.gridHeight)
                                control.setDimensions()
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
