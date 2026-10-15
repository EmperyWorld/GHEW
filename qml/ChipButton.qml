pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import Ghew

// Small labeled control shared by rails, diff chrome, and the mentor desk.
Rectangle {
    id: root

    property string label: ""
    property string tip: ""
    property bool active: false
    property color accent: Theme.copper
    property color labelColor: Theme.mist
    signal clicked()

    radius: Theme.radiusSm
    border.width: 1
    border.color: !root.enabled ? Theme.inkLine
                  : (root.active ? root.accent : Theme.inkLine)
    color: !root.enabled ? "transparent"
           : (area.containsMouse || root.active ? Theme.inkPanel : "transparent")
    opacity: root.enabled ? 1 : 0.4
    implicitHeight: 24
    implicitWidth: labelText.implicitWidth + Theme.space12

    Text {
        id: labelText
        anchors.centerIn: parent
        text: root.label
        color: root.active ? root.accent : root.labelColor
        font.family: Theme.uiFamily
        font.pixelSize: Theme.typeMeta
        font.weight: root.active ? Font.DemiBold : Font.Normal
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        Controls.ToolTip.visible: containsMouse && root.tip.length > 0
        Controls.ToolTip.delay: 400
        Controls.ToolTip.text: root.tip
        onClicked: root.clicked()
    }
}
