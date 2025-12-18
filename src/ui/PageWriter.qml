pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import base

Item {
    id: control

    required property string pageId
    required property string name
    required property string iconId

    required property WidgetModel model

    function save() {
        saveTimer.start()
    }

    Timer {
        id: saveTimer
        repeat: false
        running: false
        interval: 10

        onTriggered: () => {
            // Widgets
            const widgets = model.items()
            for (let widget of widgets) {
                const widgetId = widget.widgetId

                UISettings.setUISetting(widgetId, "name", widget.name)
                UISettings.setUISetting(widgetId, "type", widget.type)
                UISettings.setUISetting(widgetId, "xGrid", widget.xGrid)
                UISettings.setUISetting(widgetId, "yGrid", widget.yGrid)
                UISettings.setUISetting(widgetId, "widthGrid", widget.widthGrid)
                UISettings.setUISetting(widgetId, "heightGrid", widget.heightGrid)

                // Additional per-widget settings
                let additionalSettings = widget.config.entries()
                UISettings.setUISetting(widgetId, "additionalSettings", additionalSettings.join(","))
                for (let setting of additionalSettings) {
                    UISettings.setUISetting(widgetId, setting, widget.config.get(setting))
                }
            }

            // Page
            UISettings.setUISetting(control.pageId, "name", control.name)
            UISettings.setUISetting(control.pageId, "iconId", control.iconId)
        }
    }
}
