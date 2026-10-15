pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

// Status line under the split: path, busy, last exit, delta, audit.
Rectangle {
    id: root

    color: Theme.inkElevated
    implicitHeight: bar.implicitHeight + Theme.space8
    clip: true

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 1
        color: Theme.inkLine
    }

    SoftBar {
        id: bar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: Theme.space12
        anchors.rightMargin: Theme.space12
        spacing: Theme.space12

        Rectangle {
            Layout.preferredWidth: 8
            width: 8
            height: 8
            radius: 4
            color: !App.repo.isOpen ? Theme.inkLine
                   : (App.busy ? Theme.copperSoft
                      : (App.lastExitCode === 0 ? Theme.safe : Theme.destructive))
        }

        Text {
            text: App.repo.isOpen ? App.repo.rootPath : I18n.i18n("Репозиторий не открыт")
            color: Theme.mist
            font.family: Theme.monoFamily
            font.pixelSize: Theme.typeMeta
            elide: Text.ElideMiddle
            width: Math.min(implicitWidth, Math.max(120, root.width * 0.35))
        }

        Text {
            visible: App.busy
            text: App.cancellable ? I18n.i18n("идёт команда") : I18n.i18n("занято")
            color: Theme.copperSoft
            font.family: Theme.uiFamily
            font.pixelSize: Theme.typeMeta
        }

        Text {
            visible: !App.busy && App.lastOutput.length > 0
            text: I18n.i18n("exit %1", App.lastExitCode)
            color: App.lastExitCode === 0 ? Theme.safe : Theme.destructive
            font.family: Theme.monoFamily
            font.pixelSize: Theme.typeMeta
        }

        Text {
            visible: App.lastStatusDelta.length > 0
            text: App.lastStatusDelta
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: Theme.typeMeta
            elide: Text.ElideRight
            width: Math.min(implicitWidth, 180)
        }

        ChipButton {
            visible: App.repo.isOpen
            label: I18n.i18n("терминал")
            tip: I18n.i18n("Открыть терминал в корне репозитория")
            onClicked: App.openTerminalInRepo()
        }

        ChipButton {
            label: I18n.i18n("аудит")
            tip: App.auditLogPath()
            onClicked: App.copyText(App.auditLogPath())
        }
    }
}
