pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

Item {
    id: root
    focus: true
    activeFocusOnTab: true

    property string fileFilter: ""
    // "" = все · staged · unstaged · untracked · conflict · mixed
    property string stateFilter: ""
    property bool filesExpanded: true
    property bool commitsExpanded: true
    property bool stashesExpanded: true
    property bool branchesExpanded: true
    property bool tagsExpanded: true
    property bool remotesExpanded: true
    property bool worktreesExpanded: true
    property bool submodulesExpanded: true
    property bool hooksExpanded: true
    property bool reflogExpanded: true
    // 0 изменения · 1 история · 2 ветки · 3 ещё
    property int section: 0
    // hist: 0 коммиты · 1 reflog
    property int histSub: 0
    // refs: 0 ветки · 1 теги · 2 remotes · 3 worktrees
    property int refsSub: 0
    // more: 0 stash · 1 submodules · 2 hooks
    property int moreSub: 0
    property var selectedPaths: ({})
    property int focusedFileIndex: -1
    property var _pathCache: []
    property var _stateCache: []

    function rebuildPathCache() {
        const paths = []
        const states = []
        const n = App.repo.files.count
        for (let i = 0; i < n; ++i) {
            const path = App.repo.files.pathAt(i)
            const st = App.repo.files.stateAt(i)
            if (path && root.matchesFilter(path, st)) {
                paths.push(path)
                states.push(st)
            }
        }
        root._pathCache = paths
        root._stateCache = states
        if (root.focusedFileIndex >= paths.length)
            root.focusedFileIndex = paths.length - 1
    }

    function moveFileFocus(delta) {
        const paths = root._pathCache
        if (!paths.length)
            return
        let i = root.focusedFileIndex
        if (i < 0 || i >= paths.length)
            i = delta > 0 ? -1 : 0
        i = Math.max(0, Math.min(paths.length - 1, i + delta))
        root.focusedFileIndex = i
        const path = paths[i]
        const st = root._stateCache[i] || ""
        const staged = st === "staged"
        App.loadFileDiff(path, staged)
    }

    Connections {
        target: App.repo
        function onChanged() { root.rebuildPathCache() }
    }
    onFileFilterChanged: root.rebuildPathCache()

    Keys.enabled: root.visible && root.section === 0 && App.repo.dirtyCount > 0 && root._pathCache.length > 0
    Keys.onPressed: function (event) {
        if (event.key === Qt.Key_J || event.key === Qt.Key_Down) {
            root.moveFileFocus(1)
            event.accepted = true
        } else if (event.key === Qt.Key_K || event.key === Qt.Key_Up) {
            root.moveFileFocus(-1)
            event.accepted = true
        }
    }

    function toggleSelected(path) {
        const next = Object.assign({}, root.selectedPaths)
        if (next[path])
            delete next[path]
        else
            next[path] = true
        root.selectedPaths = next
    }

    function selectedPathList() {
        return Object.keys(root.selectedPaths)
    }

    function stageSelected() {
        const paths = root.selectedPathList()
        if (paths.length === 0)
            return
        // Prefill first path; for several files open add_all with preview context.
        App.prepareCommand("add_path", { path: paths.join("\n") })
    }

    function stateColor(fileState) {
        if (fileState === "conflict")
            return Theme.destructive
        if (fileState === "staged")
            return Theme.safe
        if (fileState === "untracked")
            return Theme.copperSoft
        if (fileState === "mixed")
            return Theme.mutates
        return Theme.mist
    }

    function stateLabel(fileState) {
        if (fileState === "staged")
            return I18n.i18n("индекс")
        if (fileState === "unstaged")
            return I18n.i18n("рабочая")
        if (fileState === "untracked")
            return I18n.i18n("новый")
        if (fileState === "conflict")
            return I18n.i18n("конфликт")
        if (fileState === "mixed")
            return I18n.i18n("оба")
        return fileState
    }

    function matchesFilter(path, fileState) {
        if (root.stateFilter.length && fileState !== root.stateFilter)
            return false
        if (!root.fileFilter.length)
            return true
        return path.toLowerCase().indexOf(root.fileFilter.toLowerCase()) >= 0
    }

    function countState(st) {
        let n = 0
        const total = App.repo.files.count
        for (let i = 0; i < total; ++i) {
            if (App.repo.files.stateAt(i) === st)
                n++
        }
        return n
    }

    function focusChanges(stateKey) {
        root.section = 0
        root.stateFilter = stateKey || ""
        root.filesExpanded = true
    }

    property int _prevConflictCount: 0

    readonly property string focusedPath: {
        if (root.focusedFileIndex < 0 || root.focusedFileIndex >= root._pathCache.length)
            return ""
        return root._pathCache[root.focusedFileIndex]
    }

    readonly property string focusedFileState: {
        const p = root.focusedPath
        if (!p.length)
            return ""
        const n = App.repo.files.count
        for (let i = 0; i < n; ++i) {
            if (App.repo.files.pathAt(i) === p)
                return App.repo.files.stateAt(i)
        }
        return ""
    }

    Connections {
        target: App.repo
        function onChanged() {
            const n = App.repo.conflictCount
            if (n > 0 && root._prevConflictCount === 0)
                root.focusChanges("conflict")
            root._prevConflictCount = n
        }
    }

    function persistStatusUi() {
        App.saveUiState({
            statusSection: root.section,
            statusStateFilter: root.stateFilter,
            statusHistSub: root.histSub,
            statusRefsSub: root.refsSub,
            statusMoreSub: root.moreSub
        })
    }

    Component.onCompleted: {
        root.rebuildPathCache()
        const s = App.loadUiState()
        if (s.statusSection !== undefined && s.statusSection >= 0 && s.statusSection <= 3)
            root.section = s.statusSection
        if (s.statusStateFilter !== undefined)
            root.stateFilter = s.statusStateFilter
        if (s.statusHistSub !== undefined)
            root.histSub = s.statusHistSub
        if (s.statusRefsSub !== undefined)
            root.refsSub = s.statusRefsSub
        if (s.statusMoreSub !== undefined)
            root.moreSub = s.statusMoreSub
    }

    onSectionChanged: root.persistStatusUi()
    onStateFilterChanged: {
        root.rebuildPathCache()
        root.persistStatusUi()
    }
    onHistSubChanged: root.persistStatusUi()
    onRefsSubChanged: root.persistStatusUi()
    onMoreSubChanged: root.persistStatusUi()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 3
            Repeater {
                model: [
                    { label: I18n.i18n("Изменения"), tip: App.repo.dirtyCount },
                    { label: I18n.i18n("История"), tip: App.repo.commits.totalCount },
                    { label: I18n.i18n("Ветки"), tip: App.repo.branches.count },
                    { label: I18n.i18n("Ещё"), tip: App.repo.stashes.count + App.repo.submodules.count }
                ]
                delegate: Rectangle {
                    id: seg
                    required property int index
                    required property var modelData
                    Layout.fillWidth: true
                    radius: Theme.radiusSm
                    color: root.section === seg.index ? Theme.inkPanel : "transparent"
                    border.color: root.section === seg.index ? Theme.copper : Theme.inkLine
                    border.width: 1
                    implicitHeight: 26
                    Row {
                        anchors.centerIn: parent
                        spacing: 3
                        Text {
                            text: seg.modelData.label
                            color: root.section === seg.index ? Theme.paper : Theme.mist
                            font.family: Theme.uiFamily
                            font.pixelSize: 10
                            font.weight: root.section === seg.index ? Font.DemiBold : Font.Normal
                        }
                        Text {
                            visible: Number(seg.modelData.tip) > 0
                            text: String(seg.modelData.tip)
                            color: root.section === seg.index ? Theme.copperSoft : Theme.mist
                            font.family: Theme.monoFamily
                            font.pixelSize: 9
                            opacity: 0.85
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.section = seg.index
                    }
                }
            }
            Controls.ToolButton {
                icon.name: "view-refresh"
                implicitWidth: 28
                implicitHeight: 26
                onClicked: App.refreshRepo()
                Controls.ToolTip.text: I18n.i18n("Обновить")
                Controls.ToolTip.visible: hovered
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 5
            visible: root.section === 0

        // —— Clean: next steps ——
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6
            visible: App.repo.dirtyCount === 0

            Text {
                Layout.fillWidth: true
                text: I18n.i18n("Рабочее дерево чистое.")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 12
                opacity: 0.75
            }

            Text {
                visible: App.repo.hasUpstream && (App.repo.ahead > 0 || App.repo.behind > 0)
                text: I18n.i18n("Дальше")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 10
                font.capitalization: Font.AllUppercase
                opacity: 0.5
            }

            Flow {
                Layout.fillWidth: true
                spacing: 4
                visible: App.repo.hasUpstream && (App.repo.ahead > 0 || App.repo.behind > 0)

                Rectangle {
                    visible: App.repo.behind > 0
                    radius: Theme.radiusSm
                    border.color: Theme.destructive
                    border.width: 1
                    color: pullArea.containsMouse ? Theme.inkPanel : "transparent"
                    implicitHeight: 22
                    implicitWidth: pullLab.implicitWidth + 12
                    Text {
                        id: pullLab
                        anchors.centerIn: parent
                        text: I18n.i18n("pull ↓%1", App.repo.behind)
                        color: Theme.destructive
                        font.family: Theme.monoFamily
                        font.pixelSize: 11
                    }
                    MouseArea {
                        id: pullArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: App.prepareCommand("pull", {})
                    }
                }
                Rectangle {
                    visible: App.repo.ahead > 0
                    radius: Theme.radiusSm
                    border.color: Theme.safe
                    border.width: 1
                    color: pushArea.containsMouse ? Theme.inkPanel : "transparent"
                    implicitHeight: 22
                    implicitWidth: pushLab.implicitWidth + 12
                    Text {
                        id: pushLab
                        anchors.centerIn: parent
                        text: I18n.i18n("push ↑%1", App.repo.ahead)
                        color: Theme.safe
                        font.family: Theme.monoFamily
                        font.pixelSize: 11
                    }
                    MouseArea {
                        id: pushArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: App.prepareCommand("push", {})
                    }
                }
                Rectangle {
                    radius: Theme.radiusSm
                    border.color: Theme.inkLine
                    border.width: 1
                    color: fetchArea.containsMouse ? Theme.inkPanel : "transparent"
                    implicitHeight: 22
                    implicitWidth: fetchLab.implicitWidth + 12
                    Text {
                        id: fetchLab
                        anchors.centerIn: parent
                        text: "fetch"
                        color: Theme.mist
                        font.family: Theme.monoFamily
                        font.pixelSize: 11
                    }
                    MouseArea {
                        id: fetchArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: App.prepareCommand("fetch", {})
                    }
                }
            }
        }

        // —— Dirty files ——
        Flow {
            Layout.fillWidth: true
            spacing: 3
            visible: App.repo.dirtyCount > 0

            Repeater {
                model: [
                    { key: "", label: I18n.i18n("все"), tip: App.repo.dirtyCount },
                    { key: "staged", label: "S", tip: App.repo.stagedCount },
                    { key: "unstaged", label: "U", tip: App.repo.unstagedCount },
                    { key: "untracked", label: "?", tip: App.repo.untrackedCount },
                    { key: "conflict", label: "!", tip: root.countState("conflict") },
                    { key: "mixed", label: "M", tip: root.countState("mixed") }
                ]
                delegate: Rectangle {
                    id: sf
                    required property var modelData
                    visible: !sf.modelData.key.length || Number(sf.modelData.tip) > 0
                    radius: Theme.radiusSm
                    color: root.stateFilter === sf.modelData.key ? Theme.inkPanel : "transparent"
                    border.color: root.stateFilter === sf.modelData.key ? Theme.copper : Theme.inkLine
                    border.width: 1
                    implicitHeight: 22
                    implicitWidth: sfRow.implicitWidth + 10
                    Row {
                        id: sfRow
                        anchors.centerIn: parent
                        spacing: 3
                        Text {
                            text: sf.modelData.label
                            color: root.stateFilter === sf.modelData.key ? Theme.paper : Theme.mist
                            font.family: Theme.monoFamily
                            font.pixelSize: 10
                            font.weight: root.stateFilter === sf.modelData.key ? Font.DemiBold : Font.Normal
                        }
                        Text {
                            visible: Number(sf.modelData.tip) > 0
                            text: String(sf.modelData.tip)
                            color: Theme.mist
                            font.family: Theme.monoFamily
                            font.pixelSize: 9
                            opacity: 0.8
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.stateFilter = sf.modelData.key
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            visible: App.repo.dirtyCount > 0

            Controls.TextField {
                id: fileFilterField
                Layout.fillWidth: true
                placeholderText: I18n.i18n("Фильтр…")
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: 11
                onTextChanged: root.fileFilter = text
                background: Rectangle {
                    implicitHeight: Theme.controlH
                    radius: Theme.radiusSm
                    color: Theme.inkPanel
                    border.color: fileFilterField.activeFocus ? Theme.copper : Theme.inkLine
                    border.width: 1
                }
            }

            Text {
                visible: root.selectedPathList().length > 0
                text: I18n.i18n("в индекс (%1)", root.selectedPathList().length)
                color: Theme.copperSoft
                font.family: Theme.uiFamily
                font.pixelSize: 11
                font.underline: true
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.stageSelected()
                }
            }
        }

        Text {
            visible: App.repo.dirtyCount > 0 && root._pathCache.length === 0
            Layout.fillWidth: true
            text: I18n.i18n("Нет файлов по фильтру.")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 12
            opacity: 0.7
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(160, Math.max(56, root._pathCache.length * 28))
            Layout.fillHeight: root._pathCache.length > 0
            visible: App.repo.dirtyCount > 0 && root._pathCache.length > 0
            clip: true

            ListView {
                model: App.repo.files
                spacing: 2
                clip: true
                delegate: Rectangle {
                    id: fileDel
                    required property string path
                    required property string code
                    required property string fileState
                    required property string summary
                    width: ListView.view.width
                    height: root.matchesFilter(fileDel.path, fileDel.fileState) ? 26 : 0
                    visible: root.matchesFilter(fileDel.path, fileDel.fileState)
                    radius: Theme.radiusSm
                    color: {
                        if (root.focusedFileIndex >= 0 && root._pathCache[root.focusedFileIndex] === fileDel.path)
                            return "#2a3548"
                        return root.selectedPaths[fileDel.path] ? "#243040" : Theme.inkPanel
                    }
                    border.color: root.selectedPaths[fileDel.path] || (root.focusedFileIndex >= 0 && root._pathCache[root.focusedFileIndex] === fileDel.path)
                                  ? Theme.copper : Theme.inkLine
                    border.width: visible ? 1 : 0
                    clip: true

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: 6
                        spacing: 6
                        visible: fileDel.visible
                        z: 1

                        Controls.CheckBox {
                            checked: !!root.selectedPaths[fileDel.path]
                            onToggled: root.toggleSelected(fileDel.path)
                            scale: 0.85
                        }

                        Text {
                            text: fileDel.code
                            color: root.stateColor(fileDel.fileState)
                            font.family: Theme.monoFamily
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            Layout.preferredWidth: 20
                        }
                        Text {
                            text: fileDel.path
                            color: Theme.paper
                            font.family: Theme.monoFamily
                            font.pixelSize: 11
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                        Text {
                            text: root.stateLabel(fileDel.fileState)
                            color: root.stateColor(fileDel.fileState)
                            font.family: Theme.uiFamily
                            font.pixelSize: 9
                            font.capitalization: Font.AllUppercase
                            opacity: 0.85
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        enabled: fileDel.visible
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        Controls.ToolTip.visible: containsMouse && fileDel.summary.length > 0
                        Controls.ToolTip.text: fileDel.summary
                        onClicked: {
                            const staged = fileDel.fileState === "staged" || fileDel.fileState === "mixed"
                            App.loadFileDiff(fileDel.path, staged && fileDel.fileState === "staged")
                            const idx = root._pathCache.indexOf(fileDel.path)
                            if (idx >= 0)
                                root.focusedFileIndex = idx
                            root.forceActiveFocus()
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: root.focusedFileState === "conflict" && root.focusedPath.length > 0

            Text {
                text: I18n.i18n("Конфликт")
                color: Theme.destructive
                font.family: Theme.uiFamily
                font.pixelSize: 10
                font.weight: Font.DemiBold
            }
            Text {
                text: I18n.i18n("ours")
                color: Theme.copperSoft
                font.family: Theme.uiFamily
                font.pixelSize: 11
                font.underline: true
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: App.prepareCommand("checkout_ours", { path: root.focusedPath })
                }
            }
            Text {
                text: I18n.i18n("theirs")
                color: Theme.copperSoft
                font.family: Theme.uiFamily
                font.pixelSize: 11
                font.underline: true
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: App.prepareCommand("checkout_theirs", { path: root.focusedPath })
                }
            }
            Text {
                text: I18n.i18n("решено")
                color: Theme.safe
                font.family: Theme.uiFamily
                font.pixelSize: 11
                font.underline: true
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: App.prepareCommand("add_path", { path: root.focusedPath })
                }
            }
            Text {
                text: I18n.i18n("редактор")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 11
                font.underline: true
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: App.openInEditor(root.focusedPath)
                }
            }
            Item { Layout.fillWidth: true }
        }

        Text {
            Layout.fillWidth: true
            visible: App.repo.dirtyCount > 0 && root._pathCache.length > 0
            text: I18n.i18n("j/k · ↑/↓")
            color: Theme.mist
            font.family: Theme.monoFamily
            font.pixelSize: 9
            opacity: 0.45
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: App.repo.diffStat.length > 40 ? 56 : 32
            clip: true
            visible: App.repo.dirtyCount > 0 && App.repo.diffStat.length > 0

            Text {
                width: parent ? parent.width - 8 : 200
                wrapMode: Text.WrapAnywhere
                text: App.repo.diffStat
                color: Theme.mist
                font.family: Theme.monoFamily
                font.pixelSize: 10
                opacity: 0.85
            }
        }

        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6
            visible: root.section === 1

        RowLayout {
            Layout.fillWidth: true
            spacing: 3
            Repeater {
                model: [
                    {
                        label: I18n.i18n("Коммиты"),
                        tip: App.repo.commits.filter.length
                             ? (App.repo.commits.count + "/" + App.repo.commits.totalCount)
                             : App.repo.commits.totalCount
                    },
                    { label: "Reflog", tip: App.repo.reflog.count }
                ]
                delegate: Rectangle {
                    id: hsub
                    required property int index
                    required property var modelData
                    Layout.fillWidth: true
                    radius: Theme.radiusSm
                    color: root.histSub === hsub.index ? Theme.inkPanel : "transparent"
                    border.color: root.histSub === hsub.index ? Theme.copper : Theme.inkLine
                    border.width: 1
                    implicitHeight: 24
                    Row {
                        anchors.centerIn: parent
                        spacing: 3
                        Text {
                            text: hsub.modelData.label
                            color: root.histSub === hsub.index ? Theme.paper : Theme.mist
                            font.family: Theme.uiFamily
                            font.pixelSize: 10
                            font.weight: root.histSub === hsub.index ? Font.DemiBold : Font.Normal
                        }
                        Text {
                            visible: Number(hsub.modelData.tip) > 0 || String(hsub.modelData.tip).indexOf("/") >= 0
                            text: String(hsub.modelData.tip)
                            color: Theme.mist
                            font.family: Theme.monoFamily
                            font.pixelSize: 9
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.histSub = hsub.index
                    }
                }
            }
        }

        // —— Commits ——
        Controls.TextField {
            id: commitFilter
            Layout.fillWidth: true
            visible: root.histSub === 0 && App.repo.commits.totalCount > 0
            placeholderText: I18n.i18n("Фильтр: сообщение / автор / hash")
            color: Theme.paper
            font.family: Theme.uiFamily
            font.pixelSize: 11
            onTextChanged: App.repo.commits.filter = text
            background: Rectangle {
                implicitHeight: 26
                radius: Theme.radiusSm
                color: Theme.inkPanel
                border.color: commitFilter.activeFocus ? Theme.copper : Theme.inkLine
                border.width: 1
            }
        }

        Text {
            visible: root.histSub === 0 && App.repo.commits.totalCount === 0
            Layout.fillWidth: true
            text: I18n.i18n("Нет коммитов.")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 12
            opacity: 0.7
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(160, Math.max(48, App.repo.commits.count * 26))
            Layout.fillHeight: root.histSub === 0 && App.repo.commits.count > 0
            clip: true
            visible: root.histSub === 0 && App.repo.commits.count > 0

            ListView {
                model: App.repo.commits
                spacing: 2
                clip: true
                delegate: Item {
                    id: commitDel
                    required property string hash
                    required property string shortHash
                    required property string subject
                    required property string relativeTime
                    width: ListView.view.width
                    height: 26
                    RowLayout {
                        anchors.fill: parent
                        spacing: 6
                        Text {
                            text: commitDel.shortHash
                            color: Theme.copper
                            font.family: Theme.monoFamily
                            font.pixelSize: 11
                        }
                        Text {
                            text: commitDel.subject
                            color: Theme.paper
                            font.family: Theme.uiFamily
                            font.pixelSize: 11
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            text: commitDel.relativeTime
                            color: Theme.mist
                            font.family: Theme.uiFamily
                            font.pixelSize: 10
                            opacity: 0.7
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                        Controls.ToolTip.visible: containsMouse
                        Controls.ToolTip.text: I18n.i18n("ЛКМ — show · Ctrl=A · Shift=B · ПКМ/колесо — hash")
                        onClicked: function (mouse) {
                            if (mouse.button === Qt.MiddleButton
                                    || (mouse.button === Qt.RightButton && !(mouse.modifiers & Qt.ShiftModifier))) {
                                App.copyText(commitDel.shortHash)
                            } else if (mouse.button === Qt.RightButton || mouse.modifiers & Qt.ShiftModifier) {
                                App.setCompareEndpoint(commitDel.hash, false)
                            } else if (mouse.modifiers & Qt.ControlModifier) {
                                App.setCompareEndpoint(commitDel.hash, true)
                            } else {
                                App.loadCommitShow(commitDel.hash)
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            visible: root.histSub === 0 && (App.repo.compareA.length > 0 || App.repo.compareB.length > 0)
            Text {
                text: I18n.i18n("A→B")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 10
                font.capitalization: Font.AllUppercase
                opacity: 0.5
            }
            Text {
                text: (App.repo.compareA ? App.repo.compareA.substring(0, 7) : "—")
                      + " → "
                      + (App.repo.compareB ? App.repo.compareB.substring(0, 7) : "—")
                color: Theme.copperSoft
                font.family: Theme.monoFamily
                font.pixelSize: 11
                Layout.fillWidth: true
                elide: Text.ElideMiddle
            }
            Text {
                text: I18n.i18n("Ctrl=A · Shift=B · ПКМ=hash")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 9
                opacity: 0.55
            }
        }

        Text {
            visible: root.histSub === 1 && App.repo.reflog.count === 0
            Layout.fillWidth: true
            text: I18n.i18n("Reflog пуст.")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 12
            opacity: 0.7
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: Math.min(160, Math.max(40, App.repo.reflog.count * 22))
            clip: true
            visible: root.histSub === 1 && App.repo.reflog.count > 0

            ListView {
                model: App.repo.reflog
                spacing: 2
                clip: true
                delegate: Item {
                    id: rf
                    required property string selector
                    required property string hash
                    required property string message
                    width: ListView.view.width
                    height: 22
                    RowLayout {
                        anchors.fill: parent
                        spacing: 6
                        Text {
                            text: rf.selector
                            color: Theme.copper
                            font.family: Theme.monoFamily
                            font.pixelSize: 10
                        }
                        Text {
                            text: rf.hash
                            color: Theme.mist
                            font.family: Theme.monoFamily
                            font.pixelSize: 10
                        }
                        Text {
                            text: rf.message
                            color: Theme.paper
                            font.family: Theme.uiFamily
                            font.pixelSize: 10
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: App.loadCommitShow(rf.hash)
                    }
                }
            }
        }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6
            visible: root.section === 2

        RowLayout {
            Layout.fillWidth: true
            spacing: 3
            Repeater {
                model: [
                    { label: I18n.i18n("Ветки"), tip: App.repo.branches.count },
                    { label: I18n.i18n("Теги"), tip: App.repo.tags.count },
                    { label: "Remote", tip: App.repo.remotes.count },
                    { label: "WT", tip: App.repo.worktrees.count }
                ]
                delegate: Rectangle {
                    id: rsub
                    required property int index
                    required property var modelData
                    Layout.fillWidth: true
                    radius: Theme.radiusSm
                    color: root.refsSub === rsub.index ? Theme.inkPanel : "transparent"
                    border.color: root.refsSub === rsub.index ? Theme.copper : Theme.inkLine
                    border.width: 1
                    implicitHeight: 24
                    Row {
                        anchors.centerIn: parent
                        spacing: 3
                        Text {
                            text: rsub.modelData.label
                            color: root.refsSub === rsub.index ? Theme.paper : Theme.mist
                            font.family: Theme.uiFamily
                            font.pixelSize: 10
                            font.weight: root.refsSub === rsub.index ? Font.DemiBold : Font.Normal
                        }
                        Text {
                            visible: Number(rsub.modelData.tip) > 0
                            text: String(rsub.modelData.tip)
                            color: Theme.mist
                            font.family: Theme.monoFamily
                            font.pixelSize: 9
                            opacity: 0.8
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.refsSub = rsub.index
                    }
                }
            }
        }

        // —— Branches ——
        RowLayout {
            Layout.fillWidth: true
            visible: root.refsSub === 0
            Text {
                text: I18n.i18n("Ветки · %1", App.repo.branches.count)
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: 11
                font.letterSpacing: 1.2
                font.capitalization: Font.AllUppercase
                opacity: 0.65
                Layout.fillWidth: true
            }
            Text {
                text: I18n.i18n("+ ветка")
                color: Theme.copperSoft
                font.family: Theme.uiFamily
                font.pixelSize: 11
                font.underline: branchAddHover.containsMouse
                MouseArea {
                    id: branchAddHover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: App.openBranchDialog()
                }
            }
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: root.refsSub === 0

            ListView {
                model: App.repo.branches
                spacing: 2
                clip: true
                delegate: Item {
                    id: br
                    required property string name
                    required property bool current
                    required property bool remote
                    required property string upstream
                    required property int ahead
                    required property int behind
                    width: ListView.view.width
                    height: 26

                    RowLayout {
                        anchors.fill: parent
                        spacing: 6

                        Rectangle {
                            Layout.preferredWidth: 6
                            Layout.preferredHeight: 6
                            Layout.alignment: Qt.AlignVCenter
                            radius: 3
                            color: br.current ? Theme.copper : (br.remote ? Theme.mist : Theme.safe)
                            opacity: br.current || !br.remote ? 1 : 0.45
                        }
                        Text {
                            text: br.name
                            color: br.current ? Theme.paper : Theme.mist
                            font.family: Theme.monoFamily
                            font.pixelSize: 11
                            font.weight: br.current ? Font.DemiBold : Font.Normal
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            visible: !br.remote && (br.ahead > 0 || br.behind > 0)
                            text: (br.ahead > 0 ? ("↑" + br.ahead) : "") + (br.behind > 0 ? (" ↓" + br.behind) : "")
                            color: br.behind > 0 ? Theme.destructive : Theme.safe
                            font.family: Theme.monoFamily
                            font.pixelSize: 10
                        }
                        Text {
                            visible: br.upstream.length > 0 && !br.remote
                            text: br.upstream
                            color: Theme.mist
                            font.family: Theme.monoFamily
                            font.pixelSize: 10
                            opacity: 0.65
                            elide: Text.ElideRight
                            Layout.maximumWidth: 90
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: !br.remote
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: Qt.PointingHandCursor
                        Controls.ToolTip.visible: containsMouse
                        Controls.ToolTip.text: br.remote
                            ? br.name
                            : I18n.i18n("ЛКМ — switch/copy · ПКМ — track · Shift+ПКМ — delete · Ctrl+ПКМ — rename")
                        onClicked: function (mouse) {
                            if (br.remote)
                                return
                            if (mouse.button === Qt.RightButton) {
                                if (mouse.modifiers & Qt.ControlModifier) {
                                    App.prepareCommand("branch_rename", {
                                        old: br.current ? "" : br.name,
                                        name: ""
                                    })
                                } else if (mouse.modifiers & Qt.ShiftModifier) {
                                    if (!br.current)
                                        App.prepareCommand("branch_delete", { name: br.name, mode: "-d" })
                                } else if (App.repo.remotes.count > 0) {
                                    const remote = App.repo.remotes.nameAt(0)
                                    const shortName = br.name.indexOf("/") >= 0
                                        ? br.name.split("/").slice(1).join("/")
                                        : br.name
                                    App.prepareCommand("branch_set_upstream", {
                                        upstream: remote + "/" + shortName,
                                        branch: br.current ? "" : br.name
                                    })
                                }
                            } else if (br.current) {
                                App.copyText(br.name)
                            } else {
                                App.prepareCommand("switch", { name: br.name })
                            }
                        }
                    }
                }
            }
        }


        RowLayout {
            Layout.fillWidth: true
            visible: root.refsSub === 1
            Text {
                text: I18n.i18n("Теги · %1", App.repo.tags.count)
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: 11
                font.letterSpacing: 1.2
                font.capitalization: Font.AllUppercase
                opacity: 0.65
                Layout.fillWidth: true
            }
            Text {
                text: I18n.i18n("+ тег")
                color: Theme.copperSoft
                font.family: Theme.uiFamily
                font.pixelSize: 11
                font.underline: tagAddHover.containsMouse
                MouseArea {
                    id: tagAddHover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: App.openTagDialog()
                }
            }
        }

        Text {
            visible: root.refsSub === 1 && App.repo.tags.count === 0
            Layout.fillWidth: true
            text: I18n.i18n("Нет тегов.")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 12
            opacity: 0.7
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: Math.min(160, Math.max(28, App.repo.tags.count * 26))
            clip: true
            visible: root.refsSub === 1 && App.repo.tags.count > 0

            ListView {
                model: App.repo.tags
                spacing: 2
                clip: true
                delegate: Item {
                    id: tagDel
                    required property string name
                    required property string hash
                    required property string shortHash
                    required property string relativeTime
                    required property string subject
                    width: ListView.view.width
                    height: 24
                    RowLayout {
                        anchors.fill: parent
                        spacing: 6
                        Text {
                            text: tagDel.name
                            color: Theme.copperSoft
                            font.family: Theme.monoFamily
                            font.pixelSize: 11
                            elide: Text.ElideRight
                            Layout.maximumWidth: 100
                        }
                        Text {
                            text: tagDel.shortHash
                            color: Theme.mist
                            font.family: Theme.monoFamily
                            font.pixelSize: 10
                        }
                        Text {
                            text: tagDel.subject.length ? tagDel.subject : tagDel.relativeTime
                            color: Theme.paper
                            font.family: Theme.uiFamily
                            font.pixelSize: 10
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: function (mouse) {
                            if (mouse.button === Qt.RightButton) {
                                App.prepareCommand("tag_delete", { name: tagDel.name })
                            } else {
                                App.loadCommitShow(tagDel.hash)
                            }
                        }
                    }
                }
            }
        }


        Text {
            visible: root.refsSub === 2
            text: I18n.i18n("Remotes · %1", App.repo.remotes.count)
            color: Theme.paper
            font.family: Theme.uiFamily
            font.pixelSize: 11
            font.letterSpacing: 1.2
            font.capitalization: Font.AllUppercase
            opacity: 0.65
            Layout.fillWidth: true
        }

        Text {
            visible: root.refsSub === 2 && App.repo.remotes.count === 0
            Layout.fillWidth: true
            text: I18n.i18n("Нет remotes.")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 12
            opacity: 0.7
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: Math.min(120, Math.max(28, App.repo.remotes.count * 28))
            clip: true
            visible: root.refsSub === 2 && App.repo.remotes.count > 0

            ListView {
                model: App.repo.remotes
                spacing: 2
                clip: true
                delegate: Item {
                    id: remoteDel
                    required property string name
                    required property string fetchUrl
                    required property string pushUrl
                    width: ListView.view.width
                    height: 26
                    RowLayout {
                        anchors.fill: parent
                        spacing: 6
                        Text {
                            text: remoteDel.name
                            color: Theme.copperSoft
                            font.family: Theme.monoFamily
                            font.pixelSize: 11
                        }
                        Text {
                            text: remoteDel.fetchUrl
                            color: Theme.mist
                            font.family: Theme.monoFamily
                            font.pixelSize: 10
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: App.copyText(remoteDel.fetchUrl)
                    }
                }
            }
        }


        RowLayout {
            Layout.fillWidth: true
            visible: root.refsSub === 3
            Text {
                text: I18n.i18n("Worktrees · %1", App.repo.worktrees.count)
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: 11
                font.letterSpacing: 1.2
                font.capitalization: Font.AllUppercase
                opacity: 0.65
                Layout.fillWidth: true
            }
            Text {
                text: I18n.i18n("prune")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 11
                font.underline: wtPruneHover.containsMouse
                MouseArea {
                    id: wtPruneHover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: App.prepareCommand("worktree_prune", {})
                }
            }
            Text {
                text: I18n.i18n("+ wt")
                color: Theme.copperSoft
                font.family: Theme.uiFamily
                font.pixelSize: 11
                font.underline: wtAddHover.containsMouse
                MouseArea {
                    id: wtAddHover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        const base = App.repo.rootPath.replace(/\/$/, "")
                        App.prepareCommand("worktree_add", {
                            path: base + "-wt",
                            branch: ""
                        })
                    }
                }
            }
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: Math.min(140, Math.max(28, App.repo.worktrees.count * 28))
            clip: true
            visible: root.refsSub === 3

            ListView {
                model: App.repo.worktrees
                spacing: 2
                clip: true
                delegate: Item {
                    id: wtDel
                    required property string path
                    required property string branch
                    required property string head
                    required property bool isCurrent
                    required property bool locked
                    required property bool prunable
                    width: ListView.view.width
                    height: 26
                    RowLayout {
                        anchors.fill: parent
                        spacing: 6
                        Rectangle {
                            Layout.preferredWidth: 6
                            Layout.preferredHeight: 6
                            Layout.alignment: Qt.AlignVCenter
                            radius: 3
                            color: wtDel.isCurrent ? Theme.copper : (wtDel.prunable ? Theme.destructive : Theme.mist)
                        }
                        Text {
                            text: wtDel.branch.length ? wtDel.branch : wtDel.head.substring(0, 7)
                            color: wtDel.isCurrent ? Theme.paper : Theme.copperSoft
                            font.family: Theme.monoFamily
                            font.pixelSize: 11
                            font.weight: wtDel.isCurrent ? Font.DemiBold : Font.Normal
                        }
                        Text {
                            text: wtDel.path
                            color: Theme.mist
                            font.family: Theme.monoFamily
                            font.pixelSize: 10
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                        Text {
                            visible: wtDel.locked
                            text: "L"
                            color: Theme.mutates
                            font.family: Theme.monoFamily
                            font.pixelSize: 10
                        }
                        Text {
                            visible: wtDel.prunable
                            text: "P"
                            color: Theme.destructive
                            font.family: Theme.monoFamily
                            font.pixelSize: 10
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: Qt.PointingHandCursor
                        Controls.ToolTip.visible: containsMouse
                        Controls.ToolTip.text: wtDel.isCurrent
                            ? I18n.i18n("Текущее дерево · ПКМ — копировать путь")
                            : I18n.i18n("ЛКМ — открыть · ПКМ — remove · Shift+ПКМ — force remove")
                        onClicked: function (mouse) {
                            if (mouse.button === Qt.RightButton) {
                                if (wtDel.isCurrent) {
                                    App.copyText(wtDel.path)
                                } else {
                                    App.prepareCommand("worktree_remove", {
                                        path: wtDel.path,
                                        force: !!(mouse.modifiers & Qt.ShiftModifier)
                                    })
                                }
                            } else if (!wtDel.isCurrent) {
                                App.openRepository(wtDel.path)
                            } else {
                                App.copyText(wtDel.path)
                            }
                        }
                    }
                }
            }
        }

        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6
            visible: root.section === 3

        RowLayout {
            Layout.fillWidth: true
            spacing: 3
            Repeater {
                model: [
                    { label: "Stash", tip: App.repo.stashes.count },
                    { label: I18n.i18n("Модули"), tip: App.repo.submodules.count },
                    { label: "Hooks", tip: App.repo.hooks.count }
                ]
                delegate: Rectangle {
                    id: msub
                    required property int index
                    required property var modelData
                    Layout.fillWidth: true
                    radius: Theme.radiusSm
                    color: root.moreSub === msub.index ? Theme.inkPanel : "transparent"
                    border.color: root.moreSub === msub.index ? Theme.copper : Theme.inkLine
                    border.width: 1
                    implicitHeight: 24
                    Row {
                        anchors.centerIn: parent
                        spacing: 3
                        Text {
                            text: msub.modelData.label
                            color: root.moreSub === msub.index ? Theme.paper : Theme.mist
                            font.family: Theme.uiFamily
                            font.pixelSize: 10
                            font.weight: root.moreSub === msub.index ? Font.DemiBold : Font.Normal
                        }
                        Text {
                            visible: Number(msub.modelData.tip) > 0
                            text: String(msub.modelData.tip)
                            color: Theme.mist
                            font.family: Theme.monoFamily
                            font.pixelSize: 9
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.moreSub = msub.index
                    }
                }
            }
        }

        Text {
            visible: root.moreSub === 0
            text: I18n.i18n("Stash · %1", App.repo.stashes.count)
            color: Theme.paper
            font.family: Theme.uiFamily
            font.pixelSize: 11
            font.letterSpacing: 1.2
            font.capitalization: Font.AllUppercase
            opacity: 0.65
        }

        Text {
            visible: root.moreSub === 0 && App.repo.stashes.count === 0
            Layout.fillWidth: true
            text: I18n.i18n("Stash пуст.")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 12
            opacity: 0.7
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: Math.min(140, Math.max(28, App.repo.stashes.count * 26))
            clip: true
            visible: root.moreSub === 0 && App.repo.stashes.count > 0

            ListView {
                model: App.repo.stashes
                spacing: 2
                clip: true
                delegate: Item {
                    id: stashDel
                    required property string ref
                    required property string message
                    width: ListView.view.width
                    height: 24
                    RowLayout {
                        anchors.fill: parent
                        spacing: 6
                        Text {
                            text: stashDel.ref
                            color: Theme.copper
                            font.family: Theme.monoFamily
                            font.pixelSize: 11
                        }
                        Text {
                            text: stashDel.message
                            color: Theme.mist
                            font.family: Theme.uiFamily
                            font.pixelSize: 11
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        Controls.ToolTip.visible: containsMouse
                        Controls.ToolTip.text: I18n.i18n("ЛКМ — show · ПКМ — apply · Shift+ПКМ — drop")
                        onClicked: function (mouse) {
                            if (mouse.button === Qt.RightButton) {
                                if (mouse.modifiers & Qt.ShiftModifier)
                                    App.prepareCommand("stash_drop", { stash: stashDel.ref })
                                else
                                    App.prepareCommand("stash_apply", { stash: stashDel.ref })
                            } else {
                                App.loadStashShow(stashDel.ref)
                            }
                        }
                    }
                }
            }
        }


        Text {
            visible: root.moreSub === 1
            text: I18n.i18n("Submodules · %1", App.repo.submodules.count)
            color: Theme.paper
            font.family: Theme.uiFamily
            font.pixelSize: 11
            font.letterSpacing: 1.2
            font.capitalization: Font.AllUppercase
            opacity: 0.65
            Layout.fillWidth: true
        }

        Text {
            visible: root.moreSub === 1 && App.repo.submodules.count === 0
            Layout.fillWidth: true
            text: I18n.i18n("Нет субмодулей.")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 12
            opacity: 0.7
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: Math.min(140, Math.max(28, App.repo.submodules.count * 28))
            clip: true
            visible: root.moreSub === 1 && App.repo.submodules.count > 0

            ListView {
                model: App.repo.submodules
                spacing: 2
                clip: true
                delegate: Item {
                    id: smDel
                    required property string path
                    required property string sha
                    required property string describe
                    required property string status
                    width: ListView.view.width
                    height: 26
                    RowLayout {
                        anchors.fill: parent
                        spacing: 6
                        Text {
                            text: smDel.status === "ok" ? "·" : (smDel.status === "modified" ? "+" : (smDel.status === "uninitialized" ? "−" : "U"))
                            color: smDel.status === "ok" ? Theme.safe
                                   : (smDel.status === "conflict" ? Theme.destructive : Theme.mutates)
                            font.family: Theme.monoFamily
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }
                        Text {
                            text: smDel.path
                            color: Theme.paper
                            font.family: Theme.monoFamily
                            font.pixelSize: 11
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                        Text {
                            text: smDel.describe.length ? smDel.describe : smDel.sha.substring(0, 7)
                            color: Theme.mist
                            font.family: Theme.monoFamily
                            font.pixelSize: 10
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: App.prepareCommand("submodule_status", {})
                    }
                }
            }
        }


        Text {
            visible: root.moreSub === 2
            text: I18n.i18n("Hooks · %1 активных / %2", App.repo.hooks.activeCount, App.repo.hooks.count)
            color: Theme.paper
            font.family: Theme.uiFamily
            font.pixelSize: 11
            font.letterSpacing: 1.2
            font.capitalization: Font.AllUppercase
            opacity: 0.65
            Layout.fillWidth: true
        }

        Text {
            visible: root.moreSub === 2 && App.repo.hooks.count === 0
            Layout.fillWidth: true
            text: I18n.i18n("Нет hooks.")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 12
            opacity: 0.7
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: Math.min(160, Math.max(28, App.repo.hooks.count * 22))
            clip: true
            visible: root.moreSub === 2 && App.repo.hooks.count > 0

            ListView {
                model: App.repo.hooks
                spacing: 1
                clip: true
                delegate: Item {
                    id: hookDel
                    required property string name
                    required property bool executable
                    required property bool sample
                    width: ListView.view.width
                    height: 20
                    RowLayout {
                        anchors.fill: parent
                        spacing: 6
                        Text {
                            text: hookDel.executable ? "●" : "○"
                            color: hookDel.executable ? Theme.safe : Theme.inkLine
                            font.pixelSize: 10
                        }
                        Text {
                            text: hookDel.name
                            color: hookDel.sample ? Theme.mist : Theme.paper
                            font.family: Theme.monoFamily
                            font.pixelSize: 11
                            opacity: hookDel.sample ? 0.55 : 1
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }

        }

    }
}
