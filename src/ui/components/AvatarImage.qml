pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import Qt5Compat.GraphicalEffects
import base

Item {
    id: control
    width: control.size
    height: control.size
    implicitWidth: control.size
    implicitHeight: control.size

    property string initials
    property url source
    property int size: 24

    property alias showPresenceStatus: statusIndicatorContainer.visible
    property int presenceStatus
    property bool isBlocked
    property bool isUnregistered
    property alias indicatorComponent: statusIndicatorLoader.sourceComponent
    property Component menuComponent

    readonly property bool hasSource: control.source.toString() !== ""  // toString() is necessary, see QTBUG-63629

    Image {
        id: img
        cache: true
        visible: false
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        sourceSize.width: control.size
        sourceSize.height: control.size
        source: control.hasSource
                ? control.source
                : (control.initials
                   ? `image://personcoin/${control.initials}`
                   : "")
    }

    Rectangle {
        id: mask
        anchors.fill: parent
        color: 'black'
        visible: false
        radius: mask.width / 2

        Accessible.ignored: true
    }

    OpacityMask {
        id: opacityMask
        anchors.fill: parent
        source: img
        maskSource: mask
    }

    Rectangle {
        id: statusIndicatorContainer
        color: Theme.backgroundColor
        x: control.width / 2 + Math.sqrt(((control.width / 2) * (control.width / 2)) / 2) - statusIndicatorContainer.width / 2
        y: statusIndicatorContainer.x
        width: 8/24 * control.size
        height: statusIndicatorContainer.width
        radius: statusIndicatorContainer.width / 2
        visible: false

        Loader {
            id: statusIndicatorLoader
            width: parent.width - 2
            height: statusIndicatorLoader.width
            active: control.showPresenceStatus
            anchors.centerIn: parent
            sourceComponent: Component {
                PresenceStatusIndicator {}
            }

            onItemChanged: () => {
                const item = statusIndicatorLoader.item

                if (item) {
                    if (item.hasOwnProperty("status")) {
                        item.status = Qt.binding(() => control.presenceStatus)
                    }

                    if (item.hasOwnProperty("isBlocked")) {
                        item.isBlocked = Qt.binding(() => control.isBlocked)
                    }

                    if (item.hasOwnProperty("isUnregistered")) {
                        item.isUnregistered = Qt.binding(() => control.isUnregistered)
                    }
                }
            }
        }

        Accessible.ignored: true
    }

    TapHandler {
        enabled: !!control.menuComponent
        onTapped: () => {
                      const menu = control.menuComponent.createObject(control)
                      if (menu) {
                          menu.popup()
                      }
                  }
    }
}
