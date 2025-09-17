pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control

    required property string pageId

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

            let widgetId = control.pageId+"_widget"+index
            UISettings.removeUISetting(widgetId, "")
        }
    }
}
