pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import base

Item {
    id: control

    required property var tabRoot // Where to parent the tabs to

    required property var pageRoot // Where to parent the pages to
    required property var pageList

    required property PageModel model

    CommonPages {
        id: pages
    }

    CommonWidgets {
        id: widgets
    }

    function load() {
        // Generic
        let pageCount = Number(UISettings.getUISetting("generic", "pages", 0))
        let windowWidth = Number(UISettings.getUISetting("generic", "gonnectWindowWidth", 0))
        let windowHeight = Number(UISettings.getUISetting("generic", "gonnectWindowHeight", 0))
        let tabBarWidth = Number(UISettings.getUISetting("generic", "gonnectTabBarWidth", 0))
        let controlBarHeight = Number(UISettings.getUISetting("generic", "gonnectControlBarHeight", 0))

        // Pages
        let pageItems = []
        for (let i = 0; i < pageCount; i++) {
            let pageId = "page"+i

            let pageName = UISettings.getUISetting(pageId, "name", "")
            let pageIcon = UISettings.getUISetting(pageId, "icon", "")
            let widgetCount = Number(UISettings.getUISetting(pageId, "widgets", 0))

            // Widgets
            let widgetItems = []
            for (let j = 0; j < widgetCount; j++) {
                let widgetId = pageId+"_widget"+j

                let widgetName = UISettings.getUISetting(widgetId, "name", "")
                let widgetType = UISettings.getUISetting(widgetId, "type", "")
                let widgetX = Number(UISettings.getUISetting(widgetId, "x", 0))
                let widgetY = Number(UISettings.getUISetting(widgetId, "y", 0))
                let widgetWidth = Number(UISettings.getUISetting(widgetId, "width", 0))
                let widgetHeight = Number(UISettings.getUISetting(widgetId, "height", 0))

                widgetItems.push({
                                     id: widgetId,
                                     name: widgetName,
                                     type: widgetType,
                                     x: widgetX,
                                     y: widgetY,
                                     width: widgetWidth,
                                     height: widgetHeight
                                 })
            }

            pageItems.push({
                               id: pageId,
                               name: pageName,
                               icon: pageIcon,
                               widgets: widgetItems
                           })
        }

        // Loading
        for (let k = 0; k < pageItems.length; k++) {
            let pageItem = pageItems[k]

            let page = pages.base.createObject(control.pageRoot,
                                               {
                                                   pageId: pageItem.id,
                                                   name: pageItem.name,
                                                   icon: pageItem.icon,
                                                   editMode: false
                                               })
            if (page === null) {
                console.log("Could not create page component", pageItem.id)
                continue
            }

            model.add(page)

            tabRoot.createTab(pageItem.id,
                              GonnectWindow.PageType.Base,
                              pageItem.icon,
                              pageItem.name)

            pageList[pageItem.id] = page

            for (let l = 0; l < pageItem.widgets.length; l++) {
                let widgetItem = pageItem.widgets[l]
                const type = Number(widgetItem.type)

                const widgetProperties = {
                    name: widgetItem.name,
                    type: type,
                    xRelative: widgetItem.x,
                    yRelative: widgetItem.y,
                    wRelative: widgetItem.width,
                    hRelative: widgetItem.height
                }

                let widget
                switch(type) {
                    case CommonWidgets.Type.DateEvents:
                        widget = widgets.dateEvents.createObject(page,
                                                                 widgetProperties)
                        break
                    case CommonWidgets.Type.Favorites:
                        widget = widgets.favorites.createObject(page,
                                                                widgetProperties)
                        break
                    case CommonWidgets.Type.History:
                        widget = widgets.history.createObject(page,
                                                              widgetProperties)
                        break
                    case CommonWidgets.Type.Example:
                        widget = widgets.example.createObject(page,
                                                              widgetProperties)
                        break
                    default:
                        widget = null
                        console.log("Widget type unknown", type)
                }

                if (widget === null) {
                    console.log("Could not create widget component", widgetItem.id)
                    continue
                }

                page.model.add(widget)
            }
        }
    }
}
