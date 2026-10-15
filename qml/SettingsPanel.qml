pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

Item {
    id: root

    ScrollPane {
        anchors.fill: parent
        spacing: Theme.space8

        Text {
            text: I18n.i18n("LLM")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 10
            font.letterSpacing: 1.2
            font.capitalization: Font.AllUppercase
            opacity: 0.65
        }

        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 12
            opacity: 0.75
            text: I18n.i18n("OpenAI-совместимый API (llama.cpp). Только localhost / LAN.")
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 10
            rowSpacing: 6

            Text {
                text: I18n.i18n("URL")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 11
                Layout.alignment: Qt.AlignVCenter
            }
            Controls.TextField {
                id: urlField
                Layout.fillWidth: true
                text: App.llm.baseUrl
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: 12
                background: Rectangle {
                    implicitHeight: 28
                    radius: Theme.radiusSm
                    color: Theme.inkElevated
                    border.color: urlField.activeFocus ? Theme.copper : Theme.inkLine
                    border.width: 1
                }
            }

            Text {
                text: I18n.i18n("Модель")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 11
                Layout.alignment: Qt.AlignVCenter
            }
            Controls.TextField {
                id: modelField
                Layout.fillWidth: true
                text: App.llm.model
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: 12
                background: Rectangle {
                    implicitHeight: 28
                    radius: Theme.radiusSm
                    color: Theme.inkElevated
                    border.color: modelField.activeFocus ? Theme.copper : Theme.inkLine
                    border.width: 1
                }
            }

            Text {
                text: I18n.i18n("Ключ")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 11
                Layout.alignment: Qt.AlignVCenter
            }
            Controls.TextField {
                id: keyField
                Layout.fillWidth: true
                text: App.llm.apiKey
                echoMode: TextInput.Password
                placeholderText: I18n.i18n("опционально")
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: 12
                background: Rectangle {
                    implicitHeight: 28
                    radius: Theme.radiusSm
                    color: Theme.inkElevated
                    border.color: keyField.activeFocus ? Theme.copper : Theme.inkLine
                    border.width: 1
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Controls.Button {
                id: saveBtn
                text: I18n.i18n("Сохранить и проверить")
                onClicked: App.saveLlmSettings(urlField.text, modelField.text, keyField.text)
                background: Rectangle {
                    implicitHeight: 30
                    implicitWidth: 170
                    radius: Theme.radiusSm
                    color: saveBtn.hovered ? Theme.copperSoft : Theme.copper
                }
                contentItem: Text {
                    text: saveBtn.text
                    color: Theme.ink
                    font.family: Theme.uiFamily
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            Text {
                text: App.llm.statusMessage
                color: App.llm.reachable ? Theme.safe : Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 11
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.inkLine
        }

        Text {
            text: I18n.i18n("Рабочая область")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 10
            font.letterSpacing: 1.2
            font.capitalization: Font.AllUppercase
            opacity: 0.65
        }

        Controls.CheckBox {
            id: autoBox
            text: I18n.i18n("Автообновление статуса (8 с)")
            checked: App.autoRefresh
            onToggled: App.autoRefresh = checked
            contentItem: Text {
                text: autoBox.text
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: 12
                leftPadding: autoBox.indicator.width + 8
                verticalAlignment: Text.AlignVCenter
            }
        }

        Controls.CheckBox {
            id: watchBox
            text: I18n.i18n("Следить за ФС (.git / дерево)")
            checked: App.repo.watchEnabled
            onToggled: App.repo.watchEnabled = checked
            contentItem: Text {
                text: watchBox.text
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: 12
                leftPadding: watchBox.indicator.width + 8
                verticalAlignment: Text.AlignVCenter
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.inkLine
        }

        Text {
            text: I18n.i18n("Клавиши")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 10
            font.letterSpacing: 1.2
            font.capitalization: Font.AllUppercase
            opacity: 0.65
        }

        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Theme.mist
            font.family: Theme.monoFamily
            font.pixelSize: 11
            lineHeight: 1.45
            opacity: 0.9
            text: [
                "F1 — " + I18n.i18n("эта вкладка"),
                "F5 — " + I18n.i18n("обновить"),
                "Ctrl+O — " + I18n.i18n("открыть репо"),
                "Ctrl+K / / — " + I18n.i18n("поиск команд"),
                "Ctrl+P — " + I18n.i18n("палитра"),
                "Ctrl+B — " + I18n.i18n("ветки"),
                "Ctrl+T — " + I18n.i18n("тег"),
                "Ctrl+Enter — " + I18n.i18n("выполнить превью"),
                "Ctrl+Shift+T — " + I18n.i18n("терминал"),
                "j / k — " + I18n.i18n("файлы в статусе"),
                "Esc — " + I18n.i18n("закрыть диалог")
            ].join("\n")
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.inkLine
        }

        Text {
            text: I18n.i18n("Данные")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 10
            font.letterSpacing: 1.2
            font.capitalization: Font.AllUppercase
            opacity: 0.65
        }

        Controls.Button {
            id: clearRecentBtn
            text: I18n.i18n("Очистить недавние репозитории")
            enabled: App.recentRepos.count > 0
            onClicked: App.recentRepos.clear()
            background: Rectangle {
                implicitHeight: 28
                implicitWidth: 220
                radius: Theme.radiusSm
                color: "transparent"
                border.color: Theme.inkLine
                border.width: 1
            }
            contentItem: Text {
                text: clearRecentBtn.text
                color: clearRecentBtn.enabled ? Theme.mist : Theme.inkLine
                font.family: Theme.uiFamily
                font.pixelSize: 11
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

    }
}
