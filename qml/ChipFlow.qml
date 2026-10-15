pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

// Wrapping chip/action row — width from layout so chips wrap inside the viewport.
Flow {
    Layout.fillWidth: true
    Layout.maximumWidth: parent ? parent.width : undefined
    spacing: Theme.space4
}
