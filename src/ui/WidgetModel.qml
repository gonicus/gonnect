pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control

    readonly property int notifications: internal.model.reduce((sum, el) => sum + el.notifications, 0)

    readonly property alias model: internal.model

    QtObject {
        id: internal

        property list<var> model: []
    }

    signal modelUpdated()

    function items() {
        return model
    }

    function count() {
        return model.length
    }

    function add(widget: Item) {
        if (!internal.model.includes(widget)) {
            internal.model = internal.model.concat([widget])

            control.modelUpdated()
        }
    }

    function remove(widget: Item) {
        if (internal.model.includes(widget)) {
            internal.model = internal.model.filter(item => item !== widget)

            UISettings.removeUISetting(widget.widgetId, "")
            control.modelUpdated()
        }
    }

    function removeAll() {
        for (const widget of model) {
           UISettings.removeUISetting(widget.widgetId, "")
        }

        internal.model = []

        control.modelUpdated()
    }
}
