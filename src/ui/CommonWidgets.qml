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

            xRelative: 0.05
            yRelative: 0.05

            wRelative: 0.25
            hRelative: 0.50

            wMin: 300
            hMin: 350
        }
    }

    Component {
        id: compFavorites

        FavoritesWidget {
            id: widgetFavorites
            type: CommonWidgets.Type.Favorites

            xRelative: 0.05
            yRelative: 0.05

            wRelative: 0.25
            hRelative: 0.50

            wMin: 300
            hMin: 350
        }
    }

    Component {
        id: compHistory

        HistoryWidget {
            id: widgetHistory
            type: CommonWidgets.Type.History

            xRelative: 0.05
            yRelative: 0.05

            wRelative: 0.45
            hRelative: 0.65

            wMin: 550
            hMin: 500
        }
    }

    Component {
        id: compExample

        BaseWidget {
            id: widgetExample
            type: CommonWidgets.Type.Example

            xRelative: 0.05
            yRelative: 0.05

            wRelative: 0.25
            hRelative: 0.25

            wMin: 250
            hMin: 250
        }
    }
}
