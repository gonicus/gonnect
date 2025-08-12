import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

Item {
    id: control
    implicitHeight: control.iconSize
    implicitWidth: firstAidLabel.x + firstAidLabel.implicitWidth

    property int iconSize: 24

    IconLabel {
        id: firstAidIcon
        icon {
            color: "transparent"  // Shows the original colors of the icon
            source: Icons.dataWarning
            width: control.iconSize
            height: control.iconSize
        }
    }

    Label {
        id: firstAidLabel
        text: qsTr("First Aid")
        color: firstAidHoverHandler.hovered ? Theme.primaryTextColor : Theme.inactiveTextColor
        anchors {
            verticalCenter: control.verticalCenter
            left: firstAidIcon.right
            leftMargin: 5
        }
    }

    HoverHandler {
        id: firstAidHoverHandler
    }
    TapHandler {
        onTapped: () => ViewHelper.showFirstAid()
    }
}
