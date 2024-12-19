pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl
import QtQuick.Controls.impl
import base

Item {
    id: control
    implicitHeight: searchInputField.implicitHeight

    signal numberSelected(string number, string contactId)
    signal escapePressed()

    property int highlightedIndex: -1
    readonly property string text: searchInputField.text.trim()

    function activate() {
        searchInputField.forceActiveFocus()
        searchInputField.selectAll()
    }

    function clear() {
        searchInputField.text = ""
    }

    TextInput {
        id: searchInputField
        font.pixelSize: 18
        color: Theme.primaryTextColor
        anchors {
            left: parent.left
            right: parent.right
        }

        onFocusChanged: (isFocused) => {
            if (isFocused) {
                searchInputField.selectAll()
            }
        }

        Keys.onEnterPressed: () => {
            internal.dialHighlightedNumber()
        }

        Keys.onReturnPressed: () => {
            internal.dialHighlightedNumber()
        }

        Keys.onEscapePressed: () => {
            if (searchPopup.opened) {
                searchPopup.close()
            } else if (searchInputField.text.trim() != "") {
                searchInputField.text = ""
            } else {
                control.escapePressed()
            }
        }

        Keys.onDownPressed: () => {
            if (control.highlightedIndex < internal.searchListModel.totalNumbersCount - 1) {
                control.highlightedIndex++
            }
        }

        Keys.onUpPressed: () => {
            if (control.highlightedIndex > 0) {
                control.highlightedIndex--
            }
        }
    }

    Connections {
        target: SearchProvider
        function onActivateSearch(query) {
            searchInputField.text = query
        }
    }

    QtObject {
        id: internal

        property SearchListModel searchListModel: null

        function dialHighlightedNumber() {
            if (control.highlightedIndex >= 0) {
                const tel = internal.searchListModel.phoneNumberByIndex(control.highlightedIndex)
                if (tel === "") {
                    return;
                }

                const id = internal.searchListModel.contactIdByIndex(control.highlightedIndex)
                control.numberSelected(tel, id)
                control.highlightedIndex = -1
                searchInputField.text = ""
            } else {
                control.numberSelected(searchInputField.text, ViewHelper.contactIdByNumber(searchInputField.text))
                searchInputField.text = ""
            }
        }
    }

    SearchResultPopup {
        y: searchInputField.height
        width: searchInputField.width
        height: Math.min(contentItem.implicitHeight + verticalPadding * 2, control.Window.height  - control.y - control.height - topMargin - bottomMargin)
        topMargin: 12 + searchInputField.height
        highlightedIndex: control.highlightedIndex
        searchText: searchInputField.text

        Material.theme: Theme.isDarkMode ? Material.Dark : Material.Light
        Material.accent: control.Material.accent
        Material.primary: control.Material.primary

        contentItem: ListView {
            id: resultList
            clip: true
            implicitHeight: resultList.contentHeight
            highlightMoveDuration: 0
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator { }
            model: SearchListModel {
                id: searchListModel
                searchPhrase: searchInputField.text

                Component.onCompleted: () => {
                    internal.searchListModel = searchListModel
                }
            }
            delegate: Item {
                id: delg
                implicitHeight: delegateColumn.implicitHeight
                anchors {
                    left: parent?.left
                    right: parent?.right
                }

                required property int index
                required property string id
                required property string name
                required property string company
                required property var numbers
                required property int numbersCount
                required property int numbersIndexOffset

                readonly property bool isFirst: delg.index === 0
                readonly property bool isLast: delg.index === resultList.count - 1

                Rectangle {
                    visible: delg.index > 0
                    height: 1
                    color: Theme.borderColor
                    anchors {
                        top: parent.top
                        left: parent.left
                        right: parent.right
                    }
                }

                Column {
                    id: delegateColumn
                    topPadding: 12
                    bottomPadding: 12
                    spacing: 4
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    Label {
                        id: delgLabel
                        text: delg.name
                    }
                    Label {
                        font.pixelSize: delgLabel.font.pixelSize - 2
                        color: Theme.secondaryTextColor
                        text: delg.company
                        visible: delg.company !== ''
                    }

                    Repeater {
                        model: delg.numbers
                        delegate: Rectangle {
                            id: numberDelg
                            implicitHeight: numberDelgLabel.height
                            color: (numberDelg.highlightedByIndex || hoverHandler.hovered)
                                   ? Theme.highlightColor
                                   : 'transparent'
                            anchors {
                                left: parent?.left
                                right: parent?.right
                            }

                            required property int index
                            required property var modelData
                            readonly property int type: numberDelg.modelData.type
                            readonly property int numberIndex: delg.numbersIndexOffset + numberDelg.index
                            readonly property bool highlightedByIndex: control.highlightedIndex === numberDelg.numberIndex
                            readonly property string number: numberDelg.modelData.number
                            readonly property bool isSipStatusSubscriptable: numberDelg.modelData.isSipStatusSubscriptable
                            readonly property bool isFavorite: numberDelg.modelData.isFavorite
                            readonly property string typeIcon: {
                                switch (numberDelg.type) {
                                    case Contact.NumberType.Commercial:
                                        return Icons.actor
                                    case Contact.NumberType.Mobile:
                                        return Icons.smartphone
                                    case Contact.NumberType.Home:
                                        return Icons.goHome
                                    default:
                                        return ''
                                }
                            }

                            property int buddyStatus: SIPBuddyState.UNKNOWN

                            function updateBuddyStatus() {
                                numberDelg.buddyStatus = numberDelg.isSipStatusSubscriptable
                                        ? SIPManager.buddyStatus(numberDelg.number)
                                        : SIPBuddyState.UNKNOWN
                            }

                            Component.onCompleted: () => numberDelg.updateBuddyStatus()

                            Connections {
                                target: SIPManager
                                enabled: numberDelg.isSipStatusSubscriptable
                                function onBuddyStateChanged(url : string, status : int) {
                                    numberDelg.updateBuddyStatus()
                                }
                            }


                            IconLabel {
                                id: typeDelgLabel
                                visible: !numberDelg.isSipStatusSubscriptable
                                width: 16
                                icon.source: numberDelg.typeIcon
                                anchors {
                                    left: parent.left
                                    leftMargin: 17
                                    verticalCenter: parent.verticalCenter
                                }
                            }

                            BuddyStatusIndicator {
                                id: buddyStatusIndicator
                                visible: numberDelg.isSipStatusSubscriptable
                                status: numberDelg.buddyStatus
                                anchors {
                                    left: parent.left
                                    leftMargin: 20
                                    verticalCenter: parent.verticalCenter
                                }
                            }

                            Label {
                                id: numberDelgLabel
                                text: numberDelg.number
                                anchors {
                                    leftMargin: 40
                                    left: parent.left
                                    right: favIcon.left
                                    rightMargin: 20
                                }
                            }

                            FavIcon {
                                id: favIcon
                                isFavorite: numberDelg.isFavorite
                                anchors {
                                    right: parent.right
                                    verticalCenter: parent.verticalCenter
                                }

                                onToggled: () => ViewHelper.toggleFavorite(numberDelg.number)
                            }

                            HoverHandler {
                                id: hoverHandler
                            }

                            TapHandler {
                                grabPermissions: PointerHandler.TakeOverForbidden
                                gesturePolicy: TapHandler.WithinBounds
                                onTapped: () => {
                                    control.numberSelected(numberDelg.number, delg.id)
                                    control.highlightedIndex = -1
                                    searchInputField.text = ""
                                }
                            }
                        }
                    }
                }
            }
        }

        background: Rectangle {
            radius: 4
            color: parent.Material.dialogColor

            layer.enabled: control.enabled
            layer.effect: RoundedElevationEffect {
                elevation: 4
                roundedScale: Material.ExtraSmallScale
            }
        }
    }

}
