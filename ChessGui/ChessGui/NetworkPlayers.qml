import QtQuick 2.0

Rectangle {
    id:networkPlayers
    width: 180; height: 200
    Component {
        id: playerDelegate
        Item {
            width: 180; height: 40
            Column {
                Text { text: '<b>Name:</b> ' + modelData }
            }
        }
    }

    Row {
        id: controlButtons
        anchors.fill: parent
        spacing: 10

        ListView {
            anchors.fill: parent

            model:chessConnector.PlayerNames;

            delegate: playerDelegate
            highlight: Rectangle { color: "lightsteelblue"; radius: 5 }
            focus: true
        }

        Button {
            id:okButton
            text:"Ok"
            onClicked:
            {
                networkPlayers.opacity =0
            }
        }
    }
}
