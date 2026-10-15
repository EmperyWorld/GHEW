pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

// Right zone: slim chrome, optional context band, inspector stack.
Item {
    id: root

    property var tabNames: []
    property var tabGroups: []
    property int tabGroup: 0
    property int tabsIndex: 0
    property var commandList: null
    property bool contextOpen: false

    signal tabGroupRequested(int index)
    signal tabRequested(int index)
    signal focusTree(string stateKey)

    property alias paramForm: paramForm
    property alias previewPanel: previewPanel

    readonly property bool showCommandChrome: root.tabGroup === 0
    readonly property bool hasOperation: App.repo.operationKind.length > 0 || App.repo.conflictCount > 0
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

    function openPrepared(commandId) {
        if (!commandId.length || !root.commandList)
            return
        if (root.commandList.selectById(commandId))
            root.tabRequested(1)
    }

    ColumnLayout {
        id: detail
        anchors.fill: parent
        anchors.margins: Theme.space12
        spacing: Theme.space8
        clip: true
        opacity: 0.35

        OpacityAnimator {
            id: enter
            target: detail
            from: 0.35
            to: 1
            duration: 280
            easing.type: Easing.OutCubic
        }
        Component.onCompleted: enter.start()

        OperationBanner {
            Layout.fillWidth: true
            visible: root.hasOperation
            continueId: root.continueCommandId
            abortId: root.abortCommandId
            onOpenCommand: function (commandId) { root.openPrepared(commandId) }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space8

            TabRail {
                Layout.fillWidth: true
                groups: root.tabGroups
                names: root.tabNames
                group: root.tabGroup
                current: root.tabsIndex
                onGroupChosen: function (index) { root.tabGroupRequested(index) }
                onTabChosen: function (index) { root.tabRequested(index) }
            }

            ChipButton {
                label: root.contextOpen ? I18n.i18n("контекст ▴") : I18n.i18n("контекст ▾")
                tip: I18n.i18n("Пульс, коммит, быстрые действия")
                active: root.contextOpen
                onClicked: root.contextOpen = !root.contextOpen
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.space8
            visible: root.contextOpen
            clip: true

            RepoPulse {
                Layout.fillWidth: true
                onFocusState: function (stateKey) { root.focusTree(stateKey) }
            }

            CommitDock {
                Layout.fillWidth: true
                onPreviewed: root.tabRequested(3)
            }

            QuickActionBar {
                Layout.fillWidth: true
                visible: App.repo.isOpen
            }

            CommandMast {
                Layout.fillWidth: true
                visible: root.showCommandChrome
                commandTitle: root.commandList ? root.commandList.selectedTitle : ""
                synopsis: root.commandList ? root.commandList.selectedSynopsis : ""
                danger: root.commandList ? root.commandList.selectedDanger : ""
                commandId: root.commandList ? root.commandList.selectedId : ""
                favorited: root.commandList
                           ? App.favorites.contains(root.commandList.selectedId)
                           : false
                onFavoriteToggled: {
                    if (root.commandList && root.commandList.selectedId.length)
                        App.toggleFavorite(root.commandList.selectedId)
                }
            }

            KeyLegend {
                Layout.alignment: Qt.AlignLeft
                opacity: 0.9
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.inkLine
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            currentIndex: root.tabsIndex

            AdvicePanel {
                onCommandChosen: function (commandId) { root.openPrepared(commandId) }
                onCommandRun: function (commandId) { App.runCommand(commandId, {}, true) }
            }

            ColumnLayout {
                spacing: Theme.space8
                clip: true

                SectionHead {
                    Layout.fillWidth: true
                    title: I18n.i18n("Параметры")
                    meta: root.commandList ? root.commandList.selectedId : ""
                }

                ScrollPane {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ParamForm {
                        id: paramForm
                        Layout.fillWidth: true
                        commandId: root.commandList ? root.commandList.selectedId : ""
                        commandSynopsis: ""
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.space8

                    ActionButton {
                        text: I18n.i18n("Превью")
                        kind: "primary"
                        enabled: root.commandList
                                 && root.commandList.selectedId.length > 0
                                 && App.repo.isOpen && !App.busy
                        onClicked: {
                            App.previewCommand(root.commandList.selectedId, paramForm.collect())
                            if (App.lastPreviewJson.length)
                                root.tabRequested(3)
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: I18n.i18n("Превью показывает точную строку и последствия до запуска.")
                        color: Theme.mist
                        font.family: Theme.uiFamily
                        font.pixelSize: Theme.typeMeta
                        wrapMode: Text.WordWrap
                        opacity: 0.75
                    }
                }
            }

            ScrollPane {
                SectionHead {
                    Layout.fillWidth: true
                    title: I18n.i18n("Пояснение")
                }
                CommandFacts {
                    Layout.fillWidth: true
                    commandId: root.commandList ? root.commandList.selectedId : ""
                }
                EmptyHint {
                    Layout.fillWidth: true
                    visible: !root.commandList || root.commandList.selectedExplanation.length === 0
                    title: I18n.i18n("Пояснения ещё нет")
                    body: I18n.i18n("Выберите команду в каталоге слева. Здесь будет, зачем она нужна и чем опасна.")
                }
                Text {
                    Layout.fillWidth: true
                    visible: root.commandList && root.commandList.selectedExplanation.length > 0
                    wrapMode: Text.WordWrap
                    textFormat: Text.MarkdownText
                    color: Theme.paper
                    font.family: Theme.uiFamily
                    font.pixelSize: Theme.typeBody
                    lineHeight: 1.4
                    text: root.commandList ? root.commandList.selectedExplanation : ""
                }
            }

            PreviewPanel {
                id: previewPanel
                onExecuted: root.tabRequested(7)
            }

            DiffPanel {}
            HunkPanel {}
            InspectPanel {}
            OutputTerminal {}
            HistoryPanel {}
            WizardPanel {}
            SettingsPanel {}
            GithubDesk {
                onFocusAdvisor: root.tabRequested(0)
            }
        }
    }
}
