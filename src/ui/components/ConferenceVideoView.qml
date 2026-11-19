pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control

    required property ConferenceVideoModel videoModel
    required property bool isOwnVideoActive

    readonly property int tileSpacing: Theme.d
    readonly property int tileCount: remoteTileRepeater.count + 1
    readonly property var tileLayout: control.largestTileLayout(control.width, control.height, control.tileCount)

    // Largest 16:9 tile that lets all tiles fit into given area
    function largestTileLayout(availableWidth : real, availableHeight : real, count : int) : var {
        let best = { columns : 1, width : 0, height : 0 }

        for (let columns = 1; columns <= count; ++columns) {
            const rows = Math.ceil(count / columns)
            const widthByColumns = (availableWidth - (columns - 1) * control.tileSpacing) / columns
            const widthByRows = (availableHeight - (rows - 1) * control.tileSpacing) / rows * 16 / 9
            const width = Math.floor(Math.min(widthByColumns, widthByRows))

            if (width > best.width) {
                best = { columns, width, height: Math.floor(width * 9 / 16) }
            }
        }

        return best
    }

    Flow {
        id: tileFlow
        spacing: control.tileSpacing
        width: control.tileLayout.columns * control.tileLayout.width
               + (control.tileLayout.columns - 1) * control.tileSpacing
        anchors.centerIn: parent

        OwnVideoTile {
            width: control.tileLayout.width
            height: control.tileLayout.height
            isActive: control.isOwnVideoActive
        }

        Repeater {
            id: remoteTileRepeater
            model: control.videoModel
            delegate: ConferenceVideoTile {
                width: control.tileLayout.width
                height: control.tileLayout.height
            }
        }
    }
}
