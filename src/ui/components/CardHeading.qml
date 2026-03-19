pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import base

Item {
    id: control
    height: 46

    property string text: ""
    property bool showHeading: true
    property bool showDivider: false
    property alias headingMargin: headingLoader.implicitWidth

    Accessible.role: Accessible.Heading
    Accessible.name: control.text

    Loader {
        id: headingLoader
        active: control.showHeading
        sourceComponent: headingComponent

        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            leftMargin: 20
            rightMargin: 20
        }
    }

    Component {
        id: headingComponent

        RowLayout {
            id: headingLayout

            Label {
                id: headingText
                text: control.text
                font.pixelSize: 16
                font.weight: Font.Medium
                elide: Text.ElideRight
                color: Theme.secondaryTextColor
            }

            Pane {
                padding: 15
                background: Rectangle {
                    id: headingSeparator
                    visible: control.showDivider
                    height: 30
                    width: 1
                    color: Theme.borderColor
                    anchors.centerIn: parent
                }
            }

            Accessible.ignored: true
        }
    }

    Rectangle {
        color: Theme.borderColor
        height: 1
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        Accessible.ignored: true
    }
}
