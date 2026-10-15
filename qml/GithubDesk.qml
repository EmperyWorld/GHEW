pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

// Manual GitHub card: description, visibility, topics, settings, and the dressing files.
Controls.ScrollView {
    id: root

    signal focusAdvisor()

    clip: true
    contentWidth: availableWidth

    property bool pulling: false
    property string visDraft: "private"
    property bool nameDirty: false
    property bool cardDirty: false
    property bool settingsDirty: false
    property bool readmeDirty: false
    property bool licenseDirty: false
    property bool ignoreDirty: false
    property string readmeLoaded: ""
    property string licenseLoaded: ""
    property string ignoreLoaded: ""
    property string fileKind: "readme"
    property bool setIssues: false
    property bool setWiki: false
    property bool setProjects: false
    property bool setDiscussions: false
    property bool setDeleteBranch: false
    property bool setTemplate: false
    property string branchDraft: ""
    property bool policyDirty: false
    property bool setMerge: false
    property bool setSquash: false
    property bool setRebase: false
    property bool setAutoMerge: false
    property bool setUpdateBranch: false
    property bool setForking: false
    property bool setSignoff: false
    property bool setPullRequests: false
    property bool setAdvanced: false
    property bool setScanning: false
    property bool setPushProtection: false
    property string squashTitle: ""
    property string squashBody: ""
    property string mergeTitle: ""
    property string mergeBody: ""
    property string prPolicy: ""
    property bool forkDefaultOnly: false
    property bool forkRewire: false
    property bool extraDirty: false
    property string extraLoaded: ""
    property bool pagesDirty: false
    property string pagesBranch: ""
    property string pagesPath: "/"
    property string pagesCname: ""
    property bool pagesHttps: false
    property bool pagesEnabled: false
    property bool wikiDirty: false
    property string wikiLoaded: ""

    function isExtra(kind) {
        return kind === "contributing" || kind === "conduct" || kind === "security" || kind === "support"
                || kind === "funding" || kind === "codeowners" || kind === "issue" || kind === "pull"
    }

    function selectFile(kind) {
        root.fileKind = kind
        if (!root.isExtra(kind))
            return
        extraArea.text = App.github.dressText(kind)
        root.extraLoaded = extraArea.text
        root.extraDirty = false
    }

    function pagesInSync() {
        const p = App.github.pages
        if (!p.known)
            return false
        return root.pagesEnabled === p.enabled
                && root.pagesBranch === (p.branch || "")
                && root.pagesPath === (p.path || "/")
                && root.pagesCname === (p.cname || "")
                && (!p.httpsKnown || root.pagesHttps === p.https)
    }

    function pullPages() {
        const p = App.github.pages
        root.pagesEnabled = !!p.enabled
        root.pagesBranch = p.branch || App.github.defaultBranch || "main"
        root.pagesPath = p.path || "/"
        root.pagesCname = p.cname || ""
        root.pagesHttps = !!p.https
        if (!pagesBranchField.activeFocus)
            pagesBranchField.text = root.pagesBranch
        if (!pagesCnameField.activeFocus)
            pagesCnameField.text = root.pagesCname
    }

    function flagSame(known, draft, remote) {
        return !known || draft === remote
    }

    function policyInSync() {
        const p = App.github.policy
        if (!p.loaded)
            return false
        return root.flagSame(p.mergeCommitKnown, root.setMerge, p.mergeCommit)
                && root.flagSame(p.squashMergeKnown, root.setSquash, p.squashMerge)
                && root.flagSame(p.rebaseMergeKnown, root.setRebase, p.rebaseMerge)
                && root.flagSame(p.autoMergeKnown, root.setAutoMerge, p.autoMerge)
                && root.flagSame(p.updateBranchKnown, root.setUpdateBranch, p.updateBranch)
                && root.flagSame(p.allowForkingKnown, root.setForking, p.allowForking)
                && root.flagSame(p.webSignoffKnown, root.setSignoff, p.webSignoff)
                && root.flagSame(p.pullRequestsKnown, root.setPullRequests, p.pullRequests)
                && root.flagSame(p.advancedSecurityKnown, root.setAdvanced, p.advancedSecurity)
                && root.flagSame(p.secretScanningKnown, root.setScanning, p.secretScanning)
                && root.flagSame(p.pushProtectionKnown, root.setPushProtection, p.pushProtection)
                && root.flagSame(p.squashTitleKnown, root.squashTitle, p.squashTitle)
                && root.flagSame(p.squashBodyKnown, root.squashBody, p.squashBody)
                && root.flagSame(p.mergeTitleKnown, root.mergeTitle, p.mergeTitle)
                && root.flagSame(p.mergeBodyKnown, root.mergeBody, p.mergeBody)
                && root.flagSame(p.prPolicyKnown, root.prPolicy, p.prPolicy)
    }

    function policyHasControls() {
        const p = App.github.policy
        return !!(p.mergeCommitKnown || p.squashMergeKnown || p.rebaseMergeKnown || p.autoMergeKnown
                  || p.updateBranchKnown || p.allowForkingKnown || p.webSignoffKnown || p.pullRequestsKnown
                  || p.prPolicyKnown || p.squashTitleKnown || p.squashBodyKnown || p.mergeTitleKnown
                  || p.mergeBodyKnown)
    }

    function pullPolicy() {
        const p = App.github.policy
        root.setMerge = !!p.mergeCommit
        root.setSquash = !!p.squashMerge
        root.setRebase = !!p.rebaseMerge
        root.setAutoMerge = !!p.autoMerge
        root.setUpdateBranch = !!p.updateBranch
        root.setForking = !!p.allowForking
        root.setSignoff = !!p.webSignoff
        root.setPullRequests = !!p.pullRequests
        root.setAdvanced = !!p.advancedSecurity
        root.setScanning = !!p.secretScanning
        root.setPushProtection = !!p.pushProtection
        root.squashTitle = p.squashTitle || ""
        root.squashBody = p.squashBody || ""
        root.mergeTitle = p.mergeTitle || ""
        root.mergeBody = p.mergeBody || ""
        root.prPolicy = p.prPolicy || ""
    }

    function savePolicy() {
        const p = App.github.policy
        App.saveGithubPolicy({
            "mergeCommit": root.setMerge,
            "mergeCommitKnown": p.mergeCommitKnown,
            "squashMerge": root.setSquash,
            "squashMergeKnown": p.squashMergeKnown,
            "rebaseMerge": root.setRebase,
            "rebaseMergeKnown": p.rebaseMergeKnown,
            "autoMerge": root.setAutoMerge,
            "autoMergeKnown": p.autoMergeKnown,
            "updateBranch": root.setUpdateBranch,
            "updateBranchKnown": p.updateBranchKnown,
            "allowForking": root.setForking,
            "allowForkingKnown": p.allowForkingKnown,
            "webSignoff": root.setSignoff,
            "webSignoffKnown": p.webSignoffKnown,
            "pullRequests": root.setPullRequests,
            "pullRequestsKnown": p.pullRequestsKnown,
            "advancedSecurity": root.setAdvanced,
            "advancedSecurityKnown": p.advancedSecurityKnown,
            "secretScanning": root.setScanning,
            "secretScanningKnown": p.secretScanningKnown,
            "pushProtection": root.setPushProtection,
            "pushProtectionKnown": p.pushProtectionKnown,
            "squashTitle": root.squashTitle,
            "squashTitleKnown": p.squashTitleKnown,
            "squashBody": root.squashBody,
            "squashBodyKnown": p.squashBodyKnown,
            "mergeTitle": root.mergeTitle,
            "mergeTitleKnown": p.mergeTitleKnown,
            "mergeBody": root.mergeBody,
            "mergeBodyKnown": p.mergeBodyKnown,
            "prPolicy": root.prPolicy,
            "prPolicyKnown": p.prPolicyKnown
        })
    }

    function submitTopic() {
        if (topicField.text.trim().length === 0 || App.busy)
            return
        App.addGithubTopic(topicField.text)
        topicField.text = ""
    }

    function pullRemote() {
        if (root.pulling)
            return
        root.pulling = true
        if (!nameField.activeFocus && !root.nameDirty)
            nameField.text = App.github.originIsGithub ? App.github.repoName : App.github.suggestedName
        if (!descArea.activeFocus && !root.cardDirty)
            descArea.text = App.github.description
        if (!homeField.activeFocus && !root.cardDirty)
            homeField.text = App.github.homepage
        if (descArea.text === App.github.description && homeField.text.trim() === App.github.homepage
                && (App.github.visibility.length === 0 || root.visDraft === App.github.visibility))
            root.cardDirty = false
        if (nameField.text.trim() === (App.github.originIsGithub ? App.github.repoName : App.github.suggestedName))
            root.nameDirty = false
        if (root.setIssues === App.github.issuesEnabled && root.setWiki === App.github.wikiEnabled
                && root.setProjects === App.github.projectsEnabled && root.setDiscussions === App.github.discussionsEnabled
                && root.setDeleteBranch === App.github.deleteBranchOnMerge && root.setTemplate === App.github.isTemplate
                && branchField.text.trim() === App.github.defaultBranch)
            root.settingsDirty = false
        if (!root.cardDirty && App.github.visibility.length > 0)
            root.visDraft = App.github.visibility
        if (!branchField.activeFocus && !root.settingsDirty)
            root.branchDraft = App.github.defaultBranch
        if (!root.settingsDirty) {
            root.setIssues = App.github.issuesEnabled
            root.setWiki = App.github.wikiEnabled
            root.setProjects = App.github.projectsEnabled
            root.setDiscussions = App.github.discussionsEnabled
            root.setDeleteBranch = App.github.deleteBranchOnMerge
            root.setTemplate = App.github.isTemplate
            branchField.text = App.github.defaultBranch
        }
        if (root.policyInSync())
            root.policyDirty = false
        if (!root.policyDirty)
            root.pullPolicy()
        if (root.pagesInSync())
            root.pagesDirty = false
        if (!root.pagesDirty)
            root.pullPages()
        if (root.isExtra(root.fileKind) && !extraArea.activeFocus && !root.extraDirty) {
            extraArea.text = App.github.dressText(root.fileKind)
            root.extraLoaded = extraArea.text
        }
        if (!readmeArea.activeFocus && !root.readmeDirty) {
            readmeArea.text = App.github.dressText("readme")
            root.readmeLoaded = readmeArea.text
        }
        if (!licenseArea.activeFocus && !root.licenseDirty) {
            licenseArea.text = App.github.dressText("license")
            root.licenseLoaded = licenseArea.text
        }
        if (!ignoreArea.activeFocus && !root.ignoreDirty) {
            ignoreArea.text = App.github.dressText("gitignore")
            root.ignoreLoaded = ignoreArea.text
        }
        root.pulling = false
    }

    component InkBox: Rectangle {
        radius: Theme.radiusSm
        color: Theme.ink
        border.width: 1
        border.color: parent.activeFocus ? Theme.copper : Theme.inkLine
    }

    Connections {
        target: App.github
        function onChanged() { root.pullRemote() }
    }

    ColumnLayout {
        width: root.availableWidth
        spacing: Theme.space12

        SectionHead {
            Layout.fillWidth: true
            title: I18n.i18n("GitHub")
            meta: App.github.statusLine
            accent: Theme.copper
        }

        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Theme.paper
            font.family: Theme.uiFamily
            font.pixelSize: Theme.typeBody
            visible: App.github.note.length > 0
            text: App.github.note
        }

        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: Theme.typeMeta
            text: App.github.htmlUrl.length > 0
                  ? App.github.htmlUrl
                  : I18n.i18n("Поля ниже правятся вручную. Запись на GitHub идёт через gh, токен в программу не попадает.")
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            ChipButton {
                label: I18n.i18n("Обновить")
                accent: Theme.copper
                enabled: !App.busy
                onClicked: {
                    root.cardDirty = false
                    root.nameDirty = false
                    root.settingsDirty = false
                    root.readmeDirty = false
                    root.licenseDirty = false
                    root.ignoreDirty = false
                    App.github.refresh()
                }
            }
            ChipButton {
                label: I18n.i18n("Копировать ссылку")
                enabled: App.github.htmlUrl.length > 0
                onClicked: App.copyText(App.github.htmlUrl)
            }
            ChipButton {
                label: I18n.i18n("Поручить советнику")
                active: true
                accent: Theme.copper
                enabled: App.repo.isOpen && !App.busy && !App.advisor.advising
                onClicked: {
                    const name = nameField.text.trim().length > 0 ? nameField.text.trim() : App.github.suggestedName
                    App.askAdvice(I18n.i18n(
                        "Оформи этот репозиторий на GitHub и веди процесс до конца. Имя: %1. "
                        + "Репозиторий приватный, пока я явно не попрошу публичный. "
                        + "Тексты README, LICENSE и .gitignore я правлю на вкладке GitHub — не затирай их. "
                        + "Каждый следующий шаг давай блоком RUN.")
                        .arg(name))
                    root.focusAdvisor()
                }
            }
        }

        SurfaceCard {
            Layout.fillWidth: true
            kicker: I18n.i18n("Чеклист")
            meta: I18n.i18n("что ещё не оформлено")
            goldEdge: true

            Repeater {
                model: App.github.checklist
                delegate: RowLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: Theme.space8

                    Rectangle {
                        Layout.preferredWidth: 8
                        Layout.preferredHeight: 8
                        radius: 4
                        color: modelData.ok ? Theme.safe : Theme.mutates
                    }
                    Text {
                        Layout.preferredWidth: 150
                        text: modelData.title
                        color: Theme.paper
                        font.family: Theme.uiFamily
                        font.pixelSize: Theme.typeMeta
                        elide: Text.ElideRight
                    }
                    Text {
                        Layout.fillWidth: true
                        text: modelData.detail
                        color: Theme.mist
                        font.family: Theme.uiFamily
                        font.pixelSize: Theme.typeMeta
                        elide: Text.ElideRight
                    }
                }
            }
        }

        SurfaceCard {
            Layout.fillWidth: true
            kicker: I18n.i18n("Карточка")
            meta: App.github.originIsGithub ? App.github.slug : I18n.i18n("ещё без origin")

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: Theme.space8
                rowSpacing: Theme.space8

                Text {
                    text: I18n.i18n("Имя")
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: Theme.typeMeta
                }
                Controls.TextField {
                    id: nameField
                    Layout.fillWidth: true
                    color: Theme.paper
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.typeMeta
                    placeholderText: App.github.suggestedName
                    background: InkBox { implicitHeight: 28 }
                    onTextEdited: root.nameDirty = true
                }

                Text {
                    text: I18n.i18n("Описание")
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: Theme.typeMeta
                    Layout.alignment: Qt.AlignTop
                }
                Controls.TextArea {
                    id: descArea
                    Layout.fillWidth: true
                    Layout.preferredHeight: 72
                    wrapMode: TextEdit.Wrap
                    color: Theme.paper
                    font.family: Theme.uiFamily
                    font.pixelSize: Theme.typeBody
                    placeholderText: I18n.i18n("Зачем этот репозиторий")
                    background: InkBox {}
                    onTextEdited: root.cardDirty = true
                }

                Text {
                    text: I18n.i18n("Сайт")
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: Theme.typeMeta
                }
                Controls.TextField {
                    id: homeField
                    Layout.fillWidth: true
                    color: Theme.paper
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.typeMeta
                    placeholderText: "https://"
                    background: InkBox { implicitHeight: 28 }
                    onTextEdited: root.cardDirty = true
                }

                Text {
                    text: I18n.i18n("Видимость")
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: Theme.typeMeta
                }
                ChipFlow {
                    Layout.fillWidth: true
                    spacing: Theme.space8

                    ChipButton {
                        label: I18n.i18n("Приватный")
                        active: root.visDraft === "private"
                        accent: Theme.safe
                        onClicked: {
                            root.visDraft = "private"
                            root.cardDirty = true
                        }
                    }
                    ChipButton {
                        label: I18n.i18n("Публичный")
                        active: root.visDraft === "public"
                        accent: Theme.destructive
                        onClicked: {
                            root.visDraft = "public"
                            root.cardDirty = true
                        }
                    }
                    ChipButton {
                        visible: App.github.originIsGithub
                        label: "internal"
                        active: root.visDraft === "internal"
                        accent: Theme.mutates
                        onClicked: {
                            root.visDraft = "internal"
                            root.cardDirty = true
                        }
                    }
                }
            }

            ChipFlow {
                Layout.fillWidth: true
                spacing: Theme.space8

                ActionButton {
                    visible: !App.github.hasOrigin
                    text: root.visDraft === "public"
                           ? I18n.i18n("Создать публичный и отправить")
                           : I18n.i18n("Создать и отправить")
                    kind: root.visDraft === "public" ? "danger" : "primary"
                    enabled: App.repo.isOpen && !App.busy && App.github.available && App.github.authenticated
                             && !App.github.hasOrigin && App.repo.localCommits > 0
                    onClicked: App.runCommand("github_create", {
                        name: nameField.text.trim(),
                        description: descArea.text.trim(),
                        homepage: homeField.text.trim(),
                        private: root.visDraft !== "public",
                        public: root.visDraft === "public",
                        source: ".",
                        remote: "origin",
                        push: true
                    }, false)
                }
                ActionButton {
                    visible: App.github.originIsGithub
                    text: root.visDraft === "public" && App.github.visibility !== "public"
                           ? I18n.i18n("Записать и открыть публично")
                           : I18n.i18n("Записать карточку")
                    kind: root.visDraft === "public" && App.github.visibility !== "public" ? "danger" : "primary"
                    enabled: !App.busy && root.cardDirty
                    onClicked: App.saveGithubCard(descArea.text, homeField.text.trim(), root.visDraft)
                }
                ActionButton {
                    visible: App.github.originIsGithub
                    text: I18n.i18n("Переименовать")
                    kind: "ghost"
                    enabled: !App.busy && nameField.text.trim().length > 0
                             && nameField.text.trim() !== App.github.repoName
                    onClicked: App.renameGithubRepo(nameField.text.trim())
                }
                ActionButton {
                    visible: App.github.originIsGithub
                    text: I18n.i18n("Опубликовать ветку")
                    kind: "ghost"
                    enabled: !App.busy
                    onClicked: App.runCommand("github_push", {}, false)
                }
            }
        }

        SurfaceCard {
            Layout.fillWidth: true
            visible: App.github.originIsGithub
            kicker: I18n.i18n("Темы")
            meta: I18n.i18n("клик по теме снимает её")

            ChipFlow {
                Layout.fillWidth: true
                spacing: Theme.space8

                Repeater {
                    model: App.github.topics
                    delegate: ChipButton {
                        required property var modelData
                        label: modelData + "  ×"
                        accent: Theme.copper
                        enabled: !App.busy
                        onClicked: App.removeGithubTopic(modelData)
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space8

                Controls.TextField {
                    id: topicField
                    Layout.fillWidth: true
                    color: Theme.paper
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.typeMeta
                    placeholderText: I18n.i18n("новая тема")
                    background: InkBox { implicitHeight: 28 }
                    onAccepted: root.submitTopic()
                }
                ActionButton {
                    id: addTopic
                    text: I18n.i18n("Добавить")
                    kind: "ghost"
                    enabled: !App.busy && topicField.text.trim().length > 0
                    onClicked: root.submitTopic()
                }
            }
        }

        SurfaceCard {
            Layout.fillWidth: true
            visible: App.github.originIsGithub
            kicker: I18n.i18n("Настройки репозитория")
            meta: I18n.i18n("включаются и выключаются вручную")

            ChipFlow {
                Layout.fillWidth: true
                spacing: Theme.space8

                ChipButton {
                    label: I18n.i18n("Issues")
                    active: root.setIssues
                    onClicked: { root.setIssues = !root.setIssues; root.settingsDirty = true }
                }
                ChipButton {
                    label: "Wiki"
                    active: root.setWiki
                    onClicked: { root.setWiki = !root.setWiki; root.settingsDirty = true }
                }
                ChipButton {
                    label: I18n.i18n("Projects")
                    active: root.setProjects
                    onClicked: { root.setProjects = !root.setProjects; root.settingsDirty = true }
                }
                ChipButton {
                    label: I18n.i18n("Discussions")
                    active: root.setDiscussions
                    onClicked: { root.setDiscussions = !root.setDiscussions; root.settingsDirty = true }
                }
                ChipButton {
                    label: I18n.i18n("Удалять ветку после merge")
                    active: root.setDeleteBranch
                    onClicked: { root.setDeleteBranch = !root.setDeleteBranch; root.settingsDirty = true }
                }
                ChipButton {
                    label: I18n.i18n("Шаблон")
                    active: root.setTemplate
                    accent: Theme.mutates
                    onClicked: { root.setTemplate = !root.setTemplate; root.settingsDirty = true }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space8

                Text {
                    text: I18n.i18n("Ветка по умолчанию")
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: Theme.typeMeta
                }
                Controls.TextField {
                    id: branchField
                    Layout.fillWidth: true
                    color: Theme.paper
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.typeMeta
                    background: InkBox { implicitHeight: 28 }
                    onTextEdited: {
                        root.branchDraft = text
                        root.settingsDirty = true
                    }
                }
                ActionButton {
                    text: I18n.i18n("Записать настройки")
                    kind: "primary"
                    enabled: !App.busy && root.settingsDirty
                    onClicked: App.saveGithubSettings(root.setIssues, root.setWiki, root.setProjects, root.setDiscussions,
                                               root.setDeleteBranch, root.setTemplate, branchField.text.trim())
                }
            }
        }

        SurfaceCard {
            Layout.fillWidth: true
            visible: App.github.originIsGithub
            kicker: I18n.i18n("Слияние и доступ")
            meta: I18n.i18n("пишется в политику репозитория")

            Text {
                Layout.fillWidth: true
                visible: !App.github.policy.loaded
                wrapMode: Text.WordWrap
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                text: I18n.i18n("Политика ещё не прочитана. Нажмите «Обновить».")
            }
            Text {
                Layout.fillWidth: true
                visible: App.github.policy.loaded && !root.policyHasControls()
                wrapMode: Text.WordWrap
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                text: I18n.i18n("GitHub не отдал поля политики для этого входа. Чипы появятся, когда API вернёт значения.")
            }

            ChipFlow {
                Layout.fillWidth: true
                spacing: Theme.space8
                visible: App.github.policy.loaded

                ChipButton {
                    visible: App.github.policy.mergeCommitKnown
                    label: I18n.i18n("Merge-коммит")
                    active: root.setMerge
                    onClicked: { root.setMerge = !root.setMerge; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.squashMergeKnown
                    label: "Squash"
                    active: root.setSquash
                    onClicked: { root.setSquash = !root.setSquash; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.rebaseMergeKnown
                    label: "Rebase"
                    active: root.setRebase
                    onClicked: { root.setRebase = !root.setRebase; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.autoMergeKnown
                    label: I18n.i18n("Автослияние")
                    active: root.setAutoMerge
                    onClicked: { root.setAutoMerge = !root.setAutoMerge; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.updateBranchKnown
                    label: I18n.i18n("Обновлять ветку PR")
                    active: root.setUpdateBranch
                    onClicked: { root.setUpdateBranch = !root.setUpdateBranch; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.allowForkingKnown
                    label: I18n.i18n("Разрешить форк")
                    active: root.setForking
                    onClicked: { root.setForking = !root.setForking; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.webSignoffKnown
                    label: I18n.i18n("Sign-off в вебе")
                    active: root.setSignoff
                    onClicked: { root.setSignoff = !root.setSignoff; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.pullRequestsKnown
                    label: I18n.i18n("Pull requests")
                    active: root.setPullRequests
                    onClicked: { root.setPullRequests = !root.setPullRequests; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.prPolicyKnown
                    label: I18n.i18n("PR создают все")
                    active: root.prPolicy === "all"
                    onClicked: { root.prPolicy = "all"; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.prPolicyKnown
                    label: I18n.i18n("PR только соавторы")
                    active: root.prPolicy === "collaborators_only"
                    onClicked: { root.prPolicy = "collaborators_only"; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.squashTitleKnown
                    label: I18n.i18n("Squash: заголовок PR")
                    active: root.squashTitle === "PR_TITLE"
                    onClicked: { root.squashTitle = "PR_TITLE"; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.squashTitleKnown
                    label: I18n.i18n("Squash: коммит или PR")
                    active: root.squashTitle === "COMMIT_OR_PR_TITLE"
                    onClicked: { root.squashTitle = "COMMIT_OR_PR_TITLE"; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.squashBodyKnown
                    label: I18n.i18n("Squash: текст PR")
                    active: root.squashBody === "PR_BODY"
                    onClicked: { root.squashBody = "PR_BODY"; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.squashBodyKnown
                    label: I18n.i18n("Squash: сообщения коммитов")
                    active: root.squashBody === "COMMIT_MESSAGES"
                    onClicked: { root.squashBody = "COMMIT_MESSAGES"; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.squashBodyKnown
                    label: I18n.i18n("Squash: пустой текст")
                    active: root.squashBody === "BLANK"
                    onClicked: { root.squashBody = "BLANK"; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.mergeTitleKnown
                    label: I18n.i18n("Merge: заголовок PR")
                    active: root.mergeTitle === "PR_TITLE"
                    onClicked: { root.mergeTitle = "PR_TITLE"; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.mergeTitleKnown
                    label: I18n.i18n("Merge: сообщение слияния")
                    active: root.mergeTitle === "MERGE_MESSAGE"
                    onClicked: { root.mergeTitle = "MERGE_MESSAGE"; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.mergeBodyKnown
                    label: I18n.i18n("Merge: текст PR")
                    active: root.mergeBody === "PR_BODY"
                    onClicked: { root.mergeBody = "PR_BODY"; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.mergeBodyKnown
                    label: I18n.i18n("Merge: заголовок как текст")
                    active: root.mergeBody === "PR_TITLE"
                    onClicked: { root.mergeBody = "PR_TITLE"; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.mergeBodyKnown
                    label: I18n.i18n("Merge: пустой текст")
                    active: root.mergeBody === "BLANK"
                    onClicked: { root.mergeBody = "BLANK"; root.policyDirty = true }
                }
            }

            ActionButton {
                visible: App.github.policy.loaded
                text: I18n.i18n("Записать политику")
                kind: "primary"
                enabled: !App.busy && root.policyDirty
                onClicked: root.savePolicy()
            }
        }

        SurfaceCard {
            Layout.fillWidth: true
            visible: App.github.originIsGithub && (App.github.policy.secretScanningKnown
                     || App.github.policy.advancedSecurityKnown || App.github.policy.archivedKnown)
            kicker: I18n.i18n("Защита и архив")
            meta: App.github.policy.archived ? I18n.i18n("репозиторий в архиве") : I18n.i18n("репозиторий активен")

            ChipFlow {
                Layout.fillWidth: true
                spacing: Theme.space8

                ChipButton {
                    visible: App.github.policy.advancedSecurityKnown
                    label: I18n.i18n("Advanced Security")
                    active: root.setAdvanced
                    accent: Theme.mutates
                    onClicked: { root.setAdvanced = !root.setAdvanced; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.secretScanningKnown
                    label: I18n.i18n("Поиск секретов")
                    active: root.setScanning
                    accent: Theme.mutates
                    onClicked: { root.setScanning = !root.setScanning; root.policyDirty = true }
                }
                ChipButton {
                    visible: App.github.policy.pushProtectionKnown
                    label: I18n.i18n("Блок пуша секретов")
                    active: root.setPushProtection
                    accent: Theme.destructive
                    onClicked: { root.setPushProtection = !root.setPushProtection; root.policyDirty = true }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                visible: App.github.policy.archivedKnown
                spacing: Theme.space8

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: Theme.paper
                    font.family: Theme.uiFamily
                    font.pixelSize: Theme.typeMeta
                    text: App.github.policy.archived
                          ? I18n.i18n("Архив закрывает запись. Вернуть репозиторий можно отсюда же, с подтверждением.")
                          : I18n.i18n("Архив закрывает запись для всех, кроме администраторов. Перед выполнением будет превью.")
                }
                ActionButton {
                    text: App.github.policy.archived ? I18n.i18n("Вернуть из архива") : I18n.i18n("Архивировать")
                    kind: "danger"
                    enabled: !App.busy
                    onClicked: App.setGithubArchived(!App.github.policy.archived)
                }
            }

            ActionButton {
                visible: App.github.policy.secretScanningKnown || App.github.policy.advancedSecurityKnown
                         || App.github.policy.pushProtectionKnown
                text: I18n.i18n("Записать политику")
                kind: "primary"
                enabled: !App.busy && root.policyDirty
                onClicked: root.savePolicy()
            }
        }

        SurfaceCard {
            Layout.fillWidth: true
            visible: App.github.originIsGithub
            kicker: I18n.i18n("Форк, передача и удаление")
            meta: I18n.i18n("каждая кнопка останавливается на превью")

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                text: I18n.i18n("Форк по умолчанию не меняет remote. Переключение origin, передача и удаление просят подтверждение и точное имя.")
            }

            ChipFlow {
                Layout.fillWidth: true
                spacing: Theme.space8

                Controls.TextField {
                    id: forkNameField
                    implicitWidth: 180
                    color: Theme.paper
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.typeMeta
                    placeholderText: I18n.i18n("имя форка")
                    background: InkBox { implicitHeight: 28 }
                }
                Controls.TextField {
                    id: forkOrgField
                    implicitWidth: 160
                    color: Theme.paper
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.typeMeta
                    placeholderText: I18n.i18n("организация")
                    background: InkBox { implicitHeight: 28 }
                }
                ChipButton {
                    label: I18n.i18n("Только ветка по умолчанию")
                    active: root.forkDefaultOnly
                    onClicked: root.forkDefaultOnly = !root.forkDefaultOnly
                }
                ChipButton {
                    label: I18n.i18n("Форк станет origin")
                    active: root.forkRewire
                    accent: Theme.destructive
                    onClicked: root.forkRewire = !root.forkRewire
                }
                ActionButton {
                    text: I18n.i18n("Создать форк")
                    kind: root.forkRewire ? "danger" : "primary"
                    enabled: !App.busy
                    onClicked: App.forkGithubRepo(forkNameField.text, forkOrgField.text, root.forkDefaultOnly, root.forkRewire)
                }
            }

            ChipFlow {
                Layout.fillWidth: true
                spacing: Theme.space8

                Controls.TextField {
                    id: transferOwnerField
                    implicitWidth: 160
                    color: Theme.paper
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.typeMeta
                    placeholderText: I18n.i18n("новый владелец")
                    background: InkBox { implicitHeight: 28 }
                }
                Controls.TextField {
                    id: transferNameField
                    implicitWidth: 160
                    color: Theme.paper
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.typeMeta
                    placeholderText: I18n.i18n("новое имя")
                    background: InkBox { implicitHeight: 28 }
                }
                Controls.TextField {
                    id: transferTeamsField
                    implicitWidth: 140
                    color: Theme.paper
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.typeMeta
                    placeholderText: I18n.i18n("id команд")
                    background: InkBox { implicitHeight: 28 }
                }
                Controls.TextField {
                    id: transferConfirmField
                    implicitWidth: 180
                    color: Theme.paper
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.typeMeta
                    placeholderText: App.github.repoName.length > 0 ? App.github.repoName : I18n.i18n("имя репозитория")
                    background: InkBox { implicitHeight: 28 }
                }
                ActionButton {
                    text: I18n.i18n("Передать")
                    kind: "danger"
                    enabled: !App.busy && transferOwnerField.text.trim().length > 0
                             && transferConfirmField.text === App.github.repoName && App.github.repoName.length > 0
                    onClicked: App.transferGithubRepo(transferOwnerField.text, transferNameField.text,
                                                      transferTeamsField.text, transferConfirmField.text)
                }
            }

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                text: App.github.hasLfsAttributes
                      ? I18n.i18n("В .gitattributes уже есть filter=lfs. Кнопки ниже меняют выключатель на GitHub, не локальный git-lfs.")
                      : I18n.i18n("GitHub не сообщает, включён ли LFS. Включение может тарифицироваться.")
            }

            ChipFlow {
                Layout.fillWidth: true
                spacing: Theme.space8

                ActionButton {
                    text: I18n.i18n("Включить Git LFS")
                    kind: "primary"
                    enabled: !App.busy
                    onClicked: App.setGithubLfs(true)
                }
                ActionButton {
                    text: I18n.i18n("Выключить Git LFS")
                    kind: "danger"
                    enabled: !App.busy
                    onClicked: App.setGithubLfs(false)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space8

                Controls.TextField {
                    id: deleteConfirmField
                    Layout.fillWidth: true
                    color: Theme.paper
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.typeMeta
                    placeholderText: App.github.repoName.length > 0
                                     ? I18n.i18n("для удаления введите %1", App.github.repoName)
                                     : I18n.i18n("имя репозитория")
                    background: InkBox { implicitHeight: 28 }
                }
                ActionButton {
                    text: I18n.i18n("Удалить на GitHub")
                    kind: "danger"
                    enabled: !App.busy && deleteConfirmField.text === App.github.repoName && App.github.repoName.length > 0
                    onClicked: App.deleteGithubRepo(deleteConfirmField.text)
                }
            }
        }

        SurfaceCard {
            Layout.fillWidth: true
            kicker: I18n.i18n("Файлы")
            meta: App.github.dressFileNameFor(root.fileKind) || root.fileKind

            ChipFlow {
                Layout.fillWidth: true
                spacing: Theme.space8

                ChipButton {
                    label: "README"
                    active: root.fileKind === "readme"
                    accent: Theme.copper
                    onClicked: root.selectFile("readme")
                }
                ChipButton {
                    label: "LICENSE"
                    active: root.fileKind === "license"
                    accent: Theme.copper
                    onClicked: root.selectFile("license")
                }
                ChipButton {
                    label: ".gitignore"
                    active: root.fileKind === "gitignore"
                    accent: Theme.copper
                    onClicked: root.selectFile("gitignore")
                }
                ChipButton {
                    label: "CONTRIBUTING"
                    active: root.fileKind === "contributing"
                    onClicked: root.selectFile("contributing")
                }
                ChipButton {
                    label: "CONDUCT"
                    active: root.fileKind === "conduct"
                    onClicked: root.selectFile("conduct")
                }
                ChipButton {
                    label: "SECURITY"
                    active: root.fileKind === "security"
                    onClicked: root.selectFile("security")
                }
                ChipButton {
                    label: "SUPPORT"
                    active: root.fileKind === "support"
                    onClicked: root.selectFile("support")
                }
                ChipButton {
                    label: "FUNDING"
                    active: root.fileKind === "funding"
                    onClicked: root.selectFile("funding")
                }
                ChipButton {
                    label: "CODEOWNERS"
                    active: root.fileKind === "codeowners"
                    onClicked: root.selectFile("codeowners")
                }
                ChipButton {
                    label: I18n.i18n("Шаблон issue")
                    active: root.fileKind === "issue"
                    onClicked: root.selectFile("issue")
                }
                ChipButton {
                    label: I18n.i18n("Шаблон PR")
                    active: root.fileKind === "pull"
                    onClicked: root.selectFile("pull")
                }
                ChipButton {
                    visible: root.fileKind === "readme"
                    label: I18n.i18n("Вставить шаблон")
                    onClicked: {
                        readmeArea.text = App.github.readmeTemplate(nameField.text.trim(), descArea.text.trim())
                        root.readmeDirty = true
                    }
                }
                ChipButton {
                    visible: root.fileKind === "license"
                    label: "MIT"
                    active: false
                    onClicked: {
                        licenseArea.text = App.github.licenseTemplate("mit")
                        root.licenseDirty = true
                    }
                }
                ChipButton {
                    visible: root.fileKind === "license"
                    label: "BSD"
                    onClicked: {
                        licenseArea.text = App.github.licenseTemplate("bsd2")
                        root.licenseDirty = true
                    }
                }
                ChipButton {
                    visible: root.fileKind === "license"
                    label: I18n.i18n("Своя")
                    accent: Theme.mutates
                    onClicked: {
                        licenseArea.text = App.github.licenseTemplate("proprietary")
                        root.licenseDirty = true
                    }
                }
                ChipButton {
                    visible: root.isExtra(root.fileKind)
                    label: I18n.i18n("Вставить шаблон")
                    onClicked: {
                        extraArea.text = App.github.dressTemplate(root.fileKind)
                        root.extraDirty = extraArea.text !== root.extraLoaded
                    }
                }
                Controls.TextField {
                    id: catalogKey
                    visible: root.fileKind === "license" || root.fileKind === "gitignore"
                    implicitWidth: 140
                    color: Theme.paper
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.typeMeta
                    placeholderText: root.fileKind === "gitignore" ? "Python" : "mit"
                    background: InkBox { implicitHeight: 28 }
                }
                ChipButton {
                    visible: root.fileKind === "license" || root.fileKind === "gitignore"
                    label: I18n.i18n("Каталог GitHub")
                    enabled: !App.busy && catalogKey.text.trim().length > 0
                    onClicked: {
                        const text = App.fetchGithubTemplate(root.fileKind, catalogKey.text.trim())
                        if (text.length === 0)
                            return
                        if (root.fileKind === "license") {
                            licenseArea.text = text
                            root.licenseDirty = true
                        } else {
                            ignoreArea.text = text
                            root.ignoreDirty = true
                        }
                    }
                }
            }

            Controls.TextArea {
                id: readmeArea
                visible: root.fileKind === "readme"
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 220 : 0
                wrapMode: TextEdit.Wrap
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                selectByMouse: true
                placeholderText: I18n.i18n("Текст README. Шаблон только подставляет черновик, файл пишется кнопкой ниже.")
                background: InkBox {}
                onTextEdited: root.readmeDirty = text !== root.readmeLoaded
            }
            Controls.TextArea {
                id: licenseArea
                visible: root.fileKind === "license"
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 220 : 0
                wrapMode: TextEdit.Wrap
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                selectByMouse: true
                placeholderText: I18n.i18n("Текст лицензии. Существующий файл меняется только когда вы нажмёте «Записать файл».")
                background: InkBox {}
                onTextEdited: root.licenseDirty = text !== root.licenseLoaded
            }
            Controls.TextArea {
                id: ignoreArea
                visible: root.fileKind === "gitignore"
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 220 : 0
                wrapMode: TextEdit.Wrap
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                selectByMouse: true
                placeholderText: I18n.i18n("Шаблоны игнорирования, по одному на строку.")
                background: InkBox {}
                onTextEdited: root.ignoreDirty = text !== root.ignoreLoaded
            }
            Controls.TextArea {
                id: extraArea
                visible: root.isExtra(root.fileKind)
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 220 : 0
                wrapMode: TextEdit.Wrap
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                selectByMouse: true
                placeholderText: I18n.i18n("Черновик файла оформления. На диск он попадает только кнопкой «Записать файл».")
                background: InkBox {}
                onTextEdited: root.extraDirty = text !== root.extraLoaded
            }

            ActionButton {
                text: I18n.i18n("Записать файл")
                kind: "primary"
                enabled: App.repo.isOpen && !App.busy
                         && ((root.fileKind === "readme" && root.readmeDirty)
                             || (root.fileKind === "license" && root.licenseDirty)
                             || (root.fileKind === "gitignore" && root.ignoreDirty)
                             || (root.isExtra(root.fileKind) && root.extraDirty))
                onClicked: {
                    root.pulling = true
                    let body = ignoreArea.text
                    if (root.fileKind === "readme")
                        body = readmeArea.text
                    else if (root.fileKind === "license")
                        body = licenseArea.text
                    else if (root.isExtra(root.fileKind))
                        body = extraArea.text
                    const ok = App.saveDressFile(root.fileKind, body)
                    if (ok && root.fileKind === "readme") {
                        readmeArea.text = App.github.dressText("readme")
                        root.readmeLoaded = readmeArea.text
                        root.readmeDirty = false
                    } else if (ok && root.fileKind === "license") {
                        licenseArea.text = App.github.dressText("license")
                        root.licenseLoaded = licenseArea.text
                        root.licenseDirty = false
                    } else if (ok && root.fileKind === "gitignore") {
                        ignoreArea.text = App.github.dressText("gitignore")
                        root.ignoreLoaded = ignoreArea.text
                        root.ignoreDirty = false
                    } else if (ok) {
                        extraArea.text = App.github.dressText(root.fileKind)
                        root.extraLoaded = extraArea.text
                        root.extraDirty = false
                    }
                    root.pulling = false
                }
            }

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                text: I18n.i18n("Файл остаётся локальным, пока вы сами не закоммитите и не отправите ветку. Шаблон не затирает диск, пока не нажата запись.")
            }
        }

        SurfaceCard {
            Layout.fillWidth: true
            visible: App.github.originIsGithub
            kicker: I18n.i18n("GitHub Pages")
            meta: App.github.pages.htmlUrl || I18n.i18n("сайт ещё не создан")

            Text {
                Layout.fillWidth: true
                visible: !App.github.pages.known
                wrapMode: Text.WordWrap
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                text: I18n.i18n("Состояние Pages ещё не прочитано. Нажмите «Обновить».")
            }

            ChipFlow {
                Layout.fillWidth: true
                spacing: Theme.space8
                visible: App.github.pages.known

                Controls.TextField {
                    id: pagesBranchField
                    implicitWidth: 140
                    color: Theme.paper
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.typeMeta
                    placeholderText: I18n.i18n("ветка")
                    background: InkBox { implicitHeight: 28 }
                    onTextEdited: {
                        root.pagesBranch = text
                        root.pagesDirty = true
                    }
                }
                ChipButton {
                    label: "/"
                    active: root.pagesPath === "/"
                    onClicked: { root.pagesPath = "/"; root.pagesDirty = true }
                }
                ChipButton {
                    label: "/docs"
                    active: root.pagesPath === "/docs"
                    onClicked: { root.pagesPath = "/docs"; root.pagesDirty = true }
                }
                Controls.TextField {
                    id: pagesCnameField
                    implicitWidth: 180
                    color: Theme.paper
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.typeMeta
                    placeholderText: I18n.i18n("свой домен")
                    background: InkBox { implicitHeight: 28 }
                    onTextEdited: {
                        root.pagesCname = text
                        root.pagesDirty = true
                    }
                }
                ChipButton {
                    visible: App.github.pages.httpsKnown
                    label: "HTTPS"
                    active: root.pagesHttps
                    onClicked: { root.pagesHttps = !root.pagesHttps; root.pagesDirty = true }
                }
                ActionButton {
                    text: root.pagesEnabled ? I18n.i18n("Записать Pages") : I18n.i18n("Создать Pages")
                    kind: "primary"
                    enabled: !App.busy && (root.pagesDirty || !root.pagesEnabled)
                    onClicked: App.saveGithubPages(root.pagesBranch, root.pagesPath, root.pagesCname, root.pagesHttps, true)
                }
                ActionButton {
                    visible: App.github.pages.enabled
                    text: I18n.i18n("Удалить сайт")
                    kind: "danger"
                    enabled: !App.busy
                    onClicked: App.saveGithubPages(root.pagesBranch, root.pagesPath, root.pagesCname, root.pagesHttps, false)
                }
            }
        }

        SurfaceCard {
            Layout.fillWidth: true
            visible: App.github.originIsGithub
            kicker: "Wiki"
            meta: I18n.i18n("отправка подтверждается на превью")

            ChipFlow {
                Layout.fillWidth: true
                spacing: Theme.space8

                Controls.TextField {
                    id: wikiNameField
                    implicitWidth: 160
                    color: Theme.paper
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.typeMeta
                    placeholderText: "Home"
                    background: InkBox { implicitHeight: 28 }
                }
                ChipButton {
                    label: I18n.i18n("Обновить список")
                    enabled: !App.busy
                    onClicked: App.github.refreshWiki()
                }
                ChipButton {
                    label: I18n.i18n("Прочитать")
                    enabled: wikiNameField.text.trim().length > 0
                    onClicked: {
                        wikiArea.text = App.github.wikiPageText(wikiNameField.text.trim())
                        root.wikiLoaded = wikiArea.text
                        root.wikiDirty = false
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                visible: App.github.wikiPages.length > 0
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                text: App.github.wikiPages.join(", ")
            }

            Controls.TextArea {
                id: wikiArea
                Layout.fillWidth: true
                Layout.preferredHeight: 160
                wrapMode: TextEdit.Wrap
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                selectByMouse: true
                placeholderText: I18n.i18n("Текст страницы wiki. Отправка делает коммит в отдельном клоне wiki и не трогает этот репозиторий.")
                background: InkBox {}
                onTextEdited: root.wikiDirty = text !== root.wikiLoaded
            }

            ActionButton {
                text: I18n.i18n("Отправить страницу")
                kind: "danger"
                enabled: !App.busy && wikiNameField.text.trim().length > 0 && root.wikiDirty
                onClicked: App.publishWikiPage(wikiNameField.text.trim(), wikiArea.text)
            }
        }

        GithubServiceDeck {
            Layout.fillWidth: true
            visible: App.github.originIsGithub
        }

        GithubAccessDeck {
            Layout.fillWidth: true
            visible: App.github.originIsGithub
        }

        GithubIdentityDeck {
            Layout.fillWidth: true
            visible: App.github.originIsGithub
        }
    }

    Component.onCompleted: {
        if (App.repo.isOpen)
            App.github.refresh()
        else
            root.pullRemote()
    }
}
