import QtQuick
import ChessPlusPlus

Grid {
    width: parent.height<parent.width?0.95*parent.height:.95*parent.width
    height:width
    rows: 8
    columns: 8

    transform: Rotation {
        id:chessBoxesTrns
        origin.y: chessBoxes.height/2
        axis.x: 1; axis.y: 0; axis.z: 0
        angle: 180
    }

    Repeater
    {
        id:chessBoxesRptr
        model: 64
        delegate: Rectangle {
            id:chessBox
            smooth: true
            antialiasing: true
            width: (parent.height < parent.width)
                   ? parent.height / 8
                   : parent.width / 8

            height: width

            //rows and columns
            property int rowIndex: Math.floor(index/8)
            property int columnIndex: index%8
            color: ((Math.floor(index / 8) % 2) === 0)
                       ? (index % 2  === 1 ? Theme.squareDark : Theme.squareLight)
                       : (index % 2  === 0 ? Theme.squareDark : Theme.squareLight)

            //highlighting last move
            Rectangle {
                anchors.fill: parent
                color: Theme.lastMove
                opacity: index === ChessConnector.LastMoveTo ? 1 : 0.7
                visible: index === ChessConnector.LastMoveFrom
                         || index === ChessConnector.LastMoveTo
                z: 1
            }

            // King of the side that resigned / lost on time (like Lichess's red king)
            Rectangle {
                anchors.fill: parent
                visible: index === ChessConnector.FlaggedKing
                color: Theme.flaggedKing
                opacity: 0.6
                z: 2
            }

            Text {
                anchors.fill: parent
                id: name
                text: ""
                font.bold: true
                font.pointSize: 14
                rotation: 180 + chessBoard.angle

                transform: Rotation {
                    origin.x: width/2
                    axis.x: 0; axis.y: 1; axis.z: 0
                    angle: 180
                }
            }
        }
    }
}
