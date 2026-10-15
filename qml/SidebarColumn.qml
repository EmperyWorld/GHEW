pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

// Left zone: catalog / next steps above, working tree below — vertical split.
Item {
    id: root
    clip: true

    signal commandPicked(string commandId)

    property alias commandList: commandList
    property alias statusPanel: statusPanel

    Rectangle {
        anchors.fill: parent
        color: Theme.inkElevated
    }
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        z: 2
        color: Theme.inkLine
    }

    Controls.SplitView {
        anchors.fill: parent
        orientation: Qt.Vertical
        clip: true

        ColumnLayout {
            Controls.SplitView.fillHeight: true
            Controls.SplitView.minimumHeight: 120
            Controls.SplitView.preferredHeight: parent.height * 0.55
            spacing: 0
            clip: true
            width: root.width

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.controlH
                Layout.leftMargin: Theme.space12
                Layout.rightMargin: Theme.space12
                Layout.topMargin: Theme.space8

                SectionHead {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    title: I18n.i18n("Каталог")
                    countText: String(App.commandCount)
                    meta: I18n.i18n("Ctrl+K")
                }
            }

            RecentCommandStrip {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.space12
                Layout.rightMargin: Theme.space12
                Layout.bottomMargin: Theme.space4
                onCommandChosen: function (commandId) {
                    if (commandList.selectById(commandId))
                        root.commandPicked(commandId)
                }
            }

            CommandList {
                id: commandList
                Layout.fillWidth: true
                Layout.fillHeight: true
                onSelectedIdChanged: root.commandPicked(selectedId)
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                Layout.leftMargin: Theme.space8
                Layout.rightMargin: Theme.space8
                color: Theme.inkLine
                visible: nextSteps.visible
            }

            NextSteps {
                id: nextSteps
                Layout.fillWidth: true
                onCommandChosen: function (commandId) {
                    App.runCommand(commandId, {}, true)
                }
            }
        }

        ColumnLayout {
            Controls.SplitView.minimumHeight: 100
            Controls.SplitView.preferredHeight: parent.height * 0.45
            spacing: 0
            clip: true

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Theme.inkLine
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.controlH
                Layout.leftMargin: Theme.space12
                Layout.rightMargin: Theme.space8

                SectionHead {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    title: I18n.i18n("Дерево")
                    countText: App.repo.isOpen ? String(App.repo.dirtyCount) : ""
                    meta: App.repo.branch
                    accent: App.repo.conflictCount > 0 ? Theme.destructive : Theme.copper
                }
            }

            RepoStatusPanel {
                id: statusPanel
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}
