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
            wRelativeMin: 0.25
            wRelativeMax: 1.0

            hRelative: 0.50
            hRelativeMin: 0.50
            hRelativeMax: 1.0
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
            wRelativeMin: 0.25
            wRelativeMax: 1.0

            hRelative: 0.50
            hRelativeMin: 0.50
            hRelativeMax: 1.0
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
            wRelativeMin: 0.45
            wRelativeMax: 1.0

            hRelative: 0.65
            hRelativeMin: 0.65
            hRelativeMax: 1.0
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
            wRelativeMin: 0.25
            wRelativeMax: 1.0

            hRelative: 0.25
            hRelativeMin: 0.25
            hRelativeMax: 1.0
        }
    }
}
