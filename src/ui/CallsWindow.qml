import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

Item {
    id: control

    readonly property alias count: callList.count

    states: [
        State {
            when: SIPCallManager.isConferenceMode
            PropertyChanges {
                callList.interactive: false
                callList.showHangupButton: true
                callList.showHoldButton: false
                conferenceButton.visible: false
                conferenceLabel.visible: true
            }
        }

    ]

    CallList {
        id: callList
        clip: true
        showHangupButton: false
        anchors {
            top: parent.top
            bottom: conferenceButton.visible ? conferenceButton.top : parent.bottom
            bottomMargin: conferenceButton.visible ? 5 : 0
            left: parent.left
            right: separatorLine.left
        }
    }

    IconLabel {
        id: conferenceLabel
        visible: false
        spacing: 5
        icon.color: Theme.primaryTextColor
        color: Theme.primaryTextColor
        text: qsTr('Conference active')
        icon.source: Icons.showinfo
        anchors {
            horizontalCenter: callList.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 10
        }
    }

    IconLabel {
        id: conferenceButton
        visible: SIPCallManager.establishedCallsCount === 2
        spacing: 5
        icon.color: conferenceButtonHoverHandler.hovered
                    ? Theme.primaryTextColor
                    : Theme.secondaryTextColor
        color: conferenceButtonHoverHandler.hovered
               ? Theme.primaryTextColor
               : Theme.secondaryTextColor
        text: qsTr('Start conference...')
        icon.source: Icons.userGroupNew
        anchors {
            horizontalCenter: callList.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 10
        }

        Behavior on color { ColorAnimation {} }
        Behavior on icon.color { ColorAnimation {} }

        HoverHandler {
            id: conferenceButtonHoverHandler
        }

        TapHandler {
            onTapped: () => SIPCallManager.startConference()
        }
    }

    Rectangle {
        id: separatorLine
        width: 1
        color: Theme.borderColor
        anchors {
            top: parent.top
            bottom: parent.bottom

            left: parent.left
            leftMargin: 180
        }
    }

    CallDetails {
        id: callDetails
        callItem: callList.selectedItem
        anchors {
            top: parent.top
            bottom: parent.bottom
            right: parent.right
            left: separatorLine.right
        }
    }
}
