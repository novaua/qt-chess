import QtQuick 2.3
import QtQuick.Controls 1.3

ApplicationWindow {

    property string lightChessBoxColor: "#ecf0f1"
    property string darkChessBoxColor:"darkslategray"
    property string markersOfChessBoxColor:"#34495e"
    property string chessFigureGlow: "blue"
    property bool gameIsInProgress : false
    property variant win;

    title: qsTr("Chess ++")
    color: activePalette.window

    width: 768
    height: 1054
    visible: true

    Loader { id: dialogLoader }


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

                        text: "Start 2 player game"
                        onClicked: {
                            screen.state = "screen_2"

                            if (chessConnector.IsOnPlayerMode)
                            {
                                chessConnector.endGame();
                            }

                            chessConnector.startNewGame()
                            gameIsInProgress = true
                            console.log("New Game ")
                        }
                    }

                    Button {
                        id: buttonStartSingle

                        text: "Start single player game"
                        onClicked: {
                            buttonPrev.text = "Prev"
                            screen.state = "screen_4"

                            if (chessConnector.IsOnPlayerMode)
                            {
                                chessConnector.endGame();
                            }

                            chessConnector.startNewGameWithComputer()
                            gameIsInProgress = true
                            console.log("New Game with computer")
                        }
                    }

                    Button {
                        id: buttonComputerMove
                        text: "Computer move"
                        onClicked: {    
                            chessConnector.computerMove()
                            buttonPrev.text = "Undo"
                            screen.state = "screen_4"
                            gameIsInProgress = true;
                           
                            console.log("Computer move pressed!")
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
                                gameIsInProgress = true;
                            }

                            console.log("Load pressed!")
                        }
                    }

                    Button {
                        id: buttonStop
                        text: "Stop"
                        onClicked:
                        {
                            gameIsInProgress = false
                            screen.state = "screen_1"

                            chessConnector.endGame();
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
                        id: buttonPrev

                        text: "Prev"
                        onClicked: {
                            chessConnector.movePrev()
                            console.log("Moved back")
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
                        id: buttonNetworkGame
                        text: "Network Player"

                        onClicked: {

                            dialogLoader.source = "SelectPlayerDialog.qml"
                            console.log("Show net players.")
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
                PropertyChanges { target: buttonStartSingle; visible: true}
                PropertyChanges { target: buttonComputerMove; visible: false}
                PropertyChanges { target: buttonLoad; visible: true}
                PropertyChanges { target: buttonStop; visible: false}
                PropertyChanges { target: buttonSave; visible: false}
                PropertyChanges { target: buttonNext; visible: false}
                PropertyChanges { target: buttonPrev; visible: false}
            },
            State {
                name: "screen_2"
                PropertyChanges { target: buttonStart; visible: false}
                PropertyChanges { target: buttonStartSingle; visible: false}
                PropertyChanges { target: buttonComputerMove; visible: false}
                PropertyChanges { target: buttonLoad; visible: false}
                PropertyChanges { target: buttonStop; visible: true}
                PropertyChanges { target: buttonSave; visible: true}
                PropertyChanges { target: buttonNext; visible: false}
                PropertyChanges { target: buttonPrev; visible: false}
            },

            State {
                name: "screen_3"
                PropertyChanges { target: buttonStart; visible: true}
                PropertyChanges { target: buttonStartSingle; visible: true}
                PropertyChanges { target: buttonComputerMove; visible: false}
                PropertyChanges { target: buttonLoad; visible: true}
                PropertyChanges { target: buttonStop; visible: false}
                PropertyChanges { target: buttonSave; visible: false}
                PropertyChanges { target: buttonNext; visible: true}
                PropertyChanges { target: buttonPrev; visible: true}
            },
            
            State {
                name: "screen_4"
                PropertyChanges { target: buttonStart; visible: false}
                PropertyChanges { target: buttonStartSingle; visible: false}
                PropertyChanges { target: buttonComputerMove; visible: true}
                PropertyChanges { target: buttonLoad; visible: false}
                PropertyChanges { target: buttonStop; visible: true}
                PropertyChanges { target: buttonSave; visible: true}
                PropertyChanges { target: buttonNext; visible: false}
                PropertyChanges { target: buttonPrev; visible: true}
            }
        ]
    }
}
