pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

// Two-level inspector navigation: group chips, then the tabs of that group.
Item {
    id: root

    property var groups: []
    property var names: []
    property int group: 0
    property int current: 0
    signal groupChosen(int index)
    signal tabChosen(int index)

    implicitHeight: column.implicitHeight

    readonly property var visibleIndices: {
        if (!root.groups || root.group < 0 || root.group >= root.groups.length)
            return []
        return root.groups[root.group].indices || []
    }

    ColumnLayout {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.space6

        SoftBar {
            Layout.fillWidth: true
            spacing: Theme.space4

            Repeater {
                model: root.groups
                delegate: Rectangle {
                    id: grp
                    required property int index
                    required property var modelData
                    radius: Theme.radiusSm
                    color: root.group === grp.index ? Theme.inkPanel : "transparent"
                    border.color: root.group === grp.index ? Theme.copper : Theme.inkLine
                    border.width: 1
                    implicitHeight: Theme.controlH
                    implicitWidth: grpLabel.implicitWidth + Theme.space16
                    Text {
                        id: grpLabel
                        anchors.centerIn: parent
                        text: grp.modelData.title
                        color: root.group === grp.index ? Theme.paper : Theme.mist
                        font.family: Theme.uiFamily
                        font.pixelSize: Theme.typeKicker
                        font.weight: root.group === grp.index ? Font.DemiBold : Font.Medium
                        font.letterSpacing: 0.4
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.groupChosen(grp.index)
                    }
                }
            }

            Text {
                text: I18n.i18n("зона %1/%2", root.group + 1, root.groups.length)
                color: Theme.mist
                font.family: Theme.monoFamily
                font.pixelSize: Theme.typeKicker
                opacity: 0.55
                height: Theme.controlH
                verticalAlignment: Text.AlignVCenter
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 0

            Rectangle {
                Layout.preferredWidth: 2
                Layout.preferredHeight: 18
                radius: 1
                color: Theme.copper
                opacity: 0.8
            }

            Item { Layout.preferredWidth: Theme.space8 }

            Flickable {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                contentWidth: tabRow.width
                contentHeight: 28
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.HorizontalFlick

                Row {
                    id: tabRow
                    spacing: 2
                    height: 28

                    Repeater {
                        model: root.visibleIndices
                        delegate: Item {
                            id: tabItem
                            required property int modelData
                            readonly property int tabIndex: modelData
                            readonly property bool active: root.current === tabItem.tabIndex
                            width: tabLabel.implicitWidth + 20
                            height: 28

                            Rectangle {
                                anchors.fill: parent
                                radius: Theme.radiusSm
                                color: tabItem.active ? "#22c9a227" : (tabHover.containsMouse ? Theme.inkPanel : "transparent")
                            }

                            Text {
                                id: tabLabel
                                anchors.centerIn: parent
                                text: (root.names && tabItem.tabIndex < root.names.length)
                                      ? root.names[tabItem.tabIndex]
                                      : ""
                                color: tabItem.active ? Theme.paper : Theme.mist
                                font.family: Theme.uiFamily
                                font.pixelSize: Theme.typeMeta
                                font.weight: tabItem.active ? Font.DemiBold : Font.Normal
                            }

                            Rectangle {
                                anchors.bottom: parent.bottom
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: tabItem.active ? parent.width - 8 : 0
                                height: 2
                                radius: 1
                                color: Theme.copper
                                Behavior on width {
                                    NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
                                }
                            }

                            MouseArea {
                                id: tabHover
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.tabChosen(tabItem.tabIndex)
                            }
                        }
                    }
                }
            }
        }
    }
}
