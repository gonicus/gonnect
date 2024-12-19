import QtQuick
import QtQuick.Controls.Material
import base

BaseWindow {
    id: control
    width: 410
    height: 554
    visible: true
    title: qsTr("Phone Number Transmission")
    resizable: false

    minimumWidth: control.width
    minimumHeight: control.height
    maximumWidth: control.width
    maximumHeight: control.height

    required property PreferredIdentity preferredIdentity
    property bool isNew: false

    onClosing: () => {
                   if (control.preferredIdentity && !internal.didUseButton && control.isNew) {
                       SIPManager.removePreferredIdentity(control.preferredIdentity)
                   }
               }

    readonly property QtObject privateObject: QtObject {
        id: internal

        /// If the user clicked one of the main buttons; needed to determin whether something to do on window closing
        property bool didUseButton: false

        property string originalDisplayName
        property string originalPrefix
        property string originalIdentity
        property bool originalEnabled
        property bool originalAuto
        property bool originalDefault

        readonly property bool displayNameModified: displayNameTextField.text.trim() !== internal.originalDisplayName
        readonly property bool prefixModified: prefixTextField.text.trim() !== internal.originalPrefix
        readonly property bool identityModified: identityTextField.text.trim() !== internal.originalIdentity
        readonly property bool enabledModified: enabledCheckBox.checked !== internal.originalEnabled
        readonly property bool autoModified: autoCheckBox.checked !== internal.originalAuto

        readonly property bool modified: internal.displayNameModified
                                         || internal.prefixModified
                                         || internal.identityModified
                                         || internal.enabledModified
                                         || internal.autoModified

        Component.onCompleted: () => {
                                   const pi = control.preferredIdentity
                                   internal.originalDisplayName = pi.displayName
                                   internal.originalPrefix = pi.prefix
                                   internal.originalIdentity = pi.identity
                                   internal.originalEnabled = pi.enabled
                                   internal.originalAuto = pi.automatic
                               }

        function save() {
            const pi = control.preferredIdentity

            if (internal.displayNameModified) {
                pi.displayName = displayNameTextField.text.trim()
            }
            if (internal.prefixModified) {
                pi.prefix = prefixTextField.text.trim()
            }
            if (internal.identityModified) {
                pi.identity = identityTextField.text.trim()
            }
            if (internal.autoModified) {
                pi.automatic = autoCheckBox.checked
            }
            if (internal.enabledModified) {
                pi.enabled = enabledCheckBox.checked

                if (!pi.enabled && SIPManager.defaultPreferredIdentity === control.preferredIdentity.id) {
                    SIPManager.defaultPreferredIdentity = "auto"
                }
            }
        }
    }

    Item {
        anchors.fill: parent

        Column {
            id: contentCol
            topPadding: 20
            bottomPadding: 20
            spacing: 20
            anchors {
                left: parent.left
                right: parent.right
                leftMargin: 20
                rightMargin: 20
            }

            TextField {
                id: displayNameTextField
                placeholderText: qsTr("Name")
                text: internal.originalDisplayName
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }

            TextField {
                id: prefixTextField
                placeholderText: qsTr("Prefix")
                text: internal.originalPrefix
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }

            TextField {
                id: identityTextField
                placeholderText: qsTr("Identity")
                text: internal.originalIdentity
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }

            Column {
                anchors {
                    left: parent.left
                    right: parent.right
                }

                CheckBox {
                    id: enabledCheckBox
                    text: qsTr("Enabled")
                    checked: internal.originalEnabled
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }

                CheckBox {
                    id: autoCheckBox
                    text: qsTr("Automatic")
                    checked: internal.originalAuto
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
            }
        }

        Button {
            id: deleteButton
            text: qsTr("Delete")
            highlighted: true
            icon.source: Icons.editDelete
            anchors {
                bottom: parent.bottom
                bottomMargin: 5
                left: parent.left
                leftMargin: 10
            }

            Material.accent: Theme.redColor

            onClicked: () => {
                           internal.didUseButton = true
                           if (control.preferredIdentity) {
                               SIPManager.removePreferredIdentity(control.preferredIdentity)
                           }
                           control.close()
                       }
        }

        Button {
            id: saveButton
            text: qsTr("Save")
            enabled: internal.modified && control.preferredIdentity?.isValid
            highlighted: true
            icon.source: Icons.documentSave

            anchors {
                bottom: parent.bottom
                bottomMargin: 5
                right: parent.right
                rightMargin: 10
            }
            onClicked: () => {
                           internal.didUseButton = true
                           internal.save()
                           control.close()
                       }
        }
    }
}
