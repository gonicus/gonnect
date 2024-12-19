import QtQuick
import QtQuick.Controls.Material
import base

TextField {
    id: control
    echoMode: TextInput.Password
    validator: RegularExpressionValidator {
        regularExpression: control.regex
    }

    readonly property string value: ViewHelper.encryptSecret(control.text)
    property var regex
}
