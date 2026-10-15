pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

// Shortcut chips so the inspector chrome teaches the keyboard without a wall of text.
Row {
    id: root

    property var entries: [
        { keys: "Ctrl+K", hint: I18n.i18n("поиск") },
        { keys: "Ctrl+P", hint: I18n.i18n("палитра") },
        { keys: "Ctrl+Enter", hint: I18n.i18n("выполнить") },
        { keys: "F5", hint: I18n.i18n("обновить") }
    ]

    spacing: Theme.space6

    Repeater {
        model: root.entries
        delegate: Row {
            id: chip
            required property var modelData
            spacing: 4
            Rectangle {
                radius: 3
                color: Theme.inkPanel
                border.color: Theme.inkLine
                border.width: 1
                implicitHeight: 16
                implicitWidth: keyText.implicitWidth + 8
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    id: keyText
                    anchors.centerIn: parent
                    text: chip.modelData.keys
                    color: Theme.copperSoft
                    font.family: Theme.monoFamily
                    font.pixelSize: 9
                }
            }
            Text {
                text: chip.modelData.hint
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 10
                opacity: 0.7
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
