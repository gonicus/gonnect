import QtQuick
import QtQuick.Controls.Material

TextField {
    id: control
    validator: RegularExpressionValidator {
        regularExpression: control.regex
    }

    readonly property alias value: control.text
    property var regex
}
