pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

// Last catalog commands touched in this install, under the catalog title.
Item {
    id: root

    signal commandChosen(string commandId)

    visible: App.recentCommands.count > 0
    clip: true
    // Width comes from the sidebar layout only — never from chip content.
    implicitWidth: 0
    implicitHeight: visible ? rail.implicitHeight : 0

    ColumnLayout {
        id: rail
        width: root.width
        spacing: Theme.space4

        Text {
            Layout.fillWidth: true
            text: I18n.i18n("Недавние")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: Theme.typeKicker
            font.letterSpacing: 1.2
            font.capitalization: Font.AllUppercase
            opacity: 0.55
        }

        ChipFlow {
            Layout.fillWidth: true
            spacing: Theme.space4

            Repeater {
                model: App.recentCommands
                delegate: ChipButton {
                    id: chip
                    required property string commandId
                    required property int index
                    readonly property var spec: App.catalog.commandById(chip.commandId)
                    visible: chip.index < 8 && chip.spec && chip.spec.id
                    label: chip.spec && chip.spec.title ? chip.spec.title : chip.commandId
                    tip: chip.spec && chip.spec.synopsis ? chip.spec.synopsis : chip.commandId
                    accent: Theme.dangerColor(chip.spec ? chip.spec.danger : "")
                    active: false
                    onClicked: root.commandChosen(chip.commandId)
                }
            }
        }
    }
}
