pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

// Local-looking branch list: current row, ahead/behind, switch on click.
ColumnLayout {
    id: root

    signal openCommand(string commandId, var args)

    visible: App.repo.isOpen
    spacing: Theme.space6

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.space8

        SectionHead {
            Layout.fillWidth: true
            title: I18n.i18n("Ветки")
            countText: String(App.repo.branches.count)
        }

        ChipButton {
            label: I18n.i18n("Диалог")
            tip: I18n.i18n("Создать или переключить ветку")
            onClicked: App.openBranchDialog()
        }
    }

    EmptyHint {
        Layout.fillWidth: true
        visible: App.repo.branches.count === 0
        title: I18n.i18n("Веток нет")
        body: I18n.i18n("После первого коммита здесь появится текущая ветка и её соседи.")
    }

    ListView {
        id: lane
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(220, Math.max(36, Math.min(App.repo.branches.count, 6) * 36))
        visible: App.repo.branches.count > 0
        clip: true
        spacing: 4
        model: App.repo.branches
        boundsBehavior: Flickable.StopAtBounds

        delegate: Rectangle {
            id: row
            required property string name
            required property bool current
            required property bool remote
            required property string upstream
            required property int ahead
            required property int behind
            width: lane.width
            height: 32
            radius: Theme.radiusSm
            color: area.containsMouse ? Theme.inkPanel : (row.current ? "#1a2218" : "transparent")
            border.color: row.current ? Theme.safe : Theme.inkLine
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.space8
                anchors.rightMargin: Theme.space8
                spacing: Theme.space6

                Rectangle {
                    Layout.preferredWidth: 6
                    Layout.preferredHeight: 6
                    radius: 3
                    color: row.current ? Theme.safe : (row.remote ? Theme.mist : Theme.copper)
                }

                Text {
                    Layout.fillWidth: true
                    text: row.name
                    color: Theme.paper
                    font.family: Theme.monoFamily
                    font.pixelSize: 12
                    elide: Text.ElideMiddle
                }

                Text {
                    visible: row.ahead > 0 || row.behind > 0
                    text: "↑" + row.ahead + " ↓" + row.behind
                    color: Theme.mist
                    font.family: Theme.monoFamily
                    font.pixelSize: 10
                }

                Text {
                    visible: row.remote
                    text: "remote"
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: 10
                    opacity: 0.6
                }

                Text {
                    visible: !row.current && !row.remote
                    text: I18n.i18n("switch")
                    color: Theme.copperSoft
                    font.family: Theme.uiFamily
                    font.pixelSize: 11
                }
            }

            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: row.current || row.remote ? Qt.ArrowCursor : Qt.PointingHandCursor
                Controls.ToolTip.visible: containsMouse && row.upstream.length > 0
                Controls.ToolTip.text: row.upstream
                onClicked: {
                    if (!row.current && !row.remote)
                        root.openCommand("switch", { name: row.name })
                }
            }
        }
    }
}
