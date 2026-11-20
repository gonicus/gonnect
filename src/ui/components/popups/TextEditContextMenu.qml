pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Menu {
    id: control

    property alias undoEnabled: undoItem.visible
    property alias redoEnabled: redoItem.visible
    property alias cutEnabled: cutItem.visible
    property alias copyEnabled: copyItem.visible
    property alias pasteEnabled: pasteItem.visible

    signal undo
    signal redo
    signal cut
    signal copy
    signal paste

    HideableMenuItem {
        id: undoItem
        text: qsTr("Undo")
        icon.source: Icons.editUndo
        onTriggered: () => control.undo()
    }

    HideableMenuItem {
        id: redoItem
        text: qsTr("Redo")
        icon.source: Icons.editRedo
        onTriggered: () => control.redo()
    }

    HideableMenuSeparator {
        visible: undoItem.visible || redoItem.visible
    }

    HideableMenuItem {
        id: cutItem
        text: qsTr("Cut")
        icon.source: Icons.editCut
        onTriggered: () => control.cut()
    }

    HideableMenuItem {
        id: copyItem
        text: qsTr("Copy")
        icon.source: Icons.editCopy
        onTriggered: () => control.copy()
    }

    HideableMenuItem {
        id: pasteItem
        text: qsTr("Paste")
        icon.source: Icons.editPaste
        onTriggered: () => control.paste()
    }
}
