pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control
    implicitHeight: searchInputField.implicitHeight

    signal numberSelected(string number, string contactId)
    signal escapePressed()

    property int highlightedIndex: -1
    readonly property string text: searchInputField.text.trim()

    function activate() {
        searchInputField.forceActiveFocus()
        searchInputField.selectAll()
    }

    function clear() {
        searchInputField.text = ""
    }

    TextInput {
        id: searchInputField
        font.pixelSize: 18
        color: Theme.primaryTextColor
        anchors {
            left: parent.left
            right: parent.right
        }

        onFocusChanged: (isFocused) => {
            if (isFocused) {
                searchInputField.selectAll()
            }
        }

        Keys.onEnterPressed: () => {
            internal.dialHighlightedNumber()
        }

        Keys.onReturnPressed: () => {
            internal.dialHighlightedNumber()
        }

        Keys.onEscapePressed: () => {
            if (searchPopup.opened) {
                searchPopup.close()
            } else if (searchInputField.text.trim() !== "") {
                searchInputField.text = ""
            } else {
                control.escapePressed()
            }
        }

        Keys.onDownPressed: () => {
            if (control.highlightedIndex < internal.searchListModel.totalNumbersCount - 1) {
                control.highlightedIndex++
            }
        }

        Keys.onUpPressed: () => {
            if (control.highlightedIndex > 0) {
                control.highlightedIndex--
            }
        }
    }

    Connections {
        target: SearchProvider
        function onActivateSearch(query) {
            searchInputField.text = query
        }
    }

    QtObject {
        id: internal

        property SearchListModel searchListModel: null

        function dialHighlightedNumber() {
            if (control.highlightedIndex >= 0) {
                const tel = internal.searchListModel.phoneNumberByIndex(control.highlightedIndex)
                if (tel === "") {
                    return;
                }

                const id = internal.searchListModel.contactIdByIndex(control.highlightedIndex)
                control.numberSelected(tel, id)
                control.highlightedIndex = -1
                searchInputField.text = ""
            } else {
                control.numberSelected(searchInputField.text, ViewHelper.contactIdByNumber(searchInputField.text))
                searchInputField.text = ""
            }
        }
    }

    SearchResultPopup {
        id: searchPopup
        y: searchInputField.height
        width: searchInputField.width
        height: Math.min(contentItem.implicitHeight + verticalPadding * 2, control.Window.height  - control.y - control.height - topMargin - bottomMargin)
        topMargin: 12 + searchInputField.height
        searchText: searchInputField.text

        // onNumberSelected: (number, contactId) => {
        //     control.numberSelected(number, contactId)
        //     control.highlightedIndex = -1
        //     searchInputField.text = ""
        // }
    }
}
