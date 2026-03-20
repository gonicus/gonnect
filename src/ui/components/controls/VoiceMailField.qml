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

    property string accountId: ""

    property int newVoicemailCount: 0
    property int oldVoicemailCount: 0

    property bool hasNewVoicemail: control.newVoicemailCount > 0
    property bool hasVoicemail: control.hasNewVoicemail || control.oldVoicemailCount > 0

    Accessible.role: Accessible.Button
    Accessible.name: qsTr("Listen to voicemail")
    Accessible.onPressAction: () => SIPAccountManager.callVoiceBox(control.accountId)

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
            text: control.hasNewVoicemail
                  ? qsTr("%n new voice mail(s)", "", control.newVoicemailCount)
                  : qsTr("%n old voice mail(s)", "", control.oldVoicemailCount)
            font.pixelSize: 16
            font.weight: Font.Medium
            elide: Text.ElideRight
            color: Theme.secondaryTextColor

            Accessible.ignored: true
        }
    }

    Connections {
        target: SIPAccountManager
        function onVoiceMessagesWaitingChanged(accountId : string, voiceNew : int, voiceOld : int) {
            if (control.accountId !== accountId) {
                control.accountId = accountId
            }

            if (control.newVoicemailCount !== voiceNew) {
                control.newVoicemailCount = voiceNew
            }

            if (control.oldVoicemailCount !== voiceOld) {
                control.oldVoicemailCount = voiceOld
            }
        }
    }

    HoverHandler {
        id: clearButtonHoverHandler
        cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
        onTapped: () => SIPAccountManager.callVoiceBox(control.accountId)
    }
}
