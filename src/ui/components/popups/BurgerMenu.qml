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
        icon.source: "qrc:/icons/view-fullscreen.svg"
        onTriggered: () => ViewHelper.toggleFullscreen()
    }

    Action {
        text: qsTr("Shortcuts...")
        icon.source: "qrc:/icons/configure-shortcuts.svg"
        onTriggered: () => ViewHelper.showShortcuts()
    }

    Action {
        id: pageEditAction
        text: SM.uiEditMode ? qsTr("Leave edit mode") : qsTr("Customize UI...")
        icon.source: "qrc:/icons/editor.svg"
        enabled: true
        onTriggered: () => {
            if (SM.uiEditMode) {
                SM.setUiEditMode(false)
            } else {
                SM.setUiEditMode(true)
            }
        }
    }

    Action {
        text: qsTr("About...")
        icon.source: "qrc:/icons/showinfo.svg"
        onTriggered: () => ViewHelper.showAbout()
    }

    Action {
        text: qsTr("Quit")
        icon.source: "qrc:/icons/application-exit.svg"
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
