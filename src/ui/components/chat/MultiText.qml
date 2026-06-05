pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Dialogs
import base

Column {
    id: control
    width: control.availableWidth

    property ChatMessageContentText content
    property int availableWidth

    Repeater {
        id: rep
        model: control.content?.contentParts ?? null
        delegate: Item {
            id: delg
            implicitHeight: delg.isCode ? codeBlockLoader.height : textLabel.height

            required property int index
            required property ChatMessageContentPart modelData
            readonly property bool isCode: delg.modelData?.isCode ?? false
            readonly property string text: delg.modelData?.text ?? ""
            readonly property string fenceInfo: delg.modelData?.fenceInfo ?? ""

            anchors {
                left: parent?.left
                right: parent?.right
            }

            Label {
                id: textLabel
                visible: !delg.isCode
                text: delg.text
                wrapMode: Label.WordWrap
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }

            Loader {
                id: codeBlockLoader
                active: delg.isCode
                source: "qrc:/qt/qml/base/ui/components/chat/CodeBlock.qml"
                anchors {
                    left: parent.left
                    right: parent.right
                }

                Binding {
                    target: codeBlockLoader.item
                    property: "text"
                    value: delg.text
                }

                Binding {
                    target: codeBlockLoader.item
                    property: "fenceInfo"
                    value: delg.fenceInfo
                }
            }
        }
    }
}
