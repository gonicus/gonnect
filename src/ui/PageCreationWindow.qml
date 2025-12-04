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
    title: control.newPage ? qsTr("Create new dashboard page") : qsTr("Edit dashboard page")
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

    required property string pageId

    property bool newPage: false
    property int selection: -1

    signal accepted(string name, string iconId)

    function prefill(iconId : string, name : string) {
        titleEntry.text = name
        iconSelection.currentValue = iconId
    }

    Component.onCompleted: {
        titleEntry.forceActiveFocus()
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

        TextField {
            id: titleEntry
            Layout.fillWidth: true
            onAccepted: () => pageConfirm.click()
        }

        Label {
            id: iconLabel
            text: qsTr("Icon")
        }

        ComboBox {
            id: iconSelection
            valueRole: "iconId"
            Layout.preferredWidth: parent.width / 4

            model: ListModel {
                id: iconEntries

                // iconId must be one of those loaded by Icons class (e.g. Icons.userHome => "userHome")

                ListElement { iconId: "dashboardShow" }
                ListElement { iconId: "folderTable" }
                ListElement { iconId: "officeChartBar" }
                ListElement { iconId: "officeChartLine" }
                ListElement { iconId: "officeChartPie" }
                ListElement { iconId: "userHome" }
                ListElement { iconId: "folderOpen" }
                ListElement { iconId: "userGroupNew" }
                ListElement { iconId: "callStart" }
                ListElement { iconId: "dialogMessages" }
            }

            delegate: ItemDelegate {
                id: iconDelg
                width: parent.width
                contentItem: RowLayout {
                    spacing: 10
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter

                    IconLabel {
                        icon {
                            source: Icons[iconDelg.iconId]
                            width: 32
                            height: 32
                        }
                    }
                }

                required property string iconId
            }

            contentItem: RowLayout {
                spacing: 10
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter

                IconLabel {
                    leftPadding: 15
                    icon {
                        source: iconSelection.currentIndex >= 0
                                ? Icons[iconEntries.get(iconSelection.currentIndex).iconId]
                                : ""
                        width: 32
                        height: 32
                    }
                }
            }

            onCurrentIndexChanged: {
                const currentIndex = iconSelection.currentIndex
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

                onClicked: () => control.close()
            }

            Button {
                id: pageConfirm
                highlighted: true
                icon.source: Icons.checkbox
                text: control.newPage ? qsTr("Create") : qsTr("Save")

                onClicked: () => {
                    control.accepted(titleEntry.text.trim(),
                                     iconEntries.get(iconSelection.currentIndex).iconId)
                    control.close()
                }
            }
        }
    }
}
