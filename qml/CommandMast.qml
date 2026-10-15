pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

// Title block for the selected catalog command: name, synopsis, danger, favorite.
Item {
    id: root

    property string commandTitle: ""
    property string synopsis: ""
    property string danger: ""
    property string commandId: ""
    property bool favorited: false
    signal favoriteToggled()

    implicitHeight: column.implicitHeight

    ColumnLayout {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.space4

        SectionHead {
            Layout.fillWidth: true
            title: I18n.i18n("Команда")
            meta: root.commandId
            accent: Theme.copper
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space8

            Text {
                Layout.fillWidth: true
                text: root.commandTitle.length ? root.commandTitle : I18n.i18n("Выберите команду")
                color: Theme.paper
                font.family: Theme.displayFamily
                font.pixelSize: Theme.typeDisplay
                font.weight: Font.Medium
                elide: Text.ElideRight
            }

            Rectangle {
                visible: root.danger.length > 0
                radius: Theme.radiusSm
                color: "transparent"
                border.color: Theme.dangerColor(root.danger)
                border.width: 1
                implicitHeight: 22
                implicitWidth: dangerLabel.implicitWidth + 14
                Rectangle {
                    anchors.fill: parent
                    radius: parent.radius
                    color: parent.border.color
                    opacity: 0.16
                }
                Text {
                    id: dangerLabel
                    anchors.centerIn: parent
                    text: root.danger
                    color: Theme.dangerColor(root.danger)
                    font.family: Theme.uiFamily
                    font.pixelSize: 10
                    font.letterSpacing: 0.8
                    font.capitalization: Font.AllUppercase
                }
            }

            Rectangle {
                visible: root.commandId.length > 0
                radius: Theme.radiusSm
                color: favArea.containsMouse ? Theme.inkPanel : "transparent"
                border.color: root.favorited ? Theme.copper : Theme.inkLine
                border.width: 1
                implicitWidth: 28
                implicitHeight: 22
                Text {
                    anchors.centerIn: parent
                    text: root.favorited ? "★" : "☆"
                    color: root.favorited ? Theme.copperSoft : Theme.mist
                    font.pixelSize: 13
                }
                MouseArea {
                    id: favArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    Controls.ToolTip.visible: containsMouse
                    Controls.ToolTip.text: I18n.i18n("Избранное")
                    onClicked: root.favoriteToggled()
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: root.synopsis.length > 0
            text: root.synopsis
            color: Theme.mist
            font.family: Theme.monoFamily
            font.pixelSize: Theme.typeMeta
            elide: Text.ElideRight
            opacity: 0.85
        }
    }
}
