pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import base

Item {
    id: control

    required property string pageId
    required property string name
    required property string iconId

    readonly property alias grid: snapGrid
    readonly property alias gridWidth: snapGrid.width
    readonly property alias gridHeight: snapGrid.height
    readonly property alias gridCellWidth: snapGrid.cellWidth
    readonly property alias gridCellHeight: snapGrid.cellHeight

    /// Must be set to true when the page is about to be deleted. Prevents actions like saving.
    property bool isBeingDeleted: false

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
            if (!control.isBeingDeleted) {
                pageWriter.save()
            }
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
        iconId: control.iconId
        model: widgetModel
    }

    Component.onCompleted: () => pageWriter.save()
    onNameChanged: () => pageWriter.save()
    onIconIdChanged: () => pageWriter.save()

    Component {
        id: widgetSelectionWindowComponent
        WidgetSelectionWindow {
            widgetRoot: control
        }
    }

    function widgetCreationDialog() {
        if (!SM.uiHasActiveEditDialog) {
            SM.uiHasActiveEditDialog = true
            widgetSelectionWindowComponent.createObject(control).show()
        }
    }

    Item {
        id: snapGrid
        anchors {
            fill: parent
            leftMargin: 24
            bottomMargin: -16
        }

        readonly property real cellWidth: snapGrid.width / ViewHelper.numberOfGridCells()
        readonly property real cellHeight: snapGrid.height / ViewHelper.numberOfGridCells()

        Button {
            id: editShortcut
            icon.source: Icons.viewLeftNew
            text: qsTr("Add widgets")
            visible: control.emptyPage && !control.editMode
            anchors.centerIn: parent

            onClicked: () => SM.uiEditMode = true
        }
    }
}
