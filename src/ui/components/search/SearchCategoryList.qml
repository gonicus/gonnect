pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import base

Column {
    id: control
    spacing: 5

    enum Category {
        None = 0,
        Contacts = 1,
        History = 2,
        RoomsAndTeams = 4
    }

    property int selectedCategories: SearchCategoryList.Category.Contacts
                                     | SearchCategoryList.Category.History
                                     | SearchCategoryList.Category.RoomsAndTeams

    Settings {
        location: ViewHelper.userConfigPath
        category: "generic"

        property alias selectedSearchCategories: control.selectedCategories
    }

    component CategoryItem : SearchCategoryItem {
        id: catItem
        highlighted: !!(control.selectedCategories & catItem.value)
        onClicked: () => control.selectedCategories ^= catItem.value

        required property int value
    }

    CategoryItem {
        name: qsTr("Contacts")
        value: SearchCategoryList.Category.Contacts
    }
    CategoryItem {
        name: qsTr("History")
        value: SearchCategoryList.Category.History
    }
    CategoryItem {
        name: qsTr("Rooms and Teams")
        value: SearchCategoryList.Category.RoomsAndTeams
    }
}
