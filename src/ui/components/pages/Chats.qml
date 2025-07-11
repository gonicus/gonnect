pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control

    Card {
        anchors {
            top: parent.top
            right: sideBar.left
            bottom: parent.bottom
            left: parent.left
            margins: 24
        }

        ListView {
            anchors.fill: parent
            model: SimpleSortProxyModel {
                sortRoleName: "name"
                // sourceModel: RoomsModel
            }
            delegate: Item {
                id: delg
                height: 30
                anchors {
                    left: parent?.left
                    right: parent?.right
                }

                required property string id
                required property string name
                required property int unreadMessageCount

                Label {
                    text: `${delg.name} (${delg.unreadMessageCount})`
                    anchors {
                        verticalCenter: parent.verticalCenter
                    }
                }
            }
        }
    }

    Card {
        id: sideBar
        width: control.width * 1 / 4
        anchors {
            top: parent.top
            right: parent.right
            bottom: parent.bottom
            margins: 24
        }

        CallSideBar {
            id: callSideBar
            selectedSideBarMode: CallSideBar.SideBarMode.AdditionalInfo
            anchors.fill: parent
        }
    }
}
