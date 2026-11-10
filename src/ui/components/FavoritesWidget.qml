pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import base

BaseWidget {
    id: control

    property bool showPhoneFavorites: true
    property bool showConferenceFavorites: true
    property bool showChatFavorites: true

    onAdditionalSettingsLoaded: () => {
                                    const phone = control.config.get("showPhoneFavorites").trim()
                                    if (phone !== "") {
                                        control.showPhoneFavorites = phone === "true"
                                    }

                                    const conference = control.config.get("showConferenceFavorites").trim()
                                    if (conference !== "") {
                                        control.showConferenceFavorites = conference === "true"
                                    }

                                    const chat = control.config.get("showChatFavorites").trim()
                                    if (chat !== "") {
                                        control.showChatFavorites = chat === "true"
                                    }
                                }

    Rectangle {
        id: favWidget
        parent: control.root
        color: "transparent"
        anchors {
            centerIn: parent
            fill: parent
        }

        CardHeading {
            id: favHeading
            visible: true
            text: qsTr("Favorites")
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }

            actions: [
                CardHeadingMoreMenuButton {
                    id: filterHeadingButton
                    iconSource: Icons.viewSort
                    width: 22
                    onClicked: () => {
                                   const popup = filterMenuComponent.createObject(filterHeadingButton)
                                   const contentItem = filterHeadingButton.Window.window.contentItem
                                   const globalPos = contentItem.mapFromItem(filterHeadingButton, 0, filterHeadingButton.height)

                                   const openLeft = globalPos.x > contentItem.width / 2
                                   const openUp = globalPos.y > contentItem.height / 2

                                   popup.x = openLeft ? (-popup.width + filterHeadingButton.width) : 0
                                   popup.y = openUp ? -popup.height - filterHeadingButton.height : filterHeadingButton.height
                                   popup.open()
                               }
                }
            ]

            Component {
                id: filterMenuComponent

                Popup {
                    id: filterMenuPopup
                    contentWidth: Math.max(
                                      showPhoneCheckBox.implicitWidth,
                                      showConferenceCheckBox.implicitWidth,
                                      showChatCheckBox.implicitWidth
                                  ) + Theme.d * 2
                    contentHeight: filterCol.implicitHeight
                    onClosed: () => filterMenuPopup.destroy()

                    Column {
                        id: filterCol

                        CheckBox {
                            id: showPhoneCheckBox
                            text: qsTr("Phone")
                            checked: control.showPhoneFavorites
                            leftPadding: Theme.d

                            onToggled: () => {
                                           control.showPhoneFavorites = showPhoneCheckBox.checked
                                           control.config.set("showPhoneFavorites", showPhoneCheckBox.checked)
                                       }
                        }
                        CheckBox {
                            id: showConferenceCheckBox
                            text: qsTr("Conference")
                            checked: control.showConferenceFavorites
                            leftPadding: Theme.d

                            onToggled: () => {
                                           control.showConferenceFavorites = showConferenceCheckBox.checked
                                           control.config.set("showConferenceFavorites", showConferenceCheckBox.checked)
                                       }
                        }
                        CheckBox {
                            id: showChatCheckBox
                            text: qsTr("Chat")
                            checked: control.showChatFavorites
                            leftPadding: Theme.d

                            onToggled: () => {
                                           control.showChatFavorites = showChatCheckBox.checked
                                           control.config.set("showChatFavorites", showChatCheckBox.checked)
                                       }
                        }
                    }

                }
            }
        }

        FavoritesList {
            id: favList
            header: null
            visible: true
            showPhoneFavorites: control.showPhoneFavorites
            showConferenceFavorites: control.showConferenceFavorites
            showChatFavorites: control.showChatFavorites
            anchors {
                top: favHeading.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom

                leftMargin: 10
                rightMargin: 10
            }
        }

        Label {
            id: favInfo
            color: Theme.secondaryTextColor
            visible: favList.count > 0 ? false : true
            text: qsTr("No favorites to display")
            wrapMode: Label.Wrap
            width: favList.width
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors {
                centerIn: favList
            }
        }
    }
}
