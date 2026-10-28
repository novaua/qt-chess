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
    default property alias actions: actionRow.data

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
        Label {
            text: bar.text
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Row {
            id: actionRow
            spacing: Theme.s2
            visible: children.length > 0
        }
        IconButton {
            dense: true
            visible: bar.closable
            icon.source: Theme.icon("close")
            tip: "Close"
            onClicked: bar.visible = false
        }
    }
}
