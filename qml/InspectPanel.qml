pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

Item {
    id: root
    property string searchText: ""

    readonly property var lines: {
        const text = App.repo.inspectText || ""
        if (!text.length)
            return []
        const all = text.split("\n")
        if (!root.searchText.length)
            return all
        const needle = root.searchText.toLowerCase()
        return all.filter(function (line) { return line.toLowerCase().indexOf(needle) >= 0 })
    }

    function runCommitAction(item) {
        if (item.tag) {
            App.openTagDialog()
        } else if (item.id) {
            if (item.args && item.args.rev)
                App.prepareCommand(item.id, { rev: App.repo.inspectTarget })
            else
                App.prepareCommand(item.id, { commit: App.repo.inspectTarget })
        } else {
            App.setCompareEndpoint(App.repo.inspectTarget, item.asA)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        InspectChrome {
            Layout.fillWidth: true
            searchText: root.searchText
            matchCount: root.lines.length
            onSearchEdited: function (text) { root.searchText = text }
            onCopyRequested: App.copyText(App.repo.inspectText)
            onCommitAction: function (item) { root.runCommitAction(item) }
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: App.repo.inspectText.length === 0
            visible: App.repo.inspectText.length === 0
            clip: true

            OverviewDeck {
                width: root.width
                onOpenCommand: function (commandId, args) {
                    App.prepareCommand(commandId, args || {})
                }
            }
        }

        EmptyHint {
            Layout.fillWidth: true
            visible: !App.repo.isOpen
            title: I18n.i18n("Репозиторий не открыт")
            body: I18n.i18n("Откройте репозиторий — здесь появятся ветки, ссылки и подробный вывод коммитов.")
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: App.repo.inspectText.length > 0
            radius: Theme.radius
            color: "#0a0d12"
            border.color: Theme.inkLine
            border.width: 1

            Controls.ScrollView {
                anchors.fill: parent
                anchors.margins: 2
                clip: true
                Controls.TextArea {
                    readOnly: true
                    wrapMode: TextEdit.NoWrap
                    font.family: Theme.monoFamily
                    font.pixelSize: 12
                    color: Theme.paper
                    text: root.lines.join("\n")
                    background: Item {}
                    padding: 12
                }
            }
        }
    }
}
