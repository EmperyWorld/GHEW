pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

// One session command: outcome, time, replay, copy.
Rectangle {
    id: root

    required property int index
    required property string cmdline
    required property string repoRoot
    required property int exitCode
    required property bool ok
    required property string timestamp
    required property string commandId

    radius: Theme.radiusSm
    color: hover.hovered ? Theme.inkPanel : Theme.inkElevated
    border.color: root.ok ? Theme.inkLine : Theme.destructive
    border.width: 1
    implicitHeight: 52

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.space8
        spacing: Theme.space8

        Rectangle {
            Layout.preferredWidth: 4
            Layout.fillHeight: true
            radius: 2
            color: root.ok ? Theme.safe : Theme.destructive
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Text {
                Layout.fillWidth: true
                text: root.cmdline.length ? root.cmdline : root.commandId
                color: root.ok ? Theme.paper : Theme.destructive
                font.family: Theme.monoFamily
                font.pixelSize: 12
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: root.timestamp
                      + "  ·  exit " + String(root.exitCode)
                      + (root.commandId.length ? "  ·  " + root.commandId : "")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 10
                elide: Text.ElideMiddle
                opacity: 0.8
            }
        }

        ChipButton {
            visible: root.commandId.length > 0
            label: I18n.i18n("повтор")
            accent: Theme.copper
            tip: I18n.i18n("Снова открыть с теми же аргументами")
            onClicked: App.replayHistory(root.index)
        }
        ChipButton {
            label: I18n.i18n("копир.")
            tip: root.repoRoot.length ? root.cmdline + "\n" + root.repoRoot : root.cmdline
            onClicked: App.copyText(root.cmdline)
        }
    }

    HoverHandler {
        id: hover
    }
}
