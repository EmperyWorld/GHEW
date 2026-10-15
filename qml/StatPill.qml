pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import Ghew

// Compact metric used in the repo header and operation banners.
Rectangle {
    id: root

    property string label: ""
    property string valueText: "0"
    property int numericValue: 0
    property color accent: Theme.mist
    property string tip: ""
    property bool interactive: true
    signal activated()

    radius: Theme.radiusSm
    color: area.containsMouse && root.interactive ? Theme.inkPanel : "transparent"
    border.color: root.numericValue > 0 ? root.accent : Theme.inkLine
    border.width: 1
    implicitHeight: 24
    implicitWidth: row.implicitWidth + Theme.space12
    opacity: root.numericValue > 0 ? 1 : 0.5

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 4
        Text {
            text: root.label
            color: Theme.mist
            font.family: Theme.monoFamily
            font.pixelSize: Theme.typeKicker
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: root.valueText
            color: root.numericValue > 0 ? root.accent : Theme.mist
            font.family: Theme.monoFamily
            font.pixelSize: Theme.typeMeta
            font.weight: Font.DemiBold
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.interactive
        cursorShape: Qt.PointingHandCursor
        Controls.ToolTip.visible: containsMouse && root.tip.length > 0
        Controls.ToolTip.delay: 280
        Controls.ToolTip.text: root.tip
        onClicked: root.activated()
    }
}
