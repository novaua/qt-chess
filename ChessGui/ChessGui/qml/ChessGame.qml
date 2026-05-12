import QtQuick
import QtQuick.Controls
import QtQuick.Controls as QQC

ApplicationWindow {
    id: root

    property string lightChessBoxColor: "#ecf0f1"
    property string darkChessBoxColor:"darkslategray"
    property string markersOfChessBoxColor:"#34495e"
    property string chessFigureGlow: "blue"
    property bool   gameIsInProgress: false
    property bool   isDarkMode: false
    property variant win
    property string _checkmateWinner: ""
    property bool   _showGameResult: false
    property real   _boardSize: Math.min(chessBoard.width, chessBoard.height) * 0.95
    property real   _panelH: _boardSize / 16
    property var    _avatarList: [
        "wizard","unicorn","sun","flower","rabbit","mouse","girl","boy","ball","star",
        "dragon","rocket","penguin","fox","bear","cat","ninja","pirate","alien","crown"
    ]
    property string _playerAvatar:   "wizard"
    property string _opponentAvatar: "unicorn"

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

    component ModeButton: QQC.Button {
        id: mb
        checkable: true
        implicitWidth: 150
        implicitHeight: 38
        contentItem: Text {
            text: mb.text
            color: !mb.enabled
                ? (root.isDarkMode ? "#666666" : "#aaaaaa")
                : (root.isDarkMode ? "#ffffff" : "#000000")
            font: mb.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 6
            color: mb.checked
                ? "#0078d4"
                : mb.down
                    ? (root.isDarkMode ? "#282828" : "#c0c0c0")
                    : (root.isDarkMode ? "#3c3c3c" : "#e0e0e0")
            border.color: mb.checked ? "#005a9e"
                : (root.isDarkMode ? "#555555" : "#bbbbbb")
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

    Rectangle {
        id: screen
        state: "screen_1"
        anchors.fill: parent

        Image {
            id: background
            anchors.fill: parent
            source: "qrc:/app/pics/ChessBackground.jpg"
            fillMode: Image.PreserveAspectCrop
        }

        Column {
            anchors.fill: parent

            CapturedPanel {
                id: topPanel
                width: _boardSize
                height: _panelH
                anchors.horizontalCenter: parent.horizontalCenter
                visible: gameIsInProgress
                avatarPrimary:  chessConnector.IsOnPlayerMode === 0 && gameIsInProgress
                                ? "https://api.dicebear.com/9.x/bottts/svg?seed=chess-robot"
                                : "https://api.dicebear.com/9.x/adventurer/svg?seed=" + _opponentAvatar
                avatarFallback: chessConnector.IsOnPlayerMode === 0 && gameIsInProgress
                                ? "qrc:/app/pics/avatars/robot.png"
                                : "qrc:/app/pics/avatars/" + _opponentAvatar + ".png"
                pieces: chessConnector.CapturedByDark
            }

            ChessBoard {
                width: parent.width
                height: parent.height - 30 - (gameIsInProgress ? 2 * _panelH : 0)
                id: chessBoard
            }

            CapturedPanel {
                id: bottomPanel
                width: _boardSize
                height: _panelH
                anchors.horizontalCenter: parent.horizontalCenter
                visible: gameIsInProgress
                avatarPrimary:  "https://api.dicebear.com/9.x/adventurer/svg?seed=" + _playerAvatar
                avatarFallback: "qrc:/app/pics/avatars/" + _playerAvatar + ".png"
                pieces: chessConnector.CapturedByLight
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
                }

                Row {
                    anchors { right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter }
                    spacing: 8

                    Text {
                        id: statusNote
                        height: robotIcon.height
                        verticalAlignment: Text.AlignVCenter
                        color: root.isDarkMode ? "#ffffff" : "#000000"
                        text: chessConnector.MoveCount
                    }

                    Text {
                        id: statusNote1
                        height: robotIcon.height
                        verticalAlignment: Text.AlignVCenter
                        color: root.isDarkMode ? "#ffffff" : "#000000"
                        text: chessConnector.IsWhiteMove ? "white" : "black"
                    }

                    Text {
                        id: robotIcon
                        text: "🤖"
                        font.pixelSize: 18
                        verticalAlignment: Text.AlignVCenter
                    }

                    Button {
                        id: themeToggle
                        text: root.isDarkMode ? "🌙 Dark" : "☀️ Light"
                        onClicked: root.isDarkMode = !root.isDarkMode
                    }
                }
            }
        }

        // ── Start menu overlay ─────────────────────────────────────────────
        Rectangle {
            id: startMenu
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -15
            width: 340
            height: menuContent.implicitHeight + 48
            visible: screen.state === "screen_1"
            color: root.isDarkMode ? "#252525" : "#f4f4f4"
            border.color: root.isDarkMode ? "#555555" : "#cccccc"
            border.width: 1
            radius: 12

            Column {
                id: menuContent
                anchors.centerIn: parent
                spacing: 20
                width: parent.width - 48

                Text {
                    text: "Chess ++"
                    font.pixelSize: 26
                    font.bold: true
                    color: root.isDarkMode ? "#ffffff" : "#000000"
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                // Game mode radio group
                ButtonGroup { id: gameModeGroup }

                Grid {
                    columns: 2
                    spacing: 10
                    anchors.horizontalCenter: parent.horizontalCenter

                    ModeButton {
                        id: btnSingle
                        text: "Single Player"
                        ButtonGroup.group: gameModeGroup
                        checked: true
                    }
                    ModeButton {
                        id: btnTwo
                        text: "Two Player"
                        ButtonGroup.group: gameModeGroup
                    }
                    ModeButton {
                        id: btnContinue
                        text: "Continue"
                        ButtonGroup.group: gameModeGroup
                        enabled: chessConnector.CanContinue
                    }
                    ModeButton {
                        id: btnLoad
                        text: "Load"
                        ButtonGroup.group: gameModeGroup
                        enabled: chessConnector.CanLoad
                    }
                }

                // Difficulty — UI only, engine wiring is a future task
                Column {
                    spacing: 6
                    anchors.horizontalCenter: parent.horizontalCenter
                    opacity: btnSingle.checked ? 1.0 : 0.35

                    Text {
                        text: "Level: " + difficultySlider.value.toFixed(0)
                        color: root.isDarkMode ? "#ffffff" : "#000000"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    CustomSlider {
                        id: difficultySlider
                        from: 1; to: 5; value: chessConnector.LastLevel; stepSize: 1
                        width: 260
                        enabled: btnSingle.checked
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }

                // Action buttons
                Row {
                    spacing: 16
                    anchors.horizontalCenter: parent.horizontalCenter

                    Button {
                        text: "Start Game"
                        implicitHeight: 32
                        implicitWidth: 110
                        focus: true
                        font.bold: true
                        background: Rectangle {
                            radius: 4
                            color: parent.down ? "#005a9e" : "#0078d4"
                            border.color: "#005a9e"
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#ffffff"
                            font: parent.font
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            if (btnSingle.checked) {
                                screen.state = "screen_4"
                                chessConnector.startNewGameWithComputer(Math.round(difficultySlider.value))
                                gameIsInProgress = true
                            } else if (btnTwo.checked) {
                                screen.state = "screen_2"
                                chessConnector.startNewGame()
                                gameIsInProgress = true
                            } else if (btnContinue.checked) {
                                var single = chessConnector.continueGame()
                                screen.state = single ? "screen_4" : "screen_2"
                                gameIsInProgress = true
                            } else if (btnLoad.checked) {
                                if (chessConnector.loadGame()) {
                                    screen.state = "screen_3"
                                    gameIsInProgress = true
                                }
                            }
                        }
                    }

                    Button {
                        text: "Exit"
                        implicitHeight: 32
                        implicitWidth: 80
                        onClicked: Qt.quit()
                    }
                }
            }
        }

        Connections {
            target: chessConnector
            function onCheckMateResult(winner) {
                _checkmateWinner = winner
                resultDialogTimer.start()
            }
            function onNewGameStarted() {
                var idx = Math.floor(Math.random() * _avatarList.length)
                _playerAvatar = _avatarList[idx]
                var idx2 = (idx + 1 + Math.floor(Math.random() * (_avatarList.length - 1))) % _avatarList.length
                _opponentAvatar = _avatarList[idx2]
            }
        }

        Timer {
            id: resultDialogTimer
            interval: 1600
            repeat: false
            onTriggered: _showGameResult = true
        }

        GameResultDialog {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -15
            visible: _showGameResult
            winner: _checkmateWinner
            onOkClicked: {
                _showGameResult = false
                gameIsInProgress = false
                chessConnector.endGame()
                screen.state = "screen_1"
            }
        }

        states: [
            State {
                name: "screen_1"
                PropertyChanges { target: buttonStop; visible: false }
                PropertyChanges { target: buttonSave; visible: false }
                PropertyChanges { target: buttonNext; visible: false }
                PropertyChanges { target: buttonPrev; visible: false }
                PropertyChanges { target: themeToggle; visible: true }
                PropertyChanges { target: statusNote; visible: false }
                PropertyChanges { target: statusNote1; visible: false }
                PropertyChanges { target: robotIcon; visible: false }
            },
            State {
                name: "screen_2"
                PropertyChanges { target: buttonStop; visible: true }
                PropertyChanges { target: buttonSave; visible: true }
                PropertyChanges { target: buttonNext; visible: false }
                PropertyChanges { target: buttonPrev; visible: false }
                PropertyChanges { target: themeToggle; visible: false }
                PropertyChanges { target: statusNote; visible: true }
                PropertyChanges { target: statusNote1; visible: true }
                PropertyChanges { target: robotIcon; visible: false }
            },
            State {
                name: "screen_3"
                PropertyChanges { target: buttonStop; visible: true }
                PropertyChanges { target: buttonSave; visible: false }
                PropertyChanges { target: buttonNext; visible: true }
                PropertyChanges { target: buttonPrev; visible: true }
                PropertyChanges { target: themeToggle; visible: false }
                PropertyChanges { target: statusNote; visible: true }
                PropertyChanges { target: statusNote1; visible: true }
                PropertyChanges { target: robotIcon; visible: false }
            },
            State {
                name: "screen_4"
                PropertyChanges { target: buttonStop; visible: true }
                PropertyChanges { target: buttonSave; visible: true }
                PropertyChanges { target: buttonNext; visible: false }
                PropertyChanges { target: buttonPrev; visible: true }
                PropertyChanges { target: themeToggle; visible: false }
                PropertyChanges { target: statusNote; visible: true }
                PropertyChanges { target: statusNote1; visible: true }
                PropertyChanges { target: robotIcon; visible: true }
            }
        ]
    }
}
