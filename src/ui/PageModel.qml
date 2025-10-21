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

    function add(page: Item) {
        model.push(page)

        control.modelUpdated()
    }

    function remove(page: Item) {
        const index = model.indexOf(page)
        if (index !== -1) {
            model.splice(index, 1)

            UISettings.removeUISetting(page.pageId, "")

            control.modelUpdated()
        }
    }
}
