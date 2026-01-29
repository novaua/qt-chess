import QtQuick
import Qt5Compat.GraphicalEffects
import QtQuick.Effects

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
/*
    MultiEffect {
        id: selectItemGlow
        source: chessPiecesImg
        anchors.fill: chessPiecesItm
        // Enable the shadow effect to create the glow
        shadowEnabled: false
        visible: shadowEnabled
        // Configure the "shadow" to look like a glow
        shadowColor: chessFigureGlow                // cyan The color of the glow
        shadowBlur: 1.0 // Full blur strength (actual radius depends on blurMax)
        //shadowSpread: 1.0 // Ensures the glow is strong near the edges
        shadowScale: 1.1 // No scaling of the shadow
        shadowVerticalOffset: 0 // Center the "shadow" vertically
        shadowHorizontalOffset: 0 // Center the "shadow" horizontally

        // Performance note: blurMax should be set once and not animated
        blurMax: 32.0 // Maximum pixel radius of the blur
    }
*/

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
                visible: gameIsInProgress
                smooth: true
                antialiasing:true
                height:0.9*parent.height
                width:height
                fillMode: Image.PreserveAspectFit
                source:modelData!=" "?(modelData===modelData.toLowerCase()?
                                           "pics/black/"+modelData.toUpperCase()+
                                           ".png":"pics/white/"+modelData.toLowerCase()+".png"):" "
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
            //chessPiecesRptr.itemAt(chessPiecesRptr.selectedIndex).children[0].shadowEnabled=false

        chessPiecesRptr.selectedIndex=index
        chessPiecesRptr.itemAt(index).children[0].opacity=1
        //chessPiecesRptr.itemAt(index).children[0].shadowEnabled=true
    }

    function updateChessFiguresModel(index, value)
    {
        chessFiguresModel.remove(index)
        chessFiguresModel.insert(index,  { modelData: value})
    }
}
