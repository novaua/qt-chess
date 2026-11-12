import QtQuick
import ChessPlusPlus

// The selected piece's legal moves: a dot on each target square (ChessBoard.squareX/Y). The 64 dots
// stay; a selection only shows and hides them (rebuilding them on every click made slides stop, BL-14).
Item {
    id: navigationLayer
    // Read once per change, not by each of the 64 dots: " " = no move to that square.
    readonly property var moves: ChessConnector.PossibleMoves
    // Also once for all dots (BL-11): PremoveDots changes with every selection.
    readonly property color dotColor: ChessConnector.PremoveDots ? Theme.premoveDot : Theme.legalMoveDot

    Repeater{
        id:navigationLayerRptr
        objectName: "navigationLayerRptr"
        model: 64
        delegate:
            Item{
            id:navigationLayerItm
            required property int index
            x: chessBoard.squareX(index)
            y: chessBoard.squareY(index)
            width: chessBoard.squareSize
            height: width

            Rectangle{
                id:navigationLayerRec
                color: navigationLayer.dotColor

                anchors.centerIn: navigationLayerItm
                smooth: true
                antialiasing:true
                height:0.3*parent.height
                width:height
                radius: 0.5*height
                visible: AppSettings.showLegalMoves && (navigationLayer.moves[navigationLayerItm.index] ?? " ") !== " "
                // Pops in when it appears. Hiding stops it: a quick selection change would otherwise
                // leave an invisible animation running, which keeps the board rendering.
                onVisibleChanged: {
                    if (visible) {
                        if (!Theme.reduceMotion) navigationLayerPopIn.restart()
                    } else {
                        navigationLayerPopIn.stop()
                        navigationLayerRec.scale = 1
                    }
                }
            }

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
        }
    }
}
