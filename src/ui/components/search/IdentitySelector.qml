pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

ComboBox {
    id: control
    editable: false
    textRole: 'displayName'
    valueRole: 'id'
    popup.width: control.width
    model: [
        {
            displayName: qsTr("Default"),
            id: "default"
        }, {
            displayName: qsTr("Auto"),
            id: "auto"
        }
    ].concat(SIPManager.preferredIdentities)

    background: Rectangle {
        anchors.fill: parent
        color: Theme.backgroundSecondaryColor
        radius: 5
        border {
            width: 1
            color: Theme.borderColor
        }
    }

    Accessible.role: Accessible.ComboBox
    Accessible.name: qsTr("Identity selection")
    Accessible.description: qsTr("Select the preferred identity to be used in calls")

    contentItem: Label {
        text: control.displayText
        wrapMode: Label.WordWrap
        font.pixelSize: 14
        maximumLineCount: 2
        elide: Label.ElideRight
        verticalAlignment: Label.AlignVCenter
        leftPadding: 10

        Accessible.role: Accessible.ListItem
        Accessible.name: control.displayText
        Accessible.description: qsTr("Currently selected identity")
        Accessible.focusable: true
    }

    function setDefaultIdentity() {
        const selectedId = SIPManager.defaultPreferredIdentity
        const model = control.model

        for (let i = 0; i < model.length; ++i) {
            if (model[i].id === selectedId) {
                control.currentIndex = i
                return
            }
        }

        control.currentIndex = -1
    }

    Component.onCompleted: () => control.setDefaultIdentity()

    Connections {
        target: SIPManager
        function onPreferredIdentitiesChanged() {
            control.setDefaultIdentity()
        }
    }
}
