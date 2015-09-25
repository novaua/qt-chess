import QtQuick 2.0
Rectangle{
    id:pawnPromotionRec
    radius: 0.125*height
    border.color: lightChessBoxColor
    color: darkChessBoxColor

    property variant playerList

    Behavior on opacity  {
        NumberAnimation {
            easing {
                type: Easing.InCirc
                amplitude: 6.0
                period: 30
            }
        }
    }

    Text {
        id:notificatorText
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top:parent.top
        width: parent.width
        height: parent.height/4
        text:"Select Network Player"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        fontSizeMode: Text.Fit
        color: lightChessBoxColor
        minimumPixelSize: 2
        font.pixelSize: 50
    }

    ListModel {
        id: testModel
        ListElement { modelData: "Joe"}
        ListElement { modelData: "Vitaly"}
    }

    ListView {
        anchors.fill: parent
        model: playerList
        delegate: Component {
            Column {
                Text { text: modelData }
            }
        }

        spacing: 10
    }
/*
    Row{
        id:pawnPromotionRow
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom:parent.bottom
        width: parent.width
        height: 3*parent.height/4

        Repeater{
            id:pawnPromotionRptr
            model:pawnPromotionRec.parent!==null
                  ?((pawnPromotionRec.parent.side) === 2 ? ["Q","R","B","N"]:["q","r","b","n"]):0

            delegate: Image{
                height: pawnPromotionRow.width<pawnPromotionRow.height*4?pawnPromotionRow.width/4:pawnPromotionRow.height
                width: height

                smooth: true
                antialiasing:true
                fillMode: Image.PreserveAspectFit
                source:modelData!=" "? (modelData === modelData.toUpperCase()
                                        ? "pics/black/"+modelData+".png"
                                        : "pics/white/"+modelData+".png")
                                     :"";
                MouseArea{
                    anchors.fill:parent
                    onClicked: {
                        pawnPromotionTimer.start()
                        chessConnector.pawnPromote(pawnPromotionRec.parent.index,modelData)
                    }
                    onPressed: parent.scale=0.8
                    onReleased: parent.scale=1
                }
            }
        }
    }
*/
    Timer {
        id:pawnPromotionTimer
        interval: 1000;
        running: false;
        repeat: false
        onTriggered:{
            parent.opacity=0
            pawnPromotionRec.parent.source=""
        }
    }
}
