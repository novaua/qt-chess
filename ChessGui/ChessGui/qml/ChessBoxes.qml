import QtQuick
import QtQuick.Shapes
import ChessPlusPlus

// The squares with their highlights, placed by ChessBoard.squareX/Y (a1 at the bottom left).
// Each highlight is one item moved to its square, not one per square (BL-11: 6 items on each of the
// 64 squares, a check glow among them, were a sixth of the game page's time to build). They lie over
// all squares in the order they had on a square: last move, premove, check, flagged king, selected,
// drop square.
Item {
    width: chessBoard.squaresSize
    height: width

    Repeater {
        model: 64
        delegate: Rectangle {
            required property int index
            smooth: true
            antialiasing: true
            x: chessBoard.squareX(index)
            y: chessBoard.squareY(index)
            width: chessBoard.squareSize
            height: width
            // index 0 = a1: (rank + file) even is dark
            color: (Math.floor(index / 8) + index % 8) % 2 === 0 ? Theme.squareDark : Theme.squareLight
        }
    }

    // A highlight on one square; hidden without one (-1).
    component SquareMark: Rectangle {
        property int square: -1
        visible: square >= 0
        x: chessBoard.squareX(square)
        y: chessBoard.squareY(square)
        width: chessBoard.squareSize
        height: width
    }

    // The last move
    SquareMark {
        objectName: "lastMoveFromMark"
        square: AppSettings.highlightLastMove ? ChessConnector.LastMoveFrom : -1
        color: Theme.lastMove
        opacity: 0.7
    }
    SquareMark {
        objectName: "lastMoveToMark"
        square: AppSettings.highlightLastMove ? ChessConnector.LastMoveTo : -1
        color: Theme.lastMove
    }

    // The premove waiting for the opponent's move (LI-05), over the last move
    SquareMark {
        square: ChessConnector.PremoveFrom
        color: Theme.premove
    }
    SquareMark {
        square: ChessConnector.PremoveTo
        color: Theme.premove
    }

    // The king in check glows (red-yellow) and pulses, so a missed "Check" message doesn't
    // leave the player wondering why most moves are refused.
    Item {
        readonly property int square: ChessConnector.CheckedKing
        visible: square >= 0
        x: chessBoard.squareX(square)
        y: chessBoard.squareY(square)
        width: chessBoard.squareSize
        height: width

        Shape {
            id: checkGlow
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                strokeWidth: -1
                fillGradient: RadialGradient {
                    centerX: checkGlow.width / 2; centerY: checkGlow.height / 2
                    focalX: centerX; focalY: centerY
                    centerRadius: checkGlow.width * 0.71   // the corners: half the square's diagonal
                    // The king hides the middle: the ring around it carries the colour.
                    GradientStop { position: 0.0;  color: Theme.checkGlowCenter }
                    GradientStop { position: 0.4;  color: Qt.tint(Theme.checkGlowCenter, Qt.alpha(Theme.checkGlowEdge, 0.5)) }
                    GradientStop { position: 0.7;  color: Theme.checkGlowEdge }
                    GradientStop { position: 1.0;  color: Qt.alpha(Theme.checkGlowEdge, 0.4) }
                }
                PathRectangle { width: checkGlow.width; height: checkGlow.height }
            }

            SequentialAnimation on opacity {
                running: checkGlow.visible && !Theme.reduceMotion
                loops: Animation.Infinite
                alwaysRunToEnd: true
                NumberAnimation { from: 1.0; to: 0.7; duration: 700; easing.type: Easing.InOutSine }
                NumberAnimation { from: 0.7; to: 1.0; duration: 700; easing.type: Easing.InOutSine }
            }
        }
    }

    // King of the side that resigned / lost on time (like Lichess's red king)
    SquareMark {
        square: ChessConnector.FlaggedKing
        color: Theme.flaggedKing
        opacity: 0.6
    }

    // The selected piece: accent tint on its square
    SquareMark {
        square: ChessConnector.SelectedSquare
        color: Theme.selected
    }

    // The square under a dragged piece: where it would be dropped
    SquareMark {
        square: chessBoard.dropSquare
        color: "transparent"
        border.color: Theme.dropSquare
        border.width: Math.max(2, Math.round(width * 0.06))
    }
}
