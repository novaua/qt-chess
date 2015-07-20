import QtQuick 2.3
Row {
    Repeater {
        model: ["A","B","C","D","E","F","G","H"]
        delegate: Item{
            rotation: chessBoard.angle
            width:chessBoxes.width/8
            height:width/4

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
                height: parent.width
                width:height

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text:modelData
                anchors.fill: parent
                fontSizeMode: Text.Fit
                color: markersOfChessBoxColor
                minimumPixelSize: 2
                font.pixelSize: 50

                opacity:0.8
            }
        }
    }
}
