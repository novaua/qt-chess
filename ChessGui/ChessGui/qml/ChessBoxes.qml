import QtQuick
import QtQuick.Shapes
import ChessPlusPlus

// The squares with their highlights, placed by ChessBoard.squareX/Y (a1 at the bottom left).
Item {
    width: chessBoard.squaresSize
    height: width

    Repeater
    {
        id:chessBoxesRptr
        model: 64
        delegate: Rectangle {
            id:chessBox
            smooth: true
            antialiasing: true
            x: chessBoard.squareX(index)
            y: chessBoard.squareY(index)
            width: chessBoard.squareSize
            height: width

            // index 0 = a1: row = rank, column = file
            property int rowIndex: Math.floor(index/8)
            property int columnIndex: index%8
            color: (rowIndex + columnIndex) % 2 === 0 ? Theme.squareDark : Theme.squareLight   // a1 is dark

            //highlighting last move
            Rectangle {
                anchors.fill: parent
                color: Theme.lastMove
                opacity: index === ChessConnector.LastMoveTo ? 1 : 0.7
                visible: AppSettings.highlightLastMove
                         && (index === ChessConnector.LastMoveFrom || index === ChessConnector.LastMoveTo)
                z: 1
            }

            // The king in check glows (red-yellow) and pulses, so a missed "Check" message doesn't
            // leave the player wondering why most moves are refused.
            Shape {
                id: checkGlow
                anchors.fill: parent
                visible: index === ChessConnector.CheckedKing
                z: 2
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

            // King of the side that resigned / lost on time (like Lichess's red king)
            Rectangle {
                anchors.fill: parent
                visible: index === ChessConnector.FlaggedKing
                color: Theme.flaggedKing
                opacity: 0.6
                z: 3
            }

            // The selected piece: accent tint on its square
            Rectangle {
                anchors.fill: parent
                color: Theme.selected
                visible: index === ChessConnector.SelectedSquare
                z: 4
            }
        }
    }
}
