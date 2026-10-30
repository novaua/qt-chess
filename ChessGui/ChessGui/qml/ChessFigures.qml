import QtQuick
import ChessPlusPlus

// The pieces of ChessConnector.Pieces, each on its square (ChessBoard.squareX/Y). A piece keeps
// its item while it moves, so it can slide (BL-04).
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

            x: chessBoard.squareX(square)
            y: chessBoard.squareY(square)
            width: chessBoard.squareSize
            height: width
            visible: !taken

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
                source: chessPiecesItm.piece === chessPiecesItm.piece.toLowerCase()
                        ? "qrc:/piece/pics/black/" + chessPiecesItm.piece.toUpperCase() + ".png"
                        : "qrc:/piece/pics/white/" + chessPiecesItm.piece.toLowerCase() + ".png"
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

            // Fades in when the piece appears; started once, so changing Reduce animations later
            // doesn't replay it on every piece.
            PropertyAnimation {
                id: chessPiecesFadeIn
                target: chessPiecesItm;
                property: "opacity";
                easing.type: Easing.InOutCirc
                from: 0;
                to: 1.0;
                duration: 450;
                loops: 1;
            }
            Component.onCompleted: if (!Theme.reduceMotion) chessPiecesFadeIn.start()
        }
    }
}
