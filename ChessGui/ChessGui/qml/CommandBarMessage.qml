import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

// A short message ("Check", "Game saved") over the command bar's free middle (BL-13 step 3): it takes no
// room, so nothing on the page moves. ChessGame.qml lays this item over the free middle (`room` wide) and
// says where the message should centre (`centerX`); where the message would cross the title's text, the
// title is hidden under the bar's colour while it shows. A message that doesn't fit, or a page without a
// command bar, goes to `fallback` (the InfoBar under the command bar) instead.
Item {
    id: msg

    property string text
    property string severity: "info"   // info, success, caution, critical
    property int    timeout: 4000
    property real   room: 0             // the free middle's width; 0: no command bar
    property real   centerX: room / 2   // where the message centres, in this item's coordinates
    property real   titleWidth: 0       // the title's text at the free middle's left
    property var    fallback: null      // an InfoBar: show(text, severity)
    property bool   shown: false

    // Over the command bar when it fits, else the fallback bar; again when the window's size changes while
    // it shows (the free middle grows or shrinks).
    function show(message, messageSeverity) {
        metrics.text = message   // measured here: a layout's implicit width may lag a text change
        _pillWidth = 2 * Theme.s3 + Theme.iconSize + Theme.s2 + Math.ceil(metrics.advanceWidth)
        text = message
        severity = messageSeverity ?? "info"
        shown = true
        hideTimer.restart()
        _place()
    }
    property real _pillWidth: 0
    readonly property bool fits: room >= _pillWidth + 2 * Theme.s2
    onFitsChanged: if (shown) _place()
    function _place() {
        if (!fallback) return
        if (fits) fallback.visible = false
        else      fallback.show(text, severity)
    }

    opacity: shown && fits ? 1 : 0
    visible: opacity > 0
    Behavior on opacity { enabled: !Theme.reduceMotion; NumberAnimation { duration: Theme.fast } }

    Accessible.role: Accessible.AlertMessage
    Accessible.name: text

    TextMetrics {
        id: metrics
        font: label.font
    }
    Timer {
        id: hideTimer
        interval: msg.timeout
        onTriggered: msg.shown = false   // the fallback bar has its own timeout
    }

    // Over the title's text: the title is hidden up to the message (the bar's colour), so no cut word shows.
    readonly property bool _overTitle: pill.x < titleWidth + Theme.s3
    Rectangle {
        visible: msg._overTitle
        width: pill.x + pill.width
        height: parent.height
        color: Theme.layerBg
    }

    Rectangle {
        id: pill
        x: Math.round(Math.max(Theme.s2, Math.min(msg.centerX - width / 2, msg.room - width - Theme.s2)))
        anchors.verticalCenter: parent.verticalCenter
        width: 2 * Theme.s3 + icon.width + Theme.s2 + label.implicitWidth
        height: 36
        radius: Theme.radiusCard
        color: ({ success: Theme.successBg, caution: Theme.cautionBg, critical: Theme.criticalBg })[msg.severity]
               ?? Theme.cardBg
        border.color: Theme.cardStroke

        IconImage {
            id: icon
            x: Theme.s3
            anchors.verticalCenter: parent.verticalCenter
            source: Theme.icon("info")
            sourceSize.width: Theme.iconSize; sourceSize.height: Theme.iconSize
            color: ({ success: Theme.success, caution: Theme.caution, critical: Theme.critical })[msg.severity]
                   ?? Theme.accent
        }
        Label {
            id: label
            x: icon.x + icon.width + Theme.s2
            anchors.verticalCenter: parent.verticalCenter
            text: msg.text
        }

        // A click puts it away early.
        MouseArea {
            anchors.fill: parent
            onClicked: msg.shown = false
        }
    }
}
