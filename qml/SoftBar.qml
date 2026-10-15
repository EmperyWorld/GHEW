pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Ghew

// Horizontal chrome that wraps instead of escaping the viewport.
Item {
    id: root

    property alias spacing: flow.spacing
    default property alias contentData: flow.data

    Layout.fillWidth: true
    implicitHeight: flow.implicitHeight
    clip: true

    Flow {
        id: flow
        width: root.width
        spacing: Theme.space6
    }
}
