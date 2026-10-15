pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

// Compact health of the open repository: identity, tracking, dirty mix, last commit.
SurfaceCard {
    id: root

    signal focusState(string stateKey)

    property bool expanded: false

    visible: App.repo.isOpen
    goldEdge: App.repo.conflictCount > 0 || App.repo.operationKind.length > 0
    kicker: I18n.i18n("Пульс")
    meta: App.repo.branch || (App.repo.detachedHead ? I18n.i18n("detached") : "")
    pad: Theme.space8

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.space6

        Text {
            text: App.repo.branch.length ? App.repo.branch : I18n.i18n("без ветки")
            color: Theme.paper
            font.family: Theme.displayFamily
            font.pixelSize: Theme.typeTitle
            font.weight: Font.Medium
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        ChipButton {
            visible: App.repo.detachedHead
            label: "DETACHED"
            active: true
            accent: Theme.mutates
            labelColor: Theme.mutates
            tip: I18n.i18n("HEAD не на ветке")
        }
        ChipButton {
            visible: App.repo.bare
            label: "BARE"
            active: true
            accent: Theme.mist
            tip: I18n.i18n("Репозиторий без рабочего дерева")
        }
        ChipButton {
            visible: App.repo.insideSubmodule
            label: I18n.i18n("submodule")
            accent: Theme.copper
            tip: I18n.i18n("Текущий каталог внутри подмодуля")
        }
        ChipButton {
            visible: App.repo.bisectActive
            label: "BISECT"
            active: true
            accent: Theme.copperSoft
            labelColor: Theme.copperSoft
            tip: I18n.i18n("Идёт bisect")
        }
        ChipButton {
            label: root.expanded ? I18n.i18n("свернуть") : I18n.i18n("подробнее")
            active: root.expanded
            tip: I18n.i18n("Трекинг и состав правок")
            onClicked: root.expanded = !root.expanded
        }
    }

    Text {
        Layout.fillWidth: true
        visible: !root.expanded
        elide: Text.ElideRight
        color: Theme.mist
        font.family: Theme.uiFamily
        font.pixelSize: 11
        text: {
            const track = App.repo.hasUpstream
                          ? ("↑" + App.repo.ahead + " ↓" + App.repo.behind)
                          : I18n.i18n("без upstream")
            const dirty = App.repo.dirtyCount > 0
                          ? I18n.i18n("%1 правок", App.repo.dirtyCount)
                          : I18n.i18n("дерево чистое")
            return track + "  ·  " + dirty
                   + (App.repo.lastCommitSubject.length ? "  ·  " + App.repo.lastCommitSubject : "")
        }
    }

    TrackingBar {
        Layout.fillWidth: true
        visible: root.expanded
    }

    DirtyBreakdown {
        Layout.fillWidth: true
        visible: root.expanded
        onFocusState: function (stateKey) { root.focusState(stateKey) }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.space8
        visible: root.expanded && App.repo.lastCommitSubject.length > 0

        Text {
            text: I18n.i18n("HEAD")
            color: Theme.copper
            font.family: Theme.monoFamily
            font.pixelSize: 10
            font.letterSpacing: 0.8
        }
        Text {
            Layout.fillWidth: true
            text: App.repo.lastCommitSubject
            color: Theme.mist
            font.family: Theme.uiFamily
            font.pixelSize: Theme.typeMeta
            elide: Text.ElideRight
        }
    }
}
