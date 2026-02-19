import QtQuick
import QtQuick.Controls.Material

TextField {
    id: control
    validator: RegularExpressionValidator {
        regularExpression: control.regex
    }

    property string templateFieldName

    readonly property alias value: control.text
    property var regex

    Accessible.role: Accessible.EditableText
    Accessible.name: qsTr("Text input")
    Accessible.description: qsTr("Enter the desired value for the ") + control.templateFieldName
    Accessible.focusable: true
}
