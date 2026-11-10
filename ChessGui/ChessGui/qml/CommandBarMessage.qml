import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

// A message over the command bar's free middle (BL-13 step 3): "Check", "Game saved", an offer with
// Accept / Decline, the lost connection. It takes no room, so nothing on the page moves. ChessGame.qml
// lays this item over the free middle (`room` wide), says where the message should centre (`centerX`)
// and whether it wants to show (`active`); it shows only when it `fits`, and the window puts the message
// in its InfoBar otherwise. Where it would cross the title's text, the title is hidden up to it.
// Buttons go in as children.
Item {
    id: msg

    property string text
    property string severity: "info"   // info, success, caution, critical
    property string tooltip: ""        // more about it: on hover, and for screen readers
    property bool   active: false      // the window wants it shown
    property bool   closable: false    // a × (an error stays until closed)
    property bool   clickToClose: false // a click on it puts it away (the short messages)
    property real   room: 0             // the free middle's width; 0: no command bar
    property real   centerX: room / 2   // where it centres, in this item's coordinates
    property real   titleWidth: 0       // the title's text at the free middle's left
    default property alias actions: actionRow.data

    signal closeRequested()

    // Measured from the pill's content, so it is known before it shows; bindings settle before a frame
    // is drawn, so the pill and the window's InfoBar never both show.
    readonly property bool fits: room >= pill.width + 2 * Theme.s2
    readonly property bool showing: active && fits

    opacity: showing ? 1 : 0
    visible: opacity > 0
    Behavior on opacity { enabled: !Theme.reduceMotion; NumberAnimation { duration: Theme.fast } }

    Accessible.role: Accessible.AlertMessage
    Accessible.name: text
    Accessible.description: tooltip

    // Over the title's text: the title is hidden up to the message (the bar's colour), so no cut word shows.
    readonly property bool _overTitle: pill.x < titleWidth + Theme.s3
    Rectangle {
        visible: msg._overTitle
        width: pill.x + pill.width
        height: parent.height
        color: Theme.layerBg
    }

    readonly property bool _hasActions: actionRow.width > 0
    Rectangle {
        id: pill
        x: Math.round(Math.max(Theme.s2, Math.min(msg.centerX - width / 2, msg.room - width - Theme.s2)))
        anchors.verticalCenter: parent.verticalCenter
        width: 2 * Theme.s3 + icon.width + Theme.s2 + label.implicitWidth
               + (msg._hasActions ? Theme.s3 + actionRow.width : 0)
               + (msg.closable ? 2 * Theme.s1 + closeButton.width - Theme.s3 : 0)   // the × in place of the right padding
        height: msg._hasActions ? 40 : 36
        radius: Theme.radiusCard
        color: ({ success: Theme.successBg, caution: Theme.cautionBg, critical: Theme.criticalBg })[msg.severity]
               ?? Theme.cardBg
        border.color: Theme.cardStroke

        // A click on the message (not its buttons) puts a short one away early.
        MouseArea {
            anchors.fill: parent
            enabled: msg.clickToClose
            hoverEnabled: msg.tooltip !== ""
            onClicked: msg.closeRequested()
            ToolTip.visible: msg.tooltip !== "" && containsMouse
            ToolTip.text: msg.tooltip
            ToolTip.delay: 400
        }
        HoverHandler {
            id: hover
            enabled: msg.tooltip !== "" && !msg.clickToClose
        }
        ToolTip.visible: hover.hovered
        ToolTip.text: msg.tooltip
        ToolTip.delay: 400

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
        Row {
            id: actionRow
            x: label.x + label.implicitWidth + Theme.s3
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.s2
        }
        IconButton {
            id: closeButton
            visible: msg.closable
            dense: true
            x: parent.width - width - Theme.s1
            anchors.verticalCenter: parent.verticalCenter
            icon.source: Theme.icon("close")
            tip: "Close"
            onClicked: msg.closeRequested()
        }
    }
}
