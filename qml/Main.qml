pragma ComponentBehavior: Bound
import QtQuick
import org.kde.kirigami as Kirigami
import Ghew

Kirigami.ApplicationWindow {
    id: root
    title: {
        if (!App.repo.isOpen)
            return I18n.i18n("GHEW")
        const name = App.repo.rootPath.split("/").pop() || "repo"
        const mark = App.repo.dirtyCount > 0 ? (" ●" + App.repo.dirtyCount) : " ○"
        return "GHEW — " + name + mark
    }
    width: 1180
    height: 760
    minimumWidth: 720
    minimumHeight: 480
    color: Theme.ink

    CommandPalette {
        parent: root.overlay
        anchors.fill: parent
    }

    BranchDialog {
        parent: root.overlay
        anchors.fill: parent
    }

    TagDialog {
        parent: root.overlay
        anchors.fill: parent
    }

    globalDrawer: Kirigami.GlobalDrawer {
        title: I18n.i18n("GHEW")
        titleIcon: "folder-git"
        isMenu: true
        actions: [
            Kirigami.Action {
                text: I18n.i18n("Репозиторий")
                enabled: false
            },
            Kirigami.Action {
                text: I18n.i18n("Открыть…")
                icon.name: "folder-open"
                shortcut: StandardKey.Open
                onTriggered: root.openRepoAction()
            },
            Kirigami.Action {
                text: I18n.i18n("Создать…")
                icon.name: "folder-new"
                onTriggered: root.initRepoAction()
            },
            Kirigami.Action {
                text: I18n.i18n("Сменить")
                icon.name: "document-open-recent"
                enabled: App.repo.isOpen
                onTriggered: root.showWelcome()
            },
            Kirigami.Action {
                text: I18n.i18n("Обновить")
                icon.name: "view-refresh"
                shortcut: "F5"
                enabled: App.repo.isOpen
                onTriggered: App.refreshRepo()
            },
            Kirigami.Action {
                text: I18n.i18n("Терминал")
                icon.name: "utilities-terminal"
                shortcut: "Ctrl+Shift+T"
                enabled: App.repo.isOpen
                onTriggered: App.openTerminalInRepo()
            },
            Kirigami.Action { separator: true },
            Kirigami.Action {
                text: I18n.i18n("Инструменты")
                enabled: false
            },
            Kirigami.Action {
                text: I18n.i18n("Палитра команд")
                icon.name: "search"
                shortcut: "Ctrl+P"
                onTriggered: App.openPalette()
            },
            Kirigami.Action {
                text: I18n.i18n("Ветка…")
                icon.name: "vcs-branch"
                shortcut: "Ctrl+B"
                enabled: App.repo.isOpen
                onTriggered: App.openBranchDialog()
            },
            Kirigami.Action {
                text: I18n.i18n("Тег…")
                icon.name: "tag"
                shortcut: "Ctrl+T"
                enabled: App.repo.isOpen
                onTriggered: App.openTagDialog()
            }
        ]
    }

    function currentPageName() {
        return pageStack.currentItem ? pageStack.currentItem.objectName : ""
    }

    function showWelcome() {
        if (root.currentPageName() === "welcome")
            return
        pageStack.clear()
        pageStack.push(welcomeComponent)
    }

    function showWorkspace() {
        if (root.currentPageName() === "workspace")
            return
        pageStack.clear()
        pageStack.push(workspaceComponent)
    }

    function invokeOpenRepoDialog() {
        const page = pageStack.currentItem as WelcomePage
        if (page)
            page.openRepoDialog()
    }

    function openRepoAction() {
        if (root.currentPageName() !== "welcome")
            root.showWelcome()
        Qt.callLater(root.invokeOpenRepoDialog)
    }

    function invokeInitRepoDialog() {
        const page = pageStack.currentItem as WelcomePage
        if (page)
            page.openInitDialog()
    }

    function initRepoAction() {
        if (root.currentPageName() !== "welcome")
            root.showWelcome()
        Qt.callLater(root.invokeInitRepoDialog)
    }

    Component {
        id: welcomeComponent
        WelcomePage {
            objectName: "welcome"
        }
    }

    Component {
        id: workspaceComponent
        WorkspacePage {
            objectName: "workspace"
        }
    }

    pageStack.initialPage: WelcomePage {
        objectName: "welcome"
    }

    Connections {
        target: App.repo
        function onChanged() {
            if (App.repo.isOpen)
                root.showWorkspace()
            else
                root.showWelcome()
        }
    }

    Connections {
        target: App
        function onErrorOccurred(message) {
            root.showPassiveNotification(message, "short")
        }
        function onInfoOccurred(message) {
            root.showPassiveNotification(message, "short")
        }
    }

    Component.onCompleted: {
        const s = App.loadUiState()
        if (s.width > 0)
            root.width = s.width
        if (s.height > 0)
            root.height = s.height
        if (s.x >= 0 && s.y >= 0) {
            root.x = s.x
            root.y = s.y
        }
        if (App.repo.isOpen)
            root.showWorkspace()
    }

    onClosing: {
        App.saveUiState({
            width: root.width,
            height: root.height,
            x: root.x,
            y: root.y
        })
    }

    Shortcut {
        sequences: [StandardKey.Open]
        onActivated: root.openRepoAction()
    }
    Shortcut {
        sequence: "F5"
        onActivated: {
            if (App.repo.isOpen)
                App.refreshRepo()
        }
    }
    Shortcut {
        sequence: "Ctrl+L"
        onActivated: App.probeLlm()
    }
    Shortcut {
        sequence: "Ctrl+K"
        onActivated: {
            const page = root.pageStack.currentItem as WorkspacePage
            if (page)
                page.focusCommandSearch()
        }
    }
    Shortcut {
        sequence: "Ctrl+P"
        onActivated: App.openPalette()
    }
    Shortcut {
        sequence: "Escape"
        enabled: App.paletteOpen || App.branchDialogOpen || App.tagDialogOpen
        onActivated: {
            if (App.paletteOpen)
                App.closePalette()
            else if (App.branchDialogOpen)
                App.closeBranchDialog()
            else if (App.tagDialogOpen)
                App.closeTagDialog()
        }
    }
    Shortcut {
        sequence: "Ctrl+T"
        enabled: App.repo.isOpen
        onActivated: App.openTagDialog()
    }
    Shortcut {
        sequence: "Ctrl+B"
        enabled: App.repo.isOpen
        onActivated: App.openBranchDialog()
    }
}
