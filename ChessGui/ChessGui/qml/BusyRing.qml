import QtQuick
import QtQuick.Controls.impl

// Something is under way (BL-16): a turning arc, or a still clock with Reduce animations (and in the
// screenshot scenes, which reduce them: the same picture every run). Sized by its user.
// The arc turns with an Animator, on the render thread: it keeps turning while the GUI thread is busy,
// e.g. building the game page behind "Starting the game…".
Item {
    id: ring

    implicitWidth: Theme.iconSizeDense
    implicitHeight: Theme.iconSizeDense

    IconImage {
        id: arc
        anchors.fill: parent
        visible: !Theme.reduceMotion
        source: Theme.icon("spinner")
        sourceSize.width: width
        sourceSize.height: height
        color: Theme.accent
    }
    RotationAnimator {
        target: arc
        from: 0
        to: 360
        duration: 900
        loops: Animation.Infinite
        running: arc.visible && ring.visible
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
