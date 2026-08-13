pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Menu {
    id: control

    property bool inputDevices: false
    property string selectedDeviceId

    signal deviceSelected(string deviceId)

    function updateWidth() {
        let w = 0

        for (let i = 0; i < deviceMenuInstantiator.count; ++i) {
            w = Math.max(w, deviceMenuInstantiator.objectAt(i).implicitWidth)
        }

        control.width = w
    }

    Instantiator {
        id: deviceMenuInstantiator
        model: AudioManager.devices.filter(device => device.isInput === control.inputDevices)
        delegate: MenuItem {
            id: delg
            text: delg.name || qsTr("Default")
            icon.source: control.selectedDeviceId === delg.id ? Icons.checkbox : ""

            required property string id
            required property string name

            onTriggered: () => control.deviceSelected(delg.id)

            Accessible.role: Accessible.MenuItem
            Accessible.name: delg.text
            Accessible.focusable: true
            Accessible.onPressAction: () => control.deviceSelected(delg.id)
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
}
