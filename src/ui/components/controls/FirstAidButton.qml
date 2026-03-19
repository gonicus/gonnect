import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

Button {
    id: control
    visible: GlobalInfo.hasEmergencyNumbers
    text: qsTr("First Aid")
    icon.source: "qrc:/icons/ISO_7010_E004" + qsTr("QT_CULTURAL_SPHERE", "QGuiApplication") + ".svg"
    highlighted: true

    Accessible.role: Accessible.Button
    Accessible.name: firstAidLabel.text
    Accessible.description: qsTr("Open first aid menu")
    Accessible.focusable: true
    Accessible.onPressAction: ViewHelper.showFirstAid()

    Material.accent: Theme.emergencyColor

    Component.onCompleted: () => {
        control.icon.width = 24
        control.icon.height = 24
    }

    onClicked: () => ViewHelper.showFirstAid()
}
