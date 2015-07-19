import QtQuick 2.3

Rectangle {
    id: screen
    state:"screen_1"

    SystemPalette { id: activePalette }

    Item {
        width: parent.width
        anchors { top: parent.top; bottom: toolBar.top }

        Image {
            id: background
            anchors.fill: parent
            source: "qrc:///pics/ChessBackground.jpg"
            fillMode: Image.PreserveAspectCrop
        }
    }

    Item {
        width: parent.width
        anchors { top: parent.top; bottom: toolBar.top }

        Rectangle {
            width: 70;
            height: screen.height - 30
            anchors.top: parent.top
            anchors.left: parent.left
            id: movesHistory

            Text {
                text:"History!"
            }
        }

        ChessBoard {
            anchors.fill: parent
            id: chessBoard
        }
    }

    Rectangle {
        id: toolBar
        width: parent.width; height: 30
        color: activePalette.window
        anchors.bottom: screen.bottom

        Row {
            id: controlButtons
            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
            spacing: 10

            Button {
                id: buttonStart
                //anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                text: "Start"
                onClicked: {
                    screen.state = "screen_2"
                    chessConnector.startNewGame()
                    console.log("New Game started!")
                }
            }

            Button {
                id: buttonLoad
                text: "Load"
                onClicked: {
                    screen.state = "screen_3"
                    console.log("Load pressed!")
                }
            }

            Button {
                id: buttonStop
                text: "Stop"
                onClicked:
                {
                    screen.state = "screen_1"
                    console.log("This doesn't do anything yet...")
                }
            }

            Button {
                id: buttonSave

                text: "Save"
                onClicked: console.log("This doesn't do anything yet...")
            }

            Button {
                id: buttonNext

                text: "Next"
                onClicked: console.log("This doesn't do anything yet...")
            }

            Button {
                id: buttonPrev

                text: "Prev"
                onClicked: console.log("This doesn't do anything yet...")
            }
        }

        Text {
            id: statusNote
            anchors { right: parent.right; verticalCenter: parent.verticalCenter }
            text: screen.state
        }
    }

    states: [
        State {
            name: "screen_1"
            PropertyChanges { target: buttonStart; visible: true}
            PropertyChanges { target: buttonLoad; visible: true}
            PropertyChanges { target: buttonStop; visible: false}
            PropertyChanges { target: buttonSave; visible: false}
            PropertyChanges { target: buttonNext; visible: false}
            PropertyChanges { target: buttonPrev; visible: false}
        },
        State {
            name: "screen_2"
            PropertyChanges { target: buttonStart; visible: false}
            PropertyChanges { target: buttonLoad; visible: false}
            PropertyChanges { target: buttonStop; visible: true}
            PropertyChanges { target: buttonSave; visible: true}
            PropertyChanges { target: buttonNext; visible: false}
            PropertyChanges { target: buttonPrev; visible: false}
        },

        State {
            name: "screen_3"
            PropertyChanges { target: buttonStart; visible: true}
            PropertyChanges { target: buttonLoad; visible: true}
            PropertyChanges { target: buttonStop; visible: false}
            PropertyChanges { target: buttonSave; visible: false}
            PropertyChanges { target: buttonNext; visible: true}
            PropertyChanges { target: buttonPrev; visible: true}
        }
    ]
}
