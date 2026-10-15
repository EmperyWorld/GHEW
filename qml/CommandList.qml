pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

Item {
    id: root

    clip: true

    property string selectedId: ""
    property string selectedTitle: ""
    property string selectedSynopsis: ""
    property string selectedExplanation: ""
    property string selectedDanger: ""

    function applySelection(commandId, title, synopsis, explanation, danger) {
        root.selectedId = commandId
        root.selectedTitle = title
        root.selectedSynopsis = synopsis
        root.selectedExplanation = explanation
        root.selectedDanger = danger
    }

    function selectById(commandId) {
        if (!commandId || commandId.length === 0)
            return false
        App.catalog.setSearchText("")
        searchField.text = ""
        App.catalog.setGroupFilter("")
        groupBox.currentIndex = 0
        const cmd = App.catalog.commandById(commandId)
        if (!cmd || !cmd.id)
            return false
        root.applySelection(cmd.id, cmd.title, cmd.synopsis, cmd.explanation, cmd.danger)
        const n = App.catalog.count
        for (let i = 0; i < n; ++i) {
            const row = App.catalog.commandAt(i)
            if (row.id === commandId) {
                list.currentIndex = i
                return true
            }
        }
        list.currentIndex = -1
        return true
    }

    function focusSearch() {
        searchField.forceActiveFocus()
        searchField.selectAll()
    }

    readonly property var groupKeys: App.catalog.groups
    readonly property var groupLabels: {
        const labels = [I18n.i18n("Все группы")]
        for (let i = 0; i < groupKeys.length; ++i)
            labels.push(App.catalog.groupTitle(groupKeys[i]))
        return labels
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 5

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Text {
                text: I18n.i18n("Команды")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 10
                font.letterSpacing: 1.2
                font.capitalization: Font.AllUppercase
                opacity: 0.55
            }
            Item { Layout.fillWidth: true }
            Text {
                visible: App.catalog.count > 0
                text: String(App.catalog.count)
                color: Theme.mist
                font.family: Theme.monoFamily
                font.pixelSize: 10
                opacity: 0.7
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            clip: true

            Controls.TextField {
                id: searchField
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.minimumWidth: 64
                placeholderText: I18n.i18n("Поиск… Ctrl+K")
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                onTextChanged: App.catalog.setSearchText(text)
                Keys.onEscapePressed: {
                    text = ""
                    focus = false
                }
                background: Rectangle {
                    implicitHeight: Theme.controlH
                    radius: Theme.radiusSm
                    color: Theme.inkPanel
                    border.color: searchField.activeFocus ? Theme.copper : Theme.inkLine
                    border.width: 1
                }
            }

            Controls.ComboBox {
                id: groupBox
                Layout.preferredWidth: Math.min(110, Math.max(72, root.width * 0.32))
                Layout.maximumWidth: Math.min(130, root.width * 0.42)
                Layout.minimumWidth: 72
                model: root.groupLabels
                onActivated: {
                    if (currentIndex === 0)
                        App.catalog.setGroupFilter("")
                    else
                        App.catalog.setGroupFilter(root.groupKeys[currentIndex - 1])
                }

                background: Rectangle {
                    implicitHeight: Theme.controlH
                    radius: Theme.radiusSm
                    color: Theme.inkPanel
                    border.color: Theme.inkLine
                    border.width: 1
                }
                contentItem: Text {
                    leftPadding: 6
                    rightPadding: groupBox.indicator.width + 4
                    text: groupBox.displayText
                    color: Theme.paper
                    font.family: Theme.uiFamily
                    font.pixelSize: Theme.typeMeta
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }
        }

        Text {
            visible: App.catalog.count === 0
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: I18n.i18n("Ничего не найдено.")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 12
            opacity: 0.7
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            ListView {
                id: list
                model: App.catalog
                currentIndex: -1
                clip: true
                spacing: 2
                boundsBehavior: Flickable.StopAtBounds

                delegate: Item {
                    id: del
                    required property int index
                    required property string commandId
                    required property string title
                    required property string synopsis
                    required property string explanation
                    required property string danger

                    width: ListView.view.width
                    height: 40

                    readonly property bool selected: ListView.isCurrentItem

                    Rectangle {
                        anchors.fill: parent
                        radius: Theme.radiusSm
                        color: del.selected ? Theme.inkPanel : "transparent"
                        border.color: del.selected ? Theme.inkLine : "transparent"
                        border.width: 1

                        Behavior on color { ColorAnimation { duration: 120 } }
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: 2
                        height: del.selected ? 22 : 0
                        radius: 1
                        color: Theme.copper
                        Behavior on height {
                            NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
                        }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 8
                        anchors.topMargin: 5
                        anchors.bottomMargin: 5
                        spacing: 1

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: del.title
                                elide: Text.ElideRight
                                color: del.selected ? Theme.paper : Theme.mist
                                font.family: Theme.uiFamily
                                font.pixelSize: 12
                                font.weight: del.selected ? Font.DemiBold : Font.Normal
                                Layout.fillWidth: true
                            }
                            Rectangle {
                                Layout.preferredWidth: 6
                                Layout.preferredHeight: 6
                                radius: 3
                                color: Theme.dangerColor(del.danger)
                            }
                        }

                        Text {
                            text: del.synopsis
                            elide: Text.ElideRight
                            color: Theme.mist
                            font.family: Theme.monoFamily
                            font.pixelSize: 10
                            opacity: 0.6
                            Layout.fillWidth: true
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            list.currentIndex = del.index
                            root.selectedId = del.commandId
                            root.selectedTitle = del.title
                            root.selectedSynopsis = del.synopsis
                            root.selectedExplanation = del.explanation
                            root.selectedDanger = del.danger
                        }
                    }
                }
            }
        }
    }
}
