import QtQuick
import QtQuick.Controls

// A button whose action waits for an answer, e.g. from Lichess (BL-16): while `busy` a turning ring
// takes the icon's place and the text says what is going on ("Starting…"). Set idleText / idleIcon,
// not text / icon.source (the busy state replaces them), and disable it while it is busy
// (`enabled`: no second click), as the other buttons of what is starting.
//
// The style's own content (FluentWinUI3's IconLabel: icon beside the text, centred) draws the text,
// so the colours stay the style's; the ring is drawn over the icon's slot, which a blank icon keeps.
// The button is as wide busy as idle (the wider of the two), so its neighbours don't move.
Button {
    id: button

    property bool   busy: false
    property string idleText
    property url    idleIcon
    property string busyText: "Starting…"

    text: busy ? busyText : idleText
    icon.source: busy ? Theme.icon("blank") : idleIcon

    TextMetrics { id: idleMetrics; font: button.font; text: button.idleText }
    TextMetrics { id: busyMetrics; font: button.font; text: button.busyText }
    readonly property real _iconSlot: icon.width + spacing
    readonly property real _idleContent: idleMetrics.advanceWidth + (idleIcon.toString() !== "" ? _iconSlot : 0)
    readonly property real _busyContent: busyMetrics.advanceWidth + _iconSlot
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            Math.ceil(Math.max(_idleContent, _busyContent)) + leftPadding + rightPadding)

    BusyRing {
        visible: button.busy
        // The icon's slot: the content is centred, the icon first (last when mirrored).
        x: {
            const content = button.contentItem
            const start = content.x + Math.max(0, (content.width - content.implicitWidth) / 2)
            const slot = button.mirrored ? start + content.implicitWidth - button.icon.width : start
            return slot + (button.icon.width - width) / 2
        }
        anchors.verticalCenter: button.contentItem.verticalCenter
    }
}
