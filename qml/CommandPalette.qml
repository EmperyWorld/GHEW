pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

Item {
    id: root
    anchors.fill: parent
    visible: App.paletteOpen
    z: 1000

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.55
        MouseArea {
            anchors.fill: parent
            onClicked: App.closePalette()
        }
    }

    Rectangle {
        id: panel
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 72
        width: Math.min(parent.width - 48, 560)
        height: Math.min(parent.height - 120, 420)
        radius: Theme.radius
        color: Theme.inkElevated
        border.color: Theme.inkLine
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Text {
                text: I18n.i18n("Палитра · Ctrl+P")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 10
                font.letterSpacing: 1.2
                font.capitalization: Font.AllUppercase
                opacity: 0.65
            }

            Controls.TextField {
                id: query
                Layout.fillWidth: true
                placeholderText: I18n.i18n("Фильтр команд…")
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: 13
                focus: root.visible
                onVisibleChanged: if (visible) { forceActiveFocus(); selectAll() }
                onTextChanged: App.catalog.setSearchText(text)
                Keys.onEscapePressed: App.closePalette()
                Keys.onReturnPressed: {
                    if (App.catalog.count > 0) {
                        const row = App.catalog.commandAt(0)
                        App.prepareCommand(row.id, {})
                        App.closePalette()
                    }
                }
                background: Rectangle {
                    implicitHeight: 34
                    radius: Theme.radiusSm
                    color: Theme.inkPanel
                    border.color: query.activeFocus ? Theme.copper : Theme.inkLine
                    border.width: 1
                }
            }

            Text {
                visible: App.recentCommands.count > 0 && query.text.length === 0
                text: I18n.i18n("Недавние")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 10
                font.capitalization: Font.AllUppercase
                opacity: 0.7
            }

            Flow {
                Layout.fillWidth: true
                spacing: 6
                visible: App.recentCommands.count > 0 && query.text.length === 0

                Repeater {
                    model: App.recentCommands
                    delegate: Rectangle {
                        id: recentChip
                        required property string commandId
                        radius: Theme.radiusSm
                        border.color: Theme.inkLine
                        border.width: 1
                        color: recentArea.containsMouse ? Theme.inkPanel : "transparent"
                        implicitHeight: 26
                        implicitWidth: recentLabel.implicitWidth + 14
                        Text {
                            id: recentLabel
                            anchors.centerIn: parent
                            text: {
                                const c = App.catalog.commandById(recentChip.commandId)
                                return c && c.title ? c.title : recentChip.commandId
                            }
                            color: Theme.copperSoft
                            font.family: Theme.uiFamily
                            font.pixelSize: 11
                        }
                        MouseArea {
                            id: recentArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                App.prepareCommand(recentChip.commandId, {})
                                App.closePalette()
                            }
                        }
                    }
                }
            }

            Controls.ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                ListView {
                    model: App.catalog
                    spacing: 2
                    clip: true
                    delegate: Rectangle {
                        id: del
                        required property string commandId
                        required property string title
                        required property string synopsis
                        required property string danger
                        width: ListView.view.width
                        height: 40
                        radius: Theme.radiusSm
                        color: delArea.containsMouse ? Theme.inkPanel : "transparent"
                        border.color: delArea.containsMouse ? Theme.inkLine : "transparent"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 1
                            Text {
                                text: del.title
                                color: Theme.paper
                                font.family: Theme.uiFamily
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Text {
                                text: del.synopsis
                                color: Theme.mist
                                font.family: Theme.monoFamily
                                font.pixelSize: 10
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                                opacity: 0.7
                            }
                        }
                        MouseArea {
                            id: delArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                App.prepareCommand(del.commandId, {})
                                App.closePalette()
                            }
                        }
                    }
                }
            }
        }
    }

    onVisibleChanged: {
        if (!visible)
            App.catalog.setSearchText("")
    }
}
