pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

Item {
    id: control
    implicitWidth: voicemailRow.width
    implicitHeight: 30

    // Only display the icon if the widget heading is not long enough
    property double maxWidth: 0
    Connections {
        target: parent
        function onWidthChanged() {
            maxWidth = parent.width / 4
        }
    }

    property int totalVoicemailCount: control.messagesWaitingWithUnknownCount ? 1 : control.realVoicemailCount

    property int realVoicemailCount: control.newVoicemailCount + control.oldVoicemailCount
    property int newVoicemailCount: SIPAccountManager.newVoiceMessageCount
    property int oldVoicemailCount: SIPAccountManager.oldVoiceMessageCount

    property bool messagesWaitingWithUnknownCount: control.messagesWaiting && control.realVoicemailCount === 0
    property bool messagesWaiting: SIPAccountManager.messagesWaiting

    property bool hasVoicemail: control.realVoicemailCount > 0 || control.messagesWaiting
    property bool hasNewVoicemail: control.newVoicemailCount > 0 || control.messagesWaitingWithUnknownCount

    Accessible.role: Accessible.Button
    Accessible.name: qsTr("Listen to voicemail")
    Accessible.onPressAction: () => SIPAccountManager.callVoiceBox("account0")

    RowLayout {
        id: voicemailRow
        spacing: 8
        anchors.verticalCenter: parent.verticalCenter

        IconLabel {
            id: voicemailIcon
            icon {
                source: Icons.callVoicemail
                width: 16
                height: 16
            }

            Rectangle {
                id: redDot
                visible: control.hasNewVoicemail
                color: Theme.redColor
                width: 6
                height: redDot.width
                radius: redDot.width / 2
                anchors {
                    verticalCenter: voicemailIcon.bottom
                    horizontalCenter: voicemailIcon.right
                }
            }

            Accessible.ignored: true
        }

        Label {
            id: voicemailCount
            text: control.messagesWaitingWithUnknownCount
                  ? qsTr("New voice mail")
                  : control.hasNewVoicemail
                    ? qsTr("%n new voice mail(s)", "", control.newVoicemailCount)
                    : qsTr("%n old voice mail(s)", "", control.oldVoicemailCount)
            font.pixelSize: 16
            font.weight: Font.Medium
            elide: Text.ElideRight
            color: Theme.secondaryTextColor
            visible: (voicemailIcon.implicitWidth + voicemailCount.implicitWidth) < control.maxWidth

            Accessible.ignored: true
        }
    }

    HoverHandler {
        id: clearButtonHoverHandler
        cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
        onTapped: () => SIPAccountManager.callVoiceBox("account0")
    }
}
