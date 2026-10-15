pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

// Repo map shown in the inspect tab until a commit, stash, or blame takes the pane.
ColumnLayout {
    id: root

    signal openCommand(string commandId, var args)

    spacing: Theme.space12
    visible: App.repo.isOpen && App.repo.inspectText.length === 0

    EmptyHint {
        Layout.fillWidth: true
        title: I18n.i18n("Обзор репозитория")
        body: I18n.i18n("Ссылки, ветки и сравнение коммитов. Клик по коммиту, stash или Blame заменит этот обзор подробным выводом.")
    }

    RefInventory {
        Layout.fillWidth: true
        onOpenCommand: function (commandId) { root.openCommand(commandId, {}) }
    }

    BranchLane {
        Layout.fillWidth: true
        onOpenCommand: function (commandId, args) { root.openCommand(commandId, args || {}) }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Сравнение")
        meta: App.repo.compareA.length && App.repo.compareB.length ? "A…B" : ""
        visible: App.repo.commits.count > 0

        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 12
            text: I18n.i18n("Откройте коммит в истории дерева, затем назначьте его концом A или B. Пара ниже сравнивается как диапазон.")
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space8
            Text {
                text: "A  " + (App.repo.compareA.length ? App.repo.compareA : "—")
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: 12
            }
            Text {
                text: "B  " + (App.repo.compareB.length ? App.repo.compareB : "—")
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: 12
                Layout.fillWidth: true
                elide: Text.ElideMiddle
            }
        }

        ActionButton {
            visible: App.repo.compareA.length > 0 && App.repo.compareB.length > 0
            text: I18n.i18n("Показать диапазон")
            kind: "primary"
            onClicked: App.compareCommits(App.repo.compareA, App.repo.compareB)
        }
    }

    SurfaceCard {
        Layout.fillWidth: true
        kicker: I18n.i18n("Последние коммиты")
        meta: String(App.repo.commits.count)
        visible: App.repo.commits.count > 0

        ListView {
            id: log
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(160, Math.max(28, Math.min(App.repo.commits.count, 4) * 36))
            clip: true
            interactive: App.repo.commits.count > 4
            model: App.repo.commits
            spacing: 4

            delegate: Rectangle {
                id: commit
                required property int index
                required property string hash
                required property string shortHash
                required property string subject
                required property string relativeTime
                width: log.width
                height: commit.index < 4 ? 32 : 0
                visible: commit.index < 4
                radius: Theme.radiusSm
                color: area.containsMouse ? Theme.inkPanel : "transparent"
                border.color: Theme.inkLine
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.space8
                    anchors.rightMargin: Theme.space8
                    spacing: Theme.space8
                    Text {
                        text: commit.shortHash
                        color: Theme.copperSoft
                        font.family: Theme.monoFamily
                        font.pixelSize: 11
                    }
                    Text {
                        Layout.fillWidth: true
                        text: commit.subject
                        color: Theme.paper
                        font.family: Theme.uiFamily
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                    Text {
                        text: commit.relativeTime
                        color: Theme.mist
                        font.family: Theme.uiFamily
                        font.pixelSize: 10
                        opacity: 0.7
                    }
                }

                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: App.loadCommitShow(commit.hash)
                }
            }
        }
    }
}
