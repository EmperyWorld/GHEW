pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

Item {
    id: root
    property string searchText: ""

    readonly property var lines: {
        const text = App.lastOutput || ""
        if (!text.length)
            return []
        const all = text.split("\n")
        if (!root.searchText.length)
            return all
        const needle = root.searchText.toLowerCase()
        return all.filter(function (line) { return line.toLowerCase().indexOf(needle) >= 0 })
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: I18n.i18n("Вывод")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 10
                font.letterSpacing: 1.2
                font.capitalization: Font.AllUppercase
                opacity: 0.65
            }

            Text {
                visible: App.lastOutput.length > 0
                text: "exit " + App.lastExitCode
                font.family: Theme.monoFamily
                font.pixelSize: 11
                color: App.lastExitCode === 0 ? Theme.safe : Theme.destructive
            }

            Item { Layout.fillWidth: true }

            Controls.TextField {
                id: searchField
                visible: App.lastOutput.length > 0
                Layout.preferredWidth: 140
                placeholderText: I18n.i18n("Поиск…")
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: 11
                onTextChanged: root.searchText = text
                background: Rectangle {
                    implicitHeight: 24
                    radius: Theme.radiusSm
                    color: Theme.inkPanel
                    border.color: searchField.activeFocus ? Theme.copper : Theme.inkLine
                    border.width: 1
                }
            }

            Controls.Button {
                id: copyOutBtn
                visible: App.lastOutput.length > 0
                text: I18n.i18n("копир.")
                onClicked: App.copyText(App.lastOutput)
                background: Rectangle {
                    implicitHeight: 24
                    implicitWidth: 56
                    radius: Theme.radiusSm
                    color: copyOutBtn.hovered ? Theme.inkPanel : "transparent"
                    border.color: Theme.inkLine
                    border.width: 1
                }
                contentItem: Text {
                    text: copyOutBtn.text
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Text {
            visible: App.lastOutput.length > 0 && root.searchText.length > 0
            Layout.fillWidth: true
            text: I18n.i18n("Найдено строк: %1", root.lines.length)
            color: Theme.copperSoft
            font.family: Theme.monoFamily
            font.pixelSize: 11
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Theme.radius
            color: "#0a0d12"
            border.color: Theme.inkLine
            border.width: 1

            Controls.ScrollView {
                anchors.fill: parent
                anchors.margins: 2
                clip: true

                Controls.TextArea {
                    id: out
                    readOnly: true
                    wrapMode: TextEdit.Wrap
                    font.family: Theme.monoFamily
                    font.pixelSize: 12
                    color: Theme.paper
                    text: App.lastOutput.length
                          ? root.lines.join("\n")
                          : I18n.i18n("Здесь появится stdout/stderr git…")
                    background: Item {}
                    padding: 14
                }
            }
        }
    }
}
