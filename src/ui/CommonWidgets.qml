pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import base

Item {
    id: control

    property alias dateEvents: compDateEvents
    property alias favorites: compFavorites
    property alias history: compHistory
    property alias example: compExample

    enum Type {
        DateEvents,
        Favorites,
        History,
        Example
    }

    Component {
        id: compDateEvents

        DateEventsWidget {
            id: widgetDateEvents
            type: CommonWidgets.Type.DateEvents

            wMin: 300
            hMin: 350
        }
    }

    Component {
        id: compFavorites

        FavoritesWidget {
            id: widgetFavorites
            type: CommonWidgets.Type.Favorites

            wMin: 300
            hMin: 350
        }
    }

    Component {
        id: compHistory

        HistoryWidget {
            id: widgetHistory
            type: CommonWidgets.Type.History

            wMin: 550
            hMin: 500
        }
    }

    Component {
        id: compExample

        BaseWidget {
            id: widgetExample
            type: CommonWidgets.Type.Example

            wMin: 250
            hMin: 250
        }
    }
}
