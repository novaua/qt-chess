import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

// A non-blocking message under the command bar (WinUI InfoBar). Either bind `visible` (an offer
// that lasts) or call show(text, severity) for a message; `timeout` > 0 hides it after that many ms.
// Action buttons go in as children.
Rectangle {
    id: bar

    property string text
    property string severity: "info"   // info, success, caution, critical
    property int    timeout:  0
    property bool   closable: true
    // The buttons under the text (a long text with several buttons, or a narrow bar: the game page's
    // player column, BL-13); they wrap there when they don't fit in one row.
    property bool   actionsBelow: false
    default property alias actions: actionRow.data
    // The × hides the bar; a bar whose `visible` is bound sets this false and answers closed() instead.
    property bool   hideOnClose: true

    signal closed()

    function show(message, messageSeverity) {
        text = message
        severity = messageSeverity ?? "info"
        visible = true
        if (timeout > 0)
            hideTimer.restart()
    }

    visible: false
    implicitHeight: Math.max(40, content.implicitHeight + 2 * Theme.s2)
    radius: Theme.radiusCard
    color: ({ success: Theme.successBg, caution: Theme.cautionBg, critical: Theme.criticalBg })[severity]
           ?? Theme.cardBg
    border.color: Theme.cardStroke

    Accessible.role: Accessible.AlertMessage
    Accessible.name: text

    Timer {
        id: hideTimer
        interval: bar.timeout
        onTriggered: bar.visible = false
    }

    RowLayout {
        id: content
        anchors {
            left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter
            leftMargin: Theme.s3; rightMargin: bar.closable ? Theme.s1 : Theme.s3
        }
        spacing: Theme.s3

        IconImage {
            source: Theme.icon("info")
            sourceSize.width: Theme.iconSize; sourceSize.height: Theme.iconSize
            color: ({ success: Theme.success, caution: Theme.caution, critical: Theme.critical })[bar.severity]
                   ?? Theme.accent
        }
        GridLayout {
            Layout.fillWidth: true
            columns: bar.actionsBelow ? 1 : 2
            columnSpacing: Theme.s3
            rowSpacing: Theme.s2
            Label {
                text: bar.text
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Flow {
                id: actionRow
                spacing: Theme.s2
                visible: children.length > 0
                // Beside the text: as wide as its visible buttons (a Flow given no width would never
                // wrap, and one given its own implicit width keeps it). Below: the bar's width.
                readonly property real rowWidth: {
                    var w = 0, n = 0
                    for (const c of children)
                        if (c.visible) { w += c.implicitWidth; n++ }
                    return w + Math.max(0, n - 1) * spacing
                }
                Layout.fillWidth: bar.actionsBelow
                Layout.preferredWidth: bar.actionsBelow ? -1 : Math.ceil(rowWidth)
            }
        }
        IconButton {
            dense: true
            visible: bar.closable
            icon.source: Theme.icon("close")
            tip: "Close"
            onClicked: {
                bar.closed()
                if (bar.hideOnClose) bar.visible = false
            }
        }
    }
}
