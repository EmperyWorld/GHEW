pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

// Counts of refs the workspace already loaded. Each tile opens the matching catalog command.
ColumnLayout {
    id: root

    signal openCommand(string commandId)

    spacing: Theme.space8
    visible: App.repo.isOpen

    readonly property var tiles: [
        {
            label: I18n.i18n("Ветки"),
            value: App.repo.branches.count,
            commandId: "branch_list",
            accent: Theme.copper,
            tip: I18n.i18n("Список локальных и remote веток")
        },
        {
            label: I18n.i18n("Теги"),
            value: App.repo.tags.count,
            commandId: "tag_list",
            accent: Theme.copperSoft,
            tip: I18n.i18n("Метки репозитория")
        },
        {
            label: I18n.i18n("Stash"),
            value: App.repo.stashes.count,
            commandId: "stash_list",
            accent: Theme.mutates,
            tip: I18n.i18n("Отложенные правки")
        },
        {
            label: I18n.i18n("Remote"),
            value: App.repo.remotes.count,
            commandId: "remote_show",
            accent: Theme.safe,
            tip: I18n.i18n("Настроенные удалённые репозитории")
        },
        {
            label: I18n.i18n("Worktree"),
            value: App.repo.worktrees.count,
            commandId: "worktree_list",
            accent: Theme.mist,
            tip: I18n.i18n("Дополнительные рабочие деревья")
        },
        {
            label: I18n.i18n("Модули"),
            value: App.repo.submodules.count,
            commandId: "submodule_status",
            accent: Theme.mist,
            tip: I18n.i18n("Подмодули")
        },
        {
            label: I18n.i18n("Hooks"),
            value: App.repo.hooks.count,
            commandId: "",
            accent: Theme.inkLine,
            tip: I18n.i18n("Хуки в .git/hooks — только счётчик")
        },
        {
            label: I18n.i18n("Reflog"),
            value: App.repo.reflog.count,
            commandId: "reflog",
            accent: Theme.copper,
            tip: I18n.i18n("Журнал перемещений HEAD")
        }
    ]

    SectionHead {
        Layout.fillWidth: true
        title: I18n.i18n("Ссылки")
        meta: App.repo.rootPath.split("/").pop() || ""
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 4
        rowSpacing: Theme.space6
        columnSpacing: Theme.space6

        Repeater {
            model: root.tiles
            delegate: Rectangle {
                id: tile
                required property var modelData
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                radius: Theme.radiusSm
                color: area.containsMouse && tile.modelData.commandId.length ? Theme.inkPanel : "#10161e"
                border.color: tile.modelData.value > 0 ? tile.modelData.accent : Theme.inkLine
                border.width: 1
                opacity: tile.modelData.value > 0 ? 1 : 0.55

                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: Theme.space8
                    spacing: 2
                    Text {
                        text: String(tile.modelData.value)
                        color: tile.modelData.value > 0 ? tile.modelData.accent : Theme.mist
                        font.family: Theme.monoFamily
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }
                    Text {
                        width: parent.width
                        text: tile.modelData.label
                        color: Theme.mist
                        font.family: Theme.uiFamily
                        font.pixelSize: 10
                        elide: Text.ElideRight
                    }
                }

                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: tile.modelData.commandId.length > 0
                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: root.openCommand(tile.modelData.commandId)
                }
            }
        }
    }
}
