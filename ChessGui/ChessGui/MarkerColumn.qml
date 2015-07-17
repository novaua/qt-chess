import QtQuick 2.3

Column {

    Repeater {
        model: ["8","7","6","5","4","3","2","1"]
         delegate: Item{
             rotation: chessBoard.angle
             height:chessBoxes.height/8
             width:height/4

             Behavior on rotation  {
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
                 fontSizeMode: Text.Fit
                 color: markersOfChessBoxColor
                 minimumPixelSize: 2
                 font.pixelSize: 50

                 opacity:0.8
             }
         }
    }
}
