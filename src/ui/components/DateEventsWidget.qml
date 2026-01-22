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
        }

        property bool isInitializing: true

        Component.onCompleted: () => dateStatus.start()

        CardHeading {
            id: dateHeading
            visible: true
            text: qsTr("Appointments")
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
            visible: !dateList.count > 0
            text: dateWidget.isInitializing ? qsTr("Loading appointments...")
                                            : qsTr("No upcoming appointments")
            wrapMode: Label.Wrap
            width: dateList.width
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors {
                centerIn: dateList
            }
        }

        Timer {
            id: dateStatus
            interval: 30000
            repeat: false

            onTriggered: {
                dateWidget.isInitializing = false
            }
        }
    }
}
