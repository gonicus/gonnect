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
        const pageIds = UISettings.getPageIds()
        for (const pageId of pageIds) {
            const pageName = UISettings.getUISetting(pageId, "name", "")
            const pageIconId = UISettings.getUISetting(pageId, "iconId", "")
            const page = pages.base.createObject(control.pageRoot,
                                                 {
                                                     pageId: pageId,
                                                     name: pageName,
                                                     iconId: pageIconId
                                                 })
            if (!page) {
                console.log("Could not create page component", pageId)
                continue
            }

            model.add(page)

            tabRoot.createTab(pageId,
                              GonnectWindow.PageType.Base,
                              pageIconId,
                              pageName)

            pageRoot.pages[pageId] = page

            // Widgets
            const widgetIds = UISettings.getWidgetIds(pageId)
            for (const widgetId of widgetIds) {
                control.createWidget(widgetId, page)
            }
        }
    }

    function loadHomePage(pageId : string) {
        const page = pageRoot.getPage(pageId)

        const widgetIds = UISettings.getWidgetIds(pageId)
        if (widgetIds.length) {
            // Config-based layout
            for (const widgetId of widgetIds) {
                control.createWidget(widgetId, page)
            }

        } else {
            // Default page layout
            const baseId = `${pageId}-widget_`
            const basicBindings = {
                gridWidth: Qt.binding(() => page.gridWidth),
                gridHeight: Qt.binding(() => page.gridHeight),
                gridCellWidth: Qt.binding(() => page.gridCellWidth),
                gridCellHeight: Qt.binding(() => page.gridCellHeight)
            }

            const history = widgets.history.createObject(page.grid,
                                                         Object.assign({
                                                             widgetId: baseId + UISettings.generateUuid(),
                                                             name: "history",
                                                             page: page,
                                                             xGrid: 0,
                                                             yGrid: 0,
                                                             widthGrid: 33,
                                                             heightGrid: 50
                                                         }, basicBindings))
            if (history) {
                page.model.add(history)
            }

            const favorites = widgets.favorites.createObject(page.grid,
                                                             Object.assign({
                                                                 widgetId: baseId + UISettings.generateUuid(),
                                                                 name: "favorites",
                                                                 page: page,
                                                                 xGrid: 33,
                                                                 yGrid: 0,
                                                                 widthGrid: 17,
                                                                 heightGrid: 28
                                                             }, basicBindings))
            if (favorites) {
                page.model.add(favorites)
            }

            const dateEvents = widgets.dateEvents.createObject(page.grid,
                                                               Object.assign({
                                                                   widgetId: baseId + UISettings.generateUuid(),
                                                                   name: "dateevents",
                                                                   page: page,
                                                                   xGrid: 33,
                                                                   yGrid: 28,
                                                                   widthGrid: 17,
                                                                   heightGrid: 22
                                                               }, basicBindings))
            if (dateEvents) {
                page.model.add(dateEvents)
            }
        }
    }

    function createWidget(widgetId : string, page : BasePage) {
        const widgetType = Number(UISettings.getUISetting(widgetId, "type", 0))
        const widgetProperties = {
            widgetId: widgetId,
            name: UISettings.getUISetting(widgetId, "name", ""),
            page: page,

            xGrid: UISettings.getUISetting(widgetId, "xGrid", 0),
            yGrid: UISettings.getUISetting(widgetId, "yGrid", 0),
            widthGrid: UISettings.getUISetting(widgetId, "widthGrid", Math.floor(ViewHelper.numberOfGridCells() * 0.38)),
            heightGrid: UISettings.getUISetting(widgetId, "heightGrid", Math.floor(ViewHelper.numberOfGridCells() * 0.38)),

            gridWidth: Qt.binding(() => page.gridWidth),
            gridHeight: Qt.binding(() => page.gridHeight),
            gridCellWidth: Qt.binding(() => page.gridCellWidth),
            gridCellHeight: Qt.binding(() => page.gridCellHeight)
        }

        let widget = null
        switch(widgetType) {
            case CommonWidgets.Type.DateEvents:
                widget = widgets.dateEvents.createObject(page.grid, widgetProperties)
                break
            case CommonWidgets.Type.Favorites:
                widget = widgets.favorites.createObject(page.grid, widgetProperties)
                break
            case CommonWidgets.Type.History:
                widget = widgets.history.createObject(page.grid, widgetProperties)
                break
            case CommonWidgets.Type.Webview:
                widget = widgets.webview.createObject(page.grid, widgetProperties)
                break
            default:
                console.error(`Widget type ${widgetType} unknown`)
        }

        if (widget) {
            page.model.add(widget)
        } else {
            console.error("Could not create widget component", widgetId)
        }
    }
}
