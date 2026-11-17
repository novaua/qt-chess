import QtQuick
import QtQuick.Shapes
import ChessPlusPlus

// The analysis's two arrows on a judged move (BL-19 follow-up, ChessConnector.ShownAnalysis), both from
// the position before it: the move played, in its judgement's colour (?! blue, ? orange, ?? red, as
// the marks), and the engine's best move instead, in green. Over the pieces, under the move dots; they
// take no input. Placed by ChessBoard.squareX/Y, so they turn with the board.
Item {
    id: arrows
    readonly property var shown: ChessConnector.ShownAnalysis

    // An arrow from the middle of one square to near the middle of another.
    component Arrow: Shape {
        id: arrow
        property int from: -1
        property int to: -1
        property color color
        readonly property real size: chessBoard.squareSize
        readonly property real x1: chessBoard.squareX(from) + size / 2
        readonly property real y1: chessBoard.squareY(from) + size / 2
        readonly property real x2: chessBoard.squareX(to) + size / 2
        readonly property real y2: chessBoard.squareY(to) + size / 2
        readonly property real length: Math.hypot(x2 - x1, y2 - y1) - size * 0.12   // the tip short of the middle
        readonly property real shaft: size * 0.09   // half widths
        readonly property real head: size * 0.25
        readonly property real headLength: Math.min(size * 0.4, length / 2)

        visible: from >= 0 && to >= 0 && from !== to
        // Drawn along x from the first square's middle, then turned towards the second.
        x: x1
        y: y1
        transformOrigin: Item.TopLeft
        rotation: Math.atan2(y2 - y1, x2 - x1) * 180 / Math.PI
        opacity: 0.8
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeWidth: -1
            fillColor: arrow.color
            PathPolyline {
                path: {
                    const l = arrow.length, s = arrow.shaft, h = arrow.head, b = arrow.length - arrow.headLength
                    return [Qt.point(0, -s), Qt.point(b, -s), Qt.point(b, -h), Qt.point(l, 0),
                            Qt.point(b, h), Qt.point(b, s), Qt.point(0, s), Qt.point(0, -s)]
                }
            }
        }
    }

    Arrow {
        objectName: "playedMoveArrow"
        from: arrows.shown.playedFrom ?? -1
        to: arrows.shown.playedTo ?? -1
        color: Theme.markArrowColor(arrows.shown.mark ?? "")
    }
    Arrow {   // over the move played: what to look at
        objectName: "bestMoveArrow"
        from: arrows.shown.bestFrom ?? -1
        to: arrows.shown.bestTo ?? -1
        color: Theme.bestMoveArrow
    }
}
