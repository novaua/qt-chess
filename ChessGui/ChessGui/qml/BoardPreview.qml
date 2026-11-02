import QtQuick
import ChessPlusPlus

// A small, still board drawn from a FEN, e.g. the position of each unfinished game on the Online
// page (LI-04). The game board's squares, pieces and last-move tint; from the player's side
// (flipped: Black at the bottom). Not interactive: put a MouseArea over it.
Item {
    id: preview

    property string fen          // only the piece placement is used
    property bool   flipped: false
    property string lastMove     // UCI, e.g. "e2e4": both squares tinted

    implicitWidth: 160
    implicitHeight: width

    // 64 piece keys ("N", "n", "" for empty), index 0 = a1 … 63 = h8 (the board's own order).
    readonly property var _pieces: {
        var squares = new Array(64).fill("")
        var ranks = (preview.fen || "").split(" ")[0].split("/")
        for (var r = 0; r < Math.min(ranks.length, 8); ++r) {
            var file = 0
            for (var ch of ranks[r]) {
                if (ch >= "1" && ch <= "8") {
                    file += Number(ch)
                } else {
                    if (file < 8)
                        squares[(7 - r) * 8 + file] = ch
                    ++file
                }
            }
        }
        return squares
    }
    function _square(name) {   // "e4" → 28
        return name.length === 2 ? (name.charCodeAt(1) - 49) * 8 + (name.charCodeAt(0) - 97) : -1
    }
    readonly property var _lastMoveSquares: lastMove.length >= 4
        ? [_square(lastMove.substring(0, 2)), _square(lastMove.substring(2, 4))] : []

    Grid {
        id: grid
        readonly property real squareSize: Math.floor(Math.min(preview.width, preview.height) / 8)
        anchors.centerIn: parent
        columns: 8

        Repeater {
            model: 64
            delegate: Rectangle {
                required property int index
                // Top left first: rank 8 (or 1 when flipped) to the bottom, file a (or h) to the right.
                readonly property int rank: preview.flipped ? Math.floor(index / 8) : 7 - Math.floor(index / 8)
                readonly property int file: preview.flipped ? 7 - index % 8 : index % 8
                readonly property int square: rank * 8 + file

                width: grid.squareSize; height: grid.squareSize
                color: (rank + file) % 2 === 0 ? Theme.squareDark : Theme.squareLight   // a1 is dark

                Rectangle {
                    anchors.fill: parent
                    color: Theme.lastMove
                    visible: preview._lastMoveSquares.indexOf(parent.square) >= 0
                }
                Image {
                    anchors.fill: parent
                    anchors.margins: 1
                    source: preview._pieces[parent.square] ? Theme.piece(preview._pieces[parent.square]) : ""
                    sourceSize.width: width; sourceSize.height: height
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }
            }
        }
    }
}
