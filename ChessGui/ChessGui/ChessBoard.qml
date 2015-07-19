import QtQuick 2.3

Rectangle {
    id: chessBoard
    //colors
    property string brightChessBoxColor: "#ecf0f1"
    property string darkChessBoxColor:"#2c3e50"
    property string markersOfChessBoxColor:"#34495e"
    property string mainWindowColor: "white"
    property string darkFigureColor: "black"
    property string brightFigureColor: "white"
    property string markerBackColor: "#95a5a6"
    property string backRecBackColor: "#bdc3c7"
    property string backRecBorderColor: "#2c3e50"
    property string chessFigureGlow: "blue"

    property int angle:0;
    rotation:angle;
    color: "transparent"

    Behavior on rotation {
        id:mainRecBehavior
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

   Connections {
       target:chessConnector
       //onDoQmlMove:chessPiecesGrid.makeMove(move)

       onCheckNotify:showNotification("Check")

       onCheckMateNotify:showNotification("CheckMate")
   }

   Notificator
   {
       id:notificator
   }

   function showNotification(notificationText)
   {
       notificator.opacity=0.8
       notificator.children[0].text=notificationText
       notificator.resources[0].start()
   }
}

