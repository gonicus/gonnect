pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

ListView {
    id: control
    topMargin: 10
    clip: true

    property bool showPhoneFavorites: true
    property bool showConferenceFavorites: true
    property bool showChatFavorites: true

    model: FavoritesProxyModel {
        showPhone: control.showPhoneFavorites
        showJitsi: ViewHelper.isJitsiAvailable && control.showConferenceFavorites
        showChatRooms: ChatConnectorManager.isChatAvailable && control.showChatFavorites

        FavoritesModel {}
    }

    delegate: FavoriteListItemBig {
        isCompactMode: control.width < 300
    }

    Accessible.role: Accessible.List
    Accessible.name: qsTr("Favorites")
    Accessible.description: qsTr("List of all contacts that have been marked as favorites")

    header: Rectangle {
        id: headerItem
        radius: 4
        height: 25
        color: Theme.backgroundOffsetColor
        anchors {
            left: parent?.left
            right: parent?.right
        }

        Accessible.role: Accessible.Heading
        Accessible.name: favLabel.text

        Label {
            id: favLabel
            text: qsTr("Favorites")
            elide: Label.ElideRight
            anchors {
                left: parent.left
                right: parent.right
                verticalCenter: parent.verticalCenter
                margins: 10
            }

            Accessible.ignored: true
        }
    }
}
