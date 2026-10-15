pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Ghew

Kirigami.Page {
    id: page
    title: ""
    padding: 0
    // Full-bleed welcome — no duplicate GHEW chrome in the page header.
    globalToolBarStyle: Kirigami.ApplicationHeaderStyle.None
    background: Rectangle { color: Theme.ink }

    function openRepoDialog() { folderDialog.open() }
    function openCloneDialog() { cloneDialog.open() }
    function openInitDialog() { initDialog.open() }

    FolderDialog {
        id: folderDialog
        title: I18n.i18n("Выберите каталог git-репозитория")
        onAccepted: {
            const path = selectedFolder.toString().replace("file://", "")
            App.openRepository(path)
        }
    }

    FolderDialog {
        id: cloneDestDialog
        title: I18n.i18n("Куда клонировать")
        onAccepted: {
            const path = selectedFolder.toString().replace("file://", "")
            cloneDestField.text = path
        }
    }

    FolderDialog {
        id: initDestDialog
        title: I18n.i18n("Каталог нового репозитория")
        onAccepted: {
            const path = selectedFolder.toString().replace("file://", "")
            initPathField.text = path
        }
    }

    Controls.Dialog {
        id: cloneDialog
        title: I18n.i18n("Клонировать репозиторий")
        modal: true
        anchors.centerIn: parent
        width: Math.min(page.width - 48, 480)
        standardButtons: Controls.Dialog.Ok | Controls.Dialog.Cancel
        onAccepted: App.cloneRepository(cloneUrlField.text, cloneDestField.text)

        background: Rectangle {
            color: Theme.inkElevated
            border.color: Theme.inkLine
            border.width: 1
            radius: Theme.radius
        }

        GridLayout {
            width: parent ? parent.width : 400
            columns: 2
            columnSpacing: 8
            rowSpacing: 8

            Text {
                text: I18n.i18n("URL")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 11
                Layout.alignment: Qt.AlignVCenter
            }
            Controls.TextField {
                id: cloneUrlField
                Layout.fillWidth: true
                placeholderText: "https://…/repo.git"
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: 12
                background: Rectangle {
                    implicitHeight: 30
                    radius: Theme.radiusSm
                    color: Theme.inkPanel
                    border.color: Theme.inkLine
                    border.width: 1
                }
            }
            Text {
                text: I18n.i18n("Куда")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 11
                Layout.alignment: Qt.AlignVCenter
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Controls.TextField {
                    id: cloneDestField
                    Layout.fillWidth: true
                    placeholderText: I18n.i18n("Папка для клона")
                    color: Theme.paper
                    font.family: Theme.monoFamily
                    font.pixelSize: 12
                    background: Rectangle {
                        implicitHeight: 30
                        radius: Theme.radiusSm
                        color: Theme.inkPanel
                        border.color: Theme.inkLine
                        border.width: 1
                    }
                }
                Controls.Button {
                    text: "…"
                    onClicked: cloneDestDialog.open()
                    background: Rectangle {
                        implicitHeight: 30
                        implicitWidth: 32
                        radius: Theme.radiusSm
                        color: Theme.inkPanel
                        border.color: Theme.inkLine
                        border.width: 1
                    }
                    contentItem: Text {
                        text: "…"
                        color: Theme.mist
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }

    Controls.Dialog {
        id: initDialog
        title: I18n.i18n("Новый репозиторий")
        modal: true
        anchors.centerIn: parent
        width: Math.min(page.width - 48, 480)
        standardButtons: Controls.Dialog.Ok | Controls.Dialog.Cancel
        onAccepted: App.initRepository(initPathField.text, initBranchField.text, initReadmeBox.checked)

        background: Rectangle {
            color: Theme.inkElevated
            border.color: Theme.inkLine
            border.width: 1
            radius: Theme.radius
        }

        GridLayout {
            width: parent ? parent.width : 400
            columns: 2
            columnSpacing: 8
            rowSpacing: 8

            Text {
                text: I18n.i18n("Каталог")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 11
                Layout.alignment: Qt.AlignVCenter
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Controls.TextField {
                    id: initPathField
                    Layout.fillWidth: true
                    placeholderText: I18n.i18n("Путь (создастся, если нет)")
                    color: Theme.paper
                    font.family: Theme.monoFamily
                    font.pixelSize: 12
                    background: Rectangle {
                        implicitHeight: 30
                        radius: Theme.radiusSm
                        color: Theme.inkPanel
                        border.color: Theme.inkLine
                        border.width: 1
                    }
                }
                Controls.Button {
                    text: "…"
                    onClicked: initDestDialog.open()
                    background: Rectangle {
                        implicitHeight: 30
                        implicitWidth: 32
                        radius: Theme.radiusSm
                        color: Theme.inkPanel
                        border.color: Theme.inkLine
                        border.width: 1
                    }
                    contentItem: Text {
                        text: "…"
                        color: Theme.mist
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
            Text {
                text: I18n.i18n("Ветка")
                color: Theme.mist
                font.family: Theme.uiFamily
                font.pixelSize: 11
                Layout.alignment: Qt.AlignVCenter
            }
            Controls.TextField {
                id: initBranchField
                Layout.fillWidth: true
                text: "main"
                color: Theme.paper
                font.family: Theme.monoFamily
                font.pixelSize: 12
                background: Rectangle {
                    implicitHeight: 30
                    radius: Theme.radiusSm
                    color: Theme.inkPanel
                    border.color: Theme.inkLine
                    border.width: 1
                }
            }
            Item { Layout.preferredWidth: 1; Layout.preferredHeight: 1 }
            Controls.CheckBox {
                id: initReadmeBox
                checked: true
                text: I18n.i18n("README.md (без коммита)")
                contentItem: Text {
                    text: initReadmeBox.text
                    color: Theme.paper
                    font.family: Theme.uiFamily
                    font.pixelSize: 12
                    leftPadding: initReadmeBox.indicator.width + 8
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    Item {
        id: stage
        anchors.fill: parent
        clip: true
        property bool wide: width >= 880
        property real markSize: Math.max(width, height) * (wide ? 0.98 : 1.05)

        DropArea {
            id: dropArea
            anchors.fill: parent
            keys: ["text/uri-list"]
            onDropped: function (drop) {
                if (!drop.hasUrls || drop.urls.length === 0)
                    return
                let path = drop.urls[0].toString()
                if (path.startsWith("file://"))
                    path = path.replace("file://", "")
                App.openRepository(decodeURIComponent(path))
            }
        }

        // Base void
        Rectangle { anchors.fill: parent; color: "#050608" }

        // ONE composition: mark fills the viewport (edge-to-edge brand plane)
        Image {
            id: brandMark
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: stage.wide ? parent.width * 0.12 : 0
            width: stage.markSize
            height: stage.markSize
            source: "qrc:/ghew/brand/ghew-logo.png"
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
            opacity: 0
            scale: 1.04
            z: 0
        }

        // Readability veil — darkens left/bottom so copy sits ON the mark, not in empty half
        Rectangle {
            anchors.fill: parent
            z: 1
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#f2050608" }
                GradientStop { position: 0.38; color: "#99050608" }
                GradientStop { position: 0.72; color: "#33050608" }
                GradientStop { position: 1.0; color: "#00000000" }
            }
        }
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: parent.height * 0.42
            z: 1
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#00000000" }
                GradientStop { position: 0.55; color: "#99050608" }
                GradientStop { position: 1.0; color: "#e6050608" }
            }
        }

        // Menu — icon only, top-left
        Controls.ToolButton {
            id: menuBtn
            z: 4
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: 14
            width: 40
            height: 40
            opacity: 0
            onClicked: {
                const win = page.Window.window
                if (win && win.globalDrawer)
                    win.globalDrawer.drawerOpen = true
            }
            contentItem: Kirigami.Icon {
                source: "application-menu"
                color: Theme.paper
                width: 22
                height: 22
                anchors.centerIn: parent
            }
            background: Rectangle {
                radius: Theme.radiusSm
                color: menuBtn.hovered ? "#33c9a227" : "#1ac9a227"
                border.color: menuBtn.hovered ? Theme.copper : "#44c9a227"
                border.width: 1
            }
        }

        // Brand + CTA over the mark
        Item {
            id: hero
            z: 3
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: stage.wide ? Math.max(48, parent.width * 0.07) : 28
            anchors.verticalCenterOffset: stage.wide ? -8 : 40
            width: stage.wide ? Math.min(460, parent.width * 0.42) : parent.width - 56
            opacity: 0

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 0

                Text {
                    text: "GHEW"
                    color: Theme.paper
                    font.family: Theme.displayFamily
                    font.pixelSize: stage.wide ? 84 : 58
                    font.weight: Font.Medium
                    Layout.fillWidth: true
                    style: Text.Outline
                    styleColor: "#66000000"
                }

                Text {
                    Layout.topMargin: 6
                    Layout.fillWidth: true
                    text: "Git Hub Empery World"
                    color: Theme.copperSoft
                    font.family: Theme.uiFamily
                    font.pixelSize: stage.wide ? 18 : 15
                    font.weight: Font.DemiBold
                }

                Text {
                    Layout.topMargin: 4
                    Layout.fillWidth: true
                    text: I18n.i18n("от EmperyWorld")
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: 13
                    opacity: 0.85
                }

                Rectangle {
                    Layout.topMargin: 18
                    Layout.preferredWidth: 56
                    Layout.preferredHeight: 2
                    color: Theme.copper
                }

                Text {
                    Layout.topMargin: 16
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: Theme.paper
                    font.family: Theme.uiFamily
                    font.pixelSize: 15
                    lineHeight: 1.4
                    opacity: 0.92
                    text: I18n.i18n("Локальный Git с пояснением и превью до запуска.")
                }

                ColumnLayout {
                    Layout.topMargin: 26
                    Layout.fillWidth: true
                    spacing: 10

                    Controls.Button {
                        id: openBtn
                        Layout.fillWidth: true
                        Layout.maximumWidth: 360
                        text: I18n.i18n("Открыть репозиторий")
                        onClicked: page.openRepoDialog()
                        background: Rectangle {
                            implicitHeight: 54
                            radius: Theme.radiusSm
                            color: openBtn.down ? Theme.copperSoft
                                   : (openBtn.hovered ? Theme.copperSoft : Theme.copper)
                            Behavior on color { ColorAnimation { duration: 140 } }
                        }
                        contentItem: Row {
                            spacing: 10
                            anchors.centerIn: parent
                            Kirigami.Icon {
                                source: "folder-open"
                                width: 18
                                height: 18
                                color: Theme.ink
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text {
                                text: openBtn.text
                                color: Theme.ink
                                font.family: Theme.uiFamily
                                font.pixelSize: 15
                                font.weight: Font.DemiBold
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        scale: openBtn.down ? 0.98 : 1
                        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }
                    }

                    RowLayout {
                        spacing: 8
                        Layout.fillWidth: true
                        Layout.maximumWidth: 360

                        Controls.Button {
                            id: initBtn
                            Layout.fillWidth: true
                            text: I18n.i18n("Создать")
                            onClicked: page.openInitDialog()
                            background: Rectangle {
                                implicitHeight: 42
                                radius: Theme.radiusSm
                                color: initBtn.hovered ? "#33241808" : "#22000000"
                                border.color: Theme.copper
                                border.width: 1
                            }
                            contentItem: Text {
                                text: initBtn.text
                                color: Theme.copperSoft
                                font.family: Theme.uiFamily
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        Controls.Button {
                            id: cloneBtn
                            Layout.fillWidth: true
                            text: I18n.i18n("Клонировать")
                            onClicked: page.openCloneDialog()
                            background: Rectangle {
                                implicitHeight: 42
                                radius: Theme.radiusSm
                                color: cloneBtn.hovered ? "#33242830" : "#22000000"
                                border.color: "#66b7c2d0"
                                border.width: 1
                            }
                            contentItem: Text {
                                text: cloneBtn.text
                                color: Theme.paper
                                font.family: Theme.uiFamily
                                font.pixelSize: 13
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }

                Text {
                    Layout.topMargin: 20
                    Layout.fillWidth: true
                    text: dropArea.containsDrag
                          ? I18n.i18n("Отпустите, чтобы открыть")
                          : I18n.i18n("Перетащите папку с `.git` сюда")
                    color: dropArea.containsDrag ? Theme.copperSoft : Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: 12
                    opacity: dropArea.containsDrag ? 1 : 0.7
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            visible: dropArea.containsDrag
            color: Theme.copper
            opacity: 0.12
            border.color: Theme.copper
            border.width: 2
            z: 20
        }

        Item {
            id: recentStrip
            visible: App.recentRepos.count > 0
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: Math.min(recentCol.implicitHeight + 22, Math.max(72, stage.height * 0.26))
            opacity: 0
            z: 4
            clip: true

            Rectangle {
                anchors.fill: parent
                color: "#e6050608"
            }
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 1
                color: Theme.copper
                opacity: 0.35
            }

            Controls.ScrollView {
                anchors.fill: parent
                anchors.margins: 8
                clip: true
                contentWidth: availableWidth

            Column {
                id: recentCol
                width: recentStrip.width - 16
                spacing: 8

                Text {
                    text: I18n.i18n("Недавние")
                    color: Theme.mist
                    font.family: Theme.uiFamily
                    font.pixelSize: 10
                    font.letterSpacing: 1.6
                    font.capitalization: Font.AllUppercase
                    opacity: 0.7
                }

                Flow {
                    width: parent.width
                    spacing: 8

                    Repeater {
                        model: App.recentRepos
                        delegate: Item {
                            id: recentDel
                            required property string path
                            required property string name
                            required property string parentDir
                            required property int index
                            width: recentInner.implicitWidth
                            height: 34
                            visible: index < 5

                            Rectangle {
                                id: recentInner
                                anchors.fill: parent
                                implicitWidth: Math.min(280, nameLabel.implicitWidth + pathLabel.implicitWidth + 28)
                                radius: Theme.radiusSm
                                color: recentArea.containsMouse ? "#33c9a227" : "#18000000"
                                border.color: recentArea.containsMouse ? Theme.copper : "#44c9a227"
                                border.width: 1

                                Row {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left
                                    anchors.leftMargin: 10
                                    spacing: 8
                                    Text {
                                        id: nameLabel
                                        text: recentDel.name
                                        color: Theme.paper
                                        font.family: Theme.uiFamily
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                    }
                                    Text {
                                        id: pathLabel
                                        text: recentDel.parentDir
                                        color: Theme.mist
                                        font.family: Theme.monoFamily
                                        font.pixelSize: 11
                                        opacity: 0.75
                                        width: Math.min(implicitWidth, 160)
                                        elide: Text.ElideLeft
                                    }
                                }

                                MouseArea {
                                    id: recentArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                                    cursorShape: Qt.PointingHandCursor
                                    Controls.ToolTip.visible: containsMouse
                                    Controls.ToolTip.text: I18n.i18n("%1\nПКМ — убрать", recentDel.path)
                                    onClicked: function (mouse) {
                                        if (mouse.button === Qt.RightButton)
                                            App.recentRepos.remove(recentDel.path)
                                        else
                                            App.openRepository(recentDel.path)
                                    }
                                }
                            }
                        }
                    }
                }
                }
            }
        }

        ParallelAnimation {
            id: enterAnim
            running: true
            NumberAnimation {
                target: brandMark
                property: "opacity"
                to: 1
                duration: 1000
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: brandMark
                property: "scale"
                to: 1
                duration: 1100
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: hero
                property: "opacity"
                to: 1
                duration: 720
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: menuBtn
                property: "opacity"
                to: 1
                duration: 600
                easing.type: Easing.OutQuad
            }
            NumberAnimation {
                target: recentStrip
                property: "opacity"
                to: 1
                duration: 900
                easing.type: Easing.OutQuad
            }
        }
    }
}
