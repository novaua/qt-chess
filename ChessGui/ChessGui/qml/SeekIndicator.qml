import QtQuick
import QtQuick.Controls
import ChessPlusPlus
import "timecontrols.js" as TimeControls

// LI-09 (the owner's decision D5): a quick pairing seek goes on beyond the Online page, and every page's
// command bar (CommandBar) shows it while LichessClient.seek waits: a turning ring (a still clock with
// Reduce animations), the time it has waited beside it when the bar has room (showTime). A click opens
// its menu: what is sought, and Cancel seeking. Lichess's game opens from any page when someone takes it.
ToolButton {
    id: indicator

    property bool showTime: true
    property int fixedElapsed: -1    // screenshot scenes: the waited time shown, the same every run

    readonly property var seek: LichessClient.seek
    readonly property bool seeking: seek.limitSec !== undefined
    property real _now: Date.now()
    readonly property int elapsedSec: fixedElapsed >= 0 ? fixedElapsed
                                    : seeking ? Math.max(0, (_now - seek.since) / 1000) : 0
    readonly property string elapsedText: TimeControls.elapsed(elapsedSec)
    readonly property string seekText: seeking ? TimeControls.seekText(seek) : ""

    objectName: "seekIndicator"
    visible: seeking
    implicitHeight: 40
    Accessible.name: "Looking for an opponent, " + elapsedText
    ToolTip.visible: hovered && !menu.visible
    ToolTip.text: "Looking for an opponent · " + seekText
    ToolTip.delay: 400

    // Ended (paired, cancelled, failed): its menu goes with it.
    onSeekingChanged: if (!seeking) menu.close()

    Timer {
        interval: 250
        repeat: true
        running: indicator.seeking && indicator.fixedElapsed < 0
        triggeredOnStart: true
        onTriggered: indicator._now = Date.now()
    }

    contentItem: Row {
        spacing: Theme.s1
        BusyRing {
            width: Theme.iconSize
            height: Theme.iconSize
            anchors.verticalCenter: parent.verticalCenter
        }
        Label {
            objectName: "seekIndicatorTime"
            visible: indicator.showTime
            text: indicator.elapsedText
            font: Theme.caption
            color: Theme.textSecondary
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    onClicked: menu.popup(indicator, 0, indicator.height)

    Menu {
        id: menu
        objectName: "seekMenu"
        // As wide as its line: a Menu sizes itself to its items, not to a Label.
        width: Math.max(implicitWidth, menuText.implicitWidth + leftPadding + rightPadding)
        Label {
            id: menuText
            objectName: "seekMenuText"
            text: "Looking for an opponent · " + indicator.seekText + " · " + indicator.elapsedText
            font: Theme.body
            color: Theme.textSecondary
            leftPadding: Theme.s3
            rightPadding: Theme.s3
            topPadding: Theme.s2
            bottomPadding: Theme.s2
        }
        MenuSeparator {}
        MenuItem {
            objectName: "cancelSeekItem"
            text: "Cancel seeking"
            icon.source: Theme.icon("close")
            onTriggered: LichessClient.cancelSeek()
        }
    }
}
