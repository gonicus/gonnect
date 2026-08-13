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

    function add(page: Item) {
        if (!internal.model.includes(page)) {
            internal.model = internal.model.concat([page])

            control.modelUpdated()
        }
    }

    function remove(page: Item) {
        if (internal.model.includes(page)) {
            internal.model = internal.model.filter(item => item !== page)

            UISettings.removeUISetting(page.pageId, "")
            control.modelUpdated()
        }
    }
}
