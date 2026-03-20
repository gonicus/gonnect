import QtQuick

Item {
    id: control
    width: 0
    height: 0

    readonly property int notifications: internal.elements.reduce((sum, el) => sum + el.notifications, 0)

    readonly property alias elements: internal.elements

    QtObject {
        id: internal

        property list<var> elements: []
    }

    function registerElement(el : variant) {
        if (!el) {
            return
        }

        if (el && !internal.elements.includes(el)) {
            if (el instanceof Item) {
                internal.elements = internal.elements.concat([el])
            } else {
                console.error('unknown element registered', el)
            }
        }
    }

    function unregisterElement(el : variant) {
        internal.elements = internal.elements.filter(item => item !== el)
    }
}
