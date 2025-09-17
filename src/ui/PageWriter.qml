pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import base

Item {
    id: control

    required property string pageId
    required property string name
    required property string icon

    required property WidgetModel model

    function reset() {
        const widgetCount = model.count()

        // Widgets
        for (let i = 0; i < widgetCount; i++) {
            let widgetId = control.pageId+"_widget"+i

            UISettings.removeUISetting(widgetId, "")
        }

        // Page
        UISettings.removeUISetting(control.pageId, "")
    }

    function save() {
        let widgetList = model.items()
        let widgetCount = model.count()

        // Widgets
        for (let i = 0; i < widgetCount; i++) {
            let widgetItem = widgetList[i]
            let widgetId = control.pageId+"_widget"+i

            UISettings.setUISetting(widgetId, "name", widgetItem.name)
            UISettings.setUISetting(widgetId, "type", widgetItem.type)
            UISettings.setUISetting(widgetId, "x", widgetItem.xRelative)
            UISettings.setUISetting(widgetId, "y", widgetItem.yRelative)
            UISettings.setUISetting(widgetId, "width", widgetItem.wRelative)
            UISettings.setUISetting(widgetId, "height", widgetItem.hRelative)
        }

        // Page
        UISettings.setUISetting(control.pageId, "name", control.name)
        UISettings.setUISetting(control.pageId, "icon", control.icon)
        UISettings.setUISetting(control.pageId, "widgets", widgetCount)
    }
}
