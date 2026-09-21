pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import QtQuick.Layouts
import base

BaseWidget {
    id: control
    minCellWidth: 20
    minCellHeight: 15

    notifications: voiceMailField.totalVoicemailCount + control.missedCallsCount

    property int missedCallsCount: SIPCallManager.missedCalls

    Rectangle {
        id: historyWidget
        parent: control.root
        color: "transparent"
        anchors.fill: parent

        CardHeading {
            id: historyHeading
            text: qsTr("History")
            showDivider: historyHeading.voicemailVisible
            showHeading: historyWidget.width > 650 || historyHeading.showDivider === false
            anchors {
                left: parent.left
                right: parent.right
            }

            property alias searchVisible: historySearchField.visible
            property alias voicemailVisible: voiceMailField.hasVoicemail

            VoiceMailField {
                id: voiceMailField
                visible: !historyHeading.searchVisible && historyHeading.voicemailVisible
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left
                    leftMargin: 20 + historyHeading.headingMargin
                    rightMargin: 20
                }
            }

            SearchField {
                id: historySearchField
                visible: false
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left
                    right: parent.right
                    leftMargin: 20
                    rightMargin: 20 + historyHeading.actionsWidth + (historyHeading.actionsWidth > 0 ? 20 : 0)
                }

                Keys.onEscapePressed: () => {
                    if (historySearchField.text !== "") {
                        historySearchField.text = ""
                    } else {
                        showHistorySearchButton.clicked()
                    }
                }
            }

            actions: [
            ComboBox {
                id: historyFilterMediumSelector
                Layout.preferredHeight: Math.round(30 * Theme.fontSizeNormal / 13)
                font.pixelSize: Theme.fontSizeNormal
                padding: 0
                rightPadding: indicator.width + 10
                valueRole: "value"
                textRole: "label"
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: implicitWidth + Math.round(12 * Theme.fontSizeNormal / 13)
                model: [
                    {
                        value: HistoryProxyModel.MediumFilter.ALL,
                        label: qsTr('All sources')
                    }, {
                        value: HistoryProxyModel.MediumFilter.SIPCALL,
                        label: qsTr('SIP')
                    }, {
                        value: HistoryProxyModel.MediumFilter.JITSIMEET,
                        label: qsTr('Jitsi Meet')
                    }
                ]

                popup.width: Math.max(width, Math.round(12 * Theme.fontSizeNormal))

                Accessible.role: Accessible.ComboBox
                Accessible.name: qsTr("History call type picker")
                Accessible.description: qsTr("Select the call type to filter by")

                delegate: ItemDelegate {
                    id: historyFilterMediumSelectorDelg
                    width: ListView.view.width
                    text: historyFilterMediumSelectorDelg.label

                    font.family: historyFilterMediumSelector.font.family
                    font.weight: historyFilterMediumSelector.font.weight
                    font.pixelSize: historyFilterMediumSelector.font.pixelSize

                    Accessible.role: Accessible.ListItem
                    Accessible.name: historyFilterMediumSelectorDelg.label
                    Accessible.description: qsTr("Currently selected call type")
                    Accessible.focusable: true

                    required property string label
                }
            },

            ComboBox {
                id: historyFilterTypeSelector
                Layout.preferredHeight: Math.round(30 * Theme.fontSizeNormal / 13)
                font.pixelSize: Theme.fontSizeNormal
                padding: 0
                rightPadding: indicator.width + 10
                valueRole: "value"
                textRole: "label"
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: implicitWidth + Math.round(12 * Theme.fontSizeNormal / 13)
                model: [
                    {
                        value: HistoryProxyModel.TypeFilter.ALL,
                        label: qsTr('All calls')
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

                popup.width: Math.max(width, Math.round(12 * Theme.fontSizeNormal))

                Accessible.role: Accessible.ComboBox
                Accessible.name: qsTr("History call origin picker")
                Accessible.description: qsTr("Select the call origin to filter by")

                delegate: ItemDelegate {
                    id: historyFilterTypeSelectorDelg
                    width: ListView.view.width
                    text: historyFilterTypeSelectorDelg.label

                    font.family: historyFilterTypeSelector.font.family
                    font.weight: historyFilterTypeSelector.font.weight
                    font.pixelSize: historyFilterTypeSelector.font.pixelSize

                    Accessible.role: Accessible.ListItem
                    Accessible.name: historyFilterTypeSelectorDelg.label
                    Accessible.description: qsTr("Currently selected call origin")
                    Accessible.focusable: true

                    required property string label
                }
            },

            HeaderIconButton {
                id: showHistorySearchButton
                iconSource: historyHeading.searchVisible ? Icons.mobileCloseApp : Icons.systemSearch
                accessiblePurpose: historyHeading.searchVisible ? qsTr("Hide history search") : qsTr("Show history search")
                Layout.alignment: Qt.AlignVCenter

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
            ]
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
