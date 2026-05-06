import QtQuick
import QtQuick.Controls
import QtQuick.Controls as QQC

ApplicationWindow {
    id: root

    property string lightChessBoxColor: "#ecf0f1"
    property string darkChessBoxColor:"darkslategray"
    property string markersOfChessBoxColor:"#34495e"
    property string chessFigureGlow: "blue"
    property bool gameIsInProgress: false
    property bool isDarkMode: false
    property variant win

    Component.onCompleted: {
        isDarkMode = (Application.styleHints.colorScheme === Qt.ColorScheme.Dark)
    }

    component Button: QQC.Button {
        id: self
        contentItem: Text {
            text: self.text
            color: root.isDarkMode ? "#ffffff" : "#000000"
            font: self.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            implicitHeight: 24
            color: self.down
                ? (root.isDarkMode ? "#282828" : "#c0c0c0")
                : (root.isDarkMode ? "#3c3c3c" : "#e0e0e0")
            border.color: root.isDarkMode ? "#555555" : "#bbbbbb"
            radius: 4
        }
    }

    title: qsTr("Chess ++")
    color: isDarkMode ? "#1e1e1e" : "#f0f0f0"

    palette: Palette {
        window:          root.isDarkMode ? "#1e1e1e" : "#f0f0f0"
        windowText:      root.isDarkMode ? "#ffffff" : "#000000"
        button:          root.isDarkMode ? "#3c3c3c" : "#e0e0e0"
        buttonText:      root.isDarkMode ? "#ffffff" : "#000000"
        base:            root.isDarkMode ? "#2d2d2d" : "#ffffff"
        text:            root.isDarkMode ? "#ffffff" : "#000000"
        mid:             root.isDarkMode ? "#555555" : "#a0a0a0"
        light:           root.isDarkMode ? "#505050" : "#f8f8f8"
        dark:            root.isDarkMode ? "#282828" : "#c0c0c0"
        highlight:       "#0078d4"
        highlightedText: "#ffffff"
    }

    width: 768
    height: 1054
    visible: true

    Loader { id: dialogLoader }

    Rectangle {
        id: screen
        state: "screen_1"
        anchors.fill: parent

        Image {
            id: background
            anchors.fill: parent
            source: "qrc:///pics/ChessBackground.jpg"
            fillMode: Image.PreserveAspectCrop
        }

        Column {
            anchors.fill: parent

            ChessBoard {
                width: parent.width
                height: parent.height - 30
                id: chessBoard
            }

            Rectangle {
                id: toolBar
                width: parent.width
                height: 30
                color: root.isDarkMode ? "#1e1e1e" : "#f0f0f0"

                Row {
                    id: controlButtons
                    anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                    spacing: 10

                    Button {
                        id: buttonStart
                        text: "Start 2 player game"
                        onClicked: {
                            screen.state = "screen_2"
                            if (chessConnector.IsOnPlayerMode)
                                chessConnector.endGame()
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
                                chessConnector.endGame()
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
                            gameIsInProgress = true
                            console.log("Computer move pressed!")
                        }
                    }

                    Button {
                        id: buttonLoad
                        text: "Load"
                        onClicked: {
                            if (chessConnector.loadGame()) {
                                chessConnector.startNewGame()
                                screen.state = "screen_3"
                                gameIsInProgress = true
                            }
                            console.log("Load pressed!")
                        }
                    }

                    Button {
                        id: buttonStop
                        text: "Stop"
                        onClicked: {
                            gameIsInProgress = false
                            screen.state = "screen_1"
                            chessConnector.endGame()
                            console.log("Game ended!")
                        }
                    }

                    Button {
                        id: buttonSave
                        text: "Save"
                        onClicked: {
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
                    anchors { right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter }
                    spacing: 8

                    Text {
                        id: statusNote
                        color: root.isDarkMode ? "#ffffff" : "#000000"
                        text: chessConnector.MoveCount
                    }

                    Text {
                        id: statusNote1
                        color: root.isDarkMode ? "#ffffff" : "#000000"
                        text: chessConnector.IsWhiteMove ? "white" : "black"
                    }

                    Button {
                        id: themeToggle
                        text: root.isDarkMode ? "🌙 Dark" : "☀️ Light"
                        onClicked: root.isDarkMode = !root.isDarkMode
                    }
                }
            }
        }

        states: [
            State {
                name: "screen_1"
                PropertyChanges { target: buttonStart; visible: true }
                PropertyChanges { target: buttonStartSingle; visible: true }
                PropertyChanges { target: buttonComputerMove; visible: false }
                PropertyChanges { target: buttonLoad; visible: true }
                PropertyChanges { target: buttonStop; visible: false }
                PropertyChanges { target: buttonSave; visible: false }
                PropertyChanges { target: buttonNext; visible: false }
                PropertyChanges { target: buttonPrev; visible: false }
                PropertyChanges { target: themeToggle; visible: true }
                PropertyChanges { target: statusNote; visible: false }
                PropertyChanges { target: statusNote1; visible: false }
            },
            State {
                name: "screen_2"
                PropertyChanges { target: buttonStart; visible: false }
                PropertyChanges { target: buttonStartSingle; visible: false }
                PropertyChanges { target: buttonComputerMove; visible: false }
                PropertyChanges { target: buttonLoad; visible: false }
                PropertyChanges { target: buttonStop; visible: true }
                PropertyChanges { target: buttonSave; visible: true }
                PropertyChanges { target: buttonNext; visible: false }
                PropertyChanges { target: buttonPrev; visible: false }
                PropertyChanges { target: themeToggle; visible: false }
                PropertyChanges { target: statusNote; visible: true }
                PropertyChanges { target: statusNote1; visible: true }
            },
            State {
                name: "screen_3"
                PropertyChanges { target: buttonStart; visible: true }
                PropertyChanges { target: buttonStartSingle; visible: true }
                PropertyChanges { target: buttonComputerMove; visible: false }
                PropertyChanges { target: buttonLoad; visible: true }
                PropertyChanges { target: buttonStop; visible: false }
                PropertyChanges { target: buttonSave; visible: false }
                PropertyChanges { target: buttonNext; visible: true }
                PropertyChanges { target: buttonPrev; visible: true }
                PropertyChanges { target: themeToggle; visible: false }
                PropertyChanges { target: statusNote; visible: true }
                PropertyChanges { target: statusNote1; visible: true }
            },
            State {
                name: "screen_4"
                PropertyChanges { target: buttonStart; visible: false }
                PropertyChanges { target: buttonStartSingle; visible: false }
                PropertyChanges { target: buttonComputerMove; visible: true }
                PropertyChanges { target: buttonLoad; visible: false }
                PropertyChanges { target: buttonStop; visible: true }
                PropertyChanges { target: buttonSave; visible: true }
                PropertyChanges { target: buttonNext; visible: false }
                PropertyChanges { target: buttonPrev; visible: true }
                PropertyChanges { target: themeToggle; visible: false }
                PropertyChanges { target: statusNote; visible: true }
                PropertyChanges { target: statusNote1; visible: true }
            }
        ]
    }
}
