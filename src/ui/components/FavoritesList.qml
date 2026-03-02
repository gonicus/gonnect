pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

ListView {
    id: control
    topMargin: 10
    clip: true
    model: FavoritesProxyModel {
        showJitsi: ViewHelper.isJitsiAvailable

        FavoritesModel {}
    }

    Accessible.role: Accessible.List
    Accessible.name: qsTr("Favorites")
    Accessible.description: qsTr("List of all contacts that have been marked as favorites")

    delegate: FavoriteListItemSmall {}
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
