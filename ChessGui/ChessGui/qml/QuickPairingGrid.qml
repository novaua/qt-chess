import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ChessPlusPlus
import "timecontrols.js" as TimeControls

// LI-09: lichess.org's quick pairing grid, on the Online page. Each tile is an action: a click seeks a
// random opponent with that clock (seekRequested; LichessClient.startSeek). The Board API seeks only
// Rapid and Classical: the Bullet and Blitz tiles are greyed, and open lichess.org's lobby (the owner's
// decision D1). The twelfth tile, Custom, asks for the Seek dialog (customRequested, D2): a clock of
// the player's own, or correspondence. While a seek waits (LichessClient.seek), its tile shows a ring and
// the time it has waited (Custom: its clock too), and a click on it cancels the seek; the others wait. One Tab stop: the arrows move between
// the tiles, Enter or Space clicks one.
GridLayout {
    id: grid

    property bool chess960: false    // the ratings shown: the Chess960 pool, else the clock's speed
    property bool available: true    // a seek may start now (no game or challenge starting)
    property real customSince: 0     // the Seek dialog's last real-time seek (its `since`): Custom's

    signal seekRequested(int limitSec, int incSec)
    signal customRequested()

    readonly property var tiles: TimeControls.presets.map(p => Object.assign({
            lichessOnly: p.speed === "Bullet" || p.speed === "Blitz", custom: false }, p))
        .concat([{ label: "Custom", limitSec: 0, incSec: 0, speed: "", lichessOnly: false, custom: true }])
    readonly property var seek: LichessClient.seek
    readonly property bool seeking: LichessClient.seeking
    // The waiting seek's tile: Custom's for the dialog's (even with a tile's clock), else its clock's.
    readonly property int seekingIndex: {
        if (!seeking)
            return -1
        if (seek.since === customSince)
            return tiles.length - 1
        const i = tiles.findIndex(t => !t.custom && t.limitSec === seek.limitSec && t.incSec === seek.incrementSec)
        return i >= 0 ? i : tiles.length - 1
    }
    readonly property int elapsedSec: LichessClient.seekWaitedSec   // counted once, in C++, for every view
    property int current: 6   // the keyboard's tile: 10+0 to start with

    function usable(index) {
        const tile = tiles[index]
        return tile.lichessOnly || index === seekingIndex || (available && !seeking)
    }
    function activate(index) {
        const tile = tiles[index]
        if (index === seekingIndex)
            LichessClient.cancelSeek()
        else if (tile.lichessOnly)
            Qt.openUrlExternally("https://lichess.org/")
        else if (!usable(index))
            return
        else if (tile.custom)
            customRequested()
        else
            seekRequested(tile.limitSec, tile.incSec)
    }
    function move(delta) {
        const next = current + delta
        if (next >= 0 && next < tiles.length)
            current = next
    }
    function ratingText(tile) {
        if (tile.custom)
            return ""
        const perf = UserManager.lichessRatings[chess960 ? "chess960" : tile.speed.toLowerCase()]
        return perf ? String(perf.rating) + (perf.prov ? "?" : "") : ""
    }

    columns: 3
    columnSpacing: Theme.s2
    rowSpacing: Theme.s2
    uniformCellWidths: true
    activeFocusOnTab: true
    Accessible.role: Accessible.List
    Accessible.name: "Quick pairing"
    Keys.onLeftPressed:  move(-1)
    Keys.onRightPressed: move(1)
    Keys.onUpPressed:    move(-columns)
    Keys.onDownPressed:  move(columns)
    Keys.onReturnPressed: activate(current)
    Keys.onEnterPressed:  activate(current)
    Keys.onSpacePressed:  activate(current)

    Repeater {
        model: grid.tiles
        delegate: Rectangle {
            id: tile
            required property var modelData
            required property int index
            readonly property bool isSeeking: grid.seekingIndex === index
            readonly property bool usable: grid.usable(index)
            readonly property string rating: grid.ratingText(modelData)
            readonly property string tip: modelData.lichessOnly
                ? "Bullet and Blitz from this app: only a direct challenge. Play it on lichess.org"
                : isSeeking ? "Cancel seeking"
                : modelData.custom ? "Your own clock, or correspondence" : ""

            objectName: "quickTile_" + (modelData.custom ? "custom" : modelData.label)
            Layout.fillWidth: true
            Layout.preferredHeight: 76
            radius: Theme.radiusCard
            opacity: modelData.lichessOnly ? 0.45 : usable ? 1 : 0.6
            color: isSeeking ? Theme.selectedBg
                 : tileMouse.containsMouse && usable ? Theme.cardBgHover : Theme.layerBg
            border.color: isSeeking ? Theme.accent : Theme.cardStroke
            border.width: isSeeking ? 2 : 1

            Accessible.role: Accessible.Button
            Accessible.name: (modelData.custom ? "Custom" + (isSeeking ? " " + TimeControls.label(grid.seek.limitSec, grid.seek.incrementSec) : "")
                                               : modelData.label + " " + modelData.speed)
                             + (isSeeking ? ", looking for an opponent, " + TimeControls.elapsed(grid.elapsedSec)
                                : modelData.lichessOnly ? ", on lichess.org" : "")
            Accessible.onPressAction: grid.activate(index)

            ToolTip.visible: tip !== "" && tileMouse.containsMouse
            ToolTip.delay: 500
            ToolTip.text: tip

            Column {
                anchors.centerIn: parent
                spacing: 2

                // Custom while its seek waits: that seek's clock ("20+10").
                Text {
                    objectName: "quickTileLabel"
                    text: tile.modelData.custom && tile.isSeeking
                          ? TimeControls.label(grid.seek.limitSec, grid.seek.incrementSec) : tile.modelData.label
                    color: Theme.textPrimary
                    font: tile.modelData.custom && !tile.isSeeking ? Theme.bodyStrong : Theme.subtitle
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                // The speed, or while it seeks: the ring and the time it has waited.
                Text {
                    visible: !tile.isSeeking && text !== ""
                    text: tile.modelData.speed
                    color: Theme.textSecondary
                    font: Theme.caption
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Row {
                    visible: tile.isSeeking
                    spacing: Theme.s1
                    anchors.horizontalCenter: parent.horizontalCenter
                    BusyRing {
                        width: 14
                        height: 14
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        objectName: "quickElapsed"
                        text: TimeControls.elapsed(grid.elapsedSec)
                        color: Theme.textPrimary
                        font: Theme.caption
                    }
                }
                Text {
                    visible: !tile.isSeeking && text !== ""
                    text: tile.rating
                    color: Theme.textSecondary
                    font: Theme.caption
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            FocusRing { visible: grid.activeFocus && grid.current === tile.index }

            MouseArea {
                id: tileMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: tile.usable ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: {
                    grid.current = tile.index
                    grid.activate(tile.index)
                }
            }
        }
    }
}
