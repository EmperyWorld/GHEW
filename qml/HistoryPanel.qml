pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: I18n.i18n("Сессия")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 10
                font.letterSpacing: 1.2
                font.capitalization: Font.AllUppercase
                opacity: 0.65
            }
            Text {
                text: String(App.history.count)
                color: Theme.copperSoft
                font.family: Theme.monoFamily
                font.pixelSize: 11
            }
            Item { Layout.fillWidth: true }

            Controls.Button {
                id: exportBtn
                text: I18n.i18n("MD")
                onClicked: {
                    const path = App.exportSessionReport()
                    App.copyText(path)
                }
                background: Rectangle {
                    implicitHeight: 24
                    implicitWidth: 40
                    radius: Theme.radiusSm
                    color: exportBtn.hovered ? Theme.inkPanel : "transparent"
                    border.color: Theme.inkLine
                    border.width: 1
                }
                contentItem: Text {
                    text: exportBtn.text
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Controls.Button {
                id: clearBtn
                text: I18n.i18n("Очистить")
                enabled: App.history.count > 0
                onClicked: App.history.clear()
                background: Rectangle {
                    implicitHeight: 24
                    implicitWidth: 72
                    radius: Theme.radiusSm
                    color: clearBtn.enabled && clearBtn.hovered ? Theme.inkPanel : "transparent"
                    border.color: Theme.inkLine
                    border.width: 1
                }
                contentItem: Text {
                    text: clearBtn.text
                    color: clearBtn.enabled ? Theme.mist : Theme.inkLine
                    font.family: Theme.uiFamily
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Text {
            visible: App.history.count === 0
            text: I18n.i18n("Пока нет выполненных команд в этой сессии.")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 12
            opacity: 0.7
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                model: App.history
                clip: true
                spacing: 3
                delegate: HistoryEntry {
                    width: ListView.view.width
                }
            }
        }

        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 10
            opacity: 0.5
            text: I18n.i18n("Аудит JSONL: %1", App.auditLogPath())
        }
    }
}
