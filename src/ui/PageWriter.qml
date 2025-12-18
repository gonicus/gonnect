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

                // TODO: no method access, gotta love qml
                let additionalOptions = widget.config.entries()
                console.log("???", typeof widget.config, typeof widget.config.entries)
                UISettings.setUISetting(widgetId, "additionalOptions", additionalOptions)
            }

            // Page
            UISettings.setUISetting(control.pageId, "name", control.name)
            UISettings.setUISetting(control.pageId, "iconId", control.iconId)
        }
    }
}
