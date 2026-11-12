import QtQuick

// The ranks along an edge of the board. All labels have the same size (pixelSize: ChessBoard fits it once).
Column {
    id: markerColumn
    property real pixelSize

    Repeater {
        model: ["8","7","6","5","4","3","2","1"]
         delegate: Item{
             rotation: chessBoard.angle
             height: chessBoard.squareSize
             width:height/4

             Behavior on rotation  {
                 enabled: !Theme.reduceMotion
                 NumberAnimation {
                     easing {
                         type: Easing.InCirc
                         amplitude: 6.0
                         period: 30
                     }
                 }
             }

             Text {
                 anchors.centerIn: parent
                 width: parent.width
                 height: width
                 horizontalAlignment: Text.AlignHCenter
                 verticalAlignment: Text.AlignVCenter
                 text:modelData
                 color: Theme.textSecondary
                 font.pixelSize: markerColumn.pixelSize
             }
         }
    }
}
