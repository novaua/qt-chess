import QtQuick
// The files along an edge of the board. All labels have the same size (pixelSize: ChessBoard fits it once).
Row {
    id: markerRow
    property real pixelSize
    Repeater {
        model: ["A","B","C","D","E","F","G","H"]
        delegate: Item{
            rotation: chessBoard.angle
            width: chessBoard.squareSize
            height:width/4

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
                height: parent.width
                width:height

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text:modelData
                anchors.fill: parent
                color: Theme.textSecondary
                font.pixelSize: markerRow.pixelSize
            }
        }
    }
}
