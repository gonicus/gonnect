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

    // Unread chat counts are reported here, so the ChatWidget is configured to report zero
    // notifications for its own badge.
    notifications: SIPCallManager.missedCalls + ChatConnectorManager.unreadNotificationsCount

    Rectangle {
        id: activitiesWidget
        parent: control.root
        color: "transparent"
        anchors.fill: parent

        CardHeading {
            id: activitiesHeading
            text: qsTr("Activities")
            anchors {
                left: parent.left
                right: parent.right
            }

            property alias searchVisible: activitiesSearchField.visible

            SearchField {
                id: activitiesSearchField
                visible: false
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left
                    right: parent.right
                    leftMargin: 20
                    rightMargin: 20 + activitiesHeading.actionsWidth + (activitiesHeading.actionsWidth > 0 ? 20 : 0)
                }

                Keys.onEscapePressed: () => {
                                          if (activitiesSearchField.text !== "") {
                                              activitiesSearchField.text = ""
                                          } else {
                                              showActivitiesSearchButton.clicked()
                                          }
                                      }
            }

            actions: [
            ComboBox {
                id: activitiesFilterMediumSelector
                Layout.preferredHeight: Math.round(30 * Theme.fontSizeNormal / 13)
                font.pixelSize: Theme.fontSizeNormal
                padding: 0
                rightPadding: indicator.width + 10
                valueRole: "value"
                textRole: "label"
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: implicitWidth + Math.round(12 * Theme.fontSizeNormal / 13)
                model: [
                    { value: ActivitiesProxyModel.MediumFilter.ALL, label: qsTr('All activities') },
                    { value: ActivitiesProxyModel.MediumFilter.SIPCALL, label: qsTr('SIP') },
                    { value: ActivitiesProxyModel.MediumFilter.JITSIMEET, label: qsTr('Jitsi Meet') },
                    { value: ActivitiesProxyModel.MediumFilter.CHAT, label: qsTr('Chat') }
                ]

                popup.width: Math.max(width, Math.round(12 * Theme.fontSizeNormal))

                Accessible.role: Accessible.ComboBox
                Accessible.name: qsTr("Activity type picker")
                Accessible.description: qsTr("Select the activity type to filter by")

                delegate: ItemDelegate {
                    id: activitiesFilterMediumSelectorDelg
                    width: ListView.view.width
                    text: activitiesFilterMediumSelectorDelg.label
                    font.family: activitiesFilterMediumSelector.font.family
                    font.weight: activitiesFilterMediumSelector.font.weight
                    font.pixelSize: activitiesFilterMediumSelector.font.pixelSize

                    Accessible.role: Accessible.ListItem
                    Accessible.name: activitiesFilterMediumSelectorDelg.label
                    Accessible.description: qsTr("Currently selected activity type")
                    Accessible.focusable: true

                    required property string label
                }
            },

            HeaderIconButton {
                id: showActivitiesSearchButton
                iconSource: activitiesHeading.searchVisible ? Icons.mobileCloseApp : Icons.systemSearch
                accessiblePurpose: activitiesHeading.searchVisible ? qsTr("Hide activities search") : qsTr("Show activities search")
                Layout.alignment: Qt.AlignVCenter
                onClicked: () => {
                    if (activitiesHeading.searchVisible) {
                        activitiesHeading.searchVisible = false
                        activitiesSearchField.text = ""
                    } else {
                        activitiesHeading.searchVisible = true
                        activitiesSearchField.giveFocus()
                    }
                }
            }
            ]
        }

        ActivitiesList {
            id: activitiesList
            clip: true
            limit: 100
            proxyModel {
                filterText: activitiesSearchField.text.trim()
                mediumFilter: activitiesFilterMediumSelector.currentValue
            }
            anchors {
                top: activitiesHeading.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
        }
    }
}
