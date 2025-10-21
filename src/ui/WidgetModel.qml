pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control

    property var model: []

    signal modelUpdated()

    function items() {
        return model
    }

    function count() {
        return model.length
    }

    function add(widget: Item) {
        model.push(widget)

        control.modelUpdated()
    }

    function remove(widget: Item) {
        const index = model.indexOf(widget)
        if (index !== -1) {
            model.splice(index, 1)

            UISettings.removeUISetting(widget.widgetId, "")

            control.modelUpdated()
        }
    }

    function removeAll() {
        for (const widget of model) {
           UISettings.removeUISetting(widget.widgetId, "")
        }

        model.splice(0, model.length);

        control.modelUpdated()
    }
}
