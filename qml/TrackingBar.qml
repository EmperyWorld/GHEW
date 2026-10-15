pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

// Ahead / behind against upstream. Click prepares push or pull.
ColumnLayout {
    id: root

    spacing: Theme.space6

    readonly property int span: Math.max(1, App.repo.ahead + App.repo.behind)
    readonly property bool linked: App.repo.isOpen && App.repo.hasUpstream

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.space8

        Text {
            text: root.linked
                  ? (App.repo.upstream || I18n.i18n("upstream"))
                  : I18n.i18n("Нет upstream")
            color: Theme.mist
            font.family: Theme.monoFamily
            font.pixelSize: Theme.typeMeta
            elide: Text.ElideMiddle
            Layout.fillWidth: true
        }

        Text {
            visible: root.linked
            text: "↑" + App.repo.ahead + "  ↓" + App.repo.behind
            color: Theme.paper
            font.family: Theme.monoFamily
            font.pixelSize: Theme.typeMeta
            font.weight: Font.DemiBold
        }
    }

    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: 10
        visible: root.linked

        Rectangle {
            anchors.fill: parent
            radius: 4
            color: Theme.inkPanel
            border.color: Theme.inkLine
            border.width: 1
        }

        Row {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 1

            Rectangle {
                width: parent.width * App.repo.behind / root.span
                height: parent.height
                visible: App.repo.behind > 0
                color: Theme.mutates
                radius: App.repo.ahead > 0 ? 0 : 3
            }
            Rectangle {
                width: parent.width * App.repo.ahead / root.span
                height: parent.height
                visible: App.repo.ahead > 0
                color: Theme.safe
                radius: App.repo.behind > 0 ? 0 : 3
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.space6
        visible: App.repo.isOpen

        ChipButton {
            visible: root.linked && App.repo.behind > 0
            label: I18n.i18n("Pull ↓%1", App.repo.behind)
            accent: Theme.mutates
            tip: I18n.i18n("Подтянуть коммиты upstream")
            onClicked: App.prepareCommand("pull", {})
        }
        ChipButton {
            visible: root.linked && App.repo.ahead > 0 && App.repo.behind === 0
            label: I18n.i18n("Push ↑%1", App.repo.ahead)
            accent: Theme.safe
            labelColor: Theme.safe
            tip: I18n.i18n("Отправить локальные коммиты")
            onClicked: App.prepareCommand("push", {})
        }
        ChipButton {
            visible: !root.linked && App.repo.remotes.count > 0 && App.repo.branch.length > 0
            label: I18n.i18n("Привязать upstream")
            accent: Theme.copper
            tip: I18n.i18n("push -u на первый remote")
            onClicked: App.prepareCommand("push", {
                setUpstream: true,
                remote: App.repo.remotes.nameAt(0),
                branch: App.repo.branch
            })
        }
        ChipButton {
            visible: App.repo.remotes.count > 0
            label: I18n.i18n("Fetch")
            tip: I18n.i18n("Обновить сведения о remote, не сливая")
            onClicked: App.prepareCommand("fetch", {})
        }

        Item { Layout.fillWidth: true }
    }

    Text {
        Layout.fillWidth: true
        visible: root.linked && App.repo.ahead === 0 && App.repo.behind === 0
        text: I18n.i18n("Ветка совпадает с upstream.")
        color: Theme.mist
        font.family: Theme.uiFamily
        font.pixelSize: 11
        opacity: 0.75
    }
}
