import QtQuick
import ChessPlusPlus

Rectangle {
    id: chessBoard

    property int angle:0;
    rotation:angle;
    color: "transparent"

    Behavior on rotation {
        id:chessBoardBehavior
        NumberAnimation {
            id: chessBoardRotation
            easing {
                type: Easing.InElastic
                amplitude: 1.0
                period: 40
            }
        }
    }

    ChessBoxes{
        id: chessBoxes
        anchors.centerIn: parent
    }

    ChessFigures{
        id:chessPiecesGrid
        objectName:"chessPiecesGrid"
        anchors.centerIn: parent
    }

    MarkerColumn {
        id:markerColumnLeft
        anchors.top: chessBoxes.top
        anchors.topMargin: 0
        anchors.right: chessBoxes.left
        anchors.rightMargin: 0
    }

    MarkerColumn {
        id:markerColumnRight
        anchors.top: chessBoxes.top
        anchors.topMargin: 0
        anchors.left: chessBoxes.right
        anchors.rightMargin: 0
    }

    MarkerRow {
        id:markerRowTop
        antialiasing: true
        anchors.bottom:  chessBoxes.top
        anchors.bottomMargin: 0
        anchors.left:  chessBoxes.left
        anchors.leftMargin: 0
    }

    MarkerRow {
        id:markerRowBottom
        antialiasing: true
        anchors.top:  chessBoxes.bottom
        anchors.bottomMargin: 0
        anchors.left:  chessBoxes.left
        anchors.leftMargin: 0
    }

    // Messages (check, saved, …) are InfoBars in ChessGame.qml, not overlays on the board.
    Connections {
        target: ChessConnector
        function onPawnPromotionNotify(index, side) { showPawnPromotionOptions(index, side) }
    }

    NavigationLayer{
        id:navigationLayerGrid
        objectName:"navigationLayerGrid"
        anchors.centerIn: parent
    }

    Loader{
        id:loader
        anchors.centerIn: parent
        opacity: 0
        width: parent.height<parent.width?0.5*parent.height:0.5*parent.width
        height:   0.4*width
        // Cancel the board's rotation (180° when playing Black) so the dialog stays upright.
        rotation: -chessBoard.angle

        property int index;
        property int side;

        Behavior on opacity  {
            NumberAnimation {
                easing {
                    type: Easing.InCirc
                    amplitude: 6.0
                    period: 30
                }
            }
        }
    }

    function showPawnPromotionOptions(index, side)
    {
        loader.index=index
        loader.side=side
        loader.source="PawnPromotionOptions.qml";
        loader.opacity=0.8
    }
}
