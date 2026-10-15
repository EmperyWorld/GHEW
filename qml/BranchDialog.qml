pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

Item {
    id: root
    visible: App.branchDialogOpen
    z: 90

    readonly property string defaultUpstream: {
        if (!App.repo.isOpen || App.repo.remotes.count < 1 || !App.repo.branch.length)
            return ""
        return App.repo.remotes.nameAt(0) + "/" + App.repo.branch
    }

    Rectangle {
        anchors.fill: parent
        color: "#CC0A0E12"
        opacity: root.visible ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 120 } }
        MouseArea {
            anchors.fill: parent
            onClicked: App.closeBranchDialog()
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 420)
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
                text: I18n.i18n("Ветка")
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
                    text: I18n.i18n("сейчас")
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: 10
                    font.capitalization: Font.AllUppercase
                    opacity: 0.5
                }
                Text {
                    text: App.repo.branch || "—"
                    color: Theme.copperSoft
                    font.family: Theme.monoFamily
                    font.pixelSize: 12
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                Text {
                    visible: App.repo.hasUpstream
                    text: App.repo.upstream
                    color: Theme.mist
                    font.family: Theme.monoFamily
                    font.pixelSize: 10
                    opacity: 0.7
                    elide: Text.ElideRight
                    Layout.maximumWidth: 140
                }
            }

            Text {
                text: I18n.i18n("Имя")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 10
                font.capitalization: Font.AllUppercase
                opacity: 0.5
            }

            Controls.TextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: I18n.i18n("имя-ветки")
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: 13
                background: Rectangle {
                    implicitHeight: 32
                    radius: Theme.radiusSm
                    color: Theme.inkPanel
                    border.color: nameField.activeFocus ? Theme.copper : Theme.inkLine
                    border.width: 1
                }
                Keys.onEscapePressed: App.closeBranchDialog()
                Keys.onReturnPressed: createBtn.clicked()
            }

            Text {
                text: I18n.i18n("Действия")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 10
                font.capitalization: Font.AllUppercase
                opacity: 0.5
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Controls.Button {
                    id: createBtn
                    text: I18n.i18n("Создать и перейти")
                    Layout.fillWidth: true
                    enabled: nameField.text.trim().length > 0 && App.repo.isOpen && !App.busy
                    onClicked: {
                        App.closeBranchDialog()
                        App.prepareCommand("switch_create", { name: nameField.text.trim() })
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
                    id: switchBtn
                    text: I18n.i18n("Перейти")
                    enabled: nameField.text.trim().length > 0 && App.repo.isOpen && !App.busy
                    onClicked: {
                        App.closeBranchDialog()
                        App.prepareCommand("switch", { name: nameField.text.trim() })
                    }
                    background: Rectangle {
                        implicitHeight: 30
                        implicitWidth: 80
                        radius: Theme.radiusSm
                        color: "transparent"
                        border.color: Theme.inkLine
                        border.width: 1
                    }
                    contentItem: Text {
                        text: switchBtn.text
                        color: Theme.mist
                        font.family: Theme.uiFamily
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Controls.Button {
                    id: renameBtn
                    text: I18n.i18n("Переименовать")
                    Layout.fillWidth: true
                    enabled: nameField.text.trim().length > 0 && App.repo.isOpen && App.repo.branch.length > 0 && !App.busy
                    onClicked: {
                        App.closeBranchDialog()
                        App.prepareCommand("branch_rename", { name: nameField.text.trim() })
                    }
                    background: Rectangle {
                        implicitHeight: 28
                        radius: Theme.radiusSm
                        color: "transparent"
                        border.color: Theme.inkLine
                        border.width: 1
                    }
                    contentItem: Text {
                        text: renameBtn.text
                        color: renameBtn.enabled ? Theme.mist : Theme.inkLine
                        font.family: Theme.uiFamily
                        font.pixelSize: 11
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Controls.Button {
                    id: trackBtn
                    text: App.repo.hasUpstream ? I18n.i18n("Сбросить ↑") : I18n.i18n("Track")
                    enabled: App.repo.isOpen && App.repo.branch.length > 0 && !App.busy
                             && (App.repo.hasUpstream || (App.repo.remotes.count > 0 && root.defaultUpstream.length > 0))
                    onClicked: {
                        App.closeBranchDialog()
                        if (App.repo.hasUpstream)
                            App.prepareCommand("branch_unset_upstream", {})
                        else
                            App.prepareCommand("branch_set_upstream", { upstream: root.defaultUpstream })
                    }
                    background: Rectangle {
                        implicitHeight: 28
                        implicitWidth: 88
                        radius: Theme.radiusSm
                        color: "transparent"
                        border.color: Theme.inkLine
                        border.width: 1
                    }
                    contentItem: Text {
                        text: trackBtn.text
                        color: trackBtn.enabled ? Theme.copperSoft : Theme.inkLine
                        font.family: Theme.uiFamily
                        font.pixelSize: 11
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Controls.Button {
                    id: deleteBtn
                    text: I18n.i18n("Удалить…")
                    enabled: nameField.text.trim().length > 0 && App.repo.isOpen && !App.busy
                             && nameField.text.trim() !== App.repo.branch
                    onClicked: {
                        App.closeBranchDialog()
                        App.prepareCommand("branch_delete", { name: nameField.text.trim(), mode: "-d" })
                    }
                    background: Rectangle {
                        implicitHeight: 28
                        implicitWidth: 80
                        radius: Theme.radiusSm
                        color: "transparent"
                        border.color: deleteBtn.enabled ? Theme.destructive : Theme.inkLine
                        border.width: 1
                    }
                    contentItem: Text {
                        text: deleteBtn.text
                        color: deleteBtn.enabled ? Theme.destructive : Theme.inkLine
                        font.family: Theme.uiFamily
                        font.pixelSize: 11
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Controls.Button {
                    id: cancelBtn
                    text: I18n.i18n("Отмена")
                    onClicked: App.closeBranchDialog()
                    background: Rectangle {
                        implicitHeight: 28
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

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                visible: !App.repo.hasUpstream && App.repo.remotes.count > 0
                text: I18n.i18n("Track → %1").arg(root.defaultUpstream)
                color: Theme.mist
                font.family: Theme.monoFamily
                font.pixelSize: 10
                opacity: 0.7
            }
        }
    }

    Connections {
        target: App
        function onBranchDialogOpenChanged() {
            if (App.branchDialogOpen) {
                nameField.text = App.repo.branch || ""
                Qt.callLater(function () {
                    nameField.forceActiveFocus()
                    nameField.selectAll()
                })
            }
        }
    }
}
