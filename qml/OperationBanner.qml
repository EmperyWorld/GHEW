pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

// Full-width state when git is mid-operation (merge, rebase, cherry-pick, revert).
Rectangle {
    id: root

    property string continueId: ""
    property string abortId: ""
    signal openCommand(string commandId)

    visible: App.repo.isOpen && App.repo.operationKind.length > 0
    implicitHeight: visible ? row.implicitHeight + Theme.space12 : 0
    radius: Theme.radiusSm
    color: App.repo.conflictCount > 0 ? "#241416" : "#1c1810"
    border.color: App.repo.conflictCount > 0 ? Theme.destructive : Theme.copper
    border.width: 1

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: Theme.space8
        spacing: Theme.space8

        Rectangle {
            Layout.preferredWidth: 4
            Layout.fillHeight: true
            radius: 2
            color: App.repo.conflictCount > 0 ? Theme.destructive : Theme.copper
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Text {
                text: {
                    const kind = App.repo.operationKind
                    if (kind === "merge")
                        return I18n.i18n("Идёт слияние")
                    if (kind === "rebase")
                        return I18n.i18n("Идёт rebase")
                    if (kind === "cherry-pick")
                        return I18n.i18n("Идёт cherry-pick")
                    if (kind === "revert")
                        return I18n.i18n("Идёт revert")
                    return kind
                }
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: 13
                font.weight: Font.DemiBold
            }
            Text {
                text: App.repo.conflictCount > 0
                      ? I18n.i18n("%1 файл(ов) с конфликтом — выберите сторону или правьте вручную, затем продолжите.", App.repo.conflictCount)
                      : I18n.i18n("Операция не закончена. Продолжите или отмените её.")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 11
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        ActionButton {
            visible: root.continueId.length > 0
            text: I18n.i18n("Продолжить")
            kind: "primary"
            onClicked: root.openCommand(root.continueId)
        }
        ActionButton {
            visible: root.abortId.length > 0
            text: I18n.i18n("Отменить")
            kind: "danger"
            onClicked: root.openCommand(root.abortId)
        }
    }
}
