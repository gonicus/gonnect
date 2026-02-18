pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import base

BaseWidget {
    id: control
    minCellWidth: 20
    minCellHeight: 15

    Rectangle {
        id: historyWidget
        parent: control.root
        color: "transparent"
        anchors.fill: parent

        CardHeading {
            id: historyHeading
            text: qsTr("History")
            anchors {
                left: parent.left
                right: parent.right
            }

            property alias searchVisible: historySearchField.visible

            SearchField {
                id: historySearchField
                visible: false
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left
                    right: historyFilterMediumSelector.left
                    leftMargin: 20
                    rightMargin: 20
                }

                Keys.onEscapePressed: () => {
                    if (historySearchField.text !== "") {
                        historySearchField.text = ""
                    } else {
                        showHistorySearchButton.clicked()
                    }
                }
            }

            ComboBox {
                id: historyFilterMediumSelector
                height: 30
                font.pixelSize: 13
                padding: 0
                valueRole: "value"
                textRole: "label"
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: historyFilterTypeSelector.left
                    rightMargin: 10
                }

                model: [
                    {
                        value: HistoryProxyModel.MediumFilter.ALL,
                        label: qsTr('All')
                    }, {
                        value: HistoryProxyModel.MediumFilter.SIPCALL,
                        label: qsTr('SIP')
                    }, {
                        value: HistoryProxyModel.MediumFilter.JITSIMEET,
                        label: qsTr('Jitsi Meet')
                    }
                ]

                Accessible.role: Accessible.ComboBox
                Accessible.name: qsTr("History call type picker")
                Accessible.description: qsTr("Select the call type to filter by")

                delegate: ItemDelegate {
                    id: historyFilterMediumSelectorDelg
                    width: parent.width
                    text: historyFilterMediumSelectorDelg.label

                    Accessible.role: Accessible.ListItem
                    Accessible.name: historyFilterMediumSelectorDelg.label
                    Accessible.description: qsTr("Currently selected call type")
                    Accessible.focusable: true

                    required property string label
                }
            }

            ComboBox {
                id: historyFilterTypeSelector
                height: 30
                font.pixelSize: 13
                padding: 0
                valueRole: "value"
                textRole: "label"
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: showHistorySearchButton.left
                    rightMargin: 10
                }

                model: [
                    {
                        value: HistoryProxyModel.TypeFilter.ALL,
                        label: qsTr('All')
                    }, {
                        value: HistoryProxyModel.TypeFilter.INCOMING,
                        label: qsTr('Incoming')
                    }, {
                        value: HistoryProxyModel.TypeFilter.OUTGOING,
                        label: qsTr('Outgoing')
                    }, {
                        value: HistoryProxyModel.TypeFilter.MISSED,
                        label: qsTr('Missed')
                    }
                ]

                Accessible.role: Accessible.ComboBox
                Accessible.name: qsTr("History call origin picker")
                Accessible.description: qsTr("Select the call origin to filter by")

                delegate: ItemDelegate {
                    id: historyFilterTypeSelectorDelg
                    width: parent.width
                    text: historyFilterTypeSelectorDelg.label

                    Accessible.role: Accessible.ListItem
                    Accessible.name: historyFilterTypeSelectorDelg.label
                    Accessible.description: qsTr("Currently selected call origin")
                    Accessible.focusable: true

                    required property string label
                }
            }

            HeaderIconButton {
                id: showHistorySearchButton
                iconSource: historyHeading.searchVisible ? Icons.mobileCloseApp : Icons.systemSearch
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: parent.right
                    rightMargin: 20
                }

                onClicked: () => {
                    if (historyHeading.searchVisible) {
                        historyHeading.searchVisible = false
                        historySearchField.text = ""
                    } else {
                        historyHeading.searchVisible = true
                        historySearchField.giveFocus()
                    }
                }
            }
        }

        HistoryList {
            id: historyList
            height: parent.height
            clip: true
            proxyModel {
                filterText: historySearchField.text.trim()
                typeFilter: historyFilterTypeSelector.currentValue
                mediumFilter: historyFilterMediumSelector.currentValue
            }
            anchors {
                top: historyHeading.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
        }
    }
}
