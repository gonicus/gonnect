pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import base

BaseWidget {
    id: control
    minCellWidth: 20
    minCellHeight: 15

    // Unread chat counts are reported here, so the ChatWidget is configured to report zero
    // notifications for its own badge.
    notifications: SIPCallManager.missedCalls + ChatConnectorManager.unreadNotificationsCount

    Rectangle {
        id: activitiesWidget
        parent: control.root
        color: "transparent"
        anchors.fill: parent

        CardHeading {
            id: activitiesHeading
            text: qsTr("Activities")
            anchors {
                left: parent.left
                right: parent.right
            }
        }

        ActivitiesList {
            id: activitiesList
            clip: true
            limit: 100
            anchors {
                top: activitiesHeading.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
        }
    }
}
