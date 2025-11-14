pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Menu {
    id: burgerMenu

    // Togglers will be inserted here by instantiator

    MenuSeparator {
        visible: burgerMenuTogglerInstantiator.count > 0
    }

    Action {
        text: qsTr("Toggle fullscreen")
        onTriggered: () => ViewHelper.toggleFullscreen()
    }

    Action {
        text: qsTr("Shortcuts...")
        onTriggered: () => ViewHelper.showShortcuts()
    }

    Action {
        text: qsTr("About...")
        onTriggered: () => ViewHelper.showAbout()
    }

    Action {
        text: qsTr("Quit")
        onTriggered: () => ViewHelper.quitApplication()
    }

    Instantiator {
        id: burgerMenuTogglerInstantiator
        model: TogglerProxyModel {
            displayFilter: Toggler.MENU
            TogglerModel {}
        }
        delegate: Action {
            id: delg
            text: delg.name
            enabled: !delg.isBusy
            icon.color: delg.isBusy ? Theme.secondaryTextColor : Theme.primaryTextColor
            icon.source: delg.isBusy
                         ? Icons.viewRefresh
                         : (delg.isActive
                            ? Icons.checkbox
                            : '')

            required property string id
            required property string name
            required property bool isActive
            required property bool isBusy

            onTriggered: () => TogglerManager.toggleToggler(delg.id)
        }

        onObjectAdded: (index, object) => burgerMenu.insertAction(index, object)
        onObjectRemoved: (index, object) => burgerMenu.removeAction(object)
    }
}
