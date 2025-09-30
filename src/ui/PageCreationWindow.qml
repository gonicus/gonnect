pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Window
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import base

BaseWindow {
    id: control
    objectName: "pageCreationWindow"
    title: qsTr("Create new dashboard page")
    width: 600
    height: 380
    visible: true
    resizable: false
    showMinimizeButton: false
    showMaximizeButton: false

    minimumWidth: control.width
    minimumHeight: control.height
    maximumWidth: control.width
    maximumHeight: control.height

    required property var tabRoot
    required property string pageId
    required property int pageType

    property string name: ""
    property int selection: -1

    property var icons: {
        "Home": Icons.userHome,
        "Folder": Icons.folderOpen,
        "Person": Icons.userGroupNew,
        "Phone": Icons.callStart,
        "Message": Icons.dialogMessages
    }

    Column {
        id: pageOptions
        spacing: 10
        anchors {
            fill: parent
            margins: 20
        }

        Label {
            id: titleLabel
            text: qsTr("Name")
        }

        TextArea {
            id: titleEntry
            width: parent.width
        }

        Label {
            id: iconLabel
            text: qsTr("Icon")
        }

        ComboBox {
            id: iconSelection
            width: parent.width / 4
            model: ListModel {
                id: iconEntries
                ListElement { name: "Home" }
                ListElement { name: "Folder" }
                ListElement { name: "Person" }
                ListElement { name: "Phone" }
                ListElement { name: "Message" }
            }

            delegate: ItemDelegate {
                width: parent.width
                contentItem: Row {
                    spacing: 10
                    anchors.centerIn: parent
                    IconLabel {
                        icon {
                            source: icons[name]
                        }
                    }
                }

                required property string name
            }

            contentItem: Row {
                spacing: 10
                anchors.centerIn: parent
                IconLabel {
                    icon {
                        source: icons[iconEntries.get(iconSelection.currentIndex).name]
                    }
                }
            }

            onCurrentIndexChanged: {
                control.name = model.get(currentIndex).name
                control.selection = currentIndex
            }
        }

        Row {
            id: pageButtons
            spacing: 10
            layoutDirection: Qt.RightToLeft
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }

            Button {
                id: pageCancel
                width: parent.width / 4
                text: qsTr("Cancel")

                onPressed: control.close()
            }

            Button {
                id: pageConfirm
                width: parent.width / 4
                text: qsTr("Add")

                onPressed: {
                    let icon = icons[iconEntries.get(iconSelection.currentIndex).name]
                    let text = titleEntry.text

                    tabRoot.createTab(pageId, pageType, icon, text)
                    tabRoot.mainWindow.createPage(pageId, icon, text)

                    control.close()
                }
            }
        }
    }
}
