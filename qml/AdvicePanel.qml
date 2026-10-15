pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

Item {
    id: root

    signal commandChosen(string commandId)
    signal commandRun(string commandId)

    property string expandedWhyId: ""

    function whyText(commandId) {
        const cmd = App.catalog.commandById(commandId)
        if (!cmd || !cmd.explanation)
            return I18n.i18n("Пояснение для этой команды пока не задано.")
        return cmd.explanation
    }

    ScrollPane {
        anchors.fill: parent

        SoftBar {
            Layout.fillWidth: true
            spacing: Theme.space8

            Text {
                text: I18n.i18n("Рекомендации")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeKicker
                font.letterSpacing: 1.2
                font.capitalization: Font.AllUppercase
                opacity: 0.65
                height: Theme.controlHSm
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                visible: App.recommendations.count > 0
                text: String(App.recommendations.count)
                color: Theme.copperSoft
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeMeta
                height: Theme.controlHSm
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                text: App.llm.streaming
                      ? I18n.i18n("стрим… %1", App.llm.streamBuffer.length)
                      : (App.llm.reachable ? I18n.i18n("LLM ok") : I18n.i18n("offline"))
                color: App.llm.streaming ? Theme.copperSoft
                       : (App.llm.reachable ? Theme.safe : Theme.mist)
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
                height: Theme.controlHSm
                verticalAlignment: Text.AlignVCenter
            }

            Controls.Button {
                id: probeBtn
                text: I18n.i18n("Probe")
                enabled: !App.llm.busy
                onClicked: App.probeLlm()
                background: Rectangle {
                    implicitHeight: Theme.controlHSm
                    implicitWidth: 52
                    radius: Theme.radiusSm
                    color: probeBtn.hovered ? Theme.inkPanel : "transparent"
                    border.color: Theme.inkLine
                    border.width: 1
                }
                contentItem: Text {
                    text: probeBtn.text
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: Theme.typeMeta
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Controls.Button {
                id: enrichBtn
                text: App.advisor.enriching ? "…" : I18n.i18n("LLM+")
                enabled: App.repo.isOpen && !App.advisor.enriching && !App.llm.busy
                onClicked: App.enrichRecommendations()
                background: Rectangle {
                    implicitHeight: Theme.controlHSm
                    implicitWidth: 48
                    radius: Theme.radiusSm
                    color: enrichBtn.enabled && enrichBtn.hovered ? Theme.inkPanel : "transparent"
                    border.color: Theme.inkLine
                    border.width: 1
                }
                contentItem: Text {
                    text: enrichBtn.text
                    color: enrichBtn.enabled ? Theme.copperSoft : Theme.inkLine
                    font.family: Theme.uiFamily
                    font.pixelSize: Theme.typeMeta
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            visible: !App.llm.reachable
            radius: Theme.radiusSm
            color: Theme.inkElevated
            border.color: Theme.inkLine
            border.width: 1
            implicitHeight: Theme.controlH
            Text {
                anchors.fill: parent
                anchors.margins: Theme.space8
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                text: I18n.i18n("Offline · правила работают · LLM: %1", App.llm.baseUrl)
                color: Theme.mutates
                font.family: Theme.uiFamily
                font.pixelSize: Theme.typeMeta
            }
        }

        Text {
            visible: App.recommendations.count === 0
            text: I18n.i18n("Откройте репозиторий — появятся рекомендации.")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: Theme.bodyPx
            opacity: 0.7
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.space4
            visible: App.recommendations.count > 0

            Repeater {
                model: App.recommendations
                delegate: Rectangle {
                    id: rec
                    required property string commandId
                    required property string title
                    required property string reason
                    required property string source
                    Layout.fillWidth: true
                    implicitHeight: root.expandedWhyId === rec.commandId
                                    ? whyCol.implicitHeight + Theme.space12
                                    : 40
                    radius: Theme.radiusSm
                    color: Theme.inkElevated
                    border.color: Theme.inkLine
                    border.width: 1
                    clip: true

                    ColumnLayout {
                        id: whyCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: Theme.space8
                        spacing: Theme.space4

                        SoftBar {
                            Layout.fillWidth: true
                            spacing: Theme.space6

                            Text {
                                text: rec.title
                                color: Theme.paper
                                font.family: Theme.uiFamily
                                font.pixelSize: Theme.bodyPx
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                                width: Math.min(implicitWidth, 220)
                            }

                            Text {
                                text: rec.source === "llm" ? "LLM" : "R"
                                color: rec.source === "llm" ? Theme.copper : Theme.safe
                                font.family: Theme.monoFamily
                                font.pixelSize: Theme.typeKicker
                            }

                            Text {
                                text: I18n.i18n("почему")
                                color: Theme.mist
                                font.family: Theme.uiFamily
                                font.pixelSize: Theme.typeMeta
                                font.underline: true
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        root.expandedWhyId = root.expandedWhyId === rec.commandId
                                                ? ""
                                                : rec.commandId
                                    }
                                }
                            }

                            Text {
                                text: I18n.i18n("выполнить")
                                color: Theme.copperSoft
                                font.family: Theme.uiFamily
                                font.pixelSize: Theme.typeMeta
                                font.weight: Font.DemiBold
                                font.underline: true
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.commandRun(rec.commandId)
                                }
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: root.expandedWhyId !== rec.commandId
                            text: rec.reason
                            elide: Text.ElideRight
                            color: Theme.mist
                            font.family: Theme.uiFamily
                            font.pixelSize: Theme.typeMeta
                            opacity: 0.75
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: root.expandedWhyId === rec.commandId
                            wrapMode: Text.WordWrap
                            textFormat: Text.MarkdownText
                            color: Theme.paper
                            font.family: Theme.uiFamily
                            font.pixelSize: Theme.bodyPx
                            lineHeight: 1.3
                            text: rec.reason + "\n\n" + root.whyText(rec.commandId)
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.inkLine
        }

        MentorDesk {
            Layout.fillWidth: true
        }
    }
}
