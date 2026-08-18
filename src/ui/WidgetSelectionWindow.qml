pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import base

BaseWindow {
    id: control
    objectName: "widgetSelectionWindow"
    title: qsTr("Add widget")
    width: 600
    visible: true
    resizable: false
    showMinimizeButton: false
    showMaximizeButton: false

    minimumWidth: control.width
    maximumWidth: control.width

    minimumHeight: control.dynamicHeight
    maximumHeight: control.dynamicHeight

    property int currentHeight: widgetOptions.implicitHeight + control.windowHeaderPadding
    property int maxHeight: 700 + control.windowHeaderPadding
    property int dynamicHeight: control.currentHeight > control.maxHeight
                            ? control.maxHeight
                            : control.currentHeight

    required property var widgetRoot

    readonly property LoggingCategory lc: LoggingCategory {
        id: category
        name: "gonnect.qml.WidgetSelectionWindow"
        defaultLogLevel: LoggingCategory.Warning
    }

    readonly property Connections editModeConnections: Connections {
        target: SM
        function onUiEditModeChanged() {
            if (!SM.uiEditMode) {
                control.close()
            }
        }
    }

    readonly property Connections windowConnections: Connections {
        target: control
        function onClosing() {
            SM.uiHasActiveEditDialog = false
        }
    }

    readonly property Connections chatConnectorConnections: Connections {
        target: ChatConnectorManager
        function onIsChatAvailableChanged() {
            control.ensureChatEntry()
        }
    }

    CommonWidgets {
        id: widgets
    }

    property int selection: -1

    property bool chatEntryAvailable: false

    function ensureChatEntry() {
        if (ChatConnectorManager.isChatAvailable && !control.chatEntryAvailable) {
            widgetEntries.append({
                name: qsTr("Chat"),
                description: qsTr("A chat room for direct conversations and group chats"),
                type: CommonWidgets.Type.Chat,
                iconName: "dialogMessages"
            })
            control.chatEntryAvailable = true
        }
    }

    function ensureActivitiesEntry() {
        widgetEntries.append({
            name: qsTr("Activities"),
            description: qsTr("Recent calls, meetings and chat messages"),
            type: CommonWidgets.Type.Activities,
            iconName: "notifications"
        })
    }

    Component.onCompleted: {
        control.ensureChatEntry()
        control.ensureActivitiesEntry()
    }

    Flickable {
        id: widgetFlickable
        clip: true
        contentHeight: widgetOptions.implicitHeight
        anchors {
            fill: parent
        }

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            width: 10
        }

        ColumnLayout {
            id: widgetOptions
            spacing: 5
            anchors {
                fill: parent
                margins: 20
            }

            Label {
                id: titleLabel
                text: qsTr("Widget")
                Layout.alignment: Qt.AlignTop

                Accessible.role: Accessible.StaticText
                Accessible.name: qsTr("Widget selection header")
            }

            ComboBox {
                id: widgetSelection
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop

                model: ListModel {
                    id: widgetEntries

                    ListElement {
                        name: qsTr("Date Events")
                        description: qsTr("List of upcoming appointments")
                        type: CommonWidgets.Type.DateEvents
                        iconName: "acceptTimeEvent"
                    }
                    ListElement {
                        name: qsTr("Favorites")
                        description: qsTr("Quick dial for your favorite contacts and conferences")
                        type: CommonWidgets.Type.Favorites
                        iconName: "folderFavorites"
                    }
                    ListElement {
                        name: qsTr("History")
                        description: qsTr("Searchable call and conference history")
                        type: CommonWidgets.Type.History
                        iconName: "chronometer"
                    }
                    ListElement {
                        name: qsTr("Web View")
                        description: qsTr("A web-based content display")
                        type: CommonWidgets.Type.WebView
                        iconName: "openLink"
                    }
                }

                Accessible.role: Accessible.ComboBox
                Accessible.name: qsTr("Widget selection")
                Accessible.description: qsTr("Select the widget that should be added to the current dashboard page")

                delegate: ItemDelegate {
                    id: widgetDelg
                    width: parent.width

                    font.family: widgetSelection.font.family
                    font.weight: widgetSelection.font.weight
                    font.pointSize: widgetSelection.font.pointSize

                    Accessible.role: Accessible.ListItem
                    Accessible.name: widgetDelg.name
                    Accessible.description: qsTr("Currently selected widget option")
                    Accessible.focusable: true

                    required property string name
                    required property string description
                    required property string iconName

                    contentItem: RowLayout {
                        spacing: 10

                        IconLabel {
                            id: widgetSelecionPreview
                            Layout.preferredWidth: 48
                            Layout.preferredHeight: 48

                            icon {
                                source: Icons[widgetDelg.iconName]
                                width: widgetSelecionPreview.width
                                height: widgetSelecionPreview.height
                            }

                            Accessible.ignored: true
                        }

                        Label {
                            Layout.fillWidth: true

                            textFormat: Text.RichText
                            text: "<b>" + widgetDelg.name + "</b><br>"
                                  + widgetDelg.description

                            Accessible.ignored: true
                        }
                    }
                }

                contentItem: RowLayout {
                    spacing: 10

IconLabel {
                            id: widgetChoicePreview
                            Layout.preferredWidth: 48
                            Layout.preferredHeight: 48

                            icon {
                                source: Icons[widgetEntries.get(widgetSelection.currentIndex).iconName]
                                width: widgetChoicePreview.width
                                height: widgetChoicePreview.height
                            }

                        Accessible.ignored: true
                    }

                    Label {
                        Layout.fillWidth: true
                        textFormat: Text.RichText
                        text: "<b>" + widgetEntries.get(widgetSelection.currentIndex).name + "</b><br>"
                              + widgetEntries.get(widgetSelection.currentIndex).description

                        Accessible.ignored: true
                    }
                }

                onCurrentIndexChanged: {
                    const entry = widgetEntries.get(currentIndex)
                    control.selection = entry ? entry.type : -1

                    widgetSettingsModel.clear()
                    widgetSettings.roomSelected = false

                    switch (control.selection) {
                        case CommonWidgets.Type.WebView:
                            const newSettings = [
                                { name: qsTr("Title"), setting: "headerTitle", checkable: 0 },
                                { name: qsTr("URL"), setting: "lightModeUrl", checkable: 0 },
                                { name: qsTr("URL (dark mode)"), setting: "darkModeUrl", checkable: 0 },
                                { name: qsTr("Accept all certificates"), setting: "acceptAllCerts", checkable: 1 }
                            ]

                            newSettings.forEach(item => widgetSettingsModel.append(item))
                            break
                        case CommonWidgets.Type.Chat: {
                            const chatSettings = [
                                { name: qsTr("Chat room"), setting: "room", checkable: 0, roomPicker: 1 }
                            ]

                            chatSettings.forEach(item => widgetSettingsModel.append(item))
                            break
                        }
                    }
                }
            }

            ColumnLayout {
                id: widgetSettings
                spacing: 10
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.topMargin: 20
                Layout.bottomMargin: 20

                signal settingsFinished

                property bool roomSelected: false

                ListModel {
                    id: widgetSettingsModel
                }

                Repeater {
                    id: widgetSettingsInput
                    model: widgetSettingsModel
                    delegate: ColumnLayout {
                        id: widgetSettingsDelegate
                        spacing: 10
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.bottomMargin: 10

                        required property int index

                        property string value

                        Component {
                            id: delgInputComp

                            TextField {
                                id: delgInput
                                text: ""

                                Accessible.role: Accessible.EditableText
                                Accessible.name: qsTr("Settings text input")
                                Accessible.description: qsTr("Input for widget setting %1").arg(delgLabel.text)
                                Accessible.focusable: true

                                Connections {
                                    target: widgetSettings
                                    function onSettingsFinished() {
                                        widgetSettingsDelegate.value = delgInput.text
                                    }
                                }
                            }
                        }

                        Component {
                            id: delgCheckComp

                            CheckBox {
                                id: delgCheck

                                Accessible.role: Accessible.CheckBox
                                Accessible.name: qsTr("Settings checkbox")
                                Accessible.description: qsTr("Checkbox for widget setting %1").arg(delgLabel.text)
                                Accessible.focusable: true

                                Connections {
                                    target: widgetSettings
                                    function onSettingsFinished() {
                                        widgetSettingsDelegate.value = delgCheck.checked.toString()
                                    }
                                }
                            }
                        }

                        Component {
                            id: delgRoomPickerComp

                            ColumnLayout {
                                id: roomPickerContainer
                                spacing: 10

                                QtObject {
                                    id: roomPickerInternal

                                    property string selectedRoomId: ""
                                    property string roomSearchFilterText: ""

                                    readonly property Timer searchDebouncer: Timer {
                                        id: roomSearchDebounceTimer
                                        interval: 200

                                        onTriggered: () => {
                                            roomPickerInternal.roomSearchFilterText = roomSearchField.text.trim()
                                        }
                                    }
                                }

                                TextField {
                                    id: roomSearchField
                                    placeholderText: qsTr("Search for chat rooms...")
                                    Layout.fillWidth: true

                                    onTextEdited: () => roomPickerInternal.searchDebouncer.start()

                                    Accessible.role: Accessible.EditableText
                                    Accessible.name: qsTr("Chat room search input")
                                    Accessible.description: qsTr("Search input to filter the chat rooms for the widget")
                                    Accessible.focusable: true
                                }

                                Item {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 200

                                    ListView {
                                        id: roomListView
                                        clip: true
                                        anchors.fill: parent

                                        model: ChatRoomProxyModel {
                                            sourceModel: AllChatProvidersRoomProxyModel {}
                                            sortStrategy: ChatRoomProxyModel.SortStrategy.Alphabetical
                                            showSectionHeader: false
                                            filterText: roomPickerInternal.roomSearchFilterText
                                        }

                                        ScrollBar.vertical: ScrollBar {
                                            policy: ScrollBar.AsNeeded
                                            width: 10
                                        }

                                        delegate: RadioButton {
                                            id: delgRoomItem
                                            height: 48

                                            required property string roomId
                                            required property string name
                                            required property string avatarPath
                                            required property IChatProvider chatProvider

                                            anchors {
                                                left: parent?.left
                                                right: parent?.right
                                            }

                                            // The radio buttons are not interactive: their checked state is
                                            // fully driven by selectedRoomId so that clicking a row can never
                                            // uncheck itself or break the binding.
                                            checkable: false
                                            checked: roomPickerInternal.selectedRoomId === delgRoomItem.roomId

                                            // The template positions its default indicator centered when the
                                            // content item is empty, so the indicator is suppressed and drawn
                                            // as a left-aligned child instead.
                                            indicator: Item {}

                                            background: Rectangle {
                                                color: delgRoomItem.checked
                                                       ? Theme.backgroundOffsetHoveredColor
                                                       : (delgRoomItem.hovered
                                                          ? Theme.backgroundOffsetColor
                                                          : "transparent")
                                                radius: 4

                                                Accessible.ignored: true
                                            }

                                            Rectangle {
                                                width: 18
                                                height: 18
                                                radius: width / 2
                                                color: "transparent"
                                                border.width: 2
                                                border.color: delgRoomItem.checked
                                                             ? Theme.accentColor
                                                             : Theme.backgroundOffsetColor
                                                anchors {
                                                    left: parent.left
                                                    leftMargin: 12
                                                    verticalCenter: parent.verticalCenter
                                                }

                                                Rectangle {
                                                    visible: delgRoomItem.checked
                                                    width: 10
                                                    height: 10
                                                    radius: width / 2
                                                    color: Theme.accentColor
                                                    anchors.centerIn: parent
                                                }

                                                Accessible.ignored: true
                                            }

                                            // The row content is added as a plain child and centered via anchors on
                                            // the control itself.
                                            RowLayout {
                                                spacing: Theme.d
                                                anchors {
                                                    left: parent.left
                                                    leftMargin: Theme.d * 3.5
                                                    right: parent.right
                                                    rightMargin: Theme.d
                                                    verticalCenter: parent.verticalCenter
                                                }

                                                AvatarImage {
                                                    size: Theme.d * 3
                                                    source: delgRoomItem.avatarPath
                                                    initials: ViewHelper.initials(delgRoomItem.name)

                                                    Accessible.ignored: true
                                                }

                                                Label {
                                                    text: delgRoomItem.name
                                                    elide: Label.ElideRight
                                                    Layout.fillWidth: true

                                                    Accessible.role: Accessible.StaticText
                                                    Accessible.name: delgRoomItem.name
                                                }

                                                Label {
                                                    text: delgRoomItem.chatProvider?.displayName ?? ""
                                                    color: Theme.secondaryTextColor
                                                    font.pixelSize: 12

                                                    Accessible.ignored: true
                                                }
                                            }

                                            onClicked: () => {
                                                roomPickerInternal.selectedRoomId = delgRoomItem.roomId
                                                widgetSettingsDelegate.value = `${delgRoomItem.chatProvider.id}|${delgRoomItem.roomId}`
                                                widgetSettings.roomSelected = true
                                            }

                                            Accessible.role: Accessible.RadioButton
                                            Accessible.name: qsTr("Select chat room %1").arg(delgRoomItem.name)
                                            Accessible.focusable: true
                                        }
                                    }

                                    Label {
                                        id: roomPickerEmptyHint
                                        visible: roomListView.count === 0
                                        color: Theme.secondaryTextColor
                                        wrapMode: Label.Wrap
                                        anchors.centerIn: parent
                                        text: roomSearchField.text.trim().length
                                              ? qsTr("No chat rooms found.")
                                              : qsTr("No chat rooms available yet")

                                        Accessible.role: Accessible.StaticText
                                        Accessible.name: roomPickerEmptyHint.text
                                    }
                                }
                            }
                        }

                        Label {
                            id: delgLabel
                            text: widgetSettingsModel.count > 0
                                  ? widgetSettingsModel.get(widgetSettingsDelegate.index).name
                                  : ""

                            Accessible.role: Accessible.StaticText
                            Accessible.name: qsTr("Widget setting %1").arg(delgLabel.text)
                        }

                        Loader {
                            id: delgLoader
                            Layout.fillWidth: !delgLoader.isCheckable || delgLoader.isRoomPicker
                            sourceComponent: delgLoader.isRoomPicker
                                             ? delgRoomPickerComp
                                             : (delgLoader.isCheckable
                                                ? delgCheckComp
                                                : delgInputComp)

                            property bool isCheckable: widgetSettingsModel.count > 0
                                                       ? widgetSettingsModel.get(widgetSettingsDelegate.index).checkable
                                                       : false
                            property bool isRoomPicker: widgetSettingsModel.count > 0
                                                        ? widgetSettingsModel.get(widgetSettingsDelegate.index).roomPicker === 1
                                                        : false
                        }
                    }
                }
            }

            RowLayout {
                id: widgetButtons
                spacing: 10
                Layout.preferredHeight: 90
                Layout.fillWidth: true
                layoutDirection: Qt.RightToLeft
                Layout.alignment: Qt.AlignRight

                Button {
                    id: widgetCancel
                    text: qsTr("Cancel")

                    onClicked: () => control.close()

                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Cancel widget selection")
                    Accessible.description: qsTr("Cancel button to exit widget selection selection without changes")
                    Accessible.focusable: true
                    Accessible.onPressAction: () => widgetCancel.click()
                }

                Button {
                    id: widgetConfirm
                    icon.source: Icons.listAdd
                    text: qsTr("Add")
                    enabled: control.selection !== CommonWidgets.Type.Chat || widgetSettings.roomSelected

                    onClicked: () => widgetConfirm.createWidget()

                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Confirm widget selection")
                    Accessible.description: qsTr("Confirmation button to create and add the selected widget to the current dashboard")
                    Accessible.focusable: true
                    Accessible.onPressAction: () => widgetConfirm.click()

                    function createWidget() {
                        const id = `-widget_${UISettings.generateUuid()}`
                        const selection = control.selection

                        const widgetProperties = {
                            widgetId: control.widgetRoot.pageId + id,
                            page: control.widgetRoot,
                            gridWidth: Qt.binding(() => control.widgetRoot.gridWidth),
                            gridHeight: Qt.binding(() => control.widgetRoot.gridHeight),
                            gridCellWidth: Qt.binding(() => control.widgetRoot.gridCellWidth),
                            gridCellHeight: Qt.binding(() => control.widgetRoot.gridCellHeight),
                            xGrid: Math.floor(0.25 * control.widgetRoot.gridWidth / control.widgetRoot.gridCellWidth),
                            yGrid: Math.floor(0.25 * control.widgetRoot.gridHeight / control.widgetRoot.gridCellHeight),
                            widthGrid: Math.floor(0.5 * control.widgetRoot.gridWidth / control.widgetRoot.gridCellWidth),
                            heightGrid: Math.floor(0.5 * control.widgetRoot.gridHeight / control.widgetRoot.gridCellHeight),
                        }

                        let widget
                        switch (selection) {
                            case CommonWidgets.Type.DateEvents:
                                widget = widgets.dateEvents.createObject(control.widgetRoot.grid, widgetProperties)
                                break
                            case CommonWidgets.Type.Favorites:
                                widget = widgets.favorites.createObject(control.widgetRoot.grid, widgetProperties)
                                break
                            case CommonWidgets.Type.History:
                                widget = widgets.history.createObject(control.widgetRoot.grid, widgetProperties)
                                break
                            case CommonWidgets.Type.WebView:
                                widget = widgets.webview.createObject(control.widgetRoot.grid, widgetProperties)
                                break
                            case CommonWidgets.Type.Chat:
                                widget = widgets.chat.createObject(control.widgetRoot.grid, widgetProperties)
                                break
                            case CommonWidgets.Type.Activities:
                                widget = widgets.activities.createObject(control.widgetRoot.grid, widgetProperties)
                                break
                            default:
                                widget = null
                                console.error(category, `widget type ${selection} unknown`)
                        }

                        if (widget) {
                            // Per-widget settings
                            const additionalSettings = widgetSettingsInput.count
                            const hasCustomSettings = additionalSettings > 0

                            if (hasCustomSettings) {
                                widgetSettings.settingsFinished()
                                for (let i = 0; i < additionalSettings; i++) {
                                    const key = widgetSettingsModel.get(i).setting
                                    const value = widgetSettingsInput.itemAt(i).value
                                    if (key && value) {
                                        widget.config.set(key, value)
                                    }
                                }
                                widget.additionalSettingsLoaded()
                            }

                            control.widgetRoot.resetWidgetElevation()
                            control.widgetRoot.model.add(widget)
                        } else {
                            console.error(category, "could not create widget component")
                        }

                        control.close()
                    }
                }
            }
        }
    }
}
