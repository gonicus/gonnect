pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import base

Item {
    id: control

    property alias dateEvents: compDateEvents
    property alias favorites: compFavorites
    property alias history: compHistory
    property alias webview: compWebview

    enum Type {
        DateEvents,
        Favorites,
        History,
        Webview
    }

    Component {
        id: compDateEvents

        DateEventsWidget {
            id: widgetDateEvents
            type: CommonWidgets.Type.DateEvents
        }
    }

    Component {
        id: compFavorites

        FavoritesWidget {
            id: widgetFavorites
            type: CommonWidgets.Type.Favorites
        }
    }

    Component {
        id: compHistory

        HistoryWidget {
            id: widgetHistory
            type: CommonWidgets.Type.History
        }
    }

    Component {
        id: compWebview

        WebviewWidget {
            id: widgetWebview
            type: CommonWidgets.Type.Webview
        }
    }
}
