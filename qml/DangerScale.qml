pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

// Three-stop scale so danger is a position, not only a colored word.
ColumnLayout {
    id: root

    property string danger: ""
    spacing: Theme.space6
    visible: root.danger.length > 0

    readonly property var stops: [
        {
            level: "safe",
            title: I18n.i18n("Читает"),
            body: I18n.i18n("Не меняет коммиты, индекс и файлы.")
        },
        {
            level: "mutates",
            title: I18n.i18n("Меняет"),
            body: I18n.i18n("Двигает ветку, индекс или рабочее дерево. Откат обычно возможен.")
        },
        {
            level: "destructive",
            title: I18n.i18n("Стирает"),
            body: I18n.i18n("Удаляет данные или переписывает историю. Нужно подтверждение в превью.")
        }
    ]

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.space4

        Repeater {
            model: root.stops
            delegate: Rectangle {
                id: stop
                required property var modelData
                required property int index
                readonly property bool on: root.danger === stop.modelData.level
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                radius: Theme.radiusSm
                color: stop.on ? Theme.dangerColor(stop.modelData.level) : "transparent"
                opacity: stop.on ? 1 : 0.45
                border.color: Theme.dangerColor(stop.modelData.level)
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: stop.modelData.title
                    color: stop.on ? Theme.ink : Theme.dangerColor(stop.modelData.level)
                    font.family: Theme.uiFamily
                    font.pixelSize: 11
                    font.weight: stop.on ? Font.DemiBold : Font.Normal
                }
            }
        }
    }

    Text {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        color: Theme.mist
        font.family: Theme.uiFamily
        font.pixelSize: 11
        text: {
            for (let i = 0; i < root.stops.length; ++i) {
                if (root.stops[i].level === root.danger)
                    return root.stops[i].body
            }
            return ""
        }
    }
}
