pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import Ghew

// Proportional bar. Each segment is { key, label, value, color }.
Item {
    id: root

    property var segments: []
    signal segmentActivated(string key)

    implicitHeight: 14
    implicitWidth: 120

    readonly property int total: {
        let sum = 0
        const rows = root.segments || []
        for (let i = 0; i < rows.length; ++i)
            sum += Math.max(0, Number(rows[i].value) || 0)
        return sum
    }

    Rectangle {
        id: track
        anchors.fill: parent
        radius: 4
        color: Theme.inkPanel
        border.color: Theme.inkLine
        border.width: 1
        clip: true

        Row {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 1

            Repeater {
                model: root.segments
                delegate: Rectangle {
                    id: seg
                    required property var modelData
                    required property int index
                    readonly property int amount: Math.max(0, Number(seg.modelData.value) || 0)
                    width: root.total > 0
                           ? Math.max(seg.amount > 0 ? 4 : 0,
                                      (track.width - 2) * seg.amount / root.total)
                           : 0
                    height: parent.height
                    visible: seg.amount > 0
                    color: seg.modelData.color || Theme.mist
                    opacity: area.containsMouse ? 1 : 0.88

                    MouseArea {
                        id: area
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        Controls.ToolTip.visible: containsMouse
                        Controls.ToolTip.delay: 200
                        Controls.ToolTip.text: (seg.modelData.label || "") + " · " + String(seg.amount)
                        onClicked: root.segmentActivated(String(seg.modelData.key || ""))
                    }
                }
            }
        }

        Text {
            anchors.centerIn: parent
            visible: root.total === 0
            text: I18n.i18n("чисто")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 9
            opacity: 0.6
        }
    }
}
