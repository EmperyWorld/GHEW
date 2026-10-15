pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

Controls.ToolBar {
    id: root
    width: parent ? parent.width : implicitWidth
    padding: Theme.space6

    signal focusChanges(string stateFilter)

    background: Rectangle {
        color: Theme.inkElevated
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.inkLine
        }
    }

    contentItem: ColumnLayout {
        width: root.availableWidth
        spacing: Theme.space4

        SoftBar {
            Layout.fillWidth: true
            spacing: Theme.space6

            Text {
                text: "GHEW"
                color: Theme.copper
                font.family: Theme.displayFamily
                font.pixelSize: Theme.typeBody
                font.weight: Font.Medium
            }
            Text {
                text: "/"
                color: Theme.inkLine
                font.pixelSize: Theme.bodyPx
            }
            Text {
                text: {
                    const p = App.repo.rootPath
                    if (!p.length)
                        return "—"
                    const parts = p.split("/")
                    return parts[parts.length - 1] || p
                }
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: Theme.bodyPx
                font.weight: Font.DemiBold
                elide: Text.ElideMiddle
                width: Math.min(implicitWidth, 160)
                Controls.ToolTip.visible: pathHover.containsMouse && App.repo.rootPath.length
                Controls.ToolTip.text: App.repo.rootPath + "\n" + I18n.i18n("клик — папка · ПКМ — терминал")
                MouseArea {
                    id: pathHover
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    cursorShape: App.repo.isOpen ? Qt.PointingHandCursor : Qt.ArrowCursor
                    enabled: App.repo.isOpen
                    onClicked: function (mouse) {
                        if (mouse.button === Qt.RightButton)
                            App.openTerminalInRepo()
                        else
                            App.revealInFolder(".")
                    }
                }
            }
            Text {
                text: "/"
                color: Theme.inkLine
                font.pixelSize: Theme.bodyPx
            }
            Text {
                text: App.repo.branch || "—"
                color: Theme.copperSoft
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                font.underline: branchHover.containsMouse
                MouseArea {
                    id: branchHover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    enabled: App.repo.isOpen
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: function (mouse) {
                        if (mouse.button === Qt.RightButton && App.repo.branch.length)
                            App.copyText(App.repo.branch)
                        else
                            App.openBranchDialog()
                    }
                    Controls.ToolTip.visible: containsMouse
                    Controls.ToolTip.text: I18n.i18n("Ветка · Ctrl+B · ПКМ — копировать")
                }
            }
            Text {
                visible: App.repo.detachedHead || App.repo.bare || App.repo.insideSubmodule
                         || App.repo.bisectActive || App.repo.operationKind.length > 0
                text: {
                    const bits = []
                    if (App.repo.operationKind.length)
                        bits.push(App.repo.operationKind)
                    if (App.repo.bisectActive)
                        bits.push("bisect")
                    if (App.repo.bare)
                        bits.push("bare")
                    if (App.repo.detachedHead && !App.repo.bisectActive)
                        bits.push("detached")
                    if (App.repo.insideSubmodule)
                        bits.push("submodule")
                    return bits.join(" · ")
                }
                color: (App.repo.bare || App.repo.bisectActive || App.repo.conflictCount > 0)
                       ? Theme.destructive : Theme.mutates
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeKicker
            }
        }

        SoftBar {
            Layout.fillWidth: true
            spacing: Theme.space4

            StatPill {
                visible: App.repo.isOpen
                label: "S"
                valueText: String(App.repo.stagedCount)
                numericValue: App.repo.stagedCount
                accent: Theme.safe
                tip: I18n.i18n("В индексе · показать")
                onActivated: root.focusChanges("staged")
            }
            StatPill {
                visible: App.repo.isOpen
                label: "U"
                valueText: String(App.repo.unstagedCount)
                numericValue: App.repo.unstagedCount
                accent: Theme.mutates
                tip: I18n.i18n("Не в индексе · показать")
                onActivated: root.focusChanges("unstaged")
            }
            StatPill {
                visible: App.repo.isOpen
                label: "?"
                valueText: String(App.repo.untrackedCount)
                numericValue: App.repo.untrackedCount
                accent: Theme.copperSoft
                tip: I18n.i18n("Новые файлы · показать")
                onActivated: root.focusChanges("untracked")
            }
            StatPill {
                visible: App.repo.isOpen
                label: "!"
                valueText: String(App.repo.conflictCount)
                numericValue: App.repo.conflictCount
                accent: Theme.destructive
                tip: I18n.i18n("Конфликты · показать")
                onActivated: root.focusChanges("conflict")
            }

            Rectangle {
                visible: App.repo.isOpen && App.repo.hasUpstream
                radius: Theme.radiusSm
                border.color: Theme.inkLine
                border.width: 1
                color: upArea.containsMouse ? Theme.inkPanel : "transparent"
                implicitHeight: Theme.controlHSm
                implicitWidth: Math.min(upLab.implicitWidth + 10, 120)
                Text {
                    id: upLab
                    anchors.centerIn: parent
                    width: parent.width - 8
                    elide: Text.ElideMiddle
                    text: App.repo.upstream
                    color: Theme.copperSoft
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.typeKicker
                    horizontalAlignment: Text.AlignHCenter
                }
                MouseArea {
                    id: upArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: App.prepareCommand("fetch", {})
                    Controls.ToolTip.visible: containsMouse
                    Controls.ToolTip.text: I18n.i18n("Fetch")
                }
            }
            Rectangle {
                visible: App.repo.isOpen && App.repo.hasUpstream
                radius: Theme.radiusSm
                border.color: App.repo.ahead > 0 ? Theme.safe : Theme.inkLine
                border.width: 1
                color: aheadArea.containsMouse ? Theme.inkPanel : "transparent"
                implicitHeight: Theme.controlHSm
                implicitWidth: aheadLab.implicitWidth + 10
                opacity: App.repo.ahead > 0 ? 1 : 0.55
                Text {
                    id: aheadLab
                    anchors.centerIn: parent
                    text: "↑" + App.repo.ahead
                    color: App.repo.ahead > 0 ? Theme.safe : Theme.mist
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.typeMeta
                    font.weight: Font.DemiBold
                }
                MouseArea {
                    id: aheadArea
                    anchors.fill: parent
                    enabled: App.repo.ahead > 0
                    hoverEnabled: true
                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: App.prepareCommand("push", {})
                    Controls.ToolTip.visible: containsMouse
                    Controls.ToolTip.text: I18n.i18n("Push")
                }
            }
            Rectangle {
                visible: App.repo.isOpen && App.repo.hasUpstream
                radius: Theme.radiusSm
                border.color: App.repo.behind > 0 ? Theme.destructive : Theme.inkLine
                border.width: 1
                color: behindArea.containsMouse ? Theme.inkPanel : "transparent"
                implicitHeight: Theme.controlHSm
                implicitWidth: behindLab.implicitWidth + 10
                opacity: App.repo.behind > 0 ? 1 : 0.55
                Text {
                    id: behindLab
                    anchors.centerIn: parent
                    text: "↓" + App.repo.behind
                    color: App.repo.behind > 0 ? Theme.destructive : Theme.mist
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.typeMeta
                    font.weight: Font.DemiBold
                }
                MouseArea {
                    id: behindArea
                    anchors.fill: parent
                    enabled: App.repo.behind > 0
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: function (mouse) {
                        if (mouse.button === Qt.RightButton)
                            App.prepareCommand("pull_rebase", {})
                        else
                            App.prepareCommand("pull", {})
                    }
                    Controls.ToolTip.visible: containsMouse
                    Controls.ToolTip.text: I18n.i18n("Pull · ПКМ — pull --rebase")
                }
            }
            Text {
                visible: App.repo.isOpen && App.repo.ahead > 0 && App.repo.behind > 0
                text: I18n.i18n("diverged")
                color: Theme.destructive
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeKicker
                font.weight: Font.DemiBold
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: App.prepareCommand("pull_rebase", {})
                    Controls.ToolTip.visible: containsMouse
                    Controls.ToolTip.text: I18n.i18n("Истории разошлись — pull --rebase или merge, затем push")
                }
            }

            Text {
                visible: App.repo.isOpen && App.lastStatusDelta.length > 0
                text: App.lastStatusDelta
                color: Theme.copperSoft
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeKicker
                elide: Text.ElideRight
                width: Math.min(implicitWidth, 160)
                opacity: 0.85
            }

            Rectangle {
                visible: App.busy || App.llm.busy || App.llm.streaming || App.advisor.enriching || App.advisor.advising
                radius: Theme.radiusSm
                color: Theme.inkPanel
                border.color: Theme.copper
                border.width: 1
                implicitHeight: Theme.controlHSm
                implicitWidth: busyRow.implicitWidth + 10
                Row {
                    id: busyRow
                    anchors.centerIn: parent
                    spacing: Theme.space6
                    Text {
                        text: App.busy ? I18n.i18n("git…")
                              : (App.llm.streaming || App.advisor.advising ? I18n.i18n("SSE…")
                                 : (App.advisor.enriching ? I18n.i18n("LLM…") : I18n.i18n("сеть…")))
                        color: Theme.copperSoft
                        font.family: Theme.uiFamily
                        font.pixelSize: Theme.typeKicker
                    }
                    Text {
                        visible: App.cancellable || App.llm.streaming
                        text: I18n.i18n("отмена")
                        color: Theme.destructive
                        font.family: Theme.uiFamily
                        font.pixelSize: Theme.typeKicker
                        font.underline: true
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: App.cancelRunning()
                        }
                    }
                }
            }

            Row {
                spacing: 2
                Controls.ToolButton {
                    text: "−"
                    implicitWidth: 26
                    implicitHeight: Theme.controlHSm
                    enabled: Theme.uiScale > Theme.uiScaleMin + 0.001
                    onClicked: Theme.bumpUiScale(-Theme.uiScaleStep)
                    Controls.ToolTip.text: I18n.i18n("Уменьшить масштаб")
                    Controls.ToolTip.visible: hovered
                }
                Text {
                    text: Theme.uiScaleLabel
                    color: Theme.mist
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.typeKicker
                    width: 36
                    height: Theme.controlHSm
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Theme.resetUiScale()
                        Controls.ToolTip.visible: containsMouse
                        Controls.ToolTip.text: I18n.i18n("Сбросить масштаб на 100%")
                        hoverEnabled: true
                    }
                }
                Controls.ToolButton {
                    text: "+"
                    implicitWidth: 26
                    implicitHeight: Theme.controlHSm
                    enabled: Theme.uiScale < Theme.uiScaleMax - 0.001
                    onClicked: Theme.bumpUiScale(Theme.uiScaleStep)
                    Controls.ToolTip.text: I18n.i18n("Увеличить масштаб")
                    Controls.ToolTip.visible: hovered
                }
            }

            Controls.ToolButton {
                icon.name: "utilities-terminal"
                implicitWidth: 28
                implicitHeight: Theme.controlHSm
                enabled: App.repo.isOpen
                onClicked: App.openTerminalInRepo()
                Controls.ToolTip.text: I18n.i18n("Терминал в корне репо")
                Controls.ToolTip.visible: hovered
            }
            Controls.ToolButton {
                icon.name: "view-refresh"
                implicitWidth: 28
                implicitHeight: Theme.controlHSm
                onClicked: App.refreshRepo()
                Controls.ToolTip.text: I18n.i18n("Обновить (F5)")
                Controls.ToolTip.visible: hovered
            }
        }
    }
}
