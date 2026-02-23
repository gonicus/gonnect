pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import base

Item {
    id: control

    property CallItem callItem

    property bool scrollBarShow: true
    property int scrollBarWidth: 10

    Rectangle {
        anchors.fill: parent
        bottomLeftRadius: 12
        bottomRightRadius: 12
        color: Theme.backgroundColor

        TreeView {
            id: treeView
            clip: true
            anchors.fill: parent
            model: ResponseTreeModel {
                id: responseTreeModel
                callId: control.callItem?.callId ?? -1
                accountId: control.callItem?.accountId ?? ""
            }

            boundsMovement: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {
                id: verticalScrollBar
                visible: control.scrollBarShow
                width: control.scrollBarWidth
                policy: ScrollBar.AsNeeded
            }

            Accessible.role: Accessible.Tree
            Accessible.name: qsTr("Additional caller-related information")
            Accessible.description: qsTr("List of informational items regarding the caller, such as open support tickets")

            delegate: TreeViewDelegate {
                id: treeViewDelegate
                implicitWidth: control.width - control.scrollBarWidth
                implicitHeight: Math.max(20, treeItem.implicitHeight + 10)
                background: Theme.backgroundColor

                required property variant output

                Accessible.role: Accessible.Section
                Accessible.name: qsTr("Expandable response section")
                Accessible.focusable: true
                Accessible.onPressAction: () => treeViewDelegate.toggleSection()

                function toggleSection() {
                    const row = treeViewDelegate.row

                    if (treeView.isExpanded(row)) {
                        treeView.collapse(row)
                    } else {
                        treeView.expand(row)
                    }
                }

                TapHandler {
                    id: tapArea
                    onTapped: () => treeViewDelegate.toggleSection()
                }

                HoverHandler {
                    id: hoverArea
                }

                Rectangle {
                    anchors.fill: parent
                    color: hoverArea.hovered ? Theme.highlightColor : Theme.backgroundColor
                    z: 0

                    Accessible.ignored: true
                }

                contentItem: TextEdit {
                    id: treeItem
                    width: parent.width
                    text: treeViewDelegate.output
                    wrapMode: Text.Wrap
                    color: Theme.primaryTextColor
                    textFormat: Text.RichText
                    readOnly: true
                    selectByMouse: true
                    onLinkActivated: (link) => Qt.openUrlExternally(link)
                    z: 1

                    Accessible.role: Accessible.TreeItem
                    Accessible.name: treeItem.text
                    Accessible.focusable: true
                }

                indentation: 10

                indicator: Item {
                    width: 5
                    height: 20
                    x: treeViewDelegate.leftMargin + treeViewDelegate.indentation * treeViewDelegate.depth
                    anchors {
                        verticalCenter: parent.verticalCenter
                    }

                    IconLabel {
                        anchors.centerIn: parent
                        icon {
                            source: Icons.goDown
                        }
                        rotation: treeViewDelegate.expanded ? 270 : 0
                    }

                    Accessible.ignored: true
                }
            }
        }
    }
}
