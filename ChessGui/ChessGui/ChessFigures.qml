import QtQuick 2.3
import QtGraphicalEffects 1.0

Grid{

    width: parent.height<parent.width?0.95*parent.height:.95*parent.width
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
                NumberAnimation {
                    id: chessPiecesRotation
                    easing {
                        type: Easing.InCirc
                        amplitude: 6.0
                        period: 30
                    }
                }
            }

            Glow{
                id:selectItemGlow

                anchors.fill:  chessPiecesImg
                radius: 0.05*height
                samples: radius>16?16:radius
                spread: 0.5
                opacity:0
                color: chessFigureGlow
                source: chessPiecesImg

                Behavior on opacity {
                    NumberAnimation {
                        easing {
                            type: Easing.InCirc
                            amplitude: 6.0
                            period: 30
                        }
                    }
                }
            }

            Image{
                id:chessPiecesImg
                anchors.centerIn: chessPiecesItm
                smooth: true
                antialiasing:true
                height:0.9*parent.height
                width:height
                fillMode: Image.PreserveAspectFit
                source:modelData!=" "?(modelData===modelData.toUpperCase()?
                                           "pics/black/"+modelData+".png":"pics/white/"+modelData+".png"):""
            }

            PropertyAnimation {
                target: chessPiecesItm;
                property: "opacity";
                easing.type: Easing.InOutCirc
                from: 0;
                to: 1.0;
                duration: 450;
                running: true;
                loops: 1;
            }

            MouseArea
            {
                anchors.fill:parent
                onClicked: {
                    changeGlow(index)
                    chessPiecesRptr.chessfigureSelected(index)

                    //connect using signal
                    chessConnector.figureSelected(index);
                }
            }
        }
    }

    Connections {
        target: chessConnector
        onBoardChanged: {
            updateChessFiguresModel(position, newValue)
        }
    }

    function changeGlow(index)
    {
        index=parseInt(index)
        if(chessPiecesRptr.selectedIndex!==-1)
            chessPiecesRptr.itemAt(chessPiecesRptr.selectedIndex).children[0].opacity=0

        chessPiecesRptr.selectedIndex=index
        chessPiecesRptr.itemAt(index).children[0].opacity=1
    }

    function updateChessFiguresModel(index, value)
    {
        chessFiguresModel.remove(index)
        chessFiguresModel.insert(index,  { modelData: value})
    }
}
