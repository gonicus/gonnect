pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import base

Item {
    id: control

    required property var tabRoot  // Where to parent the tabs to
    required property var pageRoot // Where to parent the pages to

    required property PageModel model

    CommonPages {
        id: pages
    }

    CommonWidgets {
        id: widgets
    }

    function loadDynamicPages() {
        // Pages
        let pageIds = UISettings.getPageIds()
        for (const pageId of pageIds) {
            let pageName = UISettings.getUISetting(pageId, "name", "")
            let pageIcon = UISettings.getUISetting(pageId, "icon", "")

            let page = pages.base.createObject(control.pageRoot,
                                               {
                                                   pageId: pageId,
                                                   name: pageName,
                                                   icon: pageIcon
                                               })
            if (page === null) {
                console.log("Could not create page component", pageId)
                continue
            }

            model.add(page)

            tabRoot.createTab(pageId,
                              GonnectWindow.PageType.Base,
                              pageIcon,
                              pageName)

            pageRoot.pages[pageId] = page

            // Widgets
            let widgetIds = UISettings.getWidgetIds(pageId)
            for (const widgetId of widgetIds) {
                control.createWidget(widgetId, page)
            }
        }
    }

    function loadHomePage(pageId : string) {
        let page = pageRoot.getPage(pageId)

        let widgets = UISettings.getWidgetIds(pageId)
        if (widgets.length > 0) {
            for (const widget of widgets) {
                control.createWidget(widget, page)
            }
        } else {
            // TODO: Load a default widget selection here
        }
    }

    function createWidget(widgetId : string, page : variant) {
        let widgetName = UISettings.getUISetting(widgetId, "name", "")
        let widgetType = Number(UISettings.getUISetting(widgetId, "type", 0))
        let widgetX = Number(UISettings.getUISetting(widgetId, "x", 0))
        let widgetY = Number(UISettings.getUISetting(widgetId, "y", 0))
        let widgetWidth = Number(UISettings.getUISetting(widgetId, "width", 0))
        let widgetHeight = Number(UISettings.getUISetting(widgetId, "height", 0))

        const widgetProperties = {
            widgetId: widgetId,
            name: widgetName,
            type: widgetType,
            page: page,
            xRelative: widgetX,
            yRelative: widgetY,
            wRelative: widgetWidth,
            hRelative: widgetHeight
        }

        let widget
        switch(widgetType) {
            case CommonWidgets.Type.DateEvents:
                widget = widgets.dateEvents.createObject(page.grid,
                                                         widgetProperties)
                break
            case CommonWidgets.Type.Favorites:
                widget = widgets.favorites.createObject(page.grid,
                                                        widgetProperties)
                break
            case CommonWidgets.Type.History:
                widget = widgets.history.createObject(page.grid,
                                                      widgetProperties)
                break
            default:
                widget = null
                console.log("Widget type unknown", widgetType)
        }

        if (widget === null) {
            console.log("Could not create widget component", widgetId)
            return
        }

        page.model.add(widget)
    }
}
