import QtQuick
import QtQuick.Controls

// A button whose action waits for an answer, e.g. from Lichess (BL-16): while `busy` a spinning
// ring takes the icon's place and the text says what is going on ("Starting…"). Set idleText /
// idleIcon, not text / icon.source (the busy state replaces them), and disable it while it is busy
// (`enabled`: no second click), as the other buttons of what is starting.
Button {
    id: button

    property bool   busy: false
    property string idleText
    property url    idleIcon
    property string busyText: "Starting…"

    text: busy ? busyText : idleText
    // The icon's slot stays (a blank icon), so the text doesn't jump; the ring is drawn over it.
    icon.source: busy ? Theme.icon("blank") : idleIcon

    BusyRing {
        visible: button.busy
        // Where IconLabel draws the icon: the content centred, the icon first.
        x: button.contentItem.x + Math.max(0, (button.contentItem.width - button.contentItem.implicitWidth) / 2)
           + (width < button.icon.width ? (button.icon.width - width) / 2 : 0)
        anchors.verticalCenter: button.contentItem.verticalCenter
    }
}
