pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import base

BaseWidget {
    id: control

    Rectangle {
        id: favWidget
        parent: control.widget
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
        }

        FavoritesList {
            id: favList
            header: null
            visible: true
            delegate: FavoriteListItemBig {}
            anchors {
                top: favHeading.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom

                leftMargin: 10
                rightMargin: 10
            }
        }
    }
}
