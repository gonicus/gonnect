pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import base

ComboBox {
    id: control
    valueRole: "value"
    textRole: "text"

    model: ListModel {
        ListElement {
            value: IChatRoom.JoinRule.Invite
            text: qsTr("Invite")
        }
        ListElement {
            value: IChatRoom.JoinRule.Knock
            text: qsTr("Knock")
        }
        ListElement {
            value: IChatRoom.JoinRule.Public
            text: qsTr("Public")
        }
    }
}
