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

    ChessFiguresModel{
        id: chessFiguresModel
    }

    Repeater{
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

            Behavior on rotation  {
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

                Behavior on opacity  {
                    NumberAnimation {
                        easing {
                            type: Easing.InCirc
                            amplitude: 6.0
                            period: 30
                        }
                    }
                }
            }

            Text{
                id:chessPiecesImg
                anchors.centerIn: chessPiecesItm
                smooth: true
                antialiasing: true
                height: 0.9*parent.height
                width: height
                font.pixelSize: 0.9*parent.height

                text : getPieceCodeSymbol(modelData)
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
                }
            }
        }
    }

    function getPieceCodeSymbol(pieceChar)
    {
        var pieceBaseCode = 9812;
        var blackOffset = (pieceChar===pieceChar.toUpperCase())
                ? 6
                : 0;

        var result = "";

        switch (pieceChar.toUpperCase())
        {
        case "K":
            result =  String.fromCharCode(pieceBaseCode + blackOffset);
            break;
        case "Q":
            result =  String.fromCharCode(pieceBaseCode + blackOffset + 1);
            break;

        case "R":
            result =  String.fromCharCode(pieceBaseCode + blackOffset + 2);
            break;
        case "B":
            result =  String.fromCharCode(pieceBaseCode + blackOffset + 3);
            break;
        case "N":
            result =  String.fromCharCode(pieceBaseCode + blackOffset + 4);
            break;
        case "P":
            result =  String.fromCharCode(pieceBaseCode + blackOffset + 5);
            break;
        default:
        result = "";
        }

       return result;
    }

    function makeMove(move)
    {
        if(move[0]==="0"){
            if(chessFiguresCpp.getMovesCount()%2===0){
                if(move==="0-0"){
                    chessFiguresModel.remove(4)
                    chessFiguresModel.insert(4, {"modelData": " "})

                    chessFiguresModel.remove(6)
                    chessFiguresModel.insert(6, {"modelData": "q"})

                    chessFiguresModel.remove(7)
                    chessFiguresModel.insert(7, {"modelData": " "})

                    chessFiguresModel.remove(5)
                    chessFiguresModel.insert(5, {"modelData": "r"})
                }

                if(move==="0-0-0"){
                    chessFiguresModel.remove(4)
                    chessFiguresModel.insert(4, {"modelData": " "})

                    chessFiguresModel.remove(2)
                    chessFiguresModel.insert(2, {"modelData": "q"})

                    chessFiguresModel.remove(0)
                    chessFiguresModel.insert(0, {"modelData": " "})

                    chessFiguresModel.remove(3)
                    chessFiguresModel.insert(3, {"modelData": "r"})
                }

            }
            if(chessFiguresCpp.getMovesCount()%2===1){
                if(move==="0-0"){
                    chessFiguresModel.remove(60)
                    chessFiguresModel.insert(60, {"modelData": " "})

                    chessFiguresModel.remove(62)
                    chessFiguresModel.insert(62, {"modelData": "Q"})

                    chessFiguresModel.remove(63)
                    chessFiguresModel.insert(63, {"modelData": " "})

                    chessFiguresModel.remove(61)
                    chessFiguresModel.insert(61, {"modelData": "R"})
                }

                if(move==="0-0-0"){
                    chessFiguresModel.remove(60)
                    chessFiguresModel.insert(60, {"modelData": " "})

                    chessFiguresModel.remove(58)
                    chessFiguresModel.insert(58, {"modelData": "Q"})

                    chessFiguresModel.remove(56)
                    chessFiguresModel.insert(56, {"modelData": " "})

                    chessFiguresModel.remove(59)
                    chessFiguresModel.insert(59, {"modelData": "R"})
                }

            }
        }
        else{
            var row=move.charCodeAt(2)-49
            var col=move.charCodeAt(1)-97
            var firstPos=8*row+col
            if(move[3]==="="){
                var pawn=chessFiguresModel.get(firstPos).modelData
                var figureProm
                if(pawn===pawn.toUpperCase())
                    figureProm=move[4]
                else
                    figureProm=move[4].toLowerCase()

                chessFiguresModel.remove(firstPos)
                chessFiguresModel.insert(firstPos, {"modelData": figureProm})
            }
            else{

                row=move.charCodeAt(5)-49
                col=move.charCodeAt(4)-97
                var secondPos=8*row+col
                var figure=chessFiguresModel.get(firstPos).modelData

                //en passant
                if(move[6]==="E"&&move[7]==="."&&move[8]==="P"&&move[9]===".")
                {
                    var enPassantPawnPos=-1

                    if(figure===figure.toUpperCase())
                        row=3;
                    if(figure===figure.toLowerCase())
                        row=4;

                    enPassantPawnPos=8*row+col
                    chessFiguresModel.remove(enPassantPawnPos)
                    chessFiguresModel.insert(enPassantPawnPos, {"modelData": " "})

                }

                chessFiguresModel.remove(firstPos)
                chessFiguresModel.insert(firstPos, {"modelData": " "})

                chessFiguresModel.remove(secondPos)
                chessFiguresModel.insert(secondPos, {"modelData": figure})
            }
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
}
