pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

// Staged / unstaged / untracked / conflict as one bar plus a legend that filters the tree.
ColumnLayout {
    id: root

    signal focusState(string stateKey)

    spacing: Theme.space6

    readonly property var parts: [
        {
            key: "staged",
            label: I18n.i18n("Индекс"),
            short: "S",
            value: App.repo.stagedCount,
            color: Theme.safe
        },
        {
            key: "unstaged",
            label: I18n.i18n("Рабочая копия"),
            short: "U",
            value: App.repo.unstagedCount,
            color: Theme.mutates
        },
        {
            key: "untracked",
            label: I18n.i18n("Новые"),
            short: "?",
            value: App.repo.untrackedCount,
            color: Theme.copperSoft
        },
        {
            key: "conflict",
            label: I18n.i18n("Конфликты"),
            short: "!",
            value: App.repo.conflictCount,
            color: Theme.destructive
        }
    ]

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.space8

        Text {
            text: App.repo.dirtyCount > 0
                  ? I18n.i18n("%1 правок", App.repo.dirtyCount)
                  : I18n.i18n("Рабочее дерево чистое")
            color: App.repo.conflictCount > 0 ? Theme.destructive : Theme.paper
            font.family: Theme.uiFamily
            font.pixelSize: Theme.typeMeta
            font.weight: Font.DemiBold
        }

        Item { Layout.fillWidth: true }

        Text {
            visible: App.repo.diffStat.length > 0
            text: App.repo.diffStat
            color: Theme.mist
            font.family: Theme.monoFamily
            font.pixelSize: 10
            elide: Text.ElideLeft
            Layout.maximumWidth: 180
        }
    }

    MeterTrack {
        Layout.fillWidth: true
        Layout.preferredHeight: 12
        segments: root.parts
        onSegmentActivated: function (key) { root.focusState(key) }
    }

    Flow {
        Layout.fillWidth: true
        spacing: Theme.space4

        Repeater {
            model: root.parts
            delegate: StatPill {
                required property var modelData
                label: modelData.short
                valueText: String(modelData.value)
                numericValue: modelData.value
                accent: modelData.color
                tip: modelData.label + " · " + I18n.i18n("показать в дереве")
                onActivated: root.focusState(modelData.key)
            }
        }
    }
}
