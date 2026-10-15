pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

// Profile and organization editors. Secrets stay out of the command line.
ColumnLayout {
    id: root

    spacing: Theme.space12

    property bool hireable: false
    property string teamPrivacy: "closed"
    property string teamRole: "member"
    property string inviteRole: "direct_member"
    property string repoPermission: "read"
    property bool publicRepos: true
    property bool privateRepos: true
    property bool internalRepos: false
    property bool pages: true
    property bool publicPages: true
    property bool privatePages: true
    property bool forkPrivate: false
    property string baseRole: "read"
    property string rolePerms: "add_label"
    property string propertyType: "string"
    property string hookEvents: "push"
    property bool hookActive: true
    property bool bannerDismiss: true
    property string dependabot: "not_set"
    property string secretScanning: "not_set"
    property string pushProtection: "not_set"
    property string immutablePolicy: "none"

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
        title: I18n.i18n("Профиль и организация")
        meta: I18n.i18n("логин организации не меняется")
        accent: Theme.copper
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Профиль")
        meta: I18n.i18n("пустые поля уже записанное не затирают")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: profileName
                implicitWidth: 160
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("имя")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: profileBlog
                implicitWidth: 180
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "https://"
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: profileCompany
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("компания")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: profileLocation
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("город")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: profileTwitter
                implicitWidth: 120
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "twitter"
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton { label: "hireable"; active: root.hireable; onClicked: root.hireable = !root.hireable }
        }

        Controls.TextField {
            id: profileBio
            Layout.fillWidth: true
            color: Theme.paper
            font.family: Theme.monoFamily
            font.pixelSize: Theme.typeMeta
            placeholderText: I18n.i18n("bio, до 160 символов")
            background: InkBox { implicitHeight: 28 }
        }

        ActionButton {
            text: I18n.i18n("Записать профиль")
            kind: "primary"
            enabled: !App.busy
            onClicked: App.saveGithubProfile(profileName.text, profileBio.text, profileBlog.text, profileCompany.text,
                                             profileLocation.text, profileTwitter.text, root.hireable)
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Почта и соцаккаунты")
        meta: I18n.i18n("видимость основной почты")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: mailField
                implicitWidth: 220
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "name@example.com"
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Добавить")
                kind: "primary"
                enabled: !App.busy
                onClicked: App.saveGithubEmail(mailField.text, "add")
            }
            ActionButton {
                text: I18n.i18n("Удалить")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubEmail(mailField.text, "delete")
            }
            ActionButton {
                text: I18n.i18n("Показать основную")
                kind: "primary"
                enabled: !App.busy
                onClicked: App.saveGithubEmail("", "public")
            }
            ActionButton {
                text: I18n.i18n("Скрыть основную")
                kind: "primary"
                enabled: !App.busy
                onClicked: App.saveGithubEmail("", "private")
            }
            ActionButton {
                text: I18n.i18n("Показать список")
                kind: "primary"
                enabled: !App.busy
                onClicked: App.github.refreshEmails()
            }
        }

        Text {
            Layout.fillWidth: true
            visible: App.github.emails.length > 0
            wrapMode: Text.WordWrap
            color: Theme.mist
            font.family: Theme.monoFamily
            font.pixelSize: Theme.typeMeta
            text: App.github.emails.join("\n")
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: socialUrl
                implicitWidth: 280
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "https://www.linkedin.com/in/name"
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Добавить ссылку")
                kind: "primary"
                enabled: !App.busy
                onClicked: App.saveGithubSocial(socialUrl.text, false)
            }
            ActionButton {
                text: I18n.i18n("Снять ссылку")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubSocial(socialUrl.text, true)
            }
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Ключи")
        meta: I18n.i18n("только открытый ключ, после превью")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: keyTitle
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("название")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: keyId
                implicitWidth: 80
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("номер")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: "SSH"
                kind: "primary"
                enabled: !App.busy
                onClicked: App.saveGithubUserKey("ssh", keyTitle.text, keyBody.text, keyId.text, false)
            }
            ActionButton {
                text: "SSH signing"
                kind: "primary"
                enabled: !App.busy
                onClicked: App.saveGithubUserKey("signing", keyTitle.text, keyBody.text, keyId.text, false)
            }
            ActionButton {
                text: "GPG"
                kind: "primary"
                enabled: !App.busy
                onClicked: App.saveGithubUserKey("gpg", keyTitle.text, keyBody.text, keyId.text, false)
            }
            ActionButton {
                text: I18n.i18n("Удалить SSH")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubUserKey("ssh", "", "", keyId.text, true)
            }
            ActionButton {
                text: I18n.i18n("Удалить signing")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubUserKey("signing", "", "", keyId.text, true)
            }
            ActionButton {
                text: I18n.i18n("Удалить GPG")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubUserKey("gpg", "", "", keyId.text, true)
            }
        }

        Controls.TextArea {
            id: keyBody
            Layout.fillWidth: true
            Layout.preferredHeight: 72
            wrapMode: TextEdit.Wrap
            color: Theme.paper
            font.family: Theme.monoFamily
            font.pixelSize: Theme.typeMeta
            selectByMouse: true
            placeholderText: I18n.i18n("открытый ssh в одну строку или блок PGP PUBLIC KEY")
            background: InkBox {}
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("README профиля")
        meta: I18n.i18n("репозиторий с именем логина, не этот открытый")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: profileLogin
                implicitWidth: 160
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("логин")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: readmeSha
                implicitWidth: 280
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("SHA, если README уже есть")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Создать репозиторий")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.createGithubProfileRepo(profileLogin.text)
            }
        }

        Controls.TextArea {
            id: readmeBody
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            wrapMode: TextEdit.Wrap
            color: Theme.paper
            font.family: Theme.monoFamily
            font.pixelSize: Theme.typeMeta
            selectByMouse: true
            placeholderText: I18n.i18n("Текст README.md. Запись делает коммит в репозитории логин/логин.")
            background: InkBox {}
        }

        ActionButton {
            text: I18n.i18n("Записать README")
            kind: "danger"
            enabled: !App.busy
            onClicked: App.saveGithubProfileReadme(profileLogin.text, readmeBody.text, readmeSha.text)
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Карточка организации")
        meta: I18n.i18n("отображаемое имя, не логин")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: orgLogin
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("логин")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: orgName
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("имя")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: orgBlog
                implicitWidth: 160
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "https://"
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: orgMail
                implicitWidth: 180
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("публичная почта")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: orgTwitter
                implicitWidth: 110
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "twitter"
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: orgLocation
                implicitWidth: 120
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("город")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: orgCompany
                implicitWidth: 120
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("компания")
                background: InkBox { implicitHeight: 28 }
            }
        }

        Controls.TextField {
            id: orgDescription
            Layout.fillWidth: true
            color: Theme.paper
            font.family: Theme.monoFamily
            font.pixelSize: Theme.typeMeta
            placeholderText: I18n.i18n("описание, до 160 символов")
            background: InkBox { implicitHeight: 28 }
        }

        ActionButton {
            text: I18n.i18n("Записать карточку")
            kind: "primary"
            enabled: !App.busy
            onClicked: App.saveGithubOrgCard(orgLogin.text, orgName.text, orgDescription.text, orgBlog.text, orgMail.text,
                                             orgTwitter.text, orgLocation.text, orgCompany.text)
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Кто создаёт репозитории")
        meta: I18n.i18n("internal только у Enterprise, запись после превью")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: policyOrg
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("логин")
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton { label: "read"; active: root.repoPermission === "read"; onClicked: root.repoPermission = "read" }
            ChipButton { label: "write"; active: root.repoPermission === "write"; onClicked: root.repoPermission = "write" }
            ChipButton { label: "admin"; active: root.repoPermission === "admin"; onClicked: root.repoPermission = "admin" }
            ChipButton { label: "none"; active: root.repoPermission === "none"; onClicked: root.repoPermission = "none" }
            ChipButton { label: "public"; active: root.publicRepos; onClicked: root.publicRepos = !root.publicRepos }
            ChipButton { label: "private"; active: root.privateRepos; onClicked: root.privateRepos = !root.privateRepos }
            ChipButton { label: "internal"; active: root.internalRepos; onClicked: root.internalRepos = !root.internalRepos }
            ChipButton { label: "Pages"; active: root.pages; onClicked: root.pages = !root.pages }
            ChipButton { label: "public Pages"; active: root.publicPages; onClicked: root.publicPages = !root.publicPages }
            ChipButton { label: "private Pages"; active: root.privatePages; onClicked: root.privatePages = !root.privatePages }
            ChipButton { label: I18n.i18n("форк приватных"); active: root.forkPrivate; onClicked: root.forkPrivate = !root.forkPrivate }
        }

        ActionButton {
            text: I18n.i18n("Записать политику")
            kind: "danger"
            enabled: !App.busy
            onClicked: App.saveGithubOrgPolicy(policyOrg.text, root.repoPermission, root.publicRepos, root.privateRepos,
                                              root.internalRepos, root.pages, root.publicPages, root.privatePages, root.forkPrivate)
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Команды и люди")
        meta: I18n.i18n("удаление и приглашения после превью")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: teamOrg
                implicitWidth: 120
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("логин")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: teamName
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("имя команды")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: teamSlugField
                implicitWidth: 120
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "slug"
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: teamAbout
                implicitWidth: 160
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("описание")
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton { label: "closed"; active: root.teamPrivacy === "closed"; onClicked: root.teamPrivacy = "closed" }
            ChipButton { label: "secret"; active: root.teamPrivacy === "secret"; onClicked: root.teamPrivacy = "secret" }
            ActionButton {
                text: I18n.i18n("Создать команду")
                kind: "primary"
                enabled: !App.busy
                onClicked: App.saveGithubOrgTeam(teamOrg.text, teamName.text, teamAbout.text, root.teamPrivacy, teamSlugField.text, false)
            }
            ActionButton {
                text: I18n.i18n("Удалить команду")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubOrgTeam(teamOrg.text, "", "", root.teamPrivacy, teamSlugField.text, true)
            }
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: memberLogin
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("логин человека")
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton { label: "member"; active: root.teamRole === "member"; onClicked: root.teamRole = "member" }
            ChipButton { label: "maintainer"; active: root.teamRole === "maintainer"; onClicked: root.teamRole = "maintainer" }
            ActionButton {
                text: I18n.i18n("В команду")
                kind: "primary"
                enabled: !App.busy
                onClicked: App.saveGithubOrgTeamMember(teamOrg.text, teamSlugField.text, memberLogin.text, root.teamRole, false)
            }
            ActionButton {
                text: I18n.i18n("Из команды")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubOrgTeamMember(teamOrg.text, teamSlugField.text, memberLogin.text, root.teamRole, true)
            }
            ActionButton {
                text: I18n.i18n("Исключить из организации")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubOrgPeople(teamOrg.text, memberLogin.text, "", "", "", "remove-member")
            }
            ActionButton {
                text: I18n.i18n("Снять внешнего")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubOrgPeople(teamOrg.text, memberLogin.text, "", "", "", "remove-outside")
            }
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: inviteMail
                implicitWidth: 200
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("почта приглашения")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: inviteId
                implicitWidth: 80
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("номер")
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton { label: "direct_member"; active: root.inviteRole === "direct_member"; onClicked: root.inviteRole = "direct_member" }
            ChipButton { label: "admin"; active: root.inviteRole === "admin"; onClicked: root.inviteRole = "admin" }
            ActionButton {
                text: I18n.i18n("Пригласить")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubOrgPeople(teamOrg.text, "", inviteMail.text, "", root.inviteRole, "invite")
            }
            ActionButton {
                text: I18n.i18n("Отозвать")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubOrgPeople(teamOrg.text, "", "", inviteId.text, "", "cancel")
            }
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Роли и свойства")
        meta: I18n.i18n("роль репозитория, не роль организации")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: roleOrg
                implicitWidth: 120
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("логин")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: roleName
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("имя роли")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: roleId
                implicitWidth: 80
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("номер")
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton { label: "read"; active: root.baseRole === "read"; onClicked: root.baseRole = "read" }
            ChipButton { label: "triage"; active: root.baseRole === "triage"; onClicked: root.baseRole = "triage" }
            ChipButton { label: "write"; active: root.baseRole === "write"; onClicked: root.baseRole = "write" }
            ChipButton { label: "maintain"; active: root.baseRole === "maintain"; onClicked: root.baseRole = "maintain" }
            ChipButton { label: "add_label"; active: root.hasCsv(root.rolePerms, "add_label"); onClicked: root.rolePerms = root.toggleCsv(root.rolePerms, "add_label") }
            ChipButton { label: "remove_label"; active: root.hasCsv(root.rolePerms, "remove_label"); onClicked: root.rolePerms = root.toggleCsv(root.rolePerms, "remove_label") }
            ChipButton { label: "add_assignee"; active: root.hasCsv(root.rolePerms, "add_assignee"); onClicked: root.rolePerms = root.toggleCsv(root.rolePerms, "add_assignee") }
            ChipButton { label: "close_issue"; active: root.hasCsv(root.rolePerms, "close_issue"); onClicked: root.rolePerms = root.toggleCsv(root.rolePerms, "close_issue") }
            ChipButton { label: "manage_wiki"; active: root.hasCsv(root.rolePerms, "manage_wiki"); onClicked: root.rolePerms = root.toggleCsv(root.rolePerms, "manage_wiki") }
            ChipButton { label: "close_pull_request"; active: root.hasCsv(root.rolePerms, "close_pull_request"); onClicked: root.rolePerms = root.toggleCsv(root.rolePerms, "close_pull_request") }
            ChipButton { label: "reopen_issue"; active: root.hasCsv(root.rolePerms, "reopen_issue"); onClicked: root.rolePerms = root.toggleCsv(root.rolePerms, "reopen_issue") }
            ChipButton { label: "reopen_pull_request"; active: root.hasCsv(root.rolePerms, "reopen_pull_request"); onClicked: root.rolePerms = root.toggleCsv(root.rolePerms, "reopen_pull_request") }
            ChipButton { label: "request_pr_review"; active: root.hasCsv(root.rolePerms, "request_pr_review"); onClicked: root.rolePerms = root.toggleCsv(root.rolePerms, "request_pr_review") }
            ChipButton { label: "set_milestone"; active: root.hasCsv(root.rolePerms, "set_milestone"); onClicked: root.rolePerms = root.toggleCsv(root.rolePerms, "set_milestone") }
            ActionButton {
                text: I18n.i18n("Записать роль")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubOrgRole(roleOrg.text, roleId.text, roleName.text, "", root.baseRole, root.rolePerms, false)
            }
            ActionButton {
                text: I18n.i18n("Удалить роль")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubOrgRole(roleOrg.text, roleId.text, "", "", root.baseRole, "", true)
            }
            Controls.TextField {
                id: orgRoleUser
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("логин человека")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Назначить роль организации")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.assignGithubOrgRole(roleOrg.text, orgRoleUser.text, roleId.text, false)
            }
            ActionButton {
                text: I18n.i18n("Снять роль организации")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.assignGithubOrgRole(roleOrg.text, orgRoleUser.text, roleId.text, true)
            }
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: propertyName
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("имя_свойства")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: propertyAllowed
                implicitWidth: 180
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("значения через запятую")
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton { label: "string"; active: root.propertyType === "string"; onClicked: root.propertyType = "string" }
            ChipButton { label: "true_false"; active: root.propertyType === "true_false"; onClicked: root.propertyType = "true_false" }
            ChipButton { label: "url"; active: root.propertyType === "url"; onClicked: root.propertyType = "url" }
            ChipButton { label: "single_select"; active: root.propertyType === "single_select"; onClicked: root.propertyType = "single_select" }
            ActionButton {
                text: I18n.i18n("Записать свойство")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubOrgProperty(roleOrg.text, propertyName.text, root.propertyType, "", propertyAllowed.text, false)
            }
            ActionButton {
                text: I18n.i18n("Удалить свойство")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubOrgProperty(roleOrg.text, propertyName.text, root.propertyType, "", "", true)
            }
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Вебхук и баннер")
        meta: I18n.i18n("секрет вебхука не попадает в журнал")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: hookOrg
                implicitWidth: 120
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("логин")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: hookUrl
                implicitWidth: 220
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
            Controls.TextField {
                id: hookId
                implicitWidth: 80
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("номер")
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton { label: "push"; active: root.hasCsv(root.hookEvents, "push"); onClicked: root.hookEvents = root.toggleCsv(root.hookEvents, "push") }
            ChipButton { label: "pull_request"; active: root.hasCsv(root.hookEvents, "pull_request"); onClicked: root.hookEvents = root.toggleCsv(root.hookEvents, "pull_request") }
            ChipButton { label: "issues"; active: root.hasCsv(root.hookEvents, "issues"); onClicked: root.hookEvents = root.toggleCsv(root.hookEvents, "issues") }
            ChipButton { label: "release"; active: root.hasCsv(root.hookEvents, "release"); onClicked: root.hookEvents = root.toggleCsv(root.hookEvents, "release") }
            ChipButton { label: "workflow_run"; active: root.hasCsv(root.hookEvents, "workflow_run"); onClicked: root.hookEvents = root.toggleCsv(root.hookEvents, "workflow_run") }
            ChipButton { label: I18n.i18n("активен"); active: root.hookActive; onClicked: root.hookActive = !root.hookActive }
            ActionButton {
                text: I18n.i18n("Записать вебхук")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubOrgHook(hookOrg.text, hookId.text, hookUrl.text, root.hookEvents, hookSecret.text, root.hookActive, false)
            }
            ActionButton {
                text: I18n.i18n("Удалить вебхук")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubOrgHook(hookOrg.text, hookId.text, "", "", "", false, true)
            }
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: bannerText
                implicitWidth: 220
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("текст баннера")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: bannerExpires
                implicitWidth: 180
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "YYYY-MM-DDTHH:MM:SSZ"
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton { label: I18n.i18n("можно закрыть"); active: root.bannerDismiss; onClicked: root.bannerDismiss = !root.bannerDismiss }
            ActionButton {
                text: I18n.i18n("Поставить баннер")
                kind: "primary"
                enabled: !App.busy
                onClicked: App.saveGithubOrgBanner(hookOrg.text, bannerText.text, bannerExpires.text, root.bannerDismiss, false)
            }
            ActionButton {
                text: I18n.i18n("Снять баннер")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubOrgBanner(hookOrg.text, "", "", false, true)
            }
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Безопасность организации")
        meta: I18n.i18n("конфигурация не назначается на репозитории")

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: securityOrg
                implicitWidth: 120
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("логин")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: configName
                implicitWidth: 160
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("имя конфигурации")
                background: InkBox { implicitHeight: 28 }
            }
            Controls.TextField {
                id: configId
                implicitWidth: 80
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("номер")
                background: InkBox { implicitHeight: 28 }
            }
            ChipButton { label: "Dependabot not_set"; active: root.dependabot === "not_set"; onClicked: root.dependabot = "not_set" }
            ChipButton { label: "Dependabot enabled"; active: root.dependabot === "enabled"; onClicked: root.dependabot = "enabled" }
            ChipButton { label: "Dependabot disabled"; active: root.dependabot === "disabled"; onClicked: root.dependabot = "disabled" }
            ChipButton { label: "secrets not_set"; active: root.secretScanning === "not_set"; onClicked: root.secretScanning = "not_set" }
            ChipButton { label: "secrets enabled"; active: root.secretScanning === "enabled"; onClicked: root.secretScanning = "enabled" }
            ChipButton { label: "push not_set"; active: root.pushProtection === "not_set"; onClicked: root.pushProtection = "not_set" }
            ChipButton { label: "push enabled"; active: root.pushProtection === "enabled"; onClicked: root.pushProtection = "enabled" }
            ActionButton {
                text: I18n.i18n("Создать конфигурацию")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubOrgSecurityConfig(securityOrg.text, configName.text, root.dependabot, root.secretScanning,
                                                          root.pushProtection, "", false)
            }
            ActionButton {
                text: I18n.i18n("Удалить конфигурацию")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubOrgSecurityConfig(securityOrg.text, "", "not_set", "not_set", "not_set", configId.text, true)
            }
            Controls.TextField {
                id: attachRepos
                implicitWidth: 160
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("id репозиториев")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Назначить на выбранные")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.attachGithubOrgSecurity(securityOrg.text, configId.text, attachRepos.text)
            }
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space8

            Controls.TextField {
                id: managerSlug
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: "team-slug"
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Назначить security manager")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubOrgSecurityManager(securityOrg.text, managerSlug.text, false)
            }
            ActionButton {
                text: I18n.i18n("Снять security manager")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubOrgSecurityManager(securityOrg.text, managerSlug.text, true)
            }
            ChipButton { label: "immutable none"; active: root.immutablePolicy === "none"; onClicked: root.immutablePolicy = "none" }
            ChipButton { label: "immutable all"; active: root.immutablePolicy === "all"; onClicked: root.immutablePolicy = "all" }
            ChipButton { label: "immutable selected"; active: root.immutablePolicy === "selected"; onClicked: root.immutablePolicy = "selected" }
            Controls.TextField {
                id: immutableRepos
                implicitWidth: 140
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                placeholderText: I18n.i18n("id для selected")
                background: InkBox { implicitHeight: 28 }
            }
            ActionButton {
                text: I18n.i18n("Записать immutable releases")
                kind: "danger"
                enabled: !App.busy
                onClicked: App.saveGithubOrgImmutable(securityOrg.text, root.immutablePolicy, immutableRepos.text)
            }
        }
    }
}
