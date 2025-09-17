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

    function add(page: Item) {
        model.push(page)
    }

    function remove(page: Item) {
        const index = model.indexOf(page)
        if (index !== -1) {
            model.splice(index, 1)
        }
    }
}
