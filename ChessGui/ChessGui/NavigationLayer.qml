import QtQuick 2.2
import QtGraphicalEffects 1.0

Grid {

    width: parent.height<parent.width?0.95*parent.height:.95*parent.width
    height:width
    rows:8
    columns: 8

    transform: Rotation {
        id:navigationLayerTrns
        origin.y: chessBoxes.height/2
        axis.x: 1; axis.y: 0; axis.z: 0
        angle: 180
    }

    Repeater{
        id:navigationLayerRptr
        objectName: "navigationLayerRptr"
        model: chessConnector.PossibleMoves
        delegate:
            Item{
            id:navigationLayerItm
            rotation: 180+chessBoard.angle
            width: navigationLayerRptr.parent.height<navigationLayerRptr.parent.width?navigationLayerRptr.parent.height/8:navigationLayerRptr.parent.width/8
            height: width

            transform: Rotation {
                origin.x: width/2
                axis.x: 0; axis.y: 1; axis.z: 0
                angle: 180
            }

            Behavior on rotation  {
                NumberAnimation {
                    id: navigationLayerRotation
                    easing {
                        type: Easing.InCirc
                        amplitude: 6.0
                        period: 30
                    }
                }
            }

            Rectangle{
                id:navigationLayerRec
                color: chessFigureGlow

                anchors.centerIn: navigationLayerItm
                smooth: true
                antialiasing:true
                height:0.2*parent.height
                width:height
                radius: 0.5*height
                opacity: 0.5
                scale:0
                visible: modelData!==" "?true:false

            }

            PropertyAnimation {
                target: navigationLayerRec;
                property: "scale";
                easing.type: Easing.InOutCirc
                from: 0;
                to: 1.0;
                duration: 450;
                running: true;
                loops: 1;
            }
        }
    }
}
