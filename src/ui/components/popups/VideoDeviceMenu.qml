pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Menu {
    id: control

    property string selectedDeviceId

    signal deviceSelected(string deviceId)
    signal virtualBackgroundButtonClicked

    function updateWidth() {
        let w = 0

        for (let i = 0; i < deviceMenuInstantiator.count; ++i) {
            w = Math.max(w, deviceMenuInstantiator.objectAt(i).implicitWidth)
        }

        control.width = w
    }

    Instantiator {
        id: deviceMenuInstantiator
        model: VideoManager.devices
        delegate: MenuItem {
            id: delg
            text: delg.description
            icon.source: control.selectedDeviceId === delg.id ? Icons.checkbox : ""

            required property string id
            required property string description

            onTriggered: () => control.deviceSelected(delg.id)
        }

        onObjectAdded: (index, obj) => {
            control.insertItem(index, obj)
            control.updateWidth()
        }
        onObjectRemoved: (_, obj) => {
            control.removeItem(obj)
            control.updateWidth()
        }
    }

    MenuSeparator {}

    MenuItem {
        text: qsTr("Virtual background")
        onTriggered: () => control.virtualBackgroundButtonClicked()
    }
}
