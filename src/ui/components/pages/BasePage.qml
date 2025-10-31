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

    property bool editMode: false
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

    property alias grid: snapGrid
    property alias gridWidth: snapGrid.width
    property alias gridHeight: snapGrid.height
    property double gridDensity: 15

    signal gridResized()

    property alias model: widgetModel
    WidgetModel {
        id: widgetModel
    }

    function resetWidgetElevation() {
        for (const widget of model.items()) {
            widget.z = 0
        }
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
            widgetRoot: control
        }
    }

    function widgetCreationDialog() {
        widgetSelectionWindowComponent.createObject(control).show()
    }

    property int horizontalPadding: gridDensity * 2
    property int verticalPadding: gridDensity
    property int dotRadius: 1

    Rectangle {
        id: snapGrid
        width: Math.round((parent.width - horizontalPadding) / gridDensity) * gridDensity
        height: Math.round((parent.height - verticalPadding) / gridDensity) * gridDensity
        anchors.centerIn: parent
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

        Canvas {
            id: dotGrid
            visible: control.editMode
            anchors.fill: parent

            onPaint: {
                let ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.fillStyle = "gray"

                for (let x = 0; x <= width; x += control.gridDensity) {
                    for (let y = 0; y <= height; y += control.gridDensity) {
                        ctx.beginPath()
                        ctx.arc(x, y, dotRadius, 0, 2*Math.PI)
                        ctx.fill()
                    }
                }
            }

            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
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
