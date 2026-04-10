import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

Button {
    id: control
    visible: GlobalInfo.hasEmergencyNumbers
    text: qsTr("Emegency call")
    icon.source: "qrc:/icons/ISO_7010_E004" + ViewHelper.culturalSphereExtension + ".svg"
    highlighted: true

    padding: 0
    spacing: 6
    topPadding: 0
    leftPadding: 10
    rightPadding: 10
    bottomPadding: 0

    Accessible.role: Accessible.Button
    Accessible.name: control.text
    Accessible.description: qsTr("Open emergency call menu")
    Accessible.focusable: true
    Accessible.onPressAction: ViewHelper.showFirstAid()

    Material.accent: Theme.emergencyColor

    Component.onCompleted: () => {
        control.icon.width = 16
        control.icon.height = 16
    }

    onClicked: () => ViewHelper.showFirstAid()
}
