pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

// Path, line stats, search, view mode, and path actions above the diff body.
ColumnLayout {
    id: root

    property bool wordDiff: false
    property bool sideBySide: false
    property string searchText: ""
    property int plusLines: 0
    property int minusLines: 0
    property bool conflict: false
    property var actionGroups: []

    signal searchEdited(string text)
    signal wordToggled()
    signal splitToggled()
    signal copyRequested()
    signal actionTriggered(var item)

    spacing: Theme.space8

    readonly property var crumbs: {
        const path = App.repo.selectedDiffPath || ""
        if (!path.length)
            return []
        return path.split("/")
    }

    EmptyHint {
        Layout.fillWidth: true
        visible: App.repo.selectedDiffPath.length === 0
        title: I18n.i18n("Diff не выбран")
        body: I18n.i18n("Кликните файл в дереве слева. Здесь будут путь, счётчик строк и действия над файлом.")
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.space6
        visible: App.repo.selectedDiffPath.length > 0

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space6

            ChipFlow {
                Layout.fillWidth: true
                spacing: 2
                Repeater {
                    model: root.crumbs
                    delegate: Text {
                        required property string modelData
                        required property int index
                        text: (index > 0 ? " / " : "") + modelData
                        color: index === root.crumbs.length - 1 ? Theme.paper : Theme.mist
                        font.family: Theme.monoFamily
                        font.pixelSize: 12
                    }
                }
            }

            ChipButton {
                label: App.repo.selectedDiffStaged ? I18n.i18n("индекс") : I18n.i18n("рабочая")
                active: true
                accent: App.repo.selectedDiffStaged ? Theme.safe : Theme.mutates
                labelColor: App.repo.selectedDiffStaged ? Theme.safe : Theme.mutates
            }
            ChipButton {
                visible: root.conflict
                label: I18n.i18n("конфликт")
                active: true
                accent: Theme.destructive
                labelColor: Theme.destructive
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space8

            Text {
                text: "+" + root.plusLines
                color: Theme.safe
                font.family: Theme.monoFamily
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }
            Text {
                text: "−" + root.minusLines
                color: Theme.destructive
                font.family: Theme.monoFamily
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }
            MeterTrack {
                Layout.fillWidth: true
                Layout.preferredHeight: 8
                segments: [
                    { key: "add", label: I18n.i18n("добавлено"), value: root.plusLines, color: Theme.safe },
                    { key: "del", label: I18n.i18n("удалено"), value: root.minusLines, color: Theme.destructive }
                ]
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space6

            Controls.TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: I18n.i18n("Фильтр строк diff")
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: 12
                onTextChanged: {
                    if (text !== root.searchText)
                        root.searchEdited(text)
                }
                background: Rectangle {
                    implicitHeight: 28
                    radius: Theme.radiusSm
                    color: Theme.inkPanel
                    border.color: searchField.activeFocus ? Theme.copper : Theme.inkLine
                    border.width: 1
                }
            }

            ChipButton {
                label: "word"
                active: root.wordDiff
                tip: I18n.i18n("Пословный diff")
                onClicked: root.wordToggled()
            }
            ChipButton {
                label: "split"
                active: root.sideBySide
                tip: I18n.i18n("Две колонки")
                onClicked: root.splitToggled()
            }
            ChipButton {
                label: I18n.i18n("копир.")
                enabled: App.repo.selectedDiff.length > 0
                tip: I18n.i18n("Скопировать весь diff")
                onClicked: root.copyRequested()
            }
        }

        Repeater {
            model: root.actionGroups
            delegate: RowLayout {
                id: group
                required property var modelData
                Layout.fillWidth: true
                spacing: Theme.space6
                visible: group.modelData.hide !== true

                Text {
                    text: group.modelData.title
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: 10
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 0.8
                    Layout.preferredWidth: 72
                    elide: Text.ElideRight
                }

                ChipFlow {
                    Layout.fillWidth: true
                    spacing: Theme.space4
                    Repeater {
                        model: group.modelData.items
                        delegate: ChipButton {
                            required property var modelData
                            label: modelData.label
                            accent: group.modelData.title === I18n.i18n("Конфликт")
                                    ? Theme.destructive : Theme.copper
                            onClicked: root.actionTriggered(modelData)
                        }
                    }
                }
            }
        }
    }
}
