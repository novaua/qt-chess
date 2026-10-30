import QtQuick
import ChessPlusPlus

// The selected piece's legal moves: a dot on each target square (ChessBoard.squareX/Y).
Item {
    Repeater{
        id:navigationLayerRptr
        objectName: "navigationLayerRptr"
        model: ChessConnector.PossibleMoves
        delegate:
            Item{
            id:navigationLayerItm
            x: chessBoard.squareX(index)
            y: chessBoard.squareY(index)
            width: chessBoard.squareSize
            height: width

            Rectangle{
                id:navigationLayerRec
                color: Theme.legalMoveDot

                anchors.centerIn: navigationLayerItm
                smooth: true
                antialiasing:true
                height:0.3*parent.height
                width:height
                radius: 0.5*height
                scale: 1
                visible: AppSettings.showLegalMoves && modelData !== " "
            }

            // Pops in when the dot appears; started once, like the pieces' fade.
            PropertyAnimation {
                id: navigationLayerPopIn
                target: navigationLayerRec;
                property: "scale";
                easing.type: Easing.InOutCirc
                from: 0;
                to: 1.0;
                duration: 450;
                loops: 1;
            }
            Component.onCompleted: if (!Theme.reduceMotion) navigationLayerPopIn.start()
        }
    }
}
