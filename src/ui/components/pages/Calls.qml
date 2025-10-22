pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import Qt5Compat.GraphicalEffects
import base

Item {
    id: control

    states: [
        State {
            when: favList.count > 0 || dateList.count > 0
            AnchorChanges {
                target: historyCard
                anchors.right: verticalDragbarDummy.left
            }
            PropertyChanges {
                historyCard.anchors.rightMargin: 0
                verticalDragbarDummy.visible: true
                verticalDragbarDummy.x: settings.callsVerticalDividerRatio * control.width
                favCard.visible: true
                favCard.anchors.leftMargin: 0
            }
        }
    ]

    Settings {
        id: settings
        location: ViewHelper.userConfigPath
        category: "generic"

        property real callsVerticalDividerRatio: 3 / 4
    }

    Card {
        id: historyCard
        width: control.width
        anchors {
            left: parent.left
            top: parent.top
            bottom: togglerList.visible ? togglerList.top : parent.bottom
            right: parent.right

            leftMargin: 24
            rightMargin: 24
            bottomMargin: togglerList.visible ? 5 : 24
        }

        CardHeading {
            id: historyCardHeading
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
            }

            HeaderIconButton {
                id: showHistorySearchButton
                iconSource: historyCardHeading.searchVisible ? Icons.mobileCloseApp : Icons.systemSearch
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: parent.right
                    rightMargin: 20
                }

                onClicked: () => {
                    if (historyCardHeading.searchVisible) {
                        historyCardHeading.searchVisible = false
                        historySearchField.text = ""
                    } else {
                        historyCardHeading.searchVisible = true
                        historySearchField.giveFocus()
                    }
                }
            }
        }

        HistoryList {
            id: historyList
            height: parent.height
            rightPadding: 20
            proxyModel {
                filterText: historySearchField.text.trim()
                typeFilter: historyFilterTypeSelector.currentValue
                mediumFilter: historyFilterMediumSelector.currentValue
            }
            anchors {
                top: historyCardHeading.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                leftMargin: 20
            }
        }
    }

    TogglerList {
        id: togglerList
        clip: true
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom

            leftMargin: 10
            rightMargin: 10
            bottomMargin: 3
        }
    }

    Item {
        id: verticalDragbarDummy
        visible: false
        width: 24
        anchors {
            top: historyCard.top
            bottom: historyCard.bottom
        }

        HoverHandler {
            id: verticalDragbarDummyHoverHandler
            cursorShape: Qt.SplitHCursor
        }

        DragHandler {
            id: verticalDragbarDummyDragHandler
            yAxis.enabled: false
            xAxis {
                minimum: 1/2 * control.width
                maximum: control.width - 300
            }

            onActiveChanged: () => {
                if (!verticalDragbarDummyDragHandler.active) {  // Drag ended
                    const newRatio = verticalDragbarDummy.x / control.width
                    settings.callsVerticalDividerRatio = newRatio
                    verticalDragbarDummy.x = Qt.binding(() => newRatio * control.width)
                }
            }
        }
    }

    Card {
        id: favCard
        visible: false
        anchors {
            top: historyCard.top
            bottom: historyCard.bottom
            left: verticalDragbarDummy.right
            right: parent.right
            leftMargin: 24
            rightMargin: 24
        }

        property bool hasFavorites: false
        property bool hasDateEvents: false

        CardHeading {
            id: favCardHeading
            visible: !favCardTabBar.visible
            text: favCard.hasFavorites ? qsTr("Favorites") : qsTr("Conferences")
            anchors {
                left: parent.left
                right: parent.right
            }
        }

        TabBar {
            id: favCardTabBar
            visible: favCard.hasFavorites && favCard.hasDateEvents
            topLeftRadius: 12
            topRightRadius: 12
            anchors {
                left: parent.left
                right: parent.right
            }

            TabButton {
                id: favTabButton
                text: qsTr("Favorites")
                topLeftRadius: 12
            }
            TabButton {
                id: dateTabButton
                text: qsTr("Conferences")
                topRightRadius: 12
            }
        }

        FavoritesList {
            id: favList
            header: null
            visible: favCardTabBar.currentItem === favTabButton
                     || (favCardHeading.visible && favCard.hasFavorites)
            delegate: FavoriteListItemBig {}
            anchors {
                top: favCardTabBar.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                leftMargin: 10
                rightMargin: 10
            }
            onCountChanged: {
                favCard.hasFavorites = favList.count > 0
            }
        }

        DateEventsList {
            id: dateList
            visible: favCardTabBar.currentItem === dateTabButton
                     || (favCardHeading.visible && favCard.hasDateEvents)
            anchors {
                top: favCardTabBar.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                leftMargin: 10
                rightMargin: 10
            }
            onCountChanged: {
                favCard.hasDateEvents = dateList.count > 0
            }
        }
    }

    FirstAidButton {
        id: firstAidButton
        z: 100000
        anchors {
            verticalCenter: togglerList.verticalCenter
            right: favCard.right
            margins: 10
        }
    }
}
