pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Ghew

Kirigami.Page {
    id: page
    title: App.repo.isOpen
           ? ((App.repo.rootPath.split("/").pop() || App.repo.rootPath) + " · " + (App.repo.branch || "?"))
           : I18n.i18n("Рабочая область")
    padding: 0
    background: Rectangle { color: Theme.ink }

    readonly property var tabNames: [
        I18n.i18n("Советы"),
        I18n.i18n("Параметры"),
        I18n.i18n("Пояснение"),
        I18n.i18n("Превью"),
        I18n.i18n("Diff"),
        I18n.i18n("Hunks"),
        I18n.i18n("Просмотр"),
        I18n.i18n("Вывод"),
        I18n.i18n("История"),
        I18n.i18n("Мастера"),
        I18n.i18n("Настройки"),
        I18n.i18n("GitHub")
    ]
    // Смысловые группы вкладок (индексы в tabNames / StackLayout)
    readonly property var tabGroups: [
        { title: I18n.i18n("Команда"), indices: [1, 3, 7, 2, 0] },
        { title: I18n.i18n("Правки"), indices: [4, 5] },
        { title: I18n.i18n("Обзор"), indices: [6, 8, 11] },
        { title: I18n.i18n("Ещё"), indices: [9, 10] }
    ]
    property int tabGroup: 0
    property int tabsIndex: 0
    readonly property string activeTabLabel: page.tabNames[page.tabsIndex] || ""
    readonly property var visibleTabIndices: page.tabGroups[page.tabGroup].indices

    function setTabGroup(g) {
        page.tabGroup = g
        const idxs = page.tabGroups[g].indices
        if (idxs.indexOf(page.tabsIndex) < 0)
            page.tabsIndex = idxs[0]
    }

    function ensureTabGroupForIndex(idx) {
        for (let g = 0; g < page.tabGroups.length; ++g) {
            if (page.tabGroups[g].indices.indexOf(idx) >= 0) {
                page.tabGroup = g
                return
            }
        }
    }

    function focusCommandSearch() {
        sidebar.commandList.focusSearch()
    }

    Component.onCompleted: {
        const s = App.loadUiState()
        if (s.tabsIndex !== undefined && s.tabsIndex >= 0 && s.tabsIndex < page.tabNames.length)
            page.tabsIndex = s.tabsIndex
        if (s.tabGroup !== undefined && s.tabGroup >= 0 && s.tabGroup < page.tabGroups.length)
            page.tabGroup = s.tabGroup
        if (s.uiScale !== undefined && s.uiScale > 0)
            Theme.setUiScale(s.uiScale)
        page.ensureTabGroupForIndex(page.tabsIndex)
    }

    Connections {
        target: Theme
        function onUiScaleChanged() {
            App.saveUiState({ uiScale: Theme.uiScale })
        }
    }

    onTabsIndexChanged: {
        page.ensureTabGroupForIndex(page.tabsIndex)
        App.saveUiState({ tabsIndex: page.tabsIndex, tabGroup: page.tabGroup })
    }

    header: RepoHeader {
        onFocusChanges: function (stateFilter) {
            sidebar.statusPanel.focusChanges(stateFilter)
        }
    }

    Timer {
        id: autoRefreshTimer
        interval: 8000
        repeat: true
        running: App.repo.isOpen && App.autoRefresh && !App.busy
        onTriggered: App.refreshRepo()
    }

    Connections {
        target: App
        function onCommandRunRequested(tabIndex) {
            page.tabsIndex = tabIndex
            page.ensureTabGroupForIndex(tabIndex)
        }
        function onSelectCommandRequested(commandId, args) {
            if (sidebar.commandList.selectById(commandId)) {
                page.tabsIndex = 1
                Qt.callLater(function () {
                    detail.paramForm.applyValues(args || {})
                })
            }
        }
    }

    Connections {
        target: App.repo
        function onDiffChanged() {
            if (App.repo.selectedDiffPath.length > 0)
                page.tabsIndex = 4
        }
        function onInspectChanged() {
            if (App.repo.inspectText.length > 0)
                page.tabsIndex = 6
        }
    }

    Shortcut {
        sequence: "Ctrl+K"
        enabled: page.visible
        onActivated: page.focusCommandSearch()
    }
    Shortcut {
        sequences: ["Ctrl+Return", "Ctrl+Enter"]
        enabled: page.visible && !!App.lastPreviewJson.length && !App.busy
        onActivated: {
            page.tabsIndex = 3
            page.ensureTabGroupForIndex(3)
            detail.previewPanel.runPreview()
        }
    }
    Shortcut {
        sequence: "F1"
        enabled: page.visible
        onActivated: {
            page.tabsIndex = 10
            page.ensureTabGroupForIndex(10)
        }
    }
    Shortcut {
        sequence: "Ctrl+Shift+T"
        enabled: page.visible && App.repo.isOpen
        onActivated: App.openTerminalInRepo()
    }
    Shortcut {
        // Slash focuses search only when no editable already has focus.
        sequence: "/"
        enabled: page.visible && !(page.Window.window && page.Window.window.activeFocusItem
                                   && page.Window.window.activeFocusItem.toString().indexOf("TextInput") !== -1)
        onActivated: page.focusCommandSearch()
    }
    Shortcut {
        sequences: ["Ctrl+=", "Ctrl++"]
        enabled: page.visible
        onActivated: Theme.bumpUiScale(Theme.uiScaleStep)
    }
    Shortcut {
        sequence: "Ctrl+-"
        enabled: page.visible
        onActivated: Theme.bumpUiScale(-Theme.uiScaleStep)
    }
    Shortcut {
        sequence: "Ctrl+0"
        enabled: page.visible
        onActivated: Theme.resetUiScale()
    }

    ColumnLayout {
        id: shell
        anchors.fill: parent
        spacing: 0
        clip: true

        Controls.SplitView {
            id: mainSplit
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal
            clip: true

            property int sidebarWidth: 320
            readonly property int hostW: width > 0 ? width : 1180

            Component.onCompleted: {
                const s = App.loadUiState()
                if (s.sidebarWidth > 0)
                    mainSplit.sidebarWidth = Math.max(Theme.sidebarMin, Math.min(520, s.sidebarWidth))
            }

            onResizingChanged: {
                if (!resizing)
                    App.saveUiState({ sidebarWidth: Math.round(sidebar.width) })
            }

            SidebarColumn {
                id: sidebar
                Controls.SplitView.preferredWidth: mainSplit.sidebarWidth
                Controls.SplitView.minimumWidth: Math.min(Theme.sidebarMin, Math.max(180, mainSplit.hostW * 0.22))
                Controls.SplitView.maximumWidth: Math.min(520, Math.max(Theme.sidebarMin, mainSplit.hostW * 0.45))
                clip: true
            }

            WorkspaceDetail {
                id: detail
                Controls.SplitView.fillWidth: true
                Controls.SplitView.minimumWidth: Math.min(Theme.inspectorMin, Math.max(240, mainSplit.hostW * 0.35))
                clip: true
                tabNames: page.tabNames
                tabGroups: page.tabGroups
                tabGroup: page.tabGroup
                tabsIndex: page.tabsIndex
                commandList: sidebar.commandList
                onTabGroupRequested: function (index) { page.setTabGroup(index) }
                onTabRequested: function (index) { page.tabsIndex = index }
                onFocusTree: function (stateKey) { sidebar.statusPanel.focusChanges(stateKey) }
            }
        }

        SessionFooter {
            Layout.fillWidth: true
        }
    }
}
