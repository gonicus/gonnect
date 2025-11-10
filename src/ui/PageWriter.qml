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
        // Widgets
        let widgets = model.items()
        for (const widget of widgets) {
            let widgetId = widget.widgetId

            UISettings.removeUISetting(widgetId, "")
        }

        // Page
        UISettings.removeUISetting(control.pageId, "")
    }

    function save() {
        // Widgets
        let widgets = model.items()
        for (const widget of widgets) {
            let widgetId = widget.widgetId

            UISettings.setUISetting(widgetId, "name", widget.name)
            UISettings.setUISetting(widgetId, "type", widget.type)
            UISettings.setUISetting(widgetId, "x", widget.widget.x)
            UISettings.setUISetting(widgetId, "y", widget.widget.y)
            UISettings.setUISetting(widgetId, "width", widget.widget.width)
            UISettings.setUISetting(widgetId, "height", widget.widget.height)
        }

        // Page
        UISettings.setUISetting(control.pageId, "name", control.name)
        UISettings.setUISetting(control.pageId, "icon", control.icon)
    }
}
