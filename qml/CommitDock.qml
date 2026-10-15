pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

// Message, conventional prefix, and preview — only while the index has something to commit.
SurfaceCard {
    id: root

    signal previewed()

    visible: App.repo.isOpen && App.repo.stagedCount > 0
    kicker: I18n.i18n("Коммит")
    meta: I18n.i18n("%1 в индексе", App.repo.stagedCount)
    goldEdge: messageField.length > 0
    pad: Theme.space8

    readonly property var prefixes: ["feat", "fix", "docs", "refactor", "test", "chore"]

    function currentMessage() {
        return messageField.text.trim()
    }

    function applyPrefix(prefix) {
        const cur = messageField.text
        const stripped = cur.replace(/^(feat|fix|docs|refactor|test|chore):\s*/i, "")
        messageField.text = prefix + ": " + stripped
        messageField.forceActiveFocus()
    }

    function suggest() {
        const line = App.suggestCommitMessage()
        if (line && line.length)
            messageField.text = line
    }

    function preview() {
        const message = root.currentMessage()
        if (!message.length)
            return
        App.prepareCommand("commit", { message: message })
        App.previewCommand("commit", { message: message })
        root.previewed()
    }

    Controls.TextArea {
        id: messageField
        Layout.fillWidth: true
        Layout.preferredHeight: 64
        placeholderText: I18n.i18n("Что изменилось и зачем")
        wrapMode: TextEdit.Wrap
        color: Theme.paper
        font.family: Theme.uiFamily
        font.pixelSize: 13
        selectByMouse: true
        background: Rectangle {
            radius: Theme.radiusSm
            color: Theme.ink
            border.color: messageField.activeFocus ? Theme.copper : Theme.inkLine
            border.width: 1
        }
    }

    ChipFlow {
        Layout.fillWidth: true
        spacing: Theme.space4

        Repeater {
            model: root.prefixes
            delegate: ChipButton {
                required property string modelData
                label: modelData
                tip: I18n.i18n("Префикс сообщения")
                onClicked: root.applyPrefix(modelData)
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.space8

        ActionButton {
            text: I18n.i18n("Из файлов")
            kind: "ghost"
            enabled: !App.busy
            onClicked: root.suggest()
        }
        ActionButton {
            text: I18n.i18n("Превью коммита")
            kind: "primary"
            enabled: !App.busy && root.currentMessage().length > 0
            onClicked: root.preview()
        }
        Text {
            Layout.fillWidth: true
            text: root.currentMessage().length === 0
                  ? I18n.i18n("Пустое сообщение не уйдёт в git.")
                  : I18n.i18n("Сначала превью, потом выполнение во вкладке Превью.")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 11
            wrapMode: Text.WordWrap
        }
    }
}
