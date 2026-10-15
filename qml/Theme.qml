pragma Singleton
import QtQuick

QtObject {
    id: root

    // Ink & EmperyWorld gold — WCAG-oriented contrast on dark surface.
    readonly property color ink: "#0b0f14"
    readonly property color inkElevated: "#141a22"
    readonly property color inkPanel: "#1c2430"
    readonly property color inkLine: "#3a4658"
    readonly property color mist: "#b7c2d0"
    readonly property color paper: "#f2f5f8"
    readonly property color copper: "#c9a227"
    readonly property color copperSoft: "#e8c547"
    readonly property color safe: "#4db380"
    readonly property color mutates: "#d08a4a"
    readonly property color destructive: "#e06a5a"

    readonly property string displayFamily: "Noto Serif"
    readonly property string uiFamily: "Noto Sans"
    readonly property string monoFamily: "Hack"

    /// Workspace zoom. Persisted via App.saveUiState({ uiScale }).
    property real uiScale: 1.0
    readonly property string uiScaleLabel: Math.round(root.uiScale * 100) + "%"
    readonly property real uiScaleMin: 0.75
    readonly property real uiScaleMax: 1.35
    readonly property real uiScaleStep: 0.05

    function clampUiScale(value) {
        const stepped = Math.round(value / root.uiScaleStep) * root.uiScaleStep
        return Math.min(root.uiScaleMax, Math.max(root.uiScaleMin, Number(stepped.toFixed(2))))
    }

    function setUiScale(value) {
        root.uiScale = root.clampUiScale(value)
    }

    function bumpUiScale(delta) {
        root.setUiScale(root.uiScale + delta)
    }

    function resetUiScale() {
        root.uiScale = 1.0
    }

    function s(base) {
        return Math.max(1, Math.round(base * root.uiScale))
    }

    readonly property int radius: root.s(10)
    readonly property int radiusSm: root.s(6)
    readonly property int controlH: root.s(26)
    readonly property int controlHSm: root.s(22)
    readonly property int labelPx: root.s(10)
    readonly property int bodyPx: root.s(12)

    readonly property int space2: root.s(2)
    readonly property int space4: root.s(4)
    readonly property int space6: root.s(6)
    readonly property int space8: root.s(8)
    readonly property int space12: root.s(12)
    readonly property int space16: root.s(16)
    readonly property int space20: root.s(20)
    readonly property int space24: root.s(24)
    readonly property int space32: root.s(32)

    readonly property int typeDisplay: root.s(22)
    readonly property int typeTitle: root.s(16)
    readonly property int typeBody: root.s(13)
    readonly property int typeMeta: root.s(11)
    readonly property int typeKicker: root.s(10)

    readonly property int sidebarMin: 220
    readonly property int sidebarPref: 320
    readonly property int inspectorMin: 280

    readonly property real mistContrastHint: 8.5
    readonly property real paperContrastHint: 14.0

    function dangerColor(level) {
        if (level === "destructive")
            return root.destructive
        if (level === "mutates")
            return root.mutates
        return root.safe
    }
}
