pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

// Structured card for the selected command: identity, danger, and each parameter.
ColumnLayout {
    id: root

    property string commandId: ""
    spacing: Theme.space8

    readonly property var spec: root.commandId.length ? App.catalog.commandById(root.commandId) : ({})
    readonly property var params: root.commandId.length ? App.catalog.paramsFor(root.commandId) : []

    EmptyHint {
        Layout.fillWidth: true
        visible: !root.spec || !root.spec.id
        title: I18n.i18n("Команда не выбрана")
        body: I18n.i18n("Каталог слева. После выбора здесь будут группа, уровень опасности и поля параметров.")
    }

    SectionHead {
        Layout.fillWidth: true
        visible: root.spec && root.spec.id
        title: I18n.i18n("Карточка")
        meta: root.spec && root.spec.group ? App.catalog.groupTitle(root.spec.group) : ""
        accent: Theme.dangerColor(root.spec ? root.spec.danger : "")
    }

    GridLayout {
        Layout.fillWidth: true
        visible: root.spec && root.spec.id
        columns: 2
        columnSpacing: Theme.space12
        rowSpacing: Theme.space6

        Text { text: I18n.i18n("Идентификатор"); color: Theme.mist; font.family: Theme.uiFamily; font.pixelSize: 11 }
        Text {
            text: root.spec.id || ""
            color: Theme.paper
            font.family: Theme.monoFamily
            font.pixelSize: 12
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        Text { text: I18n.i18n("Синопсис"); color: Theme.mist; font.family: Theme.uiFamily; font.pixelSize: 11 }
        Text {
            text: root.spec.synopsis || "—"
            color: Theme.paper
            font.family: Theme.monoFamily
            font.pixelSize: 12
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }

        Text { text: I18n.i18n("Превью"); color: Theme.mist; font.family: Theme.uiFamily; font.pixelSize: 11 }
        Text {
            text: root.spec.preview || "—"
            color: Theme.copperSoft
            font.family: Theme.monoFamily
            font.pixelSize: 12
        }

        Text { text: I18n.i18n("Избранное"); color: Theme.mist; font.family: Theme.uiFamily; font.pixelSize: 11 }
        Text {
            text: App.favorites.contains(root.commandId) ? I18n.i18n("да") : I18n.i18n("нет")
            color: Theme.paper
            font.family: Theme.uiFamily
            font.pixelSize: 12
        }
    }

    DangerScale {
        Layout.fillWidth: true
        danger: root.spec && root.spec.danger ? root.spec.danger : ""
    }

    SectionHead {
        Layout.fillWidth: true
        visible: root.params.length > 0
        title: I18n.i18n("Параметры")
        countText: String(root.params.length)
        accent: Theme.mist
    }

    Repeater {
        model: root.params
        delegate: Rectangle {
            id: row
            required property var modelData
            required property int index
            Layout.fillWidth: true
            implicitHeight: body.implicitHeight + Theme.space12
            radius: Theme.radiusSm
            color: Theme.inkPanel
            border.color: Theme.inkLine
            border.width: 1

            ColumnLayout {
                id: body
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: Theme.space8
                spacing: 2

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.space6
                    Text {
                        text: row.modelData.label || row.modelData.name
                        color: Theme.paper
                        font.family: Theme.uiFamily
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Text {
                        text: row.modelData.type || ""
                        color: Theme.copperSoft
                        font.family: Theme.monoFamily
                        font.pixelSize: 10
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: (row.modelData.flag ? row.modelData.flag + "  " : "") + row.modelData.name
                    color: Theme.mist
                    font.family: Theme.monoFamily
                    font.pixelSize: 11
                    elide: Text.ElideRight
                }
            }
        }
    }
}
