pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

// Elevated zone: hairline, optional gold edge, default content slot.
Item {
    id: root

    default property alias content: body.data
    property string kicker: ""
    property string meta: ""
    property bool goldEdge: false
    property int pad: Theme.space12

    implicitHeight: column.implicitHeight + pad * 2
    clip: true

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: Theme.inkElevated
        border.color: root.goldEdge ? Theme.copper : Theme.inkLine
        border.width: 1
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: root.pad
        spacing: Theme.space8

        SectionHead {
            visible: root.kicker.length > 0
            Layout.fillWidth: true
            title: root.kicker
            meta: root.meta
            accent: root.goldEdge ? Theme.copper : Theme.mist
        }

        ColumnLayout {
            id: body
            Layout.fillWidth: true
            spacing: Theme.space8
        }
    }
}
