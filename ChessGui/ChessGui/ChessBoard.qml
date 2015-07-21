import QtQuick 2.3

Rectangle {
    id: chessBoard

    property int angle:0;
    rotation:angle;
    color: "transparent"

    Behavior on rotation {
        id:chessBoardBehavior
        NumberAnimation {
            id: chessBoardRotation
            easing {
                type: Easing.InElastic
                amplitude: 1.0
                period: 40
            }
        }
    }

    ChessBoxes{
        id: chessBoxes
        anchors.centerIn: parent
    }

    ChessFigures{
        id:chessPiecesGrid
        objectName:"chessPiecesGrid"
        anchors.centerIn: parent
    }

    MarkerColumn {
        id:markerColumnLeft
        anchors.top: chessBoxes.top
        anchors.topMargin: 0
        anchors.right: chessBoxes.left
        anchors.rightMargin: 0
    }

    MarkerColumn {
        id:markerColumnRight
        anchors.top: chessBoxes.top
        anchors.topMargin: 0
        anchors.left: chessBoxes.right
        anchors.rightMargin: 0
    }

    MarkerRow {
        id:markerRowTop
        antialiasing: true
        anchors.bottom:  chessBoxes.top
        anchors.bottomMargin: 0
        anchors.left:  chessBoxes.left
        anchors.leftMargin: 0
    }

    MarkerRow {
        id:markerRowBottom
        antialiasing: true
        anchors.top:  chessBoxes.bottom
        anchors.bottomMargin: 0
        anchors.left:  chessBoxes.left
        anchors.leftMargin: 0
    }

    Connections {
        target:chessConnector

        onCheckNotify:showNotification("Check")

        onCheckMateNotify:showNotification("CheckMate")

        onNoSavedGame:showNotification("The saved Game was not found!")
        onSavedOk:showNotification("Game saved")
    }

    NavigationLayer{
        id:navigationLayerGrid
        objectName:"navigationLayerGrid"
        anchors.centerIn: parent
    }

    Notificator
    {
        id:notificator
    }

    function showNotification(notificationText)
    {
        notificator.opacity=0.8
        notificator.children[0].text=notificationText
        notificator.resources[0].start()
    }
}

