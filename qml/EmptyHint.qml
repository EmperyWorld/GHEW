pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

// Quiet empty state — title, body, optional action.
ColumnLayout {
    id: root

    property string title: ""
    property string body: ""
    property string actionText: ""
    signal actionClicked()

    spacing: Theme.space8
    width: parent ? parent.width : implicitWidth

    Rectangle {
        Layout.preferredWidth: 28
        Layout.preferredHeight: 2
        color: Theme.copper
        opacity: 0.7
    }

    Text {
        Layout.fillWidth: true
        text: root.title
        color: Theme.paper
        font.family: Theme.uiFamily
        font.pixelSize: Theme.typeTitle
        font.weight: Font.DemiBold
        wrapMode: Text.WordWrap
        visible: root.title.length > 0
    }

    Text {
        Layout.fillWidth: true
        text: root.body
        color: Theme.mist
        font.family: Theme.uiFamily
        font.pixelSize: Theme.typeBody
        lineHeight: 1.35
        wrapMode: Text.WordWrap
        visible: root.body.length > 0
    }

    ActionButton {
        visible: root.actionText.length > 0
        text: root.actionText
        kind: "ghost"
        onClicked: root.actionClicked()
    }
}
