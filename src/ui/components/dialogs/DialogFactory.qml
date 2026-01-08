pragma Singleton

import QtQuick

QtObject {
    id: control

    readonly property LoggingCategory lc: LoggingCategory {
        id: category
        name: "gonnect.qml.DialogFactory"
        defaultLogLevel: LoggingCategory.Warning
    }

    property Item rootItem

    function createDialog(url, args) {
        const component = Qt.createComponent(url)

        if (component.status === Component.Error) {
            console.error(category, 'failed to load component "' + url + '":', component.errorString())
            return
        }

        const item = component.createObject(control.rootItem, args)
        item.closing.connect(() => item.destroy())
        item.show()
        return item
    }

    function createInfoDialog(args) {
        return createDialog("InfoDialog.qml", args)
    }
    function createConfirmDialog(args) {
        return createDialog("ConfirmDialog.qml", args)
    }
}
