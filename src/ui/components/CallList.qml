pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Universal
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

    ListView {
        id: callListView
        anchors.fill: parent
        model: CallsProxyModel { CallsModel {} }

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
}
