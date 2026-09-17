import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl

T.BusyIndicator {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 6

    property alias circleColor: indicatorImpl.color

    contentItem: BusyIndicatorImpl {
        id: indicatorImpl
        implicitWidth: control.Material.touchTarget
        implicitHeight: control.Material.touchTarget
        color: control.Material.accentColor

        running: control.running
        opacity: control.running ? 1 : 0
        Behavior on opacity { OpacityAnimator { duration: 250 } }
    }
}
