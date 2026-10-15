pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

// Title, kind, search, and context actions for blame, commit, stash, and clean preview.
ColumnLayout {
    id: root

    property string searchText: ""
    property int matchCount: 0
    signal searchEdited(string text)
    signal copyRequested()
    signal commitAction(var item)

    spacing: Theme.space8

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.space8

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Text {
                Layout.fillWidth: true
                text: App.repo.inspectTitle.length ? App.repo.inspectTitle : I18n.i18n("Просмотр")
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeBody
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
            Text {
                visible: App.repo.inspectTarget.length > 0
                text: App.repo.inspectTarget
                color: Theme.mist
                font.family: Theme.monoFamily
                font.pixelSize: 11
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }
        }

        ChipButton {
            visible: App.repo.inspectKind.length > 0
            label: App.repo.inspectKind
            active: true
            accent: Theme.copper
            labelColor: Theme.copperSoft
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.space6
        visible: App.repo.inspectText.length > 0

        Controls.TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: I18n.i18n("Фильтр строк просмотра")
            color: Theme.paper
            font.family: Theme.uiFamily
            font.pixelSize: 12
            onTextChanged: root.searchEdited(text)
            background: Rectangle {
                implicitHeight: 28
                radius: Theme.radiusSm
                color: Theme.inkPanel
                border.color: searchField.activeFocus ? Theme.copper : Theme.inkLine
                border.width: 1
            }
        }
        ChipButton {
            label: I18n.i18n("копир.")
            onClicked: root.copyRequested()
        }
        Text {
            visible: root.searchText.length > 0
            text: String(root.matchCount)
            color: Theme.copperSoft
            font.family: Theme.monoFamily
            font.pixelSize: 12
        }
    }

    ChipFlow {
        Layout.fillWidth: true
        spacing: Theme.space4
        visible: App.repo.inspectKind === "commit" && App.repo.inspectTarget.length > 0

        ChipButton {
            label: I18n.i18n("Cherry-pick")
            onClicked: root.commitAction({ id: "cherry_pick", args: { commit: true } })
        }
        ChipButton {
            label: I18n.i18n("Revert")
            accent: Theme.destructive
            labelColor: Theme.destructive
            onClicked: root.commitAction({ id: "revert", args: { commit: true } })
        }
        ChipButton {
            label: I18n.i18n("Reset --soft")
            accent: Theme.mutates
            onClicked: root.commitAction({ id: "reset_soft", args: { rev: true } })
        }
        ChipButton {
            label: I18n.i18n("Тег…")
            onClicked: root.commitAction({ tag: true })
        }
        ChipButton {
            label: I18n.i18n("Сравнить A")
            active: App.repo.compareA === App.repo.inspectTarget && App.repo.inspectTarget.length > 0
            onClicked: root.commitAction({ asA: true })
        }
        ChipButton {
            label: I18n.i18n("Сравнить B")
            active: App.repo.compareB === App.repo.inspectTarget && App.repo.inspectTarget.length > 0
            onClicked: root.commitAction({ asA: false })
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.space8
        visible: App.repo.compareA.length > 0 || App.repo.compareB.length > 0

        Text {
            text: "A"
            color: Theme.copper
            font.family: Theme.monoFamily
            font.pixelSize: 11
        }
        Text {
            text: App.repo.compareA.length ? App.repo.compareA : "—"
            color: Theme.paper
            font.family: Theme.monoFamily
            font.pixelSize: 11
            elide: Text.ElideMiddle
            Layout.preferredWidth: 90
        }
        Text {
            text: "B"
            color: Theme.copper
            font.family: Theme.monoFamily
            font.pixelSize: 11
        }
        Text {
            Layout.fillWidth: true
            text: App.repo.compareB.length ? App.repo.compareB : "—"
            color: Theme.paper
            font.family: Theme.monoFamily
            font.pixelSize: 11
            elide: Text.ElideMiddle
        }
        ActionButton {
            visible: App.repo.compareA.length > 0 && App.repo.compareB.length > 0
            text: I18n.i18n("Сравнить")
            kind: "ghost"
            onClicked: App.compareCommits(App.repo.compareA, App.repo.compareB)
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.space8
        visible: App.repo.inspectKind === "clean"

        Text {
            text: I18n.i18n("clean -fd удалит неотслеживаемые файлы. Сначала смотрите список.")
            color: Theme.destructive
            font.family: Theme.uiFamily
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        ActionButton {
            text: I18n.i18n("clean -fd…")
            kind: "danger"
            onClicked: App.prepareCommand("clean_force", {})
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.space8
        visible: App.repo.inspectKind === "stash" && App.repo.inspectTarget.length > 0

        Text {
            Layout.fillWidth: true
            text: I18n.i18n("Stash %1", App.repo.inspectTarget)
            color: Theme.mist
            font.family: Theme.monoFamily
            font.pixelSize: 12
            elide: Text.ElideMiddle
        }
        ActionButton {
            text: I18n.i18n("Применить")
            kind: "primary"
            onClicked: App.prepareCommand("stash_apply", { stash: App.repo.inspectTarget })
        }
    }
}
