pragma ComponentBehavior: Bound

import QtQuick
import base

BaseWidget {
    id: control

    Rectangle {
        id: webviewWidget
        parent: control.root
        color: "transparent"
        anchors {
            centerIn: parent
            fill: parent
        }

        WebviewItem {
            id: webviewItem
            primaryUrl: "https://uc.intranet.gonicus.de/grafana/d/edkwenuhx5i4gb/queuemonitor?orgId=2&from=now-5m&to=now&timezone=browser&kiosk=true&theme=dark&viewPanel=panel-2"
            secondaryUrl: "https://uc.intranet.gonicus.de/grafana/d/edkwenuhx5i4gb/queuemonitor?orgId=2&from=now-5m&to=now&timezone=browser&kiosk=true&theme=light&viewPanel=panel-2"

            anchors.fill: parent
        }
    }
}
