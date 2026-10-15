pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

ColumnLayout {
    id: root
    spacing: 8

    property string commandId: ""
    property string commandSynopsis: ""
    property var values: ({})
    property int formRevision: 0

    function collect() {
        return Object.assign({}, root.values)
    }

    function applyValues(map) {
        if (!map)
            return
        const next = Object.assign({}, root.values)
        for (const key of Object.keys(map))
            next[key] = map[key]
        root.values = next
        root.formRevision++
    }

    function valueOf(key) {
        return root.values[key]
    }

    function setValue(key, value) {
        const next = Object.assign({}, root.values)
        next[key] = value
        root.values = next
    }

    function reload() {
        const params = App.catalog.paramsFor(root.commandId)
        const next = {}
        listModel.clear()
        for (let i = 0; i < params.length; ++i) {
            const p = params[i]
            const key = p.name
            if (p["default"] !== undefined && p["default"] !== null)
                next[key] = p["default"]
            else
                next[key] = p.type === "bool" ? false : ""
            listModel.append({
                                 "name": p.name,
                                 "label": p.label,
                                 "type": p.type,
                                 "options": p.options || []
                             })
        }
        root.values = next
    }

    onCommandIdChanged: {
        reload()
        Qt.callLater(function () {
            if (root.commandId === "commit_amend" && App.repo.lastCommitSubject.length) {
                const cur = root.valueOf("message")
                if (!cur || !String(cur).length) {
                    root.setValue("message", App.repo.lastCommitSubject)
                    root.formRevision++
                }
            } else if (root.commandId === "commit") {
                const cur = root.valueOf("message")
                if (!cur || !String(cur).length) {
                    const s = App.suggestCommitMessage()
                    if (s.length) {
                        root.setValue("message", s)
                        root.formRevision++
                    }
                }
            }
        })
    }

    ListModel {
        id: listModel
    }

    Text {
        visible: listModel.count === 0
        text: root.commandId.length
              ? I18n.i18n("У этой команды нет параметров.")
              : I18n.i18n("Выберите команду слева.")
        color: Theme.mist
        font.family: Theme.uiFamily
        font.pixelSize: 13
        opacity: 0.75
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 4
        visible: root.commandId === "commit" || root.commandId === "commit_amend"

        Text {
            text: I18n.i18n("Шаблон сообщения")
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: 10
            font.capitalization: Font.AllUppercase
            opacity: 0.5
        }

        Flow {
            Layout.fillWidth: true
            spacing: 4

            Repeater {
                model: [
                    "feat: ",
                    "fix: ",
                    "docs: ",
                    "refactor: ",
                    "test: ",
                    "chore: "
                ]
                delegate: Rectangle {
                    id: tpl
                    required property string modelData
                    radius: Theme.radiusSm
                    border.color: Theme.inkLine
                    border.width: 1
                    color: tplArea.containsMouse ? Theme.inkPanel : "transparent"
                    implicitHeight: 22
                    implicitWidth: tplLabel.implicitWidth + 12
                    Text {
                        id: tplLabel
                        anchors.centerIn: parent
                        text: tpl.modelData.trim()
                        color: Theme.copperSoft
                        font.family: Theme.monoFamily
                        font.pixelSize: 11
                    }
                    MouseArea {
                        id: tplArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            const cur = root.valueOf("message") || ""
                            const base = String(cur).replace(/^(feat|fix|docs|refactor|test|chore):\s*/i, "")
                            root.setValue("message", tpl.modelData + base)
                            root.formRevision++
                        }
                    }
                }
            }

            Rectangle {
                visible: root.commandId === "commit" && App.repo.stagedCount > 0
                radius: Theme.radiusSm
                border.color: Theme.copper
                border.width: 1
                color: suggestArea.containsMouse ? Theme.inkPanel : "transparent"
                implicitHeight: 22
                implicitWidth: suggestLab.implicitWidth + 12
                Text {
                    id: suggestLab
                    anchors.centerIn: parent
                    text: I18n.i18n("из файлов")
                    color: Theme.copperSoft
                    font.family: Theme.uiFamily
                    font.pixelSize: 11
                }
                MouseArea {
                    id: suggestArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        const s = App.suggestCommitMessage()
                        if (s.length) {
                            root.setValue("message", s)
                            root.formRevision++
                        }
                    }
                }
            }

            Rectangle {
                visible: root.commandId === "commit_amend" && App.repo.lastCommitSubject.length > 0
                radius: Theme.radiusSm
                border.color: Theme.copper
                border.width: 1
                color: headArea.containsMouse ? Theme.inkPanel : "transparent"
                implicitHeight: 22
                implicitWidth: headLab.implicitWidth + 12
                Text {
                    id: headLab
                    anchors.centerIn: parent
                    text: I18n.i18n("из HEAD")
                    color: Theme.copperSoft
                    font.family: Theme.uiFamily
                    font.pixelSize: 11
                }
                MouseArea {
                    id: headArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.setValue("message", App.repo.lastCommitSubject)
                        root.formRevision++
                    }
                }
            }
        }
    }

    Repeater {
        model: listModel

        delegate: ColumnLayout {
            id: row
            Layout.fillWidth: true
            spacing: 3

            required property string name
            required property string label
            required property string type
            required property var options
            required property int index

            readonly property string currentValue: {
                const v = root.valueOf(row.name)
                return v === undefined || v === null ? "" : String(v)
            }
            readonly property bool currentBool: !!root.valueOf(row.name)
            readonly property string fieldPlaceholder: {
                if (row.type === "path")
                    return I18n.i18n("путь относительно корня репо")
                if (row.index === 0 && root.commandSynopsis.length)
                    return root.commandSynopsis
                return row.label
            }

            Text {
                text: row.label
                visible: row.type !== "bool"
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 11
                opacity: 0.8
            }

            Controls.CheckBox {
                id: boolBox
                visible: row.type === "bool"
                text: row.label
                checked: row.currentBool
                onToggled: root.setValue(row.name, checked)
                contentItem: Text {
                    text: boolBox.text
                    color: Theme.paper
                    font.family: Theme.uiFamily
                    font.pixelSize: 12
                    leftPadding: boolBox.indicator.width + 8
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Controls.TextField {
                visible: row.type === "string" || row.type === "path"
                Layout.fillWidth: true
                placeholderText: row.fieldPlaceholder
                text: {
                    root.formRevision
                    return row.currentValue
                }
                onTextEdited: root.setValue(row.name, text)
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: 12
                background: Rectangle {
                    implicitHeight: 30
                    radius: Theme.radiusSm
                    color: Theme.inkElevated
                    border.color: parent.activeFocus ? Theme.copper : Theme.inkLine
                    border.width: 1
                    Behavior on border.color { ColorAnimation { duration: 120 } }
                }
            }

            Controls.TextArea {
                visible: row.type === "text"
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                text: row.currentValue
                onTextChanged: root.setValue(row.name, text)
                color: Theme.paper
                font.family: Theme.uiFamily
                font.pixelSize: 12
                background: Rectangle {
                    radius: Theme.radiusSm
                    color: Theme.inkElevated
                    border.color: parent.activeFocus ? Theme.copper : Theme.inkLine
                    border.width: 1
                }
            }

            Controls.ComboBox {
                id: choiceBox
                visible: row.type === "choice"
                Layout.fillWidth: true
                model: row.options
                Component.onCompleted: {
                    const idx = model.indexOf(row.currentValue)
                    currentIndex = idx >= 0 ? idx : 0
                    if (count > 0)
                        root.setValue(row.name, model[currentIndex])
                }
                onActivated: root.setValue(row.name, model[currentIndex])

                background: Rectangle {
                    implicitHeight: 30
                    radius: Theme.radiusSm
                    color: Theme.inkElevated
                    border.color: Theme.inkLine
                    border.width: 1
                }
                contentItem: Text {
                    leftPadding: 8
                    rightPadding: choiceBox.indicator.width + 6
                    text: choiceBox.displayText
                    color: Theme.paper
                    font.family: Theme.uiFamily
                    font.pixelSize: 12
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }
        }
    }
}
