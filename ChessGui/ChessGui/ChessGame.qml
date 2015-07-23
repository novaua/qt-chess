import QtQuick 2.3
import QtQuick.Controls 1.3

ApplicationWindow {

    property string lightChessBoxColor: "#ecf0f1"
    property string darkChessBoxColor:"darkslategray"
    property string markersOfChessBoxColor:"#34495e"
    property string chessFigureGlow: "blue"

    title: qsTr("Chess ++")
    color: activePalette.window

    width: 768
    height: 1054
    visible: true

    Rectangle {
        id: screen
        state:"screen_1"
        anchors.fill: parent

        Image {
            id: background

            anchors.fill: parent
            source: "qrc:///pics/ChessBackground.jpg"
            fillMode: Image.PreserveAspectCrop
        }

        SystemPalette { id: activePalette }
        Column{
            anchors.fill: parent
            ChessBoard {
                width: parent.width
                height:parent.height - 30
                id: chessBoard
            }

            Rectangle {
                id: toolBar
                width: parent.width;
                height: 30
                color: activePalette.window

                Row {
                    id: controlButtons
                    anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                    spacing: 10

                    Button {
                        id: buttonStart

                        text: "Start"
                        onClicked: {
                            screen.state = "screen_2"

                            if (chessConnector.IsOnPlayerMode)
                            {
                                chessConnector.endGame();
                            }

                            chessConnector.startNewGame()
                            console.log("New Game ")
                        }
                    }

                    Button {
                        id: buttonLoad
                        text: "Load"
                        onClicked: {
                            if (chessConnector.loadGame())
                            {
                                chessConnector.startNewGame()
                                screen.state = "screen_3"
                            }

                            console.log("Load pressed!")
                        }
                    }

                    Button {
                        id: buttonStop
                        text: "Stop"
                        onClicked:
                        {
                            chessConnector.endGame();
                            screen.state = "screen_1"
                            console.log("Game ended!")
                        }
                    }

                    Button {
                        id: buttonSave

                        text: "Save"
                        onClicked:{
                            chessConnector.saveGame()
                            console.log("Saved!")
                        }
                    }

                    Button {
                        id: buttonNext

                        text: "Next"
                        onClicked: {
                            chessConnector.moveNext()
                            console.log("Advanced")
                        }
                    }

                    Button {
                        id: buttonPrev

                        text: "Prev"
                        onClicked: {
                            chessConnector.movePrev()
                            console.log("Moved back")
                        }
                    }
                }

                Row {
                    anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                    Text {

                        id: statusNote
                        width: 50
                        text: chessConnector.MoveCount
                    }

                    Text {
                        id: statusNote1
                        width: 50
                        text: chessConnector.IsWhiteMove ? "white":"black"
                    }
                }
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
}
