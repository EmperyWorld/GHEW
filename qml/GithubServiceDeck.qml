pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

// Issues, pull requests, discussions, projects, releases, and autolinks.
ColumnLayout {
    id: root

    spacing: Theme.space12

    property string issueLinkKind: "assignee"
    property string issueCloseReason: ""
    property bool prDraft: false
    property bool releaseDraft: true
    property bool releasePre: false
    property bool releaseCleanup: false
    property bool autolinkNumeric: false
    property string projectType: "TEXT"
    property string projectLayout: "BOARD_LAYOUT"

    component InkBox: Rectangle {
        radius: Theme.radiusSm
        color: Theme.ink
        border.width: 1
        border.color: parent.activeFocus ? Theme.copper : Theme.inkLine
    }

    SectionHead {
        Layout.fillWidth: true
        title: I18n.i18n("Сервисы репозитория")
        meta: I18n.i18n("удаление, слияние и релизы останавливаются на превью")
        accent: Theme.copper
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Списки")
        meta: I18n.i18n("чтение, без записи")

        ActionButton {
            text: I18n.i18n("Обновить списки")
            kind: "ghost"
            enabled: !App.busy
            onClicked: App.github.refreshServices()
        }

        Text {
            Layout.fillWidth: true
            visible: App.github.serviceDigest.length > 0
            wrapMode: Text.WordWrap
            color: Theme.mist
            font.family: Theme.monoFamily
            font.pixelSize: Theme.typeMeta
            text: App.github.serviceDigest
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: "Issues"
        meta: I18n.i18n("создание, правка, комментарий, pin и lock")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: issueNumber
                implicitWidth: 90
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("номер")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: issueTitle
                implicitWidth: 240
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("заголовок")
                background: InkBox { implicitHeight: 28 }
            }
        }

        Controls.TextArea {
            id: issueBody
            Layout.fillWidth: true
            Layout.preferredHeight: 90
            wrapMode: TextEdit.Wrap
            color: Theme.paper
            font.family: Theme.uiFamily
            font.pixelSize: Theme.typeMeta
            selectByMouse: true
            placeholderText: I18n.i18n("Текст issue. Пустой номер создаёт новую задачу.")
            background: InkBox {}
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            ActionButton {
                text: I18n.i18n("Создать")
                kind: "primary"
                enabled: !App.busy && issueNumber.text.trim().length === 0
                onClicked: App.saveGithubIssue("", issueTitle.text, issueBody.text)
            }
            ActionButton {
                text: I18n.i18n("Записать")
                kind: "primary"
                enabled: !App.busy && issueNumber.text.trim().length > 0
                onClicked: App.saveGithubIssue(issueNumber.text, issueTitle.text, issueBody.text)
            }
            ChipButton {
                label: "completed"
                active: root.issueCloseReason === "completed"
                onClicked: root.issueCloseReason = root.issueCloseReason === "completed" ? "" : "completed"
            }
            ChipButton {
                label: "not planned"
                active: root.issueCloseReason === "not planned"
                onClicked: root.issueCloseReason = root.issueCloseReason === "not planned" ? "" : "not planned"
            }
            ActionButton {
                text: I18n.i18n("Закрыть")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.actGithubIssue(issueNumber.text, "close", root.issueCloseReason)
            }
            ActionButton {
                text: I18n.i18n("Открыть снова")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.actGithubIssue(issueNumber.text, "reopen", "")
            }
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: issueComment
                implicitWidth: 280
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("комментарий")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Комментарий")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.actGithubIssue(issueNumber.text, "comment", issueComment.text)
            }
            ActionButton {
                text: I18n.i18n("Закрепить")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.actGithubIssue(issueNumber.text, "pin", "")
            }
            ActionButton {
                text: I18n.i18n("Снять")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.actGithubIssue(issueNumber.text, "unpin", "")
            }
            Controls.TextField {
                id: lockReason
                implicitWidth: 120
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "resolved"
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Закрыть обсуждение")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.actGithubIssue(issueNumber.text, "lock", lockReason.text)
            }
            ActionButton {
                text: I18n.i18n("Открыть обсуждение")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.actGithubIssue(issueNumber.text, "unlock", "")
            }
            ActionButton {
                text: I18n.i18n("Удалить issue")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.actGithubIssue(issueNumber.text, "delete", "")
            }
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Метки, вехи и связи")
        meta: I18n.i18n("подзадачи и зависимости — по номеру")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            ChipButton {
                label: I18n.i18n("исполнитель")
                active: root.issueLinkKind === "assignee"
                onClicked: root.issueLinkKind = "assignee"
            }
            ChipButton {
                label: I18n.i18n("метка")
                active: root.issueLinkKind === "label"
                onClicked: root.issueLinkKind = "label"
            }
            ChipButton {
                label: I18n.i18n("веха")
                active: root.issueLinkKind === "milestone"
                onClicked: root.issueLinkKind = "milestone"
            }
            ChipButton {
                label: I18n.i18n("подзадача")
                active: root.issueLinkKind === "subissue"
                onClicked: root.issueLinkKind = "subissue"
            }
            ChipButton {
                label: I18n.i18n("блокирует")
                active: root.issueLinkKind === "blocked"
                onClicked: root.issueLinkKind = "blocked"
            }
            ChipButton {
                label: I18n.i18n("блокирует другие")
                active: root.issueLinkKind === "blocking"
                onClicked: root.issueLinkKind = "blocking"
            }
            ChipButton {
                label: I18n.i18n("родитель")
                active: root.issueLinkKind === "parent"
                onClicked: root.issueLinkKind = "parent"
            }
            Controls.TextField {
                id: issueLinkValue
                implicitWidth: 180
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("@me, имя или номер")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Добавить связь")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.linkGithubIssue(issueNumber.text, root.issueLinkKind, issueLinkValue.text, false)
            }
            ActionButton {
                text: I18n.i18n("Убрать связь")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.linkGithubIssue(issueNumber.text, root.issueLinkKind, issueLinkValue.text, true)
            }
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: labelName
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("метка")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: labelColor
                implicitWidth: 90
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "E99695"
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: labelAbout
                implicitWidth: 180
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("описание")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: labelRename
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("новое имя")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Создать метку")
                kind: "primary"
                enabled: !App.busy
                onClicked: App.createGithubLabel(labelName.text, labelColor.text, labelAbout.text)
            }
            ActionButton {
                text: I18n.i18n("Изменить метку")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.editGithubLabel(labelName.text, labelColor.text, labelAbout.text, labelRename.text)
            }
            ActionButton {
                text: I18n.i18n("Удалить метку")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.deleteGithubLabel(labelName.text)
            }
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: milestoneNumber
                implicitWidth: 90
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("номер вехи")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: milestoneTitle
                implicitWidth: 180
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("название вехи")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Открыть веху")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.saveGithubMilestone(milestoneNumber.text, milestoneTitle.text, "open")
            }
            ActionButton {
                text: I18n.i18n("Закрыть веху")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.saveGithubMilestone(milestoneNumber.text, milestoneTitle.text, "closed")
            }
            ActionButton {
                text: I18n.i18n("Удалить веху")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubMilestone(milestoneNumber.text, milestoneTitle.text, "delete")
            }
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: "Pull requests"
        meta: I18n.i18n("создание не пушит ветку")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: prNumber
                implicitWidth: 90
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("номер")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: prTitle
                implicitWidth: 220
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("заголовок")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: prBase
                implicitWidth: 120
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("база")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: prHead
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("ветка, иначе текущая")
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton {
                label: I18n.i18n("черновик")
                active: root.prDraft
                onClicked: root.prDraft = !root.prDraft
            }
        }

        Controls.TextArea {
            id: prBody
            Layout.fillWidth: true
            Layout.preferredHeight: 90
            wrapMode: TextEdit.Wrap
            color: Theme.paper
            font.family: Theme.uiFamily
            font.pixelSize: Theme.typeMeta
            selectByMouse: true
            placeholderText: I18n.i18n("Текст pull request. Тот же текст уходит в ревью, если нажата кнопка ревью.")
            background: InkBox {}
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            ActionButton {
                text: I18n.i18n("Создать")
                kind: "primary"
                enabled: !App.busy && prNumber.text.trim().length === 0
                onClicked: App.saveGithubPull("", prTitle.text, prBody.text, prBase.text, prHead.text, root.prDraft)
            }
            ActionButton {
                text: I18n.i18n("Записать")
                kind: "primary"
                enabled: !App.busy && prNumber.text.trim().length > 0
                onClicked: App.saveGithubPull(prNumber.text, prTitle.text, prBody.text, prBase.text, prHead.text, false)
            }
            ActionButton {
                text: I18n.i18n("Одобрить")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.reviewGithubPull(prNumber.text, "approve", prBody.text)
            }
            ActionButton {
                text: I18n.i18n("Замечание")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.reviewGithubPull(prNumber.text, "comment", prBody.text)
            }
            ActionButton {
                text: I18n.i18n("Просить правки")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.reviewGithubPull(prNumber.text, "request", prBody.text)
            }
            ActionButton {
                text: I18n.i18n("Готов к ревью")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.readyGithubPull(prNumber.text, false)
            }
            ActionButton {
                text: I18n.i18n("В черновик")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.readyGithubPull(prNumber.text, true)
            }
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            ActionButton {
                text: I18n.i18n("Слить merge")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.mergeGithubPull(prNumber.text, "merge")
            }
            ActionButton {
                text: I18n.i18n("Слить squash")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.mergeGithubPull(prNumber.text, "squash")
            }
            ActionButton {
                text: I18n.i18n("Слить rebase")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.mergeGithubPull(prNumber.text, "rebase")
            }
            ActionButton {
                text: I18n.i18n("Обновить ветку")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.updateGithubPullBranch(prNumber.text, false)
            }
            ActionButton {
                text: I18n.i18n("Обновить rebase")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.updateGithubPullBranch(prNumber.text, true)
            }
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: "Discussions"
        meta: I18n.i18n("тема, правка и комментарий")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: discussionNumber
                implicitWidth: 90
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("номер")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: discussionTitle
                implicitWidth: 220
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("заголовок")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: discussionCategory
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("категория")
                background: InkBox { implicitHeight: 28 }
            }
        }

        Controls.TextArea {
            id: discussionBody
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            wrapMode: TextEdit.Wrap
            color: Theme.paper
            font.family: Theme.uiFamily
            font.pixelSize: Theme.typeMeta
            selectByMouse: true
            placeholderText: I18n.i18n("Текст обсуждения или комментария.")
            background: InkBox {}
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            ActionButton {
                text: I18n.i18n("Создать тему")
                kind: "primary"
                enabled: !App.busy && discussionNumber.text.trim().length === 0
                onClicked: App.saveGithubDiscussion("", discussionTitle.text, discussionBody.text, discussionCategory.text)
            }
            ActionButton {
                text: I18n.i18n("Записать тему")
                kind: "primary"
                enabled: !App.busy && discussionNumber.text.trim().length > 0
                onClicked: App.saveGithubDiscussion(discussionNumber.text, discussionTitle.text, discussionBody.text, discussionCategory.text)
            }
            ActionButton {
                text: I18n.i18n("Комментарий")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.commentGithubDiscussion(discussionNumber.text, discussionBody.text, false)
            }
            Controls.TextField {
                id: discussionCommentId
                implicitWidth: 160
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "DC_…"
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Удалить комментарий")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.commentGithubDiscussion(discussionCommentId.text, "", true)
            }
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: "Projects"
        meta: I18n.i18n("поля, карточки и виды. Нужен scope project")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: projectTitle
                implicitWidth: 200
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("название проекта")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Создать проект")
                kind: "primary"
                enabled: !App.busy
                onClicked: App.createGithubProject(projectTitle.text)
            }
            Controls.TextField {
                id: projectNumber
                implicitWidth: 110
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("номер проекта")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Привязать к репозиторию")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.linkGithubProject(projectNumber.text)
            }
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: projectFieldName
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("имя поля")
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton {
                label: I18n.i18n("текст")
                active: root.projectType === "TEXT"
                onClicked: root.projectType = "TEXT"
            }
            ChipButton {
                label: I18n.i18n("список")
                active: root.projectType === "SINGLE_SELECT"
                onClicked: root.projectType = "SINGLE_SELECT"
            }
            ChipButton {
                label: I18n.i18n("дата")
                active: root.projectType === "DATE"
                onClicked: root.projectType = "DATE"
            }
            ChipButton {
                label: I18n.i18n("число")
                active: root.projectType === "NUMBER"
                onClicked: root.projectType = "NUMBER"
            }
            Controls.TextField {
                id: projectOptions
                visible: root.projectType === "SINGLE_SELECT"
                implicitWidth: 180
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("один, два, три")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Создать поле")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.createGithubProjectField(projectNumber.text, projectFieldName.text, root.projectType, projectOptions.text)
            }
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: projectItemTitle
                implicitWidth: 180
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("карточка")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: projectItemBody
                implicitWidth: 200
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("текст карточки")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Создать карточку")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.createGithubProjectItem(projectNumber.text, projectItemTitle.text, projectItemBody.text)
            }
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: projectUrl
                implicitWidth: 280
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "https://github.com/owner/repo/issues/1"
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: projectFieldEdit
                implicitWidth: 120
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("поле")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: projectValue
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("значение")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Добавить ссылку")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.addGithubProjectItem(projectNumber.text, projectUrl.text)
            }
            ActionButton {
                text: I18n.i18n("Записать поле")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.editGithubProjectItem(projectNumber.text, projectUrl.text, projectFieldEdit.text, projectValue.text)
            }
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: projectViewName
                implicitWidth: 160
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("название вида")
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton {
                label: I18n.i18n("доска")
                active: root.projectLayout === "BOARD_LAYOUT"
                onClicked: root.projectLayout = "BOARD_LAYOUT"
            }
            ChipButton {
                label: I18n.i18n("таблица")
                active: root.projectLayout === "TABLE_LAYOUT"
                onClicked: root.projectLayout = "TABLE_LAYOUT"
            }
            ChipButton {
                label: I18n.i18n("дорожная карта")
                active: root.projectLayout === "ROADMAP_LAYOUT"
                onClicked: root.projectLayout = "ROADMAP_LAYOUT"
            }
            ActionButton {
                text: I18n.i18n("Создать вид")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.createGithubProjectView(projectNumber.text, projectViewName.text, root.projectLayout)
            }
            Controls.TextField {
                id: projectViewId
                implicitWidth: 180
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "PVTV_…"
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Переименовать вид")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.editGithubProjectView(projectNumber.text, projectViewId.text, projectViewName.text, false)
            }
            ActionButton {
                text: I18n.i18n("Удалить вид")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.editGithubProjectView(projectNumber.text, projectViewId.text, "", true)
            }
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Релизы")
        meta: I18n.i18n("создание может завести тег, если его ещё нет")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: releaseTag
                implicitWidth: 120
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("тег")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: releaseTitle
                implicitWidth: 180
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("заголовок")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: releaseTarget
                implicitWidth: 120
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("ветка или SHA")
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton {
                label: I18n.i18n("черновик")
                active: root.releaseDraft
                onClicked: root.releaseDraft = !root.releaseDraft
            }
            ChipButton {
                label: I18n.i18n("предрелиз")
                active: root.releasePre
                onClicked: root.releasePre = !root.releasePre
            }
            ChipButton {
                label: I18n.i18n("удалить тег")
                active: root.releaseCleanup
                onClicked: root.releaseCleanup = !root.releaseCleanup
            }
        }

        Controls.TextArea {
            id: releaseNotes
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            wrapMode: TextEdit.Wrap
            color: Theme.paper
            font.family: Theme.uiFamily
            font.pixelSize: Theme.typeMeta
            selectByMouse: true
            placeholderText: I18n.i18n("Текст релиза.")
            background: InkBox {}
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            ActionButton {
                text: I18n.i18n("Создать релиз")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.createGithubRelease(releaseTag.text, releaseTitle.text, releaseNotes.text, root.releaseDraft, root.releasePre, releaseTarget.text)
            }
            ActionButton {
                text: I18n.i18n("Записать релиз")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.editGithubRelease(releaseTag.text, releaseTitle.text, releaseNotes.text, root.releaseDraft, root.releasePre, releaseTarget.text)
            }
            ActionButton {
                text: I18n.i18n("Удалить релиз")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.deleteGithubRelease(releaseTag.text, root.releaseCleanup)
            }
            Controls.TextField {
                id: releaseFile
                implicitWidth: 260
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("полный путь к файлу")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Прикрепить файл")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.uploadGithubRelease(releaseTag.text, releaseFile.text)
            }
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Автоссылки")
        meta: I18n.i18n("нужна роль администратора репозитория")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: autolinkPrefix
                implicitWidth: 120
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "TICKET-"
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: autolinkUrl
                implicitWidth: 280
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "https://example.com/TICKET?query=<num>"
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton {
                label: I18n.i18n("только цифры")
                active: root.autolinkNumeric
                onClicked: root.autolinkNumeric = !root.autolinkNumeric
            }
            ActionButton {
                text: I18n.i18n("Создать автоссылку")
                kind: "primary"
                enabled: !App.busy
                onClicked: App.createGithubAutolink(autolinkPrefix.text, autolinkUrl.text, root.autolinkNumeric)
            }
            Controls.TextField {
                id: autolinkId
                implicitWidth: 80
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "id"
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Удалить автоссылку")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.deleteGithubAutolink(autolinkId.text)
            }
        }
    }
}
