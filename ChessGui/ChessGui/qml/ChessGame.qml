import QtQuick
import QtQuick.Controls
import QtQuick.Controls as QQC
import QtCore
import QtMultimedia
import Qt5Compat.GraphicalEffects

ApplicationWindow {
    id: root

    property string lightChessBoxColor: "#ecf0f1"
    property string darkChessBoxColor:"darkslategray"
    property string markersOfChessBoxColor:"#34495e"
    property string chessFigureGlow: "blue"
    property bool   gameIsInProgress: false
    property bool   isDarkMode: appSettings.darkMode
    property variant win
    property string _checkmateWinner: ""
    property bool   _showGameResult:    false
    property bool   _showSettings:      false
    property bool   _showMoveInput:     false
    property bool   _showProfile:             false
    property bool   _showPlayerPicker:        false
    property bool   _pendingPlayerPlaysWhite: true
    property bool   _showOnlineDialog:        false
    property bool   _showChallengeDialog:     false
    property string _pendingChallengeId:      ""
    property string _pendingChallengeUrl:     ""
    property string _onlineGameId:            ""
    property real   _boardSize: Math.min(chessBoard.width, chessBoard.height) * 0.95
    // _panelH is computed analytically to avoid a binding loop:
    //   _boardSize → chessBoard.height → centralItem.height → _panelH → _boardSize
    // Portrait (width-limited):  panelH = rootW * 0.95 / 16
    // Landscape (height-limited): solving panelH = (rootH-30-2*panelH)*0.95/16
    //   gives panelH = (rootH-30)*0.95 / (16*(1+0.95/8)) = (rootH-30)*0.95/17.9
    property real   _panelH: {
        if (!gameIsInProgress) return 0
        var effH = root.height - 30
        return (root.width * 1.11875 < effH)
            ? root.width * 0.95 / 16    // portrait: board is width-limited
            : effH * 0.95 / 17.9        // landscape: board is height-limited
    }
    readonly property real _historyPanelW: 160
    readonly property bool _showHistoryPanel: gameIsInProgress && (root.width > root.height * 1.15)

    function clearPendingChallenge() {
        userManager.clearPendingChallenge()
        _pendingChallengeId  = ""
        _pendingChallengeUrl = ""
        _showChallengeDialog = false
    }

    Settings {
        property alias width:  root.width
        property alias height: root.height
        property alias x:      root.x
        property alias y:      root.y
    }

    Settings {
        id: appSettings
        property bool darkMode:     false
        property bool musicEnabled: true
    }

    SoundEffect { id: sndMove;         source: "qrc:/sounds/move.wav" }
    SoundEffect { id: sndMoveOpponent; source: "qrc:/sounds/move_opponent.wav" }
    SoundEffect { id: sndCheck;     source: "qrc:/sounds/check.wav" }
    SoundEffect { id: sndCheckmate; source: "qrc:/sounds/checkmate.wav" }
    SoundEffect { id: sndWin;       source: "qrc:/sounds/win.wav" }

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

                    Button {
                        id: buttonRobotMove
                        text: "Robot"
                        enabled: !chessConnector.EngineThinking
                        onClicked: chessConnector.robotMove()
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

                    Rectangle {
                        id: menuButton
                        width: 28; height: 28
                        radius: 6
                        color:        root.isDarkMode ? "#3c3c3c" : "#e0e0e0"
                        border.color: root.isDarkMode ? "#555555" : "#bbbbbb"
                        border.width: 1
                        anchors.verticalCenter: parent.verticalCenter

                        Image {
                            id: menuIconImg
                            anchors.centerIn: parent
                            width: 16; height: 16
                            source: "qrc:/app/pics/menu_icon.svg"
                            visible: false
                        }
                        ColorOverlay {
                            anchors.fill: menuIconImg
                            source: menuIconImg
                            color: root.isDarkMode ? "#ffffff" : "#09102B"
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: appMenu.popup()
                        }

                        QQC.Menu {
                            id: appMenu
                            QQC.MenuItem {
                                text: "Settings"
                                onTriggered: _showSettings = true
                            }
                            QQC.MenuItem {
                                text: "Profile"
                                onTriggered: _showProfile = true
                            }
                            QQC.MenuItem {
                                text: "Switch User"
                                onTriggered: {
                                    chessConnector.endGame()
                                    userManager.logout()
                                }
                            }
                        }
                    }

                }
            }

            CapturedPanel {
                id: topPanel
                width: _boardSize
                height: _panelH
                anchors.horizontalCenter: parent.horizontalCenter
                visible: gameIsInProgress
                avatarUrl: avatarProvider.opponentUrl
                pieces: chessConnector.PlayerPlaysWhite ? chessConnector.CapturedByDark : chessConnector.CapturedByLight
                isActive: gameIsInProgress && (chessConnector.IsWhiteMove === 1) !== chessConnector.PlayerPlaysWhite
            }

            Item {
                width: parent.width
                height: parent.height - 30 - (gameIsInProgress ? 2 * _panelH : 0)

                ChessBoard {
                    id: chessBoard
                    anchors.fill: parent
                }

                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    visible: chessConnector.EngineThinking
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.WaitCursor
                    }
                }

                MoveHistoryPanel {
                    id: moveHistoryPanel
                    visible: _showHistoryPanel
                    width: _historyPanelW
                    anchors { top: parent.top; bottom: parent.bottom; right: parent.right; margins: 6 }
                }
            }

            CapturedPanel {
                id: bottomPanel
                width: _boardSize
                height: _panelH
                anchors.horizontalCenter: parent.horizontalCenter
                visible: gameIsInProgress
                avatarUrl: avatarProvider.playerUrl
                pieces: chessConnector.PlayerPlaysWhite ? chessConnector.CapturedByLight : chessConnector.CapturedByDark
                isActive: gameIsInProgress && (chessConnector.IsWhiteMove === 1) === chessConnector.PlayerPlaysWhite
            }
        }

        // ── Start menu overlay ─────────────────────────────────────────────
        Rectangle {
            id: startMenu
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -15
            width: 360
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

                Column {
                    spacing: 10
                    anchors.horizontalCenter: parent.horizontalCenter

                    Row {
                        spacing: 10
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
                    }

                    ModeButton {
                        id: btnOnline
                        text: "Online ♟"
                        implicitWidth: 310
                        ButtonGroup.group: gameModeGroup
                    }

                    Row {
                        spacing: 10
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
                }

                // Color selection
                Row {
                    spacing: 10
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: btnSingle.checked || btnTwo.checked

                    ButtonGroup { id: colorGroup }

                    ModeButton {
                        id: btnColorWhite
                        text: "White ♔"
                        implicitWidth: 120
                        ButtonGroup.group: colorGroup
                        checked: chessConnector.PlayerPlaysWhite
                    }
                    ModeButton {
                        id: btnColorBlack
                        text: "Black ♚"
                        implicitWidth: 120
                        ButtonGroup.group: colorGroup
                        checked: !chessConnector.PlayerPlaysWhite
                    }
                }

                // Difficulty
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
                            if (btnOnline.checked) {
                                if (userManager.hasPendingChallenge) {
                                    _pendingChallengeId  = userManager.pendingChallengeId
                                    _pendingChallengeUrl = userManager.pendingChallengeUrl
                                    _showChallengeDialog = true
                                    lichessClient.waitForGameStart(_pendingChallengeId)
                                } else {
                                    _showOnlineDialog = true
                                }
                                return
                            }
                            if (btnSingle.checked) {
                                chessConnector.setPlayerPlaysWhite(btnColorWhite.checked)
                                screen.state = "screen_4"
                                chessConnector.startNewGameWithComputer(Math.round(difficultySlider.value))
                                gameIsInProgress = true
                            } else if (btnTwo.checked) {
                                if (userManager.userCount >= 2) {
                                    _pendingPlayerPlaysWhite = btnColorWhite.checked
                                    _showPlayerPicker = true
                                } else {
                                    chessConnector.setPlayerPlaysWhite(btnColorWhite.checked)
                                    screen.state = "screen_2"
                                    chessConnector.startNewGame()
                                    gameIsInProgress = true
                                    chessBoard.angle = chessConnector.PlayerPlaysWhite ? 0 : 180
                                }
                            } else if (btnContinue.checked) {
                                var single = chessConnector.continueGame()
                                screen.state = single ? "screen_4" : "screen_2"
                                gameIsInProgress = true
                                chessBoard.angle = chessConnector.PlayerPlaysWhite ? 0 : 180
                            } else if (btnLoad.checked) {
                                if (chessConnector.loadGame()) {
                                    screen.state = "screen_3"
                                    gameIsInProgress = true
                                    chessBoard.angle = chessConnector.PlayerPlaysWhite ? 0 : 180
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
            function onMoveCountChanged() {
                if (!appSettings.musicEnabled || !gameIsInProgress || chessConnector.MoveCount === 0) 
                    return
                var snd = (chessConnector.MoveCount % 2 === 1) ? sndMove : sndMoveOpponent
                if (snd.status === SoundEffect.Ready) 
                    snd.play()
            }

            function onCheckNotify() {
                if (appSettings.musicEnabled) 
                    sndCheck.play()
            }

            function onCheckMateNotify() {
                if (appSettings.musicEnabled) 
                    sndCheckmate.play()
            }
            function onCheckMateResult(winner) {
                var isWhite = winner === "White Won"
                var name = isWhite ? avatarProvider.playerName : avatarProvider.opponentName
                _checkmateWinner = (isWhite ? "White " : "Black ") + name + " Won"
                resultDialogTimer.start()
                if (appSettings.musicEnabled) 
                    sndWin.play()
            }
        }

        Connections {
            target: lichessClient

            function onChallengeCreated(gameId, joinUrl) {
                _pendingChallengeId  = gameId
                _pendingChallengeUrl = joinUrl
                userManager.savePendingChallenge(gameId, joinUrl)
                _showOnlineDialog    = false
                _showChallengeDialog = true
            }

            function onChallengeCanceled() {
                clearPendingChallenge()
                _showOnlineDialog = true
            }

            function onGameStarted(gameId, playingAsWhite, opponentName, opponentAvatarUrl) {
                clearPendingChallenge()
                _onlineGameId = gameId
                chessConnector.startOnlineGame(gameId, playingAsWhite)
                avatarProvider.setOpponentFromUser(opponentAvatarUrl !== "" ? opponentAvatarUrl : "unicorn")
                screen.state = "screen_2"
                gameIsInProgress = true
                chessBoard.angle = chessConnector.PlayerPlaysWhite ? 0 : 180
                _showOnlineDialog = false
            }

            function onGameEnded(status, winner) {
                lichessClient.stopStream()
                _onlineGameId = ""

                const winnerName = ({ "white": "White", "black": "Black" })[winner] ?? ""
                const reason     = ({ "mate": " by Checkmate", "resign": " by Resignation",
                                      "outoftime": " on Time" })[status] ?? ""

                if      (status === "aborted") _checkmateWinner = "Game Aborted"
                else if (winnerName !== "")    _checkmateWinner = winnerName + " Won" + reason
                else                           _checkmateWinner = "Draw"

                resultDialogTimer.start()
            }

            function onNetworkError(message) {
                if (_showChallengeDialog) {
                    clearPendingChallenge()
                    _showOnlineDialog = true
                } else if (_onlineGameId !== "") {
                    networkErrorBanner.show(message)
                }
            }
        }

        Connections {
            target: chessConnector
            function onNewGameStarted(isComputerGame) {
                chessBoard.angle = chessConnector.PlayerPlaysWhite ? 0 : 180
            }
            function onPlayerPlaysWhiteChanged() {
                if (gameIsInProgress)
                    chessBoard.angle = chessConnector.PlayerPlaysWhite ? 0 : 180
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

        // ── Settings dialog ───────────────────────────────────────────────
        SettingsDialog {
            id: settingsDialog
            anchors.centerIn: parent
            visible: _showSettings
            z: 20
            onCloseRequested: _showSettings = false
        }

        // ── Move-input cheat dialog (Alt+S in two-player mode) ────────────
        Shortcut {
            sequence: "Alt+S"
            enabled: screen.state === "screen_2"
            onActivated: {
                _showMoveInput = true
                applyMovesDialog.focusInput()
            }
        }

        ApplyMovesDialog {
            id: applyMovesDialog
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -15
            visible: _showMoveInput
            z: 20
            onCloseRequested: _showMoveInput = false
        }

        // ── Lichess network error banner ─────────────────────────────────────
        Rectangle {
            id: networkErrorBanner
            anchors { top: parent.top; topMargin: 8; horizontalCenter: parent.horizontalCenter }
            width: Math.min(parent.width - 32, 420)
            height: bannerText.implicitHeight + 20
            radius: 8
            color: "#c0392b"
            visible: false
            z: 30

            function show(msg) {
                bannerText.text = "⚠ " + msg
                visible = true
                bannerTimer.restart()
            }

            Text {
                id: bannerText
                anchors { centerIn: parent; margins: 10 }
                width: parent.width - 20
                color: "#ffffff"
                font.pixelSize: 13
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }

            Timer {
                id: bannerTimer
                interval: 4000
                onTriggered: networkErrorBanner.visible = false
            }

            MouseArea {
                anchors.fill: parent
                onClicked: networkErrorBanner.visible = false
            }
        }

        // ── Lichess online dialog ────────────────────────────────────────────
        LichessGameDialog {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -15
            visible: _showOnlineDialog
            z: 20
            onCloseRequested: _showOnlineDialog = false
        }

        // ── Pending challenge dialog ─────────────────────────────────────────
        ChallengeDialog {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -15
            visible: _showChallengeDialog
            z: 20
            challengeId:  _pendingChallengeId
            challengeUrl: _pendingChallengeUrl
        }

        // ── Profile dialog ───────────────────────────────────────────────────
        CreateUserDialog {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -15
            visible: _showProfile
            z: 20
            editMode: true
            initialName:   userManager.activeUserName
            initialAvatar: userManager.activeUserAvatar
            onConfirmed: function(name, avatar) {
                userManager.updateProfile(name, avatar)
                _showProfile = false
            }
            onCancelled: _showProfile = false
        }

        // ── Second-player picker (two-player with ≥2 users) ─────────────────
        SelectPlayerDialog {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -15
            visible: _showPlayerPicker
            z: 20
            onConfirmed: function(opponentAvatarName) {
                _showPlayerPicker = false
                chessConnector.setPlayerPlaysWhite(_pendingPlayerPlaysWhite)
                screen.state = "screen_2"
                chessConnector.startNewGame()
                if (opponentAvatarName !== "")
                    avatarProvider.setOpponentFromUser(opponentAvatarName)
                gameIsInProgress = true
                chessBoard.angle = chessConnector.PlayerPlaysWhite ? 0 : 180
            }
            onCloseRequested: _showPlayerPicker = false
        }

        // ── Login / Create-user overlay (shown when not logged in) ──────────
        Rectangle {
            id: authOverlay
            anchors.fill: parent
            z: 50
            visible: !userManager.isLoggedIn
            color: "transparent"

            property bool _showCreate: !userManager.hasUsers

            UserLoginScreen {
                anchors.fill: parent
                visible: !authOverlay._showCreate
                onAddUserRequested: authOverlay._showCreate = true
            }

            Item {
                anchors.fill: parent
                visible: authOverlay._showCreate

                Rectangle {
                    anchors.fill: parent
                    color: root.isDarkMode ? "#1e1e1e" : "#f0f0f0"
                    Image {
                        anchors.fill: parent
                        source: "qrc:/app/pics/ChessBackground.jpg"
                        fillMode: Image.PreserveAspectCrop
                        opacity: 0.35
                    }
                }

                CreateUserDialog {
                    anchors.centerIn: parent
                    anchors.verticalCenterOffset: -20
                    editMode: false
                    onConfirmed: function(name, avatar) {
                        userManager.createUser(name, avatar)
                        authOverlay._showCreate = false
                    }
                }
            }
        }

        states: [
            State {
                name: "screen_1"
                PropertyChanges { target: buttonStop; visible: false }
                PropertyChanges { target: buttonSave; visible: false }
                PropertyChanges { target: buttonNext; visible: false }
                PropertyChanges { target: buttonPrev; visible: false }
                PropertyChanges { target: buttonRobotMove; visible: false }
                PropertyChanges { target: statusNote; visible: false }
                PropertyChanges { target: statusNote1; visible: false }
                PropertyChanges { target: robotIcon; visible: false }
                PropertyChanges { target: menuButton; visible: true }
            },
            State {
                name: "screen_2"
                PropertyChanges { target: buttonStop; visible: true }
                PropertyChanges { target: buttonSave; visible: true }
                PropertyChanges { target: buttonNext; visible: false }
                PropertyChanges { target: buttonPrev; visible: false }
                PropertyChanges { target: buttonRobotMove; visible: true }
                PropertyChanges { target: statusNote; visible: true }
                PropertyChanges { target: statusNote1; visible: true }
                PropertyChanges { target: robotIcon; visible: false }
                PropertyChanges { target: menuButton; visible: false }
            },
            State {
                name: "screen_3"
                PropertyChanges { target: buttonStop; visible: true }
                PropertyChanges { target: buttonSave; visible: false }
                PropertyChanges { target: buttonNext; visible: true }
                PropertyChanges { target: buttonPrev; visible: true }
                PropertyChanges { target: buttonRobotMove; visible: false }
                PropertyChanges { target: statusNote; visible: true }
                PropertyChanges { target: statusNote1; visible: true }
                PropertyChanges { target: robotIcon; visible: false }
                PropertyChanges { target: menuButton; visible: false }
            },
            State {
                name: "screen_4"
                PropertyChanges { target: buttonStop; visible: true }
                PropertyChanges { target: buttonSave; visible: true }
                PropertyChanges { target: buttonNext; visible: false }
                PropertyChanges { target: buttonPrev; visible: true }
                PropertyChanges { target: buttonRobotMove; visible: false }
                PropertyChanges { target: statusNote; visible: true }
                PropertyChanges { target: statusNote1; visible: true }
                PropertyChanges { target: robotIcon; visible: true }
                PropertyChanges { target: menuButton; visible: false }
            }
        ]
    }
}
