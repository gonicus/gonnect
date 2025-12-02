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
            icon.source: Icons.listAdd
            height: control.buttonHeight
            text: qsTr("Add page")
            enabled: control.tabRoot.dynamicPageCount < control.tabRoot.dynamicPageLimit

            onPressed: {
                control.tabRoot.pageCreationDialog()
            }
        }

        Button {
            icon.source: Icons.viewLeftNew
            height: control.buttonHeight
            text: qsTr("Add widget")
            enabled: control.tabRoot.selectedPageType === GonnectWindow.PageType.Base

            onPressed: {
                let page = control.pageRoot.getPage(control.tabRoot.selectedPageId)
                if (page) {
                    page.widgetCreationDialog()
                }
            }
        }

        Button {
            highlighted: true
            Material.accent: Theme.greenColor
            icon.source: Icons.objectSelectSymbolic
            height: control.buttonHeight
            text: qsTr("Save")
            enabled: SM.uiDirtyState

            onClicked: {
                SM.setUiSaveState(true)
                SM.setUiEditMode(false)
            }
        }

        Button {
            highlighted: true
            Material.accent: Theme.redColor
            icon.source: Icons.objectSelectSymbolic
            height: control.buttonHeight
            text: qsTr("Cancel")

            onClicked: {
                SM.setUiEditMode(false)
            }
        }
    }
}
