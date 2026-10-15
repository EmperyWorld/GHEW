pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

// Collaborators, Actions, security controls, and codespaces.
ColumnLayout {
    id: root

    spacing: Theme.space12

    property string permission: "push"
    property string secretApp: "actions"
    property string alertKind: "dependabot"
    property string alertState: "dismissed"
    property string alertReason: "tolerable_risk"
    property string rulesetMode: "active"
    property string rulesetRules: "deletion"
    property int reviews: 1
    property bool enforceAdmins: false
    property bool blockForce: true
    property bool blockDelete: true
    property bool keyReadOnly: true
    property string webhookEvents: "push"
    property bool webhookActive: true
    property string severity: "low"
    property string ecosystem: "npm"
    property string interaction: "collaborators_only"
    property bool protectedBranches: false

    function hasCsv(list, name) {
        return list.split(",").indexOf(name) >= 0
    }

    function toggleCsv(list, name) {
        const parts = list.split(",").filter(part => part.length > 0)
        const index = parts.indexOf(name)
        if (index >= 0)
            parts.splice(index, 1)
        else
            parts.push(name)
        return parts.join(",")
    }

    component InkBox: Rectangle {
        radius: Theme.radiusSm
        color: Theme.ink
        border.width: 1
        border.color: parent.activeFocus ? Theme.copper : Theme.inkLine
    }

    SectionHead {
        Layout.fillWidth: true
        title: I18n.i18n("Доступ, Actions и безопасность")
        meta: I18n.i18n("секрет не попадает в превью")
        accent: Theme.copper
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Соавторы")
        meta: I18n.i18n("приглашение и отзыв после превью")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: collabLogin
                implicitWidth: 160
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("логин")
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton { label: "pull"; active: root.permission === "pull"; onClicked: root.permission = "pull" }
            ChipButton { label: "triage"; active: root.permission === "triage"; onClicked: root.permission = "triage" }
            ChipButton { label: "push"; active: root.permission === "push"; onClicked: root.permission = "push" }
            ChipButton { label: "maintain"; active: root.permission === "maintain"; onClicked: root.permission = "maintain" }
            ChipButton { label: "admin"; active: root.permission === "admin"; onClicked: root.permission = "admin" }
            ActionButton {
                text: I18n.i18n("Пригласить")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubCollaborator(collabLogin.text, root.permission, false)
            }
            ActionButton {
                text: I18n.i18n("Убрать доступ")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubCollaborator(collabLogin.text, root.permission, true)
            }
            Controls.TextField {
                id: invitationId
                implicitWidth: 110
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("номер приглашения")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Отозвать приглашение")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.deleteGithubInvitation(invitationId.text)
            }
            ActionButton {
                text: I18n.i18n("Принять приглашение")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.acceptGithubInvitation(invitationId.text)
            }
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: "Actions"
        meta: I18n.i18n("запуск и удаление после превью")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: workflowName
                implicitWidth: 160
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "ci.yml"
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: workflowRef
                implicitWidth: 120
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("ветка")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: workflowKey
                implicitWidth: 100
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("поле")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: workflowValue
                implicitWidth: 120
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("значение")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Включить")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.actGithubWorkflow("enable", workflowName.text, "", "", "")
            }
            ActionButton {
                text: I18n.i18n("Выключить")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.actGithubWorkflow("disable", workflowName.text, "", "", "")
            }
            ActionButton {
                text: I18n.i18n("Запустить")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.actGithubWorkflow("run", workflowName.text, workflowRef.text, workflowKey.text, workflowValue.text)
            }
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: runId
                implicitWidth: 120
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("номер запуска")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Отменить")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.actGithubRun("cancel", runId.text, false)
            }
            ActionButton {
                text: I18n.i18n("Повторить")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.actGithubRun("rerun", runId.text, false)
            }
            ActionButton {
                text: I18n.i18n("Повторить сбойные")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.actGithubRun("rerun", runId.text, true)
            }
            ActionButton {
                text: I18n.i18n("Удалить запуск")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.actGithubRun("delete", runId.text, false)
            }
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Секреты и переменные")
        meta: I18n.i18n("значение секрета уходит только во вход gh")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: secretName
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("имя")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: secretValue
                implicitWidth: 180
                echoMode: TextInput.Password
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("значение")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: secretEnv
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("окружение, если нужно")
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton { label: "actions"; active: root.secretApp === "actions"; onClicked: root.secretApp = "actions" }
            ChipButton { label: "dependabot"; active: root.secretApp === "dependabot"; onClicked: root.secretApp = "dependabot" }
            ChipButton { label: "codespaces"; active: root.secretApp === "codespaces"; onClicked: root.secretApp = "codespaces" }
            ActionButton {
                text: I18n.i18n("Записать секрет")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubSecret(secretName.text, secretValue.text, secretEnv.text, root.secretApp, false)
            }
            ActionButton {
                text: I18n.i18n("Удалить секрет")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubSecret(secretName.text, "", secretEnv.text, root.secretApp, true)
            }
            ActionButton {
                text: I18n.i18n("Записать переменную")
                kind: "primary"
                enabled: !App.busy
                onClicked: App.saveGithubVariable(secretName.text, secretValue.text, secretEnv.text, false)
            }
            ActionButton {
                text: I18n.i18n("Удалить переменную")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubVariable(secretName.text, "", secretEnv.text, true)
            }
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Окружения и кэш")
        meta: I18n.i18n("один кэш, не все сразу")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: envName
                implicitWidth: 160
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("окружение")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: envWait
                implicitWidth: 110
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("минуты")
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton {
                label: I18n.i18n("только защищённые ветки")
                active: root.protectedBranches
                onClicked: root.protectedBranches = !root.protectedBranches
            }
            Controls.TextField {
                id: envReviewers
                implicitWidth: 220
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "User:1,Team:2"
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Записать окружение")
                kind: "primary"
                enabled: !App.busy
                onClicked: App.saveGithubEnvironment(envName.text, envWait.text, root.protectedBranches, envReviewers.text, false)
            }
            ActionButton {
                text: I18n.i18n("Удалить окружение")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubEnvironment(envName.text, envWait.text, root.protectedBranches, "", true)
            }
            Controls.TextField {
                id: cacheKey
                implicitWidth: 180
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("id или ключ кэша")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Удалить кэш")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.deleteGithubCache(cacheKey.text)
            }
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Оповещения")
        meta: I18n.i18n("Dependabot, code scanning, secret scanning")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            ChipButton {
                label: "Dependabot"
                active: root.alertKind === "dependabot"
                onClicked: { root.alertKind = "dependabot"; root.alertState = "dismissed"; root.alertReason = "tolerable_risk" }
            }
            ChipButton {
                label: "code scanning"
                active: root.alertKind === "code"
                onClicked: { root.alertKind = "code"; root.alertState = "dismissed"; root.alertReason = "false positive" }
            }
            ChipButton {
                label: "secret scanning"
                active: root.alertKind === "secret"
                onClicked: { root.alertKind = "secret"; root.alertState = "resolved"; root.alertReason = "false_positive" }
            }
            Controls.TextField {
                id: alertNumber
                implicitWidth: 90
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("номер")
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton { label: "open"; active: root.alertState === "open"; onClicked: root.alertState = "open" }
            ChipButton {
                visible: root.alertKind !== "secret"
                label: "dismissed"
                active: root.alertState === "dismissed"
                onClicked: root.alertState = "dismissed"
            }
            ChipButton {
                visible: root.alertKind === "secret"
                label: "resolved"
                active: root.alertState === "resolved"
                onClicked: root.alertState = "resolved"
            }
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8
            visible: root.alertState !== "open"

            ChipButton { visible: root.alertKind === "dependabot"; label: "tolerable_risk"; active: root.alertReason === "tolerable_risk"; onClicked: root.alertReason = "tolerable_risk" }
            ChipButton { visible: root.alertKind === "dependabot"; label: "not_used"; active: root.alertReason === "not_used"; onClicked: root.alertReason = "not_used" }
            ChipButton { visible: root.alertKind === "dependabot"; label: "fix_started"; active: root.alertReason === "fix_started"; onClicked: root.alertReason = "fix_started" }
            ChipButton { visible: root.alertKind === "dependabot"; label: "inaccurate"; active: root.alertReason === "inaccurate"; onClicked: root.alertReason = "inaccurate" }
            ChipButton { visible: root.alertKind === "dependabot"; label: "no_bandwidth"; active: root.alertReason === "no_bandwidth"; onClicked: root.alertReason = "no_bandwidth" }
            ChipButton { visible: root.alertKind === "code"; label: "false positive"; active: root.alertReason === "false positive"; onClicked: root.alertReason = "false positive" }
            ChipButton { visible: root.alertKind === "code"; label: "won't fix"; active: root.alertReason === "won't fix"; onClicked: root.alertReason = "won't fix" }
            ChipButton { visible: root.alertKind === "code"; label: "used in tests"; active: root.alertReason === "used in tests"; onClicked: root.alertReason = "used in tests" }
            ChipButton { visible: root.alertKind === "code"; label: "mitigated"; active: root.alertReason === "mitigated"; onClicked: root.alertReason = "mitigated" }
            ChipButton { visible: root.alertKind === "secret"; label: "false_positive"; active: root.alertReason === "false_positive"; onClicked: root.alertReason = "false_positive" }
            ChipButton { visible: root.alertKind === "secret"; label: "wont_fix"; active: root.alertReason === "wont_fix"; onClicked: root.alertReason = "wont_fix" }
            ChipButton { visible: root.alertKind === "secret"; label: "revoked"; active: root.alertReason === "revoked"; onClicked: root.alertReason = "revoked" }
            ChipButton { visible: root.alertKind === "secret"; label: "used_in_tests"; active: root.alertReason === "used_in_tests"; onClicked: root.alertReason = "used_in_tests" }
        }

        ActionButton {
            text: I18n.i18n("Записать статус")
            kind: "primary"
            enabled: !App.busy
            onClicked: App.saveGithubAlert(root.alertKind, alertNumber.text, root.alertState, root.alertReason)
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: "Ruleset"
        meta: I18n.i18n("удаление ветки, запрет force и линейная история")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: rulesetId
                implicitWidth: 90
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("номер")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: rulesetName
                implicitWidth: 160
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("имя")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: rulesetRef
                implicitWidth: 180
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "refs/heads/main"
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton { label: "active"; active: root.rulesetMode === "active"; onClicked: root.rulesetMode = "active" }
            ChipButton { label: "evaluate"; active: root.rulesetMode === "evaluate"; onClicked: root.rulesetMode = "evaluate" }
            ChipButton { label: "disabled"; active: root.rulesetMode === "disabled"; onClicked: root.rulesetMode = "disabled" }
            ChipButton {
                label: "deletion"
                active: root.hasCsv(root.rulesetRules, "deletion")
                onClicked: root.rulesetRules = root.toggleCsv(root.rulesetRules, "deletion")
            }
            ChipButton {
                label: "non_fast_forward"
                active: root.hasCsv(root.rulesetRules, "non_fast_forward")
                onClicked: root.rulesetRules = root.toggleCsv(root.rulesetRules, "non_fast_forward")
            }
            ChipButton {
                label: "required_linear_history"
                active: root.hasCsv(root.rulesetRules, "required_linear_history")
                onClicked: root.rulesetRules = root.toggleCsv(root.rulesetRules, "required_linear_history")
            }
            ActionButton {
                text: I18n.i18n("Записать ruleset")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubRuleset(rulesetId.text, rulesetName.text, rulesetRef.text, root.rulesetMode, root.rulesetRules)
            }
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Защита ветки")
        meta: I18n.i18n("классические правила, запись заменяет набор")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: protectBranch
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "main"
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton { label: I18n.i18n("без ревью"); active: root.reviews === 0; onClicked: root.reviews = 0 }
            ChipButton { label: I18n.i18n("1 ревью"); active: root.reviews === 1; onClicked: root.reviews = 1 }
            ChipButton { label: I18n.i18n("2 ревью"); active: root.reviews === 2; onClicked: root.reviews = 2 }
            ChipButton {
                label: I18n.i18n("админы тоже")
                active: root.enforceAdmins
                onClicked: root.enforceAdmins = !root.enforceAdmins
            }
            ChipButton {
                label: I18n.i18n("запретить force")
                active: root.blockForce
                onClicked: root.blockForce = !root.blockForce
            }
            ChipButton {
                label: I18n.i18n("запретить удаление")
                active: root.blockDelete
                onClicked: root.blockDelete = !root.blockDelete
            }
            ActionButton {
                text: I18n.i18n("Записать защиту")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubBranchProtection(protectBranch.text, root.reviews, root.enforceAdmins, root.blockForce, root.blockDelete, false)
            }
            ActionButton {
                text: I18n.i18n("Снять защиту")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubBranchProtection(protectBranch.text, root.reviews, root.enforceAdmins, root.blockForce, root.blockDelete, true)
            }
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Deploy key и вебхук")
        meta: I18n.i18n("только открытый ключ")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: keyTitle
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("название ключа")
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton {
                label: I18n.i18n("только чтение")
                active: root.keyReadOnly
                onClicked: root.keyReadOnly = !root.keyReadOnly
            }
            ActionButton {
                text: I18n.i18n("Добавить ключ")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubDeployKey(keyTitle.text, keyBody.text, root.keyReadOnly)
            }
            Controls.TextField {
                id: keyId
                implicitWidth: 90
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("номер ключа")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Удалить ключ")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.deleteGithubDeployKey(keyId.text)
            }
        }

        Controls.TextArea {
            id: keyBody
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            wrapMode: TextEdit.Wrap
            color: Theme.paper
            font.family: Theme.monoFamily
            font.pixelSize: Theme.typeMeta
            selectByMouse: true
            placeholderText: "ssh-ed25519 AAAA…"
            background: InkBox {}
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: hookId
                implicitWidth: 90
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("номер")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: hookUrl
                implicitWidth: 240
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "https://example.com/hook"
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: hookSecret
                implicitWidth: 140
                echoMode: TextInput.Password
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("секрет")
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton {
                label: "push"
                active: root.hasCsv(root.webhookEvents, "push")
                onClicked: root.webhookEvents = root.toggleCsv(root.webhookEvents, "push")
            }
            ChipButton {
                label: "pull_request"
                active: root.hasCsv(root.webhookEvents, "pull_request")
                onClicked: root.webhookEvents = root.toggleCsv(root.webhookEvents, "pull_request")
            }
            ChipButton {
                label: "issues"
                active: root.hasCsv(root.webhookEvents, "issues")
                onClicked: root.webhookEvents = root.toggleCsv(root.webhookEvents, "issues")
            }
            ChipButton {
                label: "release"
                active: root.hasCsv(root.webhookEvents, "release")
                onClicked: root.webhookEvents = root.toggleCsv(root.webhookEvents, "release")
            }
            ChipButton {
                label: I18n.i18n("активен")
                active: root.webhookActive
                onClicked: root.webhookActive = !root.webhookActive
            }
            ActionButton {
                text: I18n.i18n("Записать вебхук")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubWebhook(hookId.text, hookUrl.text, root.webhookEvents, hookSecret.text, root.webhookActive, false)
            }
            ActionButton {
                text: I18n.i18n("Удалить вебхук")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubWebhook(hookId.text, hookUrl.text, root.webhookEvents, "", root.webhookActive, true)
            }
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Советы и комментарии")
        meta: I18n.i18n("черновик совета, без публикации")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: advisoryId
                implicitWidth: 200
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "GHSA-xxxx-xxxx-xxxx"
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: advisorySummary
                implicitWidth: 200
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("краткое описание")
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton { label: "low"; active: root.severity === "low"; onClicked: root.severity = "low" }
            ChipButton { label: "medium"; active: root.severity === "medium"; onClicked: root.severity = "medium" }
            ChipButton { label: "high"; active: root.severity === "high"; onClicked: root.severity = "high" }
            ChipButton { label: "critical"; active: root.severity === "critical"; onClicked: root.severity = "critical" }
        }

        Controls.TextArea {
            id: advisoryBody
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            wrapMode: TextEdit.Wrap
            color: Theme.paper
            font.family: Theme.uiFamily
            font.pixelSize: Theme.typeMeta
            selectByMouse: true
            placeholderText: I18n.i18n("Текст совета.")
            background: InkBox {}
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            ChipButton { label: "npm"; active: root.ecosystem === "npm"; onClicked: root.ecosystem = "npm" }
            ChipButton { label: "pip"; active: root.ecosystem === "pip"; onClicked: root.ecosystem = "pip" }
            ChipButton { label: "go"; active: root.ecosystem === "go"; onClicked: root.ecosystem = "go" }
            ChipButton { label: "actions"; active: root.ecosystem === "actions"; onClicked: root.ecosystem = "actions" }
            Controls.TextField {
                id: advisoryPackage
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("пакет")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: advisoryRange
                implicitWidth: 120
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "< 1.0.0"
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Черновик совета")
                kind: "danger"
                enabled: !App.busy && advisoryId.text.trim().length === 0
                onClicked: App.saveGithubAdvisory("", advisorySummary.text, advisoryBody.text, root.severity, root.ecosystem, advisoryPackage.text, advisoryRange.text)
            }
            ActionButton {
                text: I18n.i18n("Править совет")
                kind: "ghost"
                enabled: !App.busy && advisoryId.text.trim().length > 0
                onClicked: App.saveGithubAdvisory(advisoryId.text, advisorySummary.text, advisoryBody.text, root.severity, root.ecosystem, advisoryPackage.text, advisoryRange.text)
            }
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            ChipButton {
                label: I18n.i18n("только соавторы")
                active: root.interaction === "collaborators_only"
                onClicked: root.interaction = "collaborators_only"
            }
            ChipButton {
                label: I18n.i18n("только участники")
                active: root.interaction === "contributors_only"
                onClicked: root.interaction = "contributors_only"
            }
            ChipButton {
                label: I18n.i18n("существующие пользователи")
                active: root.interaction === "existing_users"
                onClicked: root.interaction = "existing_users"
            }
            ActionButton {
                text: I18n.i18n("Ограничить комментарии")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.saveGithubInteraction(root.interaction, false)
            }
            ActionButton {
                text: I18n.i18n("Снять ограничение")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.saveGithubInteraction(root.interaction, true)
            }
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: "Codespaces"
        meta: I18n.i18n("создание и удаление после превью")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: codespaceBranch
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("ветка")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: codespaceTitle
                implicitWidth: 180
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("отображаемое имя")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Создать codespace")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.createGithubCodespace(codespaceBranch.text, codespaceTitle.text)
            }
            Controls.TextField {
                id: codespaceName
                implicitWidth: 200
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("имя существующего")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Переименовать")
                kind: "ghost"
                enabled: !App.busy
                onClicked: App.editGithubCodespace(codespaceName.text, codespaceTitle.text)
            }
            ActionButton {
                text: I18n.i18n("Удалить codespace")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.deleteGithubCodespace(codespaceName.text)
            }
        }
    }
}
