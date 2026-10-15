pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Ghew

Item {
    id: root
    signal executed()

    property var preview: {
        try {
            return App.lastPreviewJson ? JSON.parse(App.lastPreviewJson) : null
        } catch (e) {
            return null
        }
    }

    function runPreview() {
        if (!root.preview || App.busy)
            return
        if (root.preview.danger === "destructive")
            confirmDialog.open()
        else {
            App.executeLastPreview(false)
            root.executed()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: I18n.i18n("Превью")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 10
                font.letterSpacing: 1.2
                font.capitalization: Font.AllUppercase
                opacity: 0.65
                Layout.fillWidth: true
            }

            Rectangle {
                visible: root.preview && root.preview.danger === "safe"
                radius: Theme.radiusSm
                color: "transparent"
                border.color: Theme.safe
                border.width: 1
                implicitHeight: 22
                implicitWidth: dryLabel.implicitWidth + 14
                Text {
                    id: dryLabel
                    anchors.centerIn: parent
                    text: I18n.i18n("dry-run / safe")
                    color: Theme.safe
                    font.family: Theme.uiFamily
                    font.pixelSize: 10
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 0.5
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: cmdlineLabel.implicitHeight + 16
            radius: Theme.radiusSm
            color: Theme.inkElevated
            border.color: Theme.inkLine
            border.width: 1

            Text {
                id: cmdlineLabel
                anchors.left: parent.left
                anchors.right: copyCmdBtn.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 8
                anchors.rightMargin: 6
                wrapMode: Text.WrapAnywhere
                font.family: Theme.monoFamily
                font.pixelSize: 12
                font.weight: Font.DemiBold
                color: Theme.copperSoft
                text: root.preview ? root.preview.cmdline : I18n.i18n("Превью ещё не построено — вкладка Параметры")
            }

            Controls.Button {
                id: copyCmdBtn
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 6
                visible: !!root.preview
                text: I18n.i18n("копир.")
                onClicked: App.copyText(root.preview.cmdline)
                background: Rectangle {
                    implicitHeight: 24
                    implicitWidth: 56
                    radius: Theme.radiusSm
                    color: copyCmdBtn.hovered ? Theme.inkPanel : "transparent"
                    border.color: Theme.inkLine
                    border.width: 1
                }
                contentItem: Text {
                    text: copyCmdBtn.text
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Text {
                width: Math.max(root.width - 8, 100)
                wrapMode: Text.WordWrap
                textFormat: Text.MarkdownText
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: 14
                lineHeight: 1.4
                text: root.preview ? root.preview.narrative : ""
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Controls.Button {
                id: runBtn
                text: I18n.i18n("Выполнить")
                enabled: !!root.preview && !App.busy
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.text: I18n.i18n("Ctrl+Enter")
                onClicked: root.runPreview()

                background: Rectangle {
                    implicitHeight: 32
                    implicitWidth: 120
                    radius: Theme.radiusSm
                    color: !runBtn.enabled
                           ? Theme.inkPanel
                           : (runBtn.down ? Theme.copperSoft
                                          : (runBtn.hovered ? Theme.copperSoft : Theme.copper))
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

            Text {
                visible: root.preview !== null
                text: root.preview ? root.preview.danger : ""
                color: root.preview ? Theme.dangerColor(root.preview.danger) : Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 12
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 0.6
            }

            Item { Layout.fillWidth: true }

            Text {
                visible: App.busy
                text: I18n.i18n("Выполняется…")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 12
            }
        }
    }

    Kirigami.PromptDialog {
        id: confirmDialog
        title: I18n.i18n("Опасная операция")
        subtitle: I18n.i18n("Команда может привести к потере данных. Продолжить?")
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
        onAccepted: {
            App.executeLastPreview(true)
            root.executed()
        }
    }
}
