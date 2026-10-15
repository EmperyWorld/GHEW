pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

// Up to three next actions under the command catalog.
Item {
    id: root

    signal commandChosen(string commandId)

    visible: App.repo.isOpen && App.recommendations.count > 0
    clip: true
    implicitWidth: 0
    implicitHeight: visible ? column.implicitHeight : 0

    ColumnLayout {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Theme.space8
        spacing: Theme.space6

        SectionHead {
            Layout.fillWidth: true
            title: I18n.i18n("Далее")
            countText: String(Math.min(3, App.recommendations.count))
            accent: Theme.copper
        }

        Repeater {
            model: App.recommendations
            delegate: Rectangle {
                id: step
                required property int index
                required property string commandId
                required property string title
                required property string source
                required property string reason
                Layout.fillWidth: true
                visible: step.index < 3
                implicitHeight: step.visible ? inner.implicitHeight + 10 : 0
                radius: Theme.radiusSm
                color: area.containsMouse ? Theme.inkPanel : "#10161e"
                border.color: step.source === "llm" ? Theme.copper : Theme.inkLine
                border.width: 1

                Column {
                    id: inner
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: Theme.space8
                    anchors.rightMargin: Theme.space8
                    spacing: 1
                    Text {
                        width: parent.width
                        text: (step.source === "llm" ? "LLM  " : "") + step.title
                        color: Theme.paper
                        font.family: Theme.uiFamily
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        visible: step.reason.length > 0
                        text: step.reason
                        color: Theme.mist
                        font.family: Theme.uiFamily
                        font.pixelSize: 10
                        elide: Text.ElideRight
                        opacity: 0.8
                    }
                }

                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.commandChosen(step.commandId)
                }
            }
        }
    }
}
