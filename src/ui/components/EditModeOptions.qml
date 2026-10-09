pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control

    required property var tabRoot
    required property var pageRoot

    property double buttonHeight: 42

    Row {
        spacing: 15
        padding: 5
        anchors.centerIn: parent

        Button {
            id: addPageButton
            icon.source: Icons.listAdd
            height: control.buttonHeight
            text: qsTr("Add page")
            enabled: control.tabRoot.dynamicPageCount < control.tabRoot.dynamicPageLimit
            onClicked: () => control.tabRoot.openPageCreationDialog()

            Accessible.description: qsTr("Add a new dashboard page")
        }

        Button {
            id: addWidgetButton
            icon.source: Icons.viewLeftNew
            height: control.buttonHeight
            text: qsTr("Add widget")
            enabled: SelectionState.selectedPage.type === MainPageSelection.PageType.Base
            onClicked: () => {
                let page = control.pageRoot.getPage(SelectionState.selectedPage.id)
                if (page) {
                    page.widgetCreationDialog()
                }
            }

            Accessible.description: qsTr("Add a new widget to the current dashboard page")
        }

        Button {
            id: finishEditButton
            highlighted: true
            Material.accent: Theme.greenColor
            icon.source: Icons.objectSelectSymbolic
            height: control.buttonHeight
            text: qsTr("Finished")
            onClicked: () => SM.uiEditMode = false

            Accessible.description: qsTr("Finish and save all dashboard and widget changes")
        }
    }
}
