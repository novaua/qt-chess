import QtQuick
import QtQuick.Shapes
import ChessPlusPlus

Grid {
    width: Theme.boardSquares * Math.min(parent.width, parent.height)
    height:width
    rows: 8
    columns: 8

    transform: Rotation {
        id:chessBoxesTrns
        origin.y: chessBoxes.height/2
        axis.x: 1; axis.y: 0; axis.z: 0
        angle: 180
    }

    Repeater
    {
        id:chessBoxesRptr
        model: 64
        delegate: Rectangle {
            id:chessBox
            smooth: true
            antialiasing: true
            width: (parent.height < parent.width)
                   ? parent.height / 8
                   : parent.width / 8

            height: width

            // index 0 = a1 (the grid is flipped upside down): row = rank, column = file
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

            Text {
                anchors.fill: parent
                id: name
                text: ""
                font.bold: true
                font.pointSize: 14
                rotation: 180 + chessBoard.angle

                transform: Rotation {
                    origin.x: width/2
                    axis.x: 0; axis.y: 1; axis.z: 0
                    angle: 180
                }
            }
        }
    }
}
