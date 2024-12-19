pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl
import QtQuick.Controls.impl
import base

Popup {
    id: control
    transformOrigin: Item.Top
    topMargin: 12
    bottomMargin: 12
    verticalPadding: 8
    enter: null  // Transitions seem to cause that the popup is not visible sometimes...
    exit: null
    visible: resultList.count > 0

    Material.accent: Theme.accentColor
    Material.theme: Theme.isDarkMode ? Material.Dark : Material.Light

    property int highlightedIndex: -1
    property string searchText
    signal numberSelected(string number, string contactId)

    contentItem: ListView {
        id: resultList
        clip: true
        implicitHeight: resultList.contentHeight
        highlightMoveDuration: 0
        currentIndex: control.highlightedIndex
        ScrollIndicator.vertical: ScrollIndicator { }
        model: SearchListModel {
            id: searchListModel
            searchPhrase: control.searchText

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
            required property string avatarPath
            required property string subscriptableNumber
            required property bool hasAvatar
            required property var numbers
            required property int numbersCount
            required property int numbersIndexOffset

            readonly property bool isFirst: delg.index === 0
            readonly property bool isLast: delg.index === resultList.count - 1

            property int buddyStatus: SIPBuddyState.UNKNOWN

            function updateBuddyStatus() {
                delg.buddyStatus = delg.subscriptableNumber !== 0
                        ? SIPManager.buddyStatus(delg.subscriptableNumber)
                        : SIPBuddyState.UNKNOWN
            }

            Component.onCompleted: () => delg.updateBuddyStatus()

            Connections {
                target: SIPManager
                enabled: delg.subscriptableNumber !== ""
                function onBuddyStateChanged(url : string, status : int) {
                    delg.updateBuddyStatus()
                }
            }

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

                Rectangle {
                    id: avatarNameRow
                    height: 30
                    radius: 2
                    color: (avatarNameHoverHandler.hovered) ? Theme.highlightColor : 'transparent'
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    AvatarImage {
                        id: avatarImage
                        initials: ViewHelper.initials(delg.name)
                        source: delg.hasAvatar ? ("file://" + delg.avatarPath) : ""
                        showBuddyStatus: delg.subscriptableNumber !== ""
                        buddyStatus: delg.buddyStatus
                        anchors {
                            left: parent.left
                            leftMargin: 13
                            verticalCenter: parent.verticalCenter
                        }
                    }

                    Label {
                        id: delgLabel
                        text: delg.name
                        elide: Label.ElideRight
                        font.weight: Font.Medium
                        anchors {
                            left: avatarImage.right
                            leftMargin: 10
                            right: parent.right
                            verticalCenter: avatarImage.verticalCenter
                        }
                    }

                    HoverHandler {
                        id: avatarNameHoverHandler
                        enabled: false
                    }

                    TapHandler {
                        enabled: false
                        grabPermissions: PointerHandler.TakeOverForbidden
                        gesturePolicy: TapHandler.WithinBounds
                        onTapped: () => {
                            // control.numberSelected(numberDelg.number, delg.id)
                            console.log('TODO')
                        }
                    }
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
                        radius: 2
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

                            TapHandler {
                                grabPermissions: PointerHandler.TakeOverForbidden
                                gesturePolicy: TapHandler.WithinBounds
                                onTapped: () => {
                                    ViewHelper.toggleFavorite(numberDelg.number)
                                }
                            }
                        }

                        HoverHandler {
                            id: hoverHandler
                        }

                        TapHandler {
                            grabPermissions: PointerHandler.TakeOverForbidden
                            gesturePolicy: TapHandler.WithinBounds
                            onTapped: () => {
                                control.numberSelected(numberDelg.number, delg.id)
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

