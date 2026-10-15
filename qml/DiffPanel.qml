pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

Item {
    id: root

    property bool wordDiff: false
    property bool sideBySide: false
    property string searchText: ""

    readonly property var lines: {
        const text = App.repo.selectedDiff || ""
        if (!text.length)
            return []
        const all = text.split("\n")
        if (!root.searchText.length)
            return all
        const needle = root.searchText.toLowerCase()
        return all.filter(function (line) { return line.toLowerCase().indexOf(needle) >= 0 })
    }

    readonly property var lineStats: {
        const text = App.repo.selectedDiff || ""
        let plus = 0
        let minus = 0
        if (!text.length)
            return { plus: 0, minus: 0 }
        const all = text.split("\n")
        for (let i = 0; i < all.length; ++i) {
            const line = all[i]
            if (line.startsWith("+++") || line.startsWith("---"))
                continue
            if (line.startsWith("+"))
                plus++
            else if (line.startsWith("-"))
                minus++
        }
        return { plus: plus, minus: minus }
    }

    readonly property var leftLines: {
        const out = []
        for (let i = 0; i < root.lines.length; ++i) {
            const line = root.lines[i]
            if (line.startsWith("+") && !line.startsWith("+++"))
                continue
            out.push(line.startsWith("-") && !line.startsWith("---") ? line : line)
        }
        return out
    }
    readonly property var rightLines: {
        const out = []
        for (let i = 0; i < root.lines.length; ++i) {
            const line = root.lines[i]
            if (line.startsWith("-") && !line.startsWith("---"))
                continue
            out.push(line.startsWith("+") && !line.startsWith("+++") ? line : line)
        }
        return out
    }

    function reloadDiff() {
        if (!App.repo.selectedDiffPath.length)
            return
        App.loadFileDiff(App.repo.selectedDiffPath, App.repo.selectedDiffStaged, root.wordDiff)
    }

    function lineColor(line) {
        if (line.startsWith("<<<<<<<") || line.startsWith(">>>>>>>") || line.startsWith("======="))
            return Theme.destructive
        if (line.startsWith("+++") || line.startsWith("---"))
            return Theme.mist
        if (line.startsWith("+"))
            return Theme.safe
        if (line.startsWith("-"))
            return Theme.destructive
        if (line.startsWith("@@"))
            return Theme.copperSoft
        return Theme.paper
    }

    readonly property bool selectedIsConflict: {
        const p = App.repo.selectedDiffPath
        if (!p.length)
            return false
        const n = App.repo.files.count
        for (let i = 0; i < n; ++i) {
            if (App.repo.files.pathAt(i) === p && App.repo.files.stateAt(i) === "conflict")
                return true
        }
        return App.repo.selectedDiff.indexOf("<<<<<<<") >= 0
    }

    function runPathAction(item) {
        if (item.inspect)
            App.loadFileBlame(App.repo.selectedDiffPath)
        else if (item.pathAction === "copy")
            App.copyPath(App.repo.selectedDiffPath)
        else if (item.pathAction === "reveal")
            App.revealInFolder(App.repo.selectedDiffPath)
        else if (item.pathAction === "history")
            App.loadFileHistory(App.repo.selectedDiffPath)
        else if (item.pathAction === "attr")
            App.loadFileAttributes(App.repo.selectedDiffPath)
        else if (item.pathAction === "edit")
            App.openInEditor(App.repo.selectedDiffPath)
        else if (item.pathAction === "ignore")
            App.appendGitignore(App.repo.selectedDiffPath)
        else
            App.prepareCommand(item.id, item.args)
    }

    readonly property var actionGroups: [
        {
            title: I18n.i18n("Конфликт"),
            hide: !root.selectedIsConflict,
            items: [
                { label: I18n.i18n("ours"), id: "checkout_ours", args: { path: App.repo.selectedDiffPath } },
                { label: I18n.i18n("theirs"), id: "checkout_theirs", args: { path: App.repo.selectedDiffPath } },
                { label: I18n.i18n("решено"), id: "add_path", args: { path: App.repo.selectedDiffPath } },
                { label: I18n.i18n("редактор"), pathAction: "edit" }
            ]
        },
        {
            title: I18n.i18n("Индекс"),
            hide: root.selectedIsConflict,
            items: [
                { label: I18n.i18n("В индекс"), id: "add_path", args: { path: App.repo.selectedDiffPath } },
                { label: I18n.i18n("Из индекса"), id: "restore_staged", args: { path: App.repo.selectedDiffPath } },
                { label: I18n.i18n("Откатить"), id: "restore_worktree", args: { path: App.repo.selectedDiffPath } }
            ]
        },
        {
            title: I18n.i18n("Обзор"),
            items: [
                { label: I18n.i18n("Blame"), inspect: true },
                { label: I18n.i18n("История"), pathAction: "history" },
                { label: I18n.i18n("Attrs"), pathAction: "attr" }
            ]
        },
        {
            title: I18n.i18n("Файл"),
            items: [
                { label: I18n.i18n("Путь"), pathAction: "copy" },
                { label: I18n.i18n("Папка"), pathAction: "reveal" },
                { label: I18n.i18n("Редактор"), pathAction: "edit" },
                { label: I18n.i18n(".gitignore"), pathAction: "ignore" }
            ]
        }
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        DiffChrome {
            Layout.fillWidth: true
            wordDiff: root.wordDiff
            sideBySide: root.sideBySide
            searchText: root.searchText
            plusLines: root.lineStats.plus
            minusLines: root.lineStats.minus
            conflict: root.selectedIsConflict
            actionGroups: root.actionGroups
            onSearchEdited: function (text) { root.searchText = text }
            onWordToggled: {
                root.wordDiff = !root.wordDiff
                root.reloadDiff()
            }
            onSplitToggled: root.sideBySide = !root.sideBySide
            onCopyRequested: App.copyText(App.repo.selectedDiff)
            onActionTriggered: function (item) { root.runPathAction(item) }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: App.repo.selectedDiffPath.length > 0
            radius: Theme.radius
            color: "#0a0d12"
            border.color: Theme.inkLine
            border.width: 1
            clip: true

            Row {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 4
                visible: root.sideBySide

                Controls.ScrollView {
                    width: (parent.width - 4) / 2
                    height: parent.height
                    clip: true
                    Column {
                        width: Math.max(parent.width - 8, 100)
                        Repeater {
                            model: root.leftLines
                            delegate: Text {
                                required property string modelData
                                width: parent.width
                                text: modelData.length ? modelData : " "
                                color: root.lineColor(modelData)
                                font.family: Theme.monoFamily
                                font.pixelSize: 11
                                wrapMode: Text.NoWrap
                            }
                        }
                    }
                }
                Controls.ScrollView {
                    width: (parent.width - 4) / 2
                    height: parent.height
                    clip: true
                    Column {
                        width: Math.max(parent.width - 8, 100)
                        Repeater {
                            model: root.rightLines
                            delegate: Text {
                                required property string modelData
                                width: parent.width
                                text: modelData.length ? modelData : " "
                                color: root.lineColor(modelData)
                                font.family: Theme.monoFamily
                                font.pixelSize: 11
                                wrapMode: Text.NoWrap
                            }
                        }
                    }
                }
            }

            Controls.ScrollView {
                anchors.fill: parent
                anchors.margins: 4
                clip: true
                visible: !root.sideBySide

                Column {
                    width: Math.max(parent.width - 8, 200)
                    spacing: 0
                    Repeater {
                        model: root.lines
                        delegate: Text {
                            required property string modelData
                            width: parent.width
                            text: modelData.length ? modelData : " "
                            color: root.lineColor(modelData)
                            font.family: Theme.monoFamily
                            font.pixelSize: 12
                            wrapMode: Text.NoWrap
                        }
                    }
                }
            }
        }
    }
}
