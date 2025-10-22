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

    property bool editMode: true
    Connections {
        target: SM
        function onUiEditModeChanged() {
            editMode = SM.uiEditMode
        }
    }

    property bool emptyPage: true
    Connections {
        target: model
        function onModelUpdated() {
            emptyPage = model.count() === 0
        }
    }

    property alias gridWidth: snapGrid.width
    property alias gridHeight: snapGrid.height
    property alias density: snapGrid.cellSize

    signal gridResized()

    property alias model: widgetModel
    WidgetModel {
        id: widgetModel
    }

    property alias writer: pageWriter
    PageWriter {
        id: pageWriter

        pageId: control.pageId
        name: control.name
        icon: control.icon
        model: control.model
    }

    Component {
        id: widgetSelectionWindowComponent
        WidgetSelectionWindow {
            pageRoot: control
        }
    }

    function widgetCreationDialog() {
        widgetSelectionWindowComponent.createObject(control).show()
    }

    // TODO: This should be rounded up in steps as well, otherwise weird clipping will occur
    Rectangle {
        id: snapGrid
        visible: control.editMode
        anchors.fill: parent
        color: "transparent"

        onWidthChanged: () => {
            if (snapGrid.width <= 0) {
                return
            }

            control.gridResized()
        }
        onHeightChanged: () => {
            if (snapGrid.height <= 0) {
                return
            }

            control.gridResized()
        }

        property int cellSize: 15
        property int dotRadius: 1

        Canvas {
            id: dotGrid
            anchors.fill: parent

            onPaint: {
                let ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.fillStyle = "gray"

                for (let x = 0; x < width; x += snapGrid.cellSize) {
                    for (let y = 0; y < height; y += snapGrid.cellSize) {
                        ctx.beginPath()
                        ctx.arc(x, y, snapGrid.dotRadius, 0, 2*Math.PI)
                        ctx.fill()
                    }
                }
            }

            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
        }
    }

    // TODO: Provide button to get into edit mode right away instead
    Label {
        id: emptyPageInfo
        visible: control.emptyPage && !control.editMode
        anchors.centerIn: parent

        text: qsTr("This page is empty. Add widgets by using the edit mode") + " ( ≡ )"
    }
}
