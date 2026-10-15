pragma ComponentBehavior: Bound
import QtQuick
import org.kde.kirigami as Kirigami
import Ghew

// Structured action: primary (gold), ghost, quiet, danger.
// One control so workspace chrome does not restyle Button ad hoc.
Item {
    id: root

    property string text: ""
    property string iconName: ""
    property string kind: "primary"   // primary | ghost | quiet | danger
    property bool fillWidth: false
    signal clicked()

    implicitHeight: root.kind === "quiet" ? 28 : 36
    implicitWidth: row.implicitWidth + Theme.space16 * 2
    opacity: root.enabled ? 1 : 0.42

    readonly property color fg: {
        if (!root.enabled)
            return Theme.mist
        if (root.kind === "primary")
            return Theme.ink
        if (root.kind === "danger")
            return Theme.destructive
        if (root.kind === "ghost")
            return Theme.copperSoft
        return Theme.paper
    }

    readonly property color bg: {
        if (root.kind === "primary") {
            if (!root.enabled)
                return Theme.inkPanel
            if (mouse.pressed)
                return Theme.copperSoft
            if (mouse.containsMouse)
                return Theme.copperSoft
            return Theme.copper
        }
        if (mouse.containsMouse && root.enabled)
            return root.kind === "danger" ? "#33e06a5a" : Theme.inkPanel
        return "transparent"
    }

    readonly property color edge: {
        if (root.kind === "primary")
            return "transparent"
        if (root.kind === "danger")
            return Theme.destructive
        if (root.kind === "ghost")
            return Theme.copper
        return Theme.inkLine
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusSm
        color: root.bg
        border.color: root.edge
        border.width: root.kind === "primary" ? 0 : 1
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: Theme.space8
        Kirigami.Icon {
            visible: root.iconName.length > 0
            source: root.iconName
            width: 16
            height: 16
            color: root.fg
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: root.text
            color: root.fg
            font.family: Theme.uiFamily
            font.pixelSize: 13
            font.weight: root.kind === "quiet" ? Font.Normal : Font.DemiBold
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: root.clicked()
    }

    scale: mouse.pressed && root.enabled ? 0.98 : 1
    Behavior on scale { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }
}
