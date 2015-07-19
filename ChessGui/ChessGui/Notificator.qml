import QtQuick 2.4

Rectangle{
    anchors.centerIn: parent
    height: parent.height<parent.width?0.125*parent.height:0.125*parent.width
    width:  4*height
    radius: 0.125*height
    border.color: brightChessBoxColor
    opacity:0
    color: darkChessBoxColor

    Behavior on opacity  {
        NumberAnimation {
            easing {
                type: Easing.InCirc
                amplitude: 6.0
                period: 30
            }
        }
    }

    Timer {
        id:notificatorTimer
        interval: 1500;
        running: false;
        repeat: false
        onTriggered:{
            parent.opacity=0
        }
    }

    Text {
        id:notificatorText
        anchors.centerIn: parent
        width: parent.width
        height: width
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        fontSizeMode: Text.Fit
        color: brightChessBoxColor
        minimumPixelSize: 2
        font.pixelSize: 50
    }
}
