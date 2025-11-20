pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import base

Item {
    id: control

    property alias dateEvents: compDateEvents
    property alias favorites: compFavorites
    property alias history: compHistory

    enum Type {
        DateEvents,
        Favorites,
        History
    }

    Component {
        id: compDateEvents

        DateEventsWidget {
            id: widgetDateEvents
            type: CommonWidgets.Type.DateEvents

            wMin: 240
            hMin: 240
        }
    }

    Component {
        id: compFavorites

        FavoritesWidget {
            id: widgetFavorites
            type: CommonWidgets.Type.Favorites

            wMin: 240
            hMin: 240
        }
    }

    Component {
        id: compHistory

        HistoryWidget {
            id: widgetHistory
            type: CommonWidgets.Type.History

            wMin: 495
            hMin: 390
        }
    }
}
