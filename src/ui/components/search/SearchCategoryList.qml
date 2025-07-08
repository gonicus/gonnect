pragma ComponentBehavior: Bound

import QtQuick
import base

Column {
    id: control

    property SearchCategoryItem selectedItem

    Component.onCompleted: () => control.selectedItem = control.children[0]

    component CategoryItem : SearchCategoryItem {
        id: catItem
        highlighted: control.selectedItem === catItem
        onClicked: () => control.selectedItem = catItem
    }

    CategoryItem { name: qsTr("Contacts") }
    CategoryItem { name: qsTr("Messages"); enabled: false }
    CategoryItem { name: qsTr("Rooms and Teams"); enabled: false }
    CategoryItem { name: qsTr("Files"); enabled: false }
}
