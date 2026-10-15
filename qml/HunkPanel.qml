pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

Item {
    id: root
    property bool stagedMode: false

    component HChip: Rectangle {
        id: chip
        property string label: ""
        property bool active: false
        property bool danger: false
        property bool enabledChip: true
        signal clicked()
        radius: Theme.radiusSm
        opacity: chip.enabledChip ? 1 : 0.4
        border.color: chip.danger ? Theme.destructive
                      : (chip.active ? Theme.copper : Theme.inkLine)
        border.width: 1
        color: chipArea.containsMouse || chip.active ? Theme.inkPanel : "transparent"
        implicitHeight: 24
        implicitWidth: chipLabel.implicitWidth + 12
        Text {
            id: chipLabel
            anchors.centerIn: parent
            text: chip.label
            color: chip.danger ? Theme.destructive
                   : (chip.active ? Theme.copperSoft : Theme.mist)
            font.family: Theme.uiFamily
            font.pixelSize: 11
        }
        MouseArea {
            id: chipArea
            anchors.fill: parent
            enabled: chip.enabledChip
            hoverEnabled: true
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: chip.clicked()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Text {
                text: I18n.i18n("Hunks")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 10
                font.letterSpacing: 1.2
                font.capitalization: Font.AllUppercase
                opacity: 0.65
            }
            Item { Layout.fillWidth: true }
            Text {
                visible: App.hunks.count > 0
                text: App.hunks.selectedCount + "/" + App.hunks.count
                color: Theme.copperSoft
                font.family: Theme.monoFamily
                font.pixelSize: 11
            }
        }

        // Режим + загрузка
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Text {
                text: I18n.i18n("Режим")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 10
                font.capitalization: Font.AllUppercase
                opacity: 0.5
                Layout.preferredWidth: 48
            }
            HChip {
                label: I18n.i18n("unstaged")
                active: !root.stagedMode
                onClicked: root.stagedMode = false
            }
            HChip {
                label: I18n.i18n("staged")
                active: root.stagedMode
                onClicked: root.stagedMode = true
            }
            Item { Layout.fillWidth: true }
            HChip {
                label: I18n.i18n("Загрузить")
                enabledChip: App.repo.isOpen && !App.busy
                onClicked: {
                    if (root.stagedMode)
                        App.loadStagedHunks(App.repo.selectedDiffPath)
                    else
                        App.loadUnstagedHunks(App.repo.selectedDiffPath)
                }
            }
        }

        // Выбор + действие
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            visible: App.hunks.count > 0
            Text {
                text: I18n.i18n("Выбор")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 10
                font.capitalization: Font.AllUppercase
                opacity: 0.5
                Layout.preferredWidth: 48
            }
            HChip {
                label: I18n.i18n("Все")
                onClicked: App.hunks.selectAll(true)
            }
            HChip {
                label: I18n.i18n("Сброс")
                onClicked: App.hunks.selectAll(false)
            }
            Item { Layout.fillWidth: true }
            HChip {
                label: root.stagedMode
                       ? I18n.i18n("Из индекса (%1)", App.hunks.selectedCount)
                       : I18n.i18n("В индекс (%1)", App.hunks.selectedCount)
                active: App.hunks.selectedCount > 0
                enabledChip: App.hunks.selectedCount > 0 && !App.busy
                onClicked: {
                    if (root.stagedMode)
                        App.unstageSelectedHunks()
                    else
                        App.stageSelectedHunks()
                }
            }
            HChip {
                visible: !root.stagedMode
                label: I18n.i18n("Откатить")
                danger: true
                enabledChip: App.hunks.selectedCount > 0 && !App.busy
                onClicked: App.discardSelectedHunks()
            }
            HChip {
                label: ".patch"
                enabledChip: App.hunks.selectedCount > 0
                onClicked: App.exportSelectedHunksPatch()
            }
        }

        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 11
            opacity: 0.65
            text: root.stagedMode
                  ? I18n.i18n("Убрать выбранные hunks из индекса (`git apply --cached -R`).")
                  : I18n.i18n("Stage / discard / .patch для unstaged — без `add -p`.")
        }

        Text {
            visible: App.hunks.count === 0
            Layout.fillWidth: true
            text: I18n.i18n("Hunks не загружены. Сделайте правки и нажмите «Загрузить».")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 12
            opacity: 0.7
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: App.hunks.count > 0

            ListView {
                model: App.hunks
                spacing: 4
                clip: true
                delegate: Rectangle {
                    id: hunkDel
                    required property int index
                    required property string filePath
                    required property string header
                    required property string preview
                    required property bool selected
                    width: ListView.view.width
                    height: hunkCol.implicitHeight + 12
                    radius: Theme.radiusSm
                    color: Theme.inkElevated
                    border.color: hunkDel.selected ? Theme.copper : Theme.inkLine
                    border.width: 1

                    ColumnLayout {
                        id: hunkCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: 8
                        spacing: 3

                        RowLayout {
                            Layout.fillWidth: true
                            Controls.CheckBox {
                                checked: hunkDel.selected
                                onToggled: App.hunks.setSelected(hunkDel.index, checked)
                            }
                            Text {
                                text: hunkDel.filePath
                                color: Theme.copperSoft
                                font.family: Theme.monoFamily
                                font.pixelSize: 11
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: hunkDel.header
                            color: Theme.mist
                            font.family: Theme.monoFamily
                            font.pixelSize: 10
                            elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            text: hunkDel.preview
                            color: Theme.paper
                            font.family: Theme.monoFamily
                            font.pixelSize: 10
                            wrapMode: Text.NoWrap
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }
}
