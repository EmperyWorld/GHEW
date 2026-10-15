pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

// Kicker + optional count + trailing meta. Used as the title of every zone.
Item {
    id: root

    property string title: ""
    property string meta: ""
    property string countText: ""
    property color accent: Theme.copper

    implicitHeight: Math.max(kicker.implicitHeight, metaLabel.implicitHeight)
    implicitWidth: row.implicitWidth

    RowLayout {
        id: row
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.space8

        Rectangle {
            Layout.preferredWidth: 3
            Layout.preferredHeight: 12
            radius: 1
            color: root.accent
        }

        Text {
            id: kicker
            text: root.title
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: Theme.typeKicker
            font.letterSpacing: 1.4
            font.capitalization: Font.AllUppercase
            font.weight: Font.DemiBold
        }

        Rectangle {
            visible: root.countText.length > 0
            radius: Theme.radiusSm
            color: Theme.inkPanel
            border.color: Theme.inkLine
            border.width: 1
            implicitHeight: 16
            implicitWidth: countLabel.implicitWidth + 10
            Text {
                id: countLabel
                anchors.centerIn: parent
                text: root.countText
                color: Theme.copperSoft
                font.family: Theme.monoFamily
                font.pixelSize: 10
            }
        }

        Item { Layout.fillWidth: true }

        Text {
            id: metaLabel
            visible: root.meta.length > 0
            text: root.meta
            color: Theme.mist
            font.family: Theme.monoFamily
            font.pixelSize: 10
            opacity: 0.7
            elide: Text.ElideRight
            Layout.maximumWidth: 220
        }
    }
}
