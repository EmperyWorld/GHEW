pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

Item {
    id: root
    implicitHeight: col.implicitHeight

    function chip(label, commandId, enabled, args, special) {
        return {
            label: label,
            id: commandId,
            en: enabled,
            args: args || {},
            special: special || ""
        }
    }

    readonly property string continueCommandId: {
        switch (App.repo.operationKind) {
        case "merge": return "merge_continue"
        case "rebase": return "rebase_continue"
        case "cherry-pick": return "cherry_pick_continue"
        case "revert": return "revert_continue"
        }
        return ""
    }

    readonly property string abortCommandId: {
        switch (App.repo.operationKind) {
        case "merge": return "merge_abort"
        case "rebase": return "rebase_abort"
        case "cherry-pick": return "cherry_pick_abort"
        case "revert": return "revert_abort"
        }
        return ""
    }

    readonly property var groups: [
        {
            title: I18n.i18n("Дерево"),
            items: [
                root.chip(I18n.i18n("status"), "status", App.repo.isOpen, {}),
                root.chip(I18n.i18n("diff"), "diff", App.repo.isOpen, {}),
                root.chip(I18n.i18n("add all"), "add_all", App.repo.isOpen && App.repo.dirtyCount > 0, {}),
                root.chip(I18n.i18n("unstage"), "restore_staged", App.repo.isOpen && App.repo.stagedCount > 0, { path: "." }),
                root.chip(I18n.i18n("stash"), "stash_push", App.repo.isOpen && App.repo.dirtyCount > 0, {}),
                root.chip(I18n.i18n("clean?"), "clean_preview", App.repo.isOpen && App.repo.untrackedCount > 0, {}, "clean")
            ]
        },
        {
            title: I18n.i18n("Коммит"),
            items: [
                root.chip(I18n.i18n("commit"), "commit", App.repo.isOpen && App.repo.stagedCount > 0, {}),
                root.chip(I18n.i18n("amend"), "commit_amend",
                          App.repo.isOpen && (App.repo.stagedCount > 0 || App.repo.localCommits > 0), {})
            ]
        },
        {
            title: I18n.i18n("Remote"),
            items: [
                root.chip(I18n.i18n("fetch"), "fetch",
                          App.repo.isOpen && App.repo.remotes.count > 0, {}),
                root.chip(I18n.i18n("prune"), "fetch_prune",
                          App.repo.isOpen && App.repo.remotes.count > 0, {}),
                root.chip(I18n.i18n("pull"), "pull",
                          App.repo.isOpen && App.repo.hasUpstream && App.repo.behind > 0, {}),
                root.chip(I18n.i18n("rebase↓"), "pull_rebase",
                          App.repo.isOpen && App.repo.hasUpstream && App.repo.behind > 0, {}),
                root.chip(I18n.i18n("push"), "push",
                          App.repo.isOpen && App.repo.hasUpstream && App.repo.ahead > 0
                          && App.repo.behind === 0, {}),
                root.chip(I18n.i18n("push -u"), "push",
                          App.repo.isOpen && !App.repo.hasUpstream && App.repo.remotes.count > 0
                          && App.repo.localCommits > 0 && App.repo.branch.length > 0,
                          {
                              setUpstream: true,
                              remote: App.repo.remotes.nameAt(0),
                              branch: App.repo.branch
                          })
            ]
        },
        {
            title: I18n.i18n("Операция"),
            hide: !App.repo.operationKind.length,
            items: [
                root.chip(I18n.i18n("continue"), root.continueCommandId,
                          root.continueCommandId.length > 0, {}),
                root.chip(I18n.i18n("abort"), root.abortCommandId,
                          root.abortCommandId.length > 0, {}),
                root.chip(I18n.i18n("conflicts"), "diff",
                          App.repo.conflictCount > 0, {})
            ]
        },
        {
            title: I18n.i18n("Bisect"),
            hide: !App.repo.bisectActive,
            items: [
                root.chip(I18n.i18n("good"), "bisect_good", App.repo.bisectActive, {}),
                root.chip(I18n.i18n("bad"), "bisect_bad", App.repo.bisectActive, {}),
                root.chip(I18n.i18n("reset"), "bisect_reset", App.repo.bisectActive, {})
            ]
        }
    ]

    function groupVisible(g) {
        if (g.hide === true)
            return false
        const items = g.items
        for (let i = 0; i < items.length; ++i) {
            if (items[i].en)
                return true
        }
        return false
    }

    ColumnLayout {
        id: col
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 4

        Rectangle {
            Layout.fillWidth: true
            visible: App.repo.operationKind.length > 0
            radius: Theme.radiusSm
            color: Theme.inkPanel
            border.color: App.repo.conflictCount > 0 ? Theme.destructive : Theme.mutates
            border.width: 1
            implicitHeight: opBanner.implicitHeight + 10

            RowLayout {
                id: opBanner
                anchors.fill: parent
                anchors.margins: 5
                spacing: 8

                Text {
                    text: {
                        const k = App.repo.operationKind
                        if (k === "merge")
                            return I18n.i18n("Merge в процессе")
                        if (k === "rebase")
                            return I18n.i18n("Rebase в процессе")
                        if (k === "cherry-pick")
                            return I18n.i18n("Cherry-pick в процессе")
                        if (k === "revert")
                            return I18n.i18n("Revert в процессе")
                        return k
                    }
                    color: App.repo.conflictCount > 0 ? Theme.destructive : Theme.mutates
                    font.family: Theme.uiFamily
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }
                Text {
                    visible: App.repo.conflictCount > 0
                    text: I18n.i18n("%1 конфликт(ов)").arg(App.repo.conflictCount)
                    color: Theme.destructive
                    font.family: Theme.monoFamily
                    font.pixelSize: 11
                }
                Text {
                    visible: App.repo.conflictCount === 0
                    text: I18n.i18n("можно продолжить или отменить")
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: 10
                    opacity: 0.85
                }
                Item { Layout.fillWidth: true }
            }
        }

        Repeater {
            model: root.groups
            delegate: RowLayout {
                id: groupRow
                required property var modelData
                Layout.fillWidth: true
                spacing: 6
                visible: root.groupVisible(groupRow.modelData)

                Text {
                    text: groupRow.modelData.title
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: 10
                    font.capitalization: Font.AllUppercase
                    opacity: 0.5
                    Layout.preferredWidth: 52
                }

                ChipFlow {
                    spacing: Theme.space4
                    Repeater {
                        model: groupRow.modelData.items
                        delegate: Rectangle {
                            id: chipDel
                            required property var modelData
                            visible: chipDel.modelData.en
                            radius: Theme.radiusSm
                            color: chipArea.containsMouse ? Theme.inkPanel : "transparent"
                            border.color: Theme.inkLine
                            border.width: 1
                            implicitHeight: 22
                            implicitWidth: chipLabel.implicitWidth + 12

                            Text {
                                id: chipLabel
                                anchors.centerIn: parent
                                text: chipDel.modelData.label
                                color: Theme.mist
                                font.family: Theme.uiFamily
                                font.pixelSize: 11
                            }
                            MouseArea {
                                id: chipArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (chipDel.modelData.special === "clean")
                                        App.loadCleanPreview()
                                    else
                                        App.runCommand(chipDel.modelData.id, chipDel.modelData.args || {}, false)
                                }
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            visible: App.favorites.count > 0

            Text {
                text: I18n.i18n("★")
                color: Theme.copper
                font.family: Theme.uiFamily
                font.pixelSize: 11
                Layout.preferredWidth: 52
                horizontalAlignment: Text.AlignHCenter
            }

            ChipFlow {
                spacing: Theme.space4
                Repeater {
                    model: App.favorites
                    delegate: Rectangle {
                        id: favChip
                        required property string commandId
                        radius: Theme.radiusSm
                        color: favArea.containsMouse ? Theme.inkPanel : "transparent"
                        border.color: Theme.copper
                        border.width: 1
                        implicitHeight: 22
                        implicitWidth: favLabel.implicitWidth + 12
                        readonly property var cmd: App.catalog.commandById(commandId)

                        Text {
                            id: favLabel
                            anchors.centerIn: parent
                            text: favChip.cmd && favChip.cmd.title ? favChip.cmd.title : favChip.commandId
                            color: Theme.copperSoft
                            font.family: Theme.uiFamily
                            font.pixelSize: 11
                        }
                        MouseArea {
                            id: favArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: App.runCommand(favChip.commandId, {}, false)
                        }
                    }
                }
            }
        }
    }
}
