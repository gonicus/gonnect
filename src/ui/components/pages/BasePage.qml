pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import base

Item {
    id: control

    required property string pageId
    required property string name
    required property string icon

    readonly property alias grid: snapGrid
    readonly property alias gridWidth: snapGrid.width
    readonly property alias gridHeight: snapGrid.height
    readonly property alias gridCellWidth: dotGrid.cellWidth
    readonly property alias gridCellHeight: dotGrid.cellHeight

    property int oldGridWidth: 0
    property int oldGridHeight: 0

    property bool editMode: false
    Connections {
        target: SM
        function onUiEditModeChanged() {
            control.editMode = SM.uiEditMode
        }
    }

    property bool emptyPage: true
    Connections {
        target: widgetModel
        function onModelUpdated() {
            control.emptyPage = widgetModel.count() === 0
        }
    }

    readonly property WidgetModel model: WidgetModel {
        id: widgetModel
    }

    function resetWidgetElevation() {
        const items = widgetModel.items()
        for (const widget of items) {
            widget.z = 0
        }
    }

    property PageWriter writer: PageWriter {
        id: pageWriter
        pageId: control.pageId
        name: control.name
        icon: control.icon
        model: widgetModel
    }

    Component {
        id: widgetSelectionWindowComponent
        WidgetSelectionWindow {
            widgetRoot: control
        }
    }

    function widgetCreationDialog() {
        widgetSelectionWindowComponent.createObject(control).show()
    }

    Item {
        id: snapGrid
        anchors {
            fill: parent
            leftMargin: control.gridCellWidth
            rightMargin: 2 * control.gridCellWidth
            topMargin: control.gridCellHeight
            bottomMargin: 2 * control.gridCellHeight
        }

        onWidthChanged: () => {
            if (snapGrid.width <= 0) {
                return
            }
            control.oldGridWidth = snapGrid.width
        }
        onHeightChanged: () => {
            if (snapGrid.height <= 0) {
                return
            }
            control.oldGridHeight = snapGrid.height
        }

        Canvas {
            id: dotGrid
            visible: control.editMode
            anchors.fill: parent

            readonly property real cellWidth: dotGrid.width / ViewHelper.numberOfGridCells()
            readonly property real cellHeight: dotGrid.height / ViewHelper.numberOfGridCells()

            onWidthChanged: Qt.callLater(dotGrid.requestPaint)
            onHeightChanged: Qt.callLater(dotGrid.requestPaint)
            onCellWidthChanged: Qt.callLater(dotGrid.requestPaint)
            onCellHeightChanged: Qt.callLater(dotGrid.requestPaint)
            onPaint: {
                const ctx = dotGrid.getContext("2d")
                ctx.clearRect(0, 0, dotGrid.width, dotGrid.height)
                ctx.fillStyle = "gray"

                for (let x = 0; x <= dotGrid.width; x += dotGrid.cellWidth) {
                    for (let y = 0; y <= dotGrid.height; y += dotGrid.cellHeight) {
                        ctx.beginPath()
                        ctx.arc(x, y, 1, 0, 2 * Math.PI)
                        ctx.fill()
                    }
                }
            }
        }

        Button {
            id: editShortcut
            icon.source: Icons.viewLeftNew
            text: qsTr("Add widgets")
            visible: control.emptyPage && !control.editMode
            anchors.centerIn: parent

            onClicked: {
                SM.setUiEditMode(true)
            }
        }
    }
}
