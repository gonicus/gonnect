pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Menu {
    id: control

    property JitsiConnector jitsiConnector
    property alias typeFilter: jitsiDevicesModel.typeFilter
    property string selectedDeviceId

    signal deviceSelected(string deviceId)

    function updateWidth() {
        let w = 0

        for (let i = 0; i < videoDeviceMenuInstantiator.count; ++i) {
            w = Math.max(w, videoDeviceMenuInstantiator.objectAt(i).implicitWidth)
        }

        control.width = w
    }

    Instantiator {
        id: videoDeviceMenuInstantiator
        model: JitsiDevicesModel {
            id: jitsiDevicesModel
            jitsiConnector: control.jitsiConnector
        }
        delegate: MenuItem {
            id: delg
            text: delg.label || qsTr("Default")
            icon.source: control.selectedDeviceId === delg.deviceId ? Icons.checkbox : ""

            required property string deviceId
            required property string label

            onTriggered: () => control.deviceSelected(delg.deviceId)
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
