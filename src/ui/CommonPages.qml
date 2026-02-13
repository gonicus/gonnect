pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import base

Item {
    id: control

    property alias base: compBasePage

    Component {
        id: compBasePage

        BasePage {
            id: basePage
            visible: false
            anchors.fill: parent
        }
    }
}
