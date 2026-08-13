pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Menu {
    id: burgerMenu

    // INFO: Togglers will be inserted here by instantiator

    MenuSeparator {
        visible: burgerMenuTogglerInstantiator.count > 0
    }

    Action {
        id: fullscreenAciton
        text: qsTr("Toggle fullscreen")
        icon.source:  Icons.viewFullscreen
        onTriggered: () => ViewHelper.toggleFullscreen()

        Accessible.role: Accessible.Button
        Accessible.name: fullscreenAciton.text
        Accessible.focusable: true
        Accessible.onPressAction: () => ViewHelper.toggleFullscreen()
    }

    Action {
        id: shortcutAction
        text: qsTr("Shortcuts...")
        icon.source: Icons.configureShortcuts
        onTriggered: () => ViewHelper.showShortcuts()

        Accessible.role: Accessible.Button
        Accessible.name: shortcutAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => ViewHelper.showShortcuts()
    }

    Action {
        id: pageEditAction
        text: qsTr("Customize UI")
        icon.source: Icons.editor
        enabled: !SM.uiEditMode
        onTriggered: () => SM.uiEditMode = true

        Accessible.role: Accessible.Button
        Accessible.name: pageEditAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => SM.uiEditMode = true
    }

    Action {
        id: aboutAction
        text: qsTr("About...")
        icon.source: Icons.showinfo
        onTriggered: () => ViewHelper.showAbout()

        Accessible.role: Accessible.Button
        Accessible.name: aboutAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => ViewHelper.showAbout()
    }

    Action {
        id: quitAction
        text: qsTr("Quit")
        icon.source: Icons.applicationExit
        onTriggered: () => ViewHelper.quitApplication()

        Accessible.role: Accessible.Button
        Accessible.name: quitAction.text
        Accessible.focusable: true
        Accessible.onPressAction: () => ViewHelper.quitApplication()
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

            Accessible.role: Accessible.Button
            Accessible.name: delg.text
            Accessible.focusable: true
            Accessible.onPressAction: () => TogglerManager.toggleToggler(delg.id)
        }

        onObjectAdded: (index, object) => burgerMenu.insertAction(index, object)
        onObjectRemoved: (index, object) => burgerMenu.removeAction(object)
    }
}
