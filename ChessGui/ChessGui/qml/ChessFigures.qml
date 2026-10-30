import QtQuick
import ChessPlusPlus

// The pieces of ChessConnector.Pieces, each on its square (ChessBoard.squareX/Y). A piece keeps
// its item while it moves, so it slides; a captured one fades out (BL-04). Reduce animations:
// pieces jump and appear at once.
Item {
    Repeater {
        id:chessPiecesRptr
        objectName: "chessPiecesRptr"
        model: ChessConnector.Pieces

        delegate: Item {
            id:chessPiecesItm

            required property int pieceId
            required property int square
            required property string piece
            required property bool taken

            // A piece that changes square slides there: slideFile/slideRank are how far (in squares)
            // it still is from it, animated to 0. In squares, not pixels, so resizing the window
            // doesn't slide anything. A new position (model reset) has new items, which fade in.
            property real slideFile: 0
            property real slideRank: 0
            property int lastSquare: -1
            x: chessBoard.squareX(square) + slideFile * chessBoard.squareSize
            y: chessBoard.squareY(square) - slideRank * chessBoard.squareSize
            width: chessBoard.squareSize
            height: width
            z: taken ? 0 : 1   // the piece that captures slides over the one fading out

            onSquareChanged: {
                if (!Theme.reduceMotion && lastSquare >= 0) {
                    // From where it is drawn now, also when a slide is still under way.
                    slideFile += lastSquare % 8 - square % 8
                    slideRank += Math.floor(lastSquare / 8) - Math.floor(square / 8)
                    chessPiecesSlide.restart()
                }
                lastSquare = square
            }
            ParallelAnimation {
                id: chessPiecesSlide
                NumberAnimation {
                    target: chessPiecesItm; property: "slideFile"; to: 0
                    duration: Theme.slow; easing.type: Easing.OutCubic
                }
                NumberAnimation {
                    target: chessPiecesItm; property: "slideRank"; to: 0
                    duration: Theme.slow; easing.type: Easing.OutCubic
                }
                onFinished: chessPiecesItm.shownPiece = chessPiecesItm.piece
            }

            // A promoting pawn stays a pawn until it has arrived (and a queen taken back turns into
            // a pawn once back on the 7th rank). The choice after a pawn's move changes it in place.
            property string shownPiece: piece
            onPieceChanged: Qt.callLater(() => { if (!chessPiecesSlide.running) shownPiece = piece })

            // Fades in when it appears (shown is set once created), fades out when captured.
            property bool shown: false
            opacity: shown && !taken ? 1 : 0
            visible: opacity > 0
            Behavior on opacity {
                enabled: !Theme.reduceMotion
                NumberAnimation { duration: Theme.normal; easing.type: Easing.InOutQuad }
            }
            Component.onCompleted: {
                lastSquare = square
                shown = true
            }

            // The board turns 180° for Black; the piece turns with it the other way, to stay upright.
            rotation: chessBoard.angle
            Behavior on rotation {
                enabled: !Theme.reduceMotion
                NumberAnimation {
                    id: chessPiecesRotation
                    easing {
                        type: Easing.InCirc
                        amplitude: 6.0
                        period: 30
                    }
                }
            }

            Image{
                id:chessPiecesImg
                anchors.centerIn: chessPiecesItm
                smooth: true
                antialiasing:true
                height:0.9*parent.height
                width:height
                fillMode: Image.PreserveAspectFit
                source: Theme.piece(chessPiecesItm.shownPiece)
            }

            // Red flag on the king that resigned / lost on time
            Text {
                visible: chessPiecesItm.square === ChessConnector.FlaggedKing
                anchors { top: parent.top; right: parent.right; margins: parent.width * 0.04 }
                text: "⚑"
                color: Theme.flaggedKing
                style: Text.Outline
                styleColor: Theme.textOnColor
                font.pixelSize: parent.height * 0.36
                font.bold: true
            }
        }
    }
}
