import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

// Something is under way (BL-16): a spinning ring, or a still clock with Reduce animations (and in
// the screenshot scenes, which reduce them: the same picture every run). Sized by its user.
Item {
    id: ring

    implicitWidth: Theme.iconSizeDense
    implicitHeight: Theme.iconSizeDense

    BusyIndicator {
        anchors.fill: parent
        padding: 0
        visible: !Theme.reduceMotion
        running: visible && ring.visible
    }
    IconImage {
        anchors.fill: parent
        visible: Theme.reduceMotion
        source: Theme.icon("clock")
        sourceSize.width: width
        sourceSize.height: height
        color: Theme.accent
    }
}
