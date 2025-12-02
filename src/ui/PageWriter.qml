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

    function save() {
        // Widgets
        let widgets = model.items()
        for (const widget of widgets) {
            let widgetId = widget.widgetId

            UISettings.setUISetting(widgetId, "name", widget.name)
            UISettings.setUISetting(widgetId, "type", widget.type)
            UISettings.setUISetting(widgetId, "xGrid", widget.xGrid)
            UISettings.setUISetting(widgetId, "yGrid", widget.yGrid)
            UISettings.setUISetting(widgetId, "widthGrid", widget.widthGrid)
            UISettings.setUISetting(widgetId, "heightGrid", widget.heightGrid)
        }

        // Page
        UISettings.setUISetting(control.pageId, "name", control.name)
        UISettings.setUISetting(control.pageId, "icon", control.icon)
    }
}
