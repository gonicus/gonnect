pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import base

BaseWindow {
    id: control
    objectName: "pageCreationWindow"
    title: qsTr("Create new dashboard page")
    width: 600
    height: 340
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

    ColumnLayout {
        id: pageOptions
        spacing: 5
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
            Layout.fillWidth: true
        }

        Label {
            id: iconLabel
            text: qsTr("Icon")
        }

        ComboBox {
            id: iconSelection
            Layout.preferredWidth: parent.width / 4

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
                contentItem: RowLayout {
                    spacing: 10
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter

                    IconLabel {
                        icon {
                            source: icons[name]
                        }
                    }
                }

                required property string name
            }

            contentItem: RowLayout {
                spacing: 10
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter

                IconLabel {
                    leftPadding: 15
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

        RowLayout {
            id: pageButtons
            spacing: 10
            Layout.fillWidth: true
            layoutDirection: Qt.RightToLeft
            Layout.alignment: Qt.AlignRight | Qt.AlignBottom

            Button {
                id: pageCancel
                text: qsTr("Cancel")

                onPressed: control.close()
            }

            Button {
                id: pageConfirm
                icon.source: Icons.listAdd
                text: qsTr("Add")

                onPressed: {
                    let icon = icons[iconEntries.get(iconSelection.currentIndex).name]
                    let text = titleEntry.text

                    tabRoot.createTab(pageId, pageType, icon, text)
                    tabRoot.mainWindow.createPage(pageId, icon, text)

                    SM.setUiDirtyState(true)

                    control.close()
                }
            }
        }
    }
}
