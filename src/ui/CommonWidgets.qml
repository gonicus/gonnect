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
    property alias chat: compChat
    property alias activities: compActivities

    enum Type {
        DateEvents,
        Favorites,
        History,
        WebView,
        Chat,
        Activities
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

        WebViewWidget {
            id: widgetWebview
            type: CommonWidgets.Type.WebView
        }
    }

    Component {
        id: compChat

        ChatWidget {
            id: widgetChat
            type: CommonWidgets.Type.Chat
        }
    }

    Component {
        id: compActivities

        ActivitiesWidget {
            id: widgetActivities
            type: CommonWidgets.Type.Activities
        }
    }
}
