pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import base

BaseWidget {
    id: control

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

            Accessible.role: Accessible.StaticText
            Accessible.name: favInfo.text
        }
    }
}
