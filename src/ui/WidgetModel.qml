pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control

    property var model: []

    function items() {
        return model
    }

    function count() {
        return model.length
    }

    function add(widget: Item) {
        model.push(widget)
    }

    function remove(widget: Item) {
        const index = model.indexOf(widget)
        if (index !== -1) {
            model.splice(index, 1)

            UISettings.removeUISetting(widget.widgetId, "")
        }
    }
}
