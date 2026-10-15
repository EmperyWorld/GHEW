pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

// Mentor dialogue: new question, free-text reply, ask_user chips, RUN buttons.
ColumnLayout {
    id: root

    spacing: Theme.space8

    readonly property bool canTalk: App.repo.isOpen && !App.advisor.advising
    readonly property bool inDialogue: App.advisor.hasAskUser || App.advisor.activeTask.length > 0
    readonly property string sendLabel: App.advisor.advising
                                        ? I18n.i18n("Думает…")
                                        : (root.inDialogue ? I18n.i18n("Ответить") : I18n.i18n("Спросить"))

    readonly property var prompts: [
        I18n.i18n("Что сделать перед push?"),
        I18n.i18n("Как разобрать конфликт?"),
        I18n.i18n("Можно ли amend?"),
        I18n.i18n("Что значит этот статус?")
    ]

    function sendMessage() {
        const text = questionField.text.trim()
        if (!text.length || !root.canTalk)
            return
        if (root.inDialogue)
            App.replyMentor(text)
        else
            App.askAdvice(text)
        questionField.text = ""
    }

    Text {
        Layout.fillWidth: true
        visible: App.advisor.activeTask.length > 0
        wrapMode: Text.WordWrap
        maximumLineCount: 2
        elide: Text.ElideRight
        text: I18n.i18n("Держу задачу: %1", App.advisor.activeTask)
        color: Theme.copperSoft
        font.family: Theme.uiFamily
        font.pixelSize: Theme.typeMeta
    }

    SectionHead {
        Layout.fillWidth: true
        title: I18n.i18n("Наставник")
        meta: App.llm.streaming
              ? I18n.i18n("стрим %1", App.llm.streamBuffer.length)
              : (App.llm.reachable ? I18n.i18n("на связи") : I18n.i18n("offline"))
        accent: App.llm.reachable ? Theme.safe : Theme.mutates
    }

    ChipFlow {
        spacing: Theme.space4

        Repeater {
            model: root.prompts
            delegate: ChipButton {
                required property string modelData
                label: modelData
                enabled: root.canTalk
                onClicked: {
                    if (root.inDialogue)
                        App.replyMentor(modelData)
                    else
                        App.askAdvice(modelData)
                    questionField.text = ""
                }
            }
        }
        ChipButton {
            label: I18n.i18n("Probe")
            enabled: !App.llm.busy
            tip: App.llm.baseUrl
            onClicked: App.probeLlm()
        }
        ChipButton {
            label: App.advisor.enriching ? "…" : "LLM+"
            enabled: App.repo.isOpen && !App.advisor.enriching && !App.llm.busy
            tip: I18n.i18n("Дополнить рекомендации моделью")
            onClicked: App.enrichRecommendations()
        }
        ChipButton {
            visible: root.inDialogue
            label: I18n.i18n("Новый вопрос")
            tip: I18n.i18n("Сбросить задачу и начать новый разговор")
            enabled: root.canTalk
            onClicked: {
                App.advisor.clearDialogue()
                questionField.forceActiveFocus()
            }
        }
    }

    Text {
        Layout.fillWidth: true
        visible: !App.llm.reachable
        wrapMode: Text.WordWrap
        text: I18n.i18n("Модель недоступна. Вопросы идут по правилам. Адрес: %1", App.llm.baseUrl)
        color: Theme.mutates
        font.family: Theme.uiFamily
        font.pixelSize: Theme.typeMeta
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.space6
        visible: App.advisor.hasAskUser && !App.advisor.advising

        Rectangle {
            Layout.fillWidth: true
            radius: Theme.radiusSm
            color: Theme.inkElevated
            border.color: Theme.copper
            border.width: 1
            implicitHeight: askCol.implicitHeight + Theme.space16

            ColumnLayout {
                id: askCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: Theme.space8
                spacing: Theme.space6

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: Theme.copperSoft
                    font.family: Theme.uiFamily
                    font.pixelSize: Theme.typeBody
                    font.weight: Font.DemiBold
                    text: App.advisor.askUserPrompt
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: Theme.typeMeta
                    opacity: 0.8
                    text: App.advisor.askUserOptions.length > 0
                          ? I18n.i18n("Выберите вариант или напишите свой ответ ниже.")
                          : I18n.i18n("Напишите свой ответ в поле ниже и нажмите «Ответить».")
                }

                ChipFlow {
                    visible: App.advisor.askUserOptions.length > 0
                    spacing: Theme.space6
                    Repeater {
                        model: App.advisor.askUserOptions
                        delegate: ActionButton {
                            required property string modelData
                            text: modelData
                            kind: "ghost"
                            enabled: root.canTalk
                            onClicked: App.answerAskUser(modelData)
                        }
                    }
                }
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.space6

        Controls.TextArea {
            id: questionField
            Layout.fillWidth: true
            Layout.preferredHeight: root.inDialogue ? 88 : 40
            wrapMode: TextEdit.Wrap
            color: Theme.paper
            font.family: Theme.uiFamily
            font.pixelSize: Theme.typeBody
            enabled: root.canTalk
            placeholderText: root.inDialogue
                             ? I18n.i18n("Ваш ответ наставнику…")
                             : I18n.i18n("Спросите про текущее дерево")
            background: Rectangle {
                radius: Theme.radiusSm
                color: Theme.inkElevated
                border.color: questionField.activeFocus ? Theme.copper : Theme.inkLine
                border.width: 1
            }
            Keys.onReturnPressed: function (event) {
                if (event.modifiers & Qt.ControlModifier) {
                    root.sendMessage()
                    event.accepted = true
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space8

            Text {
                Layout.fillWidth: true
                text: I18n.i18n("Ctrl+Enter — отправить")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeKicker
                opacity: 0.55
            }

            ActionButton {
                visible: root.inDialogue
                text: I18n.i18n("Новый вопрос")
                kind: "ghost"
                enabled: root.canTalk
                onClicked: {
                    App.advisor.clearDialogue()
                    questionField.forceActiveFocus()
                }
            }

            ActionButton {
                id: sendBtn
                text: root.sendLabel
                kind: "primary"
                enabled: root.canTalk && questionField.text.trim().length > 0
                onClicked: root.sendMessage()
            }
        }
    }

    Text {
        Layout.fillWidth: true
        visible: App.advisor.lastError.length > 0
                 && !(App.llm.reachable
                      && (App.advisor.lastError.indexOf("Offline") === 0
                          || App.advisor.lastError.indexOf("LLM недоступен") >= 0))
        wrapMode: Text.WordWrap
        color: Theme.destructive
        font.family: Theme.uiFamily
        font.pixelSize: Theme.typeMeta
        text: App.advisor.lastError
    }

    Text {
        Layout.fillWidth: true
        visible: App.advisor.advising && App.advisor.lastAdvice.length === 0 && App.llm.streamBuffer.length === 0
        text: I18n.i18n("Наставник думает…")
        color: Theme.mist
        font.family: Theme.uiFamily
        font.pixelSize: Theme.typeMeta
    }

    Text {
        Layout.fillWidth: true
        visible: App.advisor.lastAdvice.length > 0
        wrapMode: Text.WordWrap
        textFormat: Text.MarkdownText
        color: Theme.paper
        font.family: Theme.uiFamily
        font.pixelSize: Theme.typeBody
        lineHeight: 1.35
        text: App.advisor.lastAdvice
    }

    ChipFlow {
        spacing: Theme.space6
        visible: App.advisor.suggestedCommands.length > 0

        Repeater {
            model: App.advisor.suggestedCommands
            delegate: ActionButton {
                required property var modelData
                text: I18n.i18n("Выполнить: %1", modelData.title || modelData.commandId)
                kind: "primary"
                enabled: !App.advisor.advising && !App.busy
                onClicked: App.runCommand(modelData.commandId, modelData.args || {}, true)
            }
        }
    }

    Text {
        Layout.fillWidth: true
        visible: App.advisor.lastAdvice.length === 0 && App.advisor.lastError.length === 0 && !App.advisor.advising
        wrapMode: Text.WordWrap
        color: Theme.mist
        font.family: Theme.uiFamily
        font.pixelSize: Theme.typeMeta
        opacity: 0.65
        text: I18n.i18n("Ответ появится здесь. На вопрос наставника отвечайте своим текстом в поле выше.")
    }
}
