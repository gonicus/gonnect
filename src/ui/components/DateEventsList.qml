pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

ListView {
    id: control
    topMargin: 20
    clip: true
    model: DateEventsModel {}
    section.property: "date"
    section.delegate: Rectangle {
        id: sectionDelg
        radius: 4
        height: 25
        color: Theme.backgroundOffsetColor
        anchors {
            left: parent?.left
            right: parent?.right
        }

        required property date section

        Label {
            anchors.centerIn: parent
            text: {
                if (ViewHelper.isToday(sectionDelg.section)) {
                    return qsTr("Today - %1").arg(sectionDelg.section.toLocaleDateString(Qt.locale(), qsTr("yyyy/MM/dd")))
                }
                if (ViewHelper.isTomorrow(sectionDelg.section)) {
                    return qsTr("Tomorrow - %1").arg(sectionDelg.section.toLocaleDateString(Qt.locale(), qsTr("yyyy/MM/dd")))
                }
                return sectionDelg.section.toLocaleDateString(Qt.locale(), qsTr("dddd - yyyy/MM/dd"))
            }
        }
    }

    QtObject {
        id: internal

        signal minuteTick

        property date lastCheckedTime: new Date()

        readonly property Timer minuteTimer: Timer {
            running: control.count > 0
            repeat: true
            interval: 5000

            onTriggered: () => {
                const now = new Date()
                if (internal.lastCheckedTime.getMinutes() !== now.getMinutes()) {
                    internal.lastCheckedTime = now
                    internal.minuteTick()
                }
            }
        }
    }


    delegate: Item {
        id: delg
        implicitHeight: innerCol.implicitHeight
        anchors {
            left: parent?.left
            right: parent?.right
        }

        required property date dateTime
        required property date endDateTime
        required property string summary
        required property string roomName
        required property bool isConfirmed

        property bool isToday
        property bool isInPast
        property bool isCurrentlyTakingPlace
        property int minutesRemainingToStart
        property int minutesRemainingToEnd

        Component.onCompleted: () => delg.updateProps()

        function updateProps() {
            const now = new Date()

            delg.minutesRemainingToStart = Math.ceil((delg.dateTime - now) / 60000)
            delg.minutesRemainingToEnd = Math.ceil((delg.endDateTime - now) / 60000)
            delg.isToday = ViewHelper.isToday(delg.dateTime)
            delg.isInPast = (delg.endDateTime.getTime() + 60000) < now.getTime()
            delg.isCurrentlyTakingPlace = delg.dateTime.getTime() < now.getTime() && now.getTime() < delg.endDateTime.getTime()
        }

        Connections {
            target: internal
            function onMinuteTick() {
                delg.updateProps()
            }
        }

        Rectangle {
            id: rowBackground
            anchors.fill: parent
            radius: 4
            color: rowHoverHandler.hovered ? Theme.backgroundOffsetHoveredColor : 'transparent'
        }

        Column {
            id: innerCol
            enabled: !delg.isInPast
            topPadding: 10
            bottomPadding: 10
            anchors {
                left: parent.left
                right: parent.right
                leftMargin: 10
                rightMargin: 10
            }

            Label {
                id: summaryLabel
                text: delg.summary
                elide: Label.ElideRight
                font.weight: delg.isCurrentlyTakingPlace ? Font.Medium : Font.Normal
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }

            Row {
                spacing: 5
                height: timeLabel.implicitHeight

                Label {
                    id: timeLabel
                    font.weight: summaryLabel.font.weight
                    text: delg.dateTime.toLocaleTimeString(Qt.locale(), qsTr("hh:mm"))
                }

                Label {
                    id: remainingMinutesLabel
                    visible: delg.isToday && !delg.isInPast
                             && ((delg.minutesRemainingToStart >= 0 && delg.minutesRemainingToStart < 120)
                                 || delg.isCurrentlyTakingPlace)
                    font.weight: summaryLabel.font.weight
                    color: Theme.secondaryTextColor
                    text: "(" + (delg.isCurrentlyTakingPlace
                          ? qsTr("till %1").arg(delg.endDateTime.toLocaleTimeString(Qt.locale(), qsTr("hh:mm")))
                          : qsTr("in %1").arg(ViewHelper.minutesToNiceText(delg.minutesRemainingToStart))) + ")"
                }
            }
        }

        Component {
            id: dateEventContextMenuComponent

            Menu {
                Action {
                    text: qsTr('Join')
                    onTriggered: () => ViewHelper.requestMeeting(delg.roomName)
                }

                Action {
                    text: qsTr('Copy room link')
                    onTriggered: () => ViewHelper.copyToClipboard(delg.roomName)
                }
            }
        }

        HoverHandler {
            id: rowHoverHandler
            enabled: control.enabled
        }

        TapHandler {
            enabled: control.enabled
            gesturePolicy: TapHandler.WithinBounds
            grabPermissions: PointerHandler.ApprovesTakeOverByAnything
            exclusiveSignals: TapHandler.SingleTap | TapHandler.DoubleTap
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onDoubleTapped: () => {
                ViewHelper.requestMeeting(delg.roomName)
            }
            onTapped: (_, mouseButton) => {
                if (mouseButton === Qt.RightButton) {
                    dateEventContextMenuComponent.createObject(delg).popup()
                }
            }
        }
    }
}
