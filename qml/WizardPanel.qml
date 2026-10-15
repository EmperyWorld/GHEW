pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

Item {
    id: root

    property int flowIndex: 0 // 0 first-commit, 1 sync, 2 bisect
    property int step: 0

    readonly property var firstCommitSteps: [
        {
            title: I18n.i18n("Проверить статус"),
            body: I18n.i18n("Убедитесь, что видны нужные изменения. Затем добавьте файлы в индекс."),
            commandId: "status",
            args: {}
        },
        {
            title: I18n.i18n("Добавить в индекс"),
            body: I18n.i18n("Добавьте все или выбранные файлы (add / add_path)."),
            commandId: "add_all",
            args: {}
        },
        {
            title: I18n.i18n("Создать коммит"),
            body: I18n.i18n("Напишите осмысленное сообщение (лучше conventional: feat/fix/docs)."),
            commandId: "commit",
            args: { message: "feat: " }
        }
    ]

    readonly property var syncSteps: [
        {
            title: I18n.i18n("Fetch"),
            body: I18n.i18n("Сначала обновите сведения о remote без слияния (при желании — fetch --prune)."),
            commandId: "fetch",
            args: {}
        },
        {
            title: I18n.i18n("Сверить историю"),
            body: I18n.i18n("Посмотрите ahead/behind и лог перед pull/push. При diverged — осторожно."),
            commandId: "log_graph",
            args: {}
        },
        {
            title: App.repo.behind > 0 ? I18n.i18n("Pull --rebase") : I18n.i18n("Push"),
            body: App.repo.behind > 0
                  ? I18n.i18n("Подтяните upstream через pull --rebase (или обычный pull). Затем push при необходимости.")
                  : I18n.i18n("Если впереди и дерево чистое — push. Без upstream — push -u."),
            commandId: App.repo.behind > 0 ? "pull_rebase" : "push",
            args: (!App.repo.hasUpstream && App.repo.remotes.count > 0 && App.repo.behind === 0)
                  ? { setUpstream: true, remote: App.repo.remotes.nameAt(0), branch: App.repo.branch }
                  : {}
        }
    ]

    readonly property var bisectSteps: [
        {
            title: I18n.i18n("Начать bisect"),
            body: I18n.i18n("Запускает бинарный поиск регрессии. Рабочее дерево лучше держать чистым."),
            commandId: "bisect_start",
            args: {}
        },
        {
            title: I18n.i18n("Пометить bad"),
            body: I18n.i18n("Текущий коммит содержит баг — `bisect bad`."),
            commandId: "bisect_bad",
            args: {}
        },
        {
            title: I18n.i18n("Пометить good"),
            body: I18n.i18n("Известный хороший коммит — `bisect good` (часто после checkout старого)."),
            commandId: "bisect_good",
            args: {}
        },
        {
            title: I18n.i18n("Завершить"),
            body: I18n.i18n("Когда виновник найден — `bisect reset`, чтобы вернуться на исходную ветку."),
            commandId: "bisect_reset",
            args: {}
        }
    ]

    readonly property var steps: root.flowIndex === 0 ? root.firstCommitSteps
                                   : (root.flowIndex === 1 ? root.syncSteps : root.bisectSteps)
    readonly property var current: root.steps[Math.min(root.step, root.steps.length - 1)]

    Connections {
        target: App.repo
        function onChanged() {
            if (App.repo.bisectActive && root.flowIndex !== 2) {
                root.flowIndex = 2
                root.step = 1
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Text {
                text: I18n.i18n("Сценарии")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 10
                font.letterSpacing: 1.2
                font.capitalization: Font.AllUppercase
                opacity: 0.65
            }
            Item { Layout.fillWidth: true }
            Controls.Button {
                id: lessonOpenBtn
                text: I18n.i18n("Открыть урок")
                visible: App.lessonRepoPath.length > 0
                onClicked: App.openLessonRepository()
                background: Rectangle {
                    implicitHeight: 24
                    implicitWidth: 100
                    radius: Theme.radiusSm
                    color: lessonOpenBtn.hovered ? Theme.inkPanel : "transparent"
                    border.color: Theme.inkLine
                    border.width: 1
                }
                contentItem: Text {
                    text: lessonOpenBtn.text
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            Controls.Button {
                id: lessonBtn
                text: I18n.i18n("Новый урок")
                onClicked: App.createLessonRepository()
                background: Rectangle {
                    implicitHeight: 24
                    implicitWidth: 96
                    radius: Theme.radiusSm
                    color: lessonBtn.hovered ? Theme.inkPanel : "transparent"
                    border.color: Theme.copper
                    border.width: 1
                }
                contentItem: Text {
                    text: lessonBtn.text
                    color: Theme.copperSoft
                    font.family: Theme.uiFamily
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Text {
            visible: App.lessonRepoPath.length > 0
            Layout.fillWidth: true
            text: App.lessonRepoPath
            color: Theme.mist
            font.family: Theme.monoFamily
            font.pixelSize: 11
            elide: Text.ElideMiddle
            opacity: 0.7
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            Repeater {
                model: [
                    { label: I18n.i18n("Первый коммит"), tip: "" },
                    { label: I18n.i18n("Синхронизация"), tip: "" },
                    { label: I18n.i18n("Bisect"), tip: App.repo.bisectActive ? "!" : "" }
                ]
                delegate: Rectangle {
                    id: flowSeg
                    required property int index
                    required property var modelData
                    Layout.fillWidth: true
                    radius: Theme.radiusSm
                    color: root.flowIndex === flowSeg.index ? Theme.inkPanel : "transparent"
                    border.color: root.flowIndex === flowSeg.index ? Theme.copper : Theme.inkLine
                    border.width: 1
                    implicitHeight: 28
                    Text {
                        anchors.centerIn: parent
                        text: flowSeg.modelData.label
                              + (flowSeg.modelData.tip.length ? (" " + flowSeg.modelData.tip) : "")
                        color: (flowSeg.index === 2 && App.repo.bisectActive)
                               ? Theme.destructive
                               : (root.flowIndex === flowSeg.index ? Theme.paper : Theme.mist)
                        font.family: Theme.uiFamily
                        font.pixelSize: 11
                        font.weight: root.flowIndex === flowSeg.index ? Font.DemiBold : Font.Normal
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.flowIndex = flowSeg.index
                            root.step = (flowSeg.index === 2 && App.repo.bisectActive) ? 1 : 0
                        }
                    }
                }
            }
        }

        // Шаги сценария — компактная полоса
        Row {
            Layout.fillWidth: true
            spacing: 4
            Repeater {
                model: root.steps.length
                delegate: Rectangle {
                    id: stepDot
                    required property int index
                    width: Math.max(18, (root.width - 40) / Math.max(root.steps.length, 1) - 4)
                    height: 4
                    radius: 2
                    color: stepDot.index <= root.step ? Theme.copper : Theme.inkLine
                }
            }
        }

        Text {
            text: I18n.i18n("Шаг %1 / %2", root.step + 1, root.steps.length)
            color: Theme.mist
            font.family: Theme.monoFamily
            font.pixelSize: 11
            opacity: 0.7
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Theme.radius
            color: Theme.inkElevated
            border.color: Theme.inkLine
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 8

                Text {
                    Layout.fillWidth: true
                    text: root.current.title
                    color: Theme.paper
                    font.family: Theme.displayFamily
                    font.pixelSize: 20
                    font.weight: Font.Medium
                    wrapMode: Text.WordWrap
                }
                Text {
                    Layout.fillWidth: true
                    text: root.current.body
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                    lineHeight: 1.35
                }
                Text {
                    Layout.fillWidth: true
                    text: root.current.commandId
                    color: Theme.copperSoft
                    font.family: Theme.monoFamily
                    font.pixelSize: 12
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Controls.Button {
                        id: backBtn
                        text: I18n.i18n("Назад")
                        enabled: root.step > 0
                        onClicked: root.step--
                        background: Rectangle {
                            implicitHeight: 30
                            implicitWidth: 72
                            radius: Theme.radiusSm
                            color: backBtn.enabled && backBtn.hovered ? Theme.inkPanel : "transparent"
                            border.color: Theme.inkLine
                            border.width: 1
                        }
                        contentItem: Text {
                            text: backBtn.text
                            color: backBtn.enabled ? Theme.mist : Theme.inkLine
                            font.family: Theme.uiFamily
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Controls.Button {
                        id: openBtn
                        text: I18n.i18n("Параметры")
                        enabled: App.repo.isOpen
                        onClicked: App.prepareCommand(root.current.commandId, root.current.args || {})
                        background: Rectangle {
                            implicitHeight: 30
                            implicitWidth: 96
                            radius: Theme.radiusSm
                            color: !openBtn.enabled ? Theme.inkPanel
                                   : (openBtn.hovered ? Theme.inkPanel : "transparent")
                            border.color: Theme.inkLine
                            border.width: 1
                            opacity: openBtn.enabled ? 1 : 0.45
                        }
                        contentItem: Text {
                            text: openBtn.text
                            color: openBtn.enabled ? Theme.mist : Theme.inkLine
                            font.family: Theme.uiFamily
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Controls.Button {
                        id: runBtn
                        text: I18n.i18n("Выполнить шаг")
                        enabled: App.repo.isOpen && !App.busy
                        onClicked: {
                            App.runCommand(root.current.commandId, root.current.args || {}, false)
                            if (root.step + 1 < root.steps.length)
                                root.step++
                        }
                        background: Rectangle {
                            implicitHeight: 30
                            implicitWidth: 120
                            radius: Theme.radiusSm
                            color: !runBtn.enabled ? Theme.inkPanel
                                   : (runBtn.hovered ? Theme.copperSoft : Theme.copper)
                            opacity: runBtn.enabled ? 1 : 0.45
                        }
                        contentItem: Text {
                            text: runBtn.text
                            color: runBtn.enabled ? Theme.ink : Theme.mist
                            font.family: Theme.uiFamily
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Controls.Button {
                        id: nextBtn
                        text: root.step + 1 < root.steps.length ? I18n.i18n("Далее") : I18n.i18n("Готово")
                        onClicked: {
                            if (root.step + 1 < root.steps.length)
                                root.step++
                            else
                                root.step = 0
                        }
                        background: Rectangle {
                            implicitHeight: 30
                            implicitWidth: 72
                            radius: Theme.radiusSm
                            color: nextBtn.hovered ? Theme.inkPanel : "transparent"
                            border.color: Theme.copper
                            border.width: 1
                        }
                        contentItem: Text {
                            text: nextBtn.text
                            color: Theme.copper
                            font.family: Theme.uiFamily
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Item { Layout.fillWidth: true }
                }
            }
        }
    }
}
