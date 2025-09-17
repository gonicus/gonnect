pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import base

BaseWidget {
    id: control

    Rectangle {
        id: dateWidget
        parent: control.root
        color: "transparent"
        anchors {
            centerIn: parent
            fill: parent
            bottomMargin: 15
        }

        CardHeading {
            id: dateHeading
            visible: true
            text: qsTr("Conferences")
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }
        }

        DateEventsList {
            id: dateList
            visible: true
            anchors {
                top: dateHeading.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom

                leftMargin: 10
                rightMargin: 10
            }
        }

        Label {
            id: dateInfo
            color: Theme.secondaryTextColor
            visible: dateList.count > 0 ? false : true
            text: qsTr("No upcoming conferences to display")
            wrapMode: Label.Wrap
            width: dateList.width
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors {
                centerIn: dateList
            }
        }
    }
}
