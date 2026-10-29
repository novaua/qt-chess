import QtQuick
import ChessPlusPlus

Grid{

    width: Theme.boardSquares * Math.min(parent.width, parent.height)
    height:width
    rows:8
    columns: 8

    transform: Rotation {
        id:chessPiecesTrns
        origin.y: chessBoxes.height/2
        axis.x: 1; axis.y: 0; axis.z: 0
        angle: 180
    }

    EmptyBoardModel {
        id: chessFiguresModel
    }

    Repeater {
        id:chessPiecesRptr
        objectName: "chessPiecesRptr"

        property int selectedIndex: -1
        signal chessfigureSelected(int index)

        model:chessFiguresModel

        delegate: Item{
            id:chessPiecesItm

            rotation: 180+chessBoard.angle
            width: chessPiecesRptr.parent.height<chessPiecesRptr.parent.width?
                       chessPiecesRptr.parent.height/8:chessPiecesRptr.parent.width/8
            height: width

            transform: Rotation {
                origin.x: width/2
                axis.x: 0; axis.y: 1; axis.z: 0
                angle: 180
            }

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
            // Selected piece: accent tint on its square
            Rectangle {
                anchors.fill: parent
                color: Theme.selected
                visible: chessPiecesRptr.selectedIndex === index && modelData !== " "
            }

            Image{
                id:chessPiecesImg
                anchors.centerIn: chessPiecesItm
                smooth: true
                antialiasing:true
                height:0.9*parent.height
                width:height
                fillMode: Image.PreserveAspectFit
                source:(modelData!=" " ? (modelData===modelData.toLowerCase()
                        ?"qrc:/piece/pics/black/"+modelData.toUpperCase()+".png"
                        :"qrc:/piece/pics/white/"+modelData.toLowerCase()+".png")
                    :"")
            }

            // Red flag on the king that resigned / lost on time
            Text {
                visible: index === ChessConnector.FlaggedKing
                anchors { top: parent.top; right: parent.right; margins: parent.width * 0.04 }
                text: "⚑"
                color: Theme.flaggedKing
                style: Text.Outline
                styleColor: Theme.textOnColor
                font.pixelSize: parent.height * 0.36
                font.bold: true
            }

            PropertyAnimation {
                target: chessPiecesItm;
                property: "opacity";
                easing.type: Easing.InOutCirc
                from: 0;
                to: 1.0;
                duration: 450;
                running: !Theme.reduceMotion;
                loops: 1;
            }

            MouseArea
            {
                anchors.fill:parent
                onClicked: {
                    chessPiecesRptr.selectedIndex = index
                    chessPiecesRptr.chessfigureSelected(index)

                    //connect using signal
                    ChessConnector.figureSelected(index);
                }
            }
        }
    }

    Connections {
        target: ChessConnector
        function onBoardChanged(position, newValue) {
            chessPiecesRptr.selectedIndex = -1   // a move was made: nothing is selected any more
            updateChessFiguresModel(position, newValue)
        }
    }

    // The game page is created when a game starts or continues: fill in the current position.
    Component.onCompleted: ChessConnector.refreshBoard()

    function updateChessFiguresModel(index, value)
    {
        if (chessFiguresModel.get(index).modelData === value) return
        chessFiguresModel.remove(index)
        chessFiguresModel.insert(index,  { modelData: value})
    }
}
