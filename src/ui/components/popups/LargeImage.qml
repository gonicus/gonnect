pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: imageItem.sourceSize.width
    implicitHeight: imageItem.sourceSize.height

    property alias source: imageItem.source

    AnimatedImage {
        id: imageItem
        anchors.centerIn: parent
        fillMode: Image.PreserveAspectFit
    }

    TapHandler {
        grabPermissions: PointerHandler.TakeOverForbidden
        gesturePolicy: TapHandler.WithinBounds
        onSingleTapped: () => {
            control.StackView.view.popCurrentItem(StackView.Immediate)
        }
    }
}
