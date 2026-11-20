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

    readonly property bool seeking: LichessClient.seeking
    // Counted once, in C++, for every view (each page's bar, the grid's tile): once a second.
    readonly property string elapsedText: seeking ? TimeControls.elapsed(LichessClient.seekWaitedSec) : ""
    readonly property string seekText: seeking ? TimeControls.seekText(LichessClient.seek) : ""

    objectName: "seekIndicator"
    visible: seeking
    implicitHeight: 40
    Accessible.name: "Looking for an opponent, " + elapsedText
    ToolTip.visible: hovered && !menu.visible
    ToolTip.text: "Looking for an opponent · " + seekText
    ToolTip.delay: 400

    // Ended (paired, cancelled, failed): its menu goes with it.
    onSeekingChanged: if (!seeking) menu.close()

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
