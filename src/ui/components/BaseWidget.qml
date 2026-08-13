pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import Qt5Compat.GraphicalEffects
import base

Item {
    id: control

    required property string widgetId
    required property int type
    required property var page

    property AdditionalSettings config: AdditionalSettings {}

    signal additionalSettingsLoaded()
    signal additionalSettingsUpdated()

    signal cleanupRequested()

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    Connections {
        target: control.config
        function onParametersUpdated() {
            control.page?.writer.save()
        }
    }

    Connections {
        target: control.page ?? null
        function onLayoutChanged() {
            control.updateGluedEdges()
        }
    }

    Connections {
        target: control.page?.model ?? null
        function onModelUpdated() {
            control.updateGluedEdges()
        }
    }

    property int notifications: 0

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

    readonly property real devicePixelRatio: control.page?.devicePixelRatio ?? 1

    // Helper properties that keep the information if two widgets are glued together
    // on some of their edges. This is required because the x/y positions of widgets
    // cannot be the same: a neighbors edge i.e. has x+1 and not the same x.
    property bool gluedLeft: false
    property bool gluedTop: false
    property bool gluedRight: false
    property bool gluedBottom: false

    readonly property bool snapsToLeftNeighbour: control.gluedLeft && !dragControl.active
    readonly property bool snapsToTopNeighbour: control.gluedTop && !dragControl.active

    readonly property real pixelX: control.snapsToLeftNeighbour
            ? control.snapToDevicePixel((control.xGrid + 1) * control.gridCellWidth) - control.widgetGap
            : control.snapToDevicePixel(control.xGrid * control.gridCellWidth)
    readonly property real pixelY: control.snapsToTopNeighbour
            ? control.snapToDevicePixel((control.yGrid + 1) * control.gridCellHeight) - control.widgetGap
            : control.snapToDevicePixel(control.yGrid * control.gridCellHeight)

    function snapToDevicePixel(value : real) : real {
        return Util.snapToDevicePixel(value, control.devicePixelRatio)
    }

    readonly property real pixelWidth: control.snapToDevicePixel(
                                            (control.xGrid + control.widthGrid) * control.gridCellWidth
                                            ) - control.pixelX
    readonly property real pixelHeight: control.snapToDevicePixel(
                                            (control.yGrid + control.heightGrid) * control.gridCellHeight
                                            ) - control.pixelY

    readonly property real widgetGap: control.snapToDevicePixel(24)

    function gridRectChanged() {
        control.page?.writer.save()
        control.page?.layoutChanged()
    }

    onXGridChanged: () => control.gridRectChanged()
    onYGridChanged: () => control.gridRectChanged()
    onWidthGridChanged: () => control.gridRectChanged()
    onHeightGridChanged: () => control.gridRectChanged()

    onPageChanged: () => control.page?.writer.save()
    Component.onCompleted: () => control.page?.writer.save()

    function updateGluedEdges() {
        let left = false
        let top = false
        let right = false
        let bottom = false

        // Check if we're glued with all other widget items on this page
        for (const other of control.page?.model.items() ?? []) {
            if (other === control) {
                continue
            }

            const sharesRow = other.yGrid < control.yGrid + control.heightGrid
                              && control.yGrid < other.yGrid + other.heightGrid
            const sharesColumn = other.xGrid < control.xGrid + control.widthGrid
                                 && control.xGrid < other.xGrid + other.widthGrid

            left = left || (sharesRow && other.xGrid + other.widthGrid === control.xGrid + 1)
            right = right || (sharesRow && other.xGrid === control.xGrid + control.widthGrid - 1)
            top = top || (sharesColumn && other.yGrid + other.heightGrid === control.yGrid + 1)
            bottom = bottom || (sharesColumn && other.yGrid === control.yGrid + control.heightGrid - 1)
        }

        control.gluedLeft = left
        control.gluedTop = top
        control.gluedRight = right
        control.gluedBottom = bottom
    }

    function makeOpaque(base : color, opacity : double) : color {
        return Qt.rgba(base.r, base.g, base.b, opacity)
    }

    // Basic widget
    Rectangle {
        id: resizableRect
        x: control.pixelX
        y: control.pixelY
        width: control.pixelWidth - control.widgetGap
        height: control.pixelHeight - control.widgetGap
        radius: resizableRect.widgetRadius
        topLeftRadius: resizableRect.cornerTopLeft
        topRightRadius: resizableRect.cornerTopRight
        bottomLeftRadius: resizableRect.cornerBottomLeft
        bottomRightRadius: resizableRect.cornerBottomRight
        color: Theme.backgroundColor

        readonly property int widgetRadius: 12

        // Disable radius on glued edges, in order to remove the rounded nodge
        // between two widgets
        readonly property int cornerTopLeft: (control.gluedLeft || control.gluedTop)
                                             ? 0 : resizableRect.widgetRadius
        readonly property int cornerTopRight: (control.gluedRight || control.gluedTop)
                                              ? 0 : resizableRect.widgetRadius
        readonly property int cornerBottomLeft: (control.gluedLeft || control.gluedBottom)
                                                ? 0 : resizableRect.widgetRadius
        readonly property int cornerBottomRight: (control.gluedRight || control.gluedBottom)
                                                 ? 0 : resizableRect.widgetRadius


        onXChanged: () => {
            if (!dragControl.active) {
                return
            }

            // Round value to grid coordinate and clamp min/max values
            const cellWidth = control.gridCellWidth
            const gridX = Util.clamp(Math.round(resizableRect.x / cellWidth),
                                     0,
                                     ViewHelper.numberOfGridCells() - control.widthGrid)
            const newVal = control.snapToDevicePixel(gridX * cellWidth)

            if (newVal !== resizableRect.x) {
                resizableRect.x = newVal
            }
        }
        onYChanged: () => {
            if (!dragControl.active) {
                return
            }

            // Round value to grid coordinate and clamp min/max values
            const cellHeight = control.gridCellHeight
            const gridY = Util.clamp(Math.round(resizableRect.y / cellHeight),
                                     0,
                                     ViewHelper.numberOfGridCells() - control.heightGrid)
            const newVal = control.snapToDevicePixel(gridY * cellHeight)

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

                Accessible.role: Accessible.Dial
                Accessible.name: qsTr("Drag widget")
                Accessible.description: qsTr("Change the position of the widget")
                Accessible.focusable: true

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

                            resizableRect.x = Qt.binding(() => control.pixelX)
                            resizableRect.y = Qt.binding(() => control.pixelY)
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
                    control.cleanupRequested()
                    control.page.model.remove(control)
                    control.destroy()
                }

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Remove widget")
                Accessible.description: qsTr("Remove the currently selected widget from the dashboard")
                Accessible.focusable: true
                Accessible.onPressAction: () => removeButton.click()
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
                    const newX = Util.clamp(oldX + Math.round((x - resizeHandleOverlay.startX) / control.gridCellWidth),
                                            0,
                                            oldX + control.widthGrid - control.minCellWidth)

                    if (newX !== oldX) {
                        control.widthGrid -= newX - oldX
                        control.xGrid = newX
                    }
                }

                function setNewY(y : real) {
                    const oldY = control.yGrid
                    const newY = Util.clamp(oldY + Math.round((y - resizeHandleOverlay.startY) / control.gridCellHeight),
                                            0,
                                            oldY + control.heightGrid - control.minCellHeight)

                    if (newY !== oldY) {
                        control.heightGrid -= newY - oldY
                        control.yGrid = newY
                    }
                }

                function setNewWidth(x : real) {
                    const delta = Math.round((x - resizeHandleOverlay.startX) / control.gridCellWidth)
                    control.widthGrid = Util.clamp(control.widthGrid + delta,
                                                    control.minCellWidth,
                                                    ViewHelper.numberOfGridCells() - control.xGrid)
                }

                function setNewHeight(y : real) {
                    const delta = Math.round((y - resizeHandleOverlay.startY) / control.gridCellHeight)
                    control.heightGrid = Util.clamp(control.heightGrid + delta,
                                                    control.minCellHeight,
                                                    ViewHelper.numberOfGridCells() - control.yGrid)
                }

                component ResizeHandle : Item {
                    id: resizeHandle
                    width: resizeHandleOverlay.indicatorSize
                    height: resizeHandleOverlay.indicatorSize

                    property alias cursorShape: resizeMouseArea.cursorShape

                    signal positionChanged(MouseEvent mouse)

                    Accessible.role: Accessible.Dial
                    Accessible.name: qsTr("Resize widget")
                    Accessible.description: qsTr("Resize the widget according to the mouse direction")
                    Accessible.focusable: true

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
                    LayoutMirroring.enabled: false

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
                    LayoutMirroring.enabled: false

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
                    LayoutMirroring.enabled: false

                    onPositionChanged: mouse => {
                        resizeHandleOverlay.setNewWidth(mouse.x)
                    }
                }

                ResizeHandle {
                    id: resizeBottomRight
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    cursorShape: Qt.SizeFDiagCursor
                    LayoutMirroring.enabled: false

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
                    LayoutMirroring.enabled: false

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
                    LayoutMirroring.enabled: false

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
