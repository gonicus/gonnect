pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

Item {
    id: control

    property CallItem selectedItem
    property bool interactive: true
    property bool showHangupButton: true
    property bool showHoldButton: true

    onCountChanged: () => {
                        if (control.count === 1) {
                            control.selectedItem = callListView.itemAtIndex(0)
                        }
                    }

    readonly property alias count: callListView.count

    Label {
        color: Theme.secondaryTextColor
        visible: control.count > 1
        text: qsTr("Drag callers onto each other to transfer call")
        wrapMode: Label.Wrap
        horizontalAlignment: Label.AlignHCenter
        anchors {
            left: parent.left
            right: parent.right
            leftMargin: 25
            rightMargin: 25
            verticalCenter: parent.verticalCenter
            verticalCenterOffset: callListView.contentHeight / 2
        }
    }

    ListView {
        id: callListView
        model: CallsProxyModel {
            hideIncomingSecondaryCallOnBusy: true

            CallsModel { }
        }
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: conferenceButton.visible ? conferenceButton.top : parent.bottom
        }

        delegate: CallItem {
            id: callDelegate
            selected: control.interactive && callDelegate === control.selectedItem
            interactive: control.interactive
            showHangupButton: control.showHangupButton
            showHoldButton: control.showHoldButton
            anchors {
                left: parent?.left
                right: parent?.right
            }

            onClicked: () => control.selectedItem = callDelegate

            Component.onCompleted: () => control.selectedItem = callDelegate
        }
    }

    Item {
        id: conferenceButton
        visible: !SIPCallManager.isConferenceMode && SIPCallManager.establishedCallsCount === 2
        implicitHeight: conferenceButtonLabel.implicitHeight + 2 * 10
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        Rectangle {
            height: 1
            color: Theme.borderColor
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
            }
        }

        IconLabel {
            id: conferenceButtonLabel
            anchors.centerIn: parent
            color: conferenceButtonHoverHandler.hovered ? Theme.foregroundHeaderIcons : Theme.secondaryTextColor
            text: qsTr("Create conference")
            spacing: 6
            icon {
                source: Icons.userGroupNew
                color: conferenceButtonHoverHandler.hovered ? Theme.foregroundHeaderIcons : Theme.secondaryTextColor
            }
        }

        HoverHandler {
            id: conferenceButtonHoverHandler
        }

        TapHandler {
            onTapped: () => SIPCallManager.startConference()
        }
    }
}
