pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

Item {
    id: root
    visible: App.tagDialogOpen
    z: 90

    Rectangle {
        anchors.fill: parent
        color: "#CC0A0E12"
        opacity: root.visible ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 120 } }
        MouseArea {
            anchors.fill: parent
            onClicked: App.closeTagDialog()
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 400)
        height: col.implicitHeight + 28
        radius: Theme.radius
        color: Theme.inkElevated
        border.color: Theme.inkLine
        border.width: 1
        scale: root.visible ? 1 : 0.96
        opacity: root.visible ? 1 : 0
        Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
        Behavior on opacity { NumberAnimation { duration: 120 } }

        ColumnLayout {
            id: col
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 14
            spacing: 8

            Text {
                text: I18n.i18n("Тег")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 10
                font.letterSpacing: 1.2
                font.capitalization: Font.AllUppercase
                opacity: 0.65
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Text {
                    text: I18n.i18n("цель")
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: 10
                    font.capitalization: Font.AllUppercase
                    opacity: 0.5
                }
                Text {
                    text: App.repo.inspectKind === "commit" && App.repo.inspectTarget.length
                          ? App.repo.inspectTarget.substring(0, 7)
                          : ("HEAD · " + (App.repo.branch || "—"))
                    color: Theme.copperSoft
                    font.family: Theme.monoFamily
                    font.pixelSize: 12
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 8
                rowSpacing: 6

                Text {
                    text: I18n.i18n("Имя")
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: 11
                    Layout.alignment: Qt.AlignVCenter
                }
                Controls.TextField {
                    id: nameField
                    Layout.fillWidth: true
                    placeholderText: "v1.0.0"
                    color: Theme.paper
                    font.family: Theme.monoFamily
                    font.pixelSize: 13
                    background: Rectangle {
                        implicitHeight: 30
                        radius: Theme.radiusSm
                        color: Theme.inkPanel
                        border.color: nameField.activeFocus ? Theme.copper : Theme.inkLine
                        border.width: 1
                    }
                    Keys.onEscapePressed: App.closeTagDialog()
                    Keys.onReturnPressed: createBtn.clicked()
                }

                Text {
                    text: I18n.i18n("Сообщ.")
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: 11
                    Layout.alignment: Qt.AlignVCenter
                }
                Controls.TextField {
                    id: msgField
                    Layout.fillWidth: true
                    text: "release"
                    placeholderText: I18n.i18n("Сообщение")
                    color: Theme.paper
                    font.family: Theme.uiFamily
                    font.pixelSize: 12
                    background: Rectangle {
                        implicitHeight: 30
                        radius: Theme.radiusSm
                        color: Theme.inkPanel
                        border.color: msgField.activeFocus ? Theme.copper : Theme.inkLine
                        border.width: 1
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Controls.Button {
                    id: createBtn
                    text: I18n.i18n("Создать тег")
                    Layout.fillWidth: true
                    enabled: nameField.text.trim().length > 0 && App.repo.isOpen && !App.busy
                    onClicked: {
                        const args = {
                            name: nameField.text.trim(),
                            message: msgField.text.trim().length ? msgField.text.trim() : "release"
                        }
                        App.closeTagDialog()
                        App.prepareCommand("tag_create", args)
                    }
                    background: Rectangle {
                        implicitHeight: 30
                        radius: Theme.radiusSm
                        color: createBtn.enabled ? (createBtn.hovered ? Theme.copperSoft : Theme.copper) : Theme.inkPanel
                        opacity: createBtn.enabled ? 1 : 0.45
                    }
                    contentItem: Text {
                        text: createBtn.text
                        color: createBtn.enabled ? Theme.ink : Theme.mist
                        font.family: Theme.uiFamily
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Controls.Button {
                    id: cancelBtn
                    text: I18n.i18n("Отмена")
                    onClicked: App.closeTagDialog()
                    background: Rectangle {
                        implicitHeight: 30
                        implicitWidth: 64
                        radius: Theme.radiusSm
                        color: "transparent"
                    }
                    contentItem: Text {
                        text: cancelBtn.text
                        color: Theme.mist
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }

    Connections {
        target: App
        function onTagDialogOpenChanged() {
            if (App.tagDialogOpen) {
                nameField.text = ""
                msgField.text = "release"
                Qt.callLater(function () { nameField.forceActiveFocus() })
            }
        }
    }
}
