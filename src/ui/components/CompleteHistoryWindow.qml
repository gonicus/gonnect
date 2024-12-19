pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

BaseWindow {
    id: control
    objectName: "historyWindow"
    width: 1200
    height: 800
    title: qsTr("History")
    resizable: true

    minimumWidth: 1200
    minimumHeight: 400

    Item {
        anchors.fill: parent

        Item {
            id: searchBox
            height: 60
            anchors {
                top: parent.top
                left: parent.left
                right: filterButton.left
                topMargin: 20
                leftMargin: 20
                rightMargin: 20
            }

            Rectangle {
                id: background
                color: 'transparent'
                radius: 6
                border.width: 1
                border.color: Theme.borderColor
                anchors.fill: parent
            }

            IconLabel {
                id: searchIcon
                icon {
                    source: Icons.systemSearch
                    width: 20
                    height: 20
                }
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left
                    leftMargin: 20
                }
            }

            Label {
                id: placeholderLabel
                text: qsTr("Number or contact")
                color: Theme.secondaryTextColor
                visible: !searchInputField.text
                font.pixelSize: 18
                anchors {
                    left: searchInputField.left
                    right: searchInputField.left
                    verticalCenter: searchInputField.verticalCenter
                }
            }

            TextInput {
                id: searchInputField
                font.pixelSize: 18
                color: Theme.primaryTextColor
                anchors {
                    left: searchIcon.right
                    leftMargin: 20
                    right: clearButtonContainer.left
                    verticalCenter: parent.verticalCenter
                }

                onFocusChanged: (isFocused) => {
                    if (isFocused) {
                        searchInputField.selectAll()
                    }
                }

                onTextEdited: () => historyList.proxyModel.filterText = searchInputField.text.trim()

                Keys.onEscapePressed: () => {
                                          searchInputField.text = ""
                                          historyList.proxyModel.filterText = ""
                                      }
            }

            Item {
                id: clearButtonContainer
                width: 60
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    right: parent.right
                }

                IconLabel {
                    id: clearButton
                    visible: searchInputField.text !== ""
                    icon {
                        source: Icons.editClear
                        width: 20
                        height: 20
                        color: clearButtonHoveredHandler.hovered ? Theme.primaryTextColor : Theme.secondaryTextColor
                    }
                    anchors {
                        verticalCenter: parent.verticalCenter
                        right: parent.right
                        rightMargin: 20
                    }

                    Behavior on color { ColorAnimation {} }
                }

                HoverHandler {
                    id: clearButtonHoveredHandler
                }

                TapHandler {
                    onTapped: () => {
                                  searchInputField.text = ""
                                  historyList.proxyModel.filterText = ""
                              }
                }
            }
        }

        Rectangle {
            id: filterButton
            width: 2 * filterButton.height
            radius: 6
            color: filterButtonHoveredHandler.hovered ? Theme.backgroundOffsetHoveredColor : Theme.backgroundOffsetColor

            anchors {
                right: parent.right
                rightMargin: 20
                top: searchBox.top
                bottom: searchBox.bottom
            }

            Behavior on color { ColorAnimation {} }

            HoverHandler {
                id: filterButtonHoveredHandler
            }

            TapHandler {
                onTapped: () => filterMenu.open()
            }

            Label {
                anchors.centerIn: parent
                text: {
                    switch (historyList.proxyModel.typeFilter) {
                    case HistoryProxyModel.TypeFilter.ALL:
                        return qsTr('All')
                    case HistoryProxyModel.TypeFilter.INCOMING:
                        return qsTr('Incoming')
                    case HistoryProxyModel.TypeFilter.OUTGOING:
                        return qsTr('Outgoing')
                    case HistoryProxyModel.TypeFilter.MISSED:
                        return qsTr('Missed')
                    }
                }
            }

            component FilterMenuItem : MenuItem {
                id: filterMenuItem
                text: qsTr('All')
                checkable: true
                checked: historyList.proxyModel.typeFilter === filterMenuItem.typeFilter
                indicator: RadioButton {
                    checked: filterMenuItem.checked
                    padding: 0
                    x: filterMenuItem.text ? (filterMenuItem.mirrored ? filterMenuItem.width - width - filterMenuItem.rightPadding : filterMenuItem.leftPadding) : filterMenuItem.leftPadding + (filterMenuItem.availableWidth - width) / 2
                    y: filterMenuItem.topPadding + (filterMenuItem.availableHeight - height) / 2
                }

                property int typeFilter

                onClicked: () => historyList.proxyModel.typeFilter = filterMenuItem.typeFilter
            }

            Menu {
                id: filterMenu
                y: filterButton.height

                FilterMenuItem {
                    text: qsTr('All')
                    typeFilter: HistoryProxyModel.TypeFilter.ALL
                }
                FilterMenuItem {
                    text: qsTr('Incoming')
                    typeFilter: HistoryProxyModel.TypeFilter.INCOMING
                }
                FilterMenuItem {
                    text: qsTr('Outgoing')
                    typeFilter: HistoryProxyModel.TypeFilter.OUTGOING
                }
                FilterMenuItem {
                    text: qsTr('Missed')
                    typeFilter: HistoryProxyModel.TypeFilter.MISSED
                }
            }
        }

        Rectangle {
            id: topBorder
            visible: historyList.hasPastCalls
            height: 1
            color: Theme.borderColor
            anchors {
                top: searchBox.bottom
                left: parent.left
                right: parent.right
                topMargin: 20
            }
        }

        HistoryList {
            id: historyList
            showScrollBar: true
            anchors {
                top: topBorder.bottom
                left: parent.left
                right: parent.right
                bottom: bottomBorder.top
                leftMargin: 20
            }
        }

        Rectangle {
            id: bottomBorder
            visible: historyList.hasPastCalls

            implicitHeight: 30
            color: Theme.paneColor
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }

            Rectangle {
                height: 1
                color: Theme.borderColor
                anchors {
                    top: bottomBorder.top
                    left: parent.left
                    right: parent.right
                }
            }

            Label {
                id: countLabel
                text: qsTr('%n entries found', "", historyList.count)
                verticalAlignment: Label.AlignVCenter
                anchors {
                    top: bottomBorder.top
                    bottom: parent.bottom
                    left: parent.left
                    leftMargin: 20
                }
            }
        }
    }
}
