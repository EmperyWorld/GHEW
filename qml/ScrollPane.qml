pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Ghew

// One scroll viewport: contentWidth matches the pane, children lay out vertically.
Item {
    id: root

    clip: true
    Layout.fillWidth: true
    Layout.fillHeight: true

    default property alias contentData: body.data
    property alias spacing: body.spacing

    Controls.ScrollView {
        id: scroller
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            id: body
            width: scroller.availableWidth
            spacing: Theme.space8
        }
    }
}
