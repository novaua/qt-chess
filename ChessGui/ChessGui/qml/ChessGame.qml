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
    property string _pendingOpponentName:     ""    // set for a direct (rematch) challenge
    property bool   _pendingRematch:          false // pending challenge is a rematch of the game on screen
    property string _onlineGameId:            ""
    property bool   _showDrawOffer:           false
    property bool   _showTakebackOffer:       false
    property bool   _opponentGone:            false // Lichess: opponent left the game
    property int    _claimSecondsLeft:        -1    // until "Claim victory" is allowed; <= 0: now
    property bool   _resultCardHidden:        false
    property bool   _toolbarResignPending:    false
    property bool   _wasOnlineGame:           false
    property bool   _showLevelsHelp:          false
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

    // ── Computer level helpers (data comes from chessConnector.Levels) ──
    readonly property var  _levels: chessConnector.Levels
    function _levelIndex(level) {
        return Math.max(1, Math.min(_levels.length, level)) - 1
    }
    function levelInfo(level) {
        return _levels[_levelIndex(level)]
    }
    function levelRatingText(info) {
        return (info.eloEstimated ? "≈" : "") + info.elo + " Elo"
    }
    // Green (weakest) → red (strongest), spread over however many levels there are.
    function levelColor(level) {
        var t = _levels.length > 1 ? _levelIndex(level) / (_levels.length - 1) : 0
        return Qt.hsla((1 - t) * 0.36, 0.65, 0.45, 1)
    }

    // Any modal-style dialog other than the levels help is open.
    readonly property bool _otherDialogOpen: _showSettings || _showMoveInput || _showProfile
        || _showPlayerPicker || _showOnlineDialog || _showChallengeDialog || !userManager.isLoggedIn
    function showLevelsHelp() {
        if (!_otherDialogOpen)
            _showLevelsHelp = true
    }

    function clearPendingChallenge() {
        userManager.clearPendingChallenge()
        _pendingChallengeId  = ""
        _pendingChallengeUrl = ""
        _pendingOpponentName = ""
        _pendingRematch      = false
        _showChallengeDialog = false
    }

    // Leaving the finished game (menu button / Escape) while a rematch is pending withdraws it.
    onGameIsInProgressChanged: {
        if (!gameIsInProgress && _pendingRematch && _pendingChallengeId !== "")
            lichessClient.cancelChallenge(_pendingChallengeId)
    }

    Settings {
        property alias width:  root.width
        property alias height: root.height
        property alias x:      root.x
        property alias y:      root.y
    }

    Settings {
        id: appSettings
        property bool darkMode:        false
        property bool musicEnabled:    true
        property int  soundVolume:     3   // 0-5; 0 = muted
        property int  lastSoundVolume: 3   // remembers last non-zero level for restore
    }

    // Perceptual loudness curve: linear 0-5 feels far too loud at level 1,
    // since human hearing perceives volume roughly logarithmically.
    readonly property real _sfxVolume: Math.pow(appSettings.soundVolume / 5.0, 2)

    SoundEffect { id: sndMove;         source: "qrc:/sounds/move.wav";          volume: _sfxVolume }
    SoundEffect { id: sndMoveOpponent; source: "qrc:/sounds/move_opponent.wav"; volume: _sfxVolume }
    SoundEffect { id: sndCheck;     source: "qrc:/sounds/check.wav";     volume: _sfxVolume }
    SoundEffect { id: sndCheckmate; source: "qrc:/sounds/checkmate.wav"; volume: _sfxVolume }
    SoundEffect { id: sndWin;       source: "qrc:/sounds/win.wav";       volume: _sfxVolume }

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
                    visible: !_toolbarResignPending
                    anchors { left: menuButton.right; leftMargin: 6; verticalCenter: parent.verticalCenter }
                    spacing: 6

                    ActionButton {
                        id: buttonSave; label: "Save"; tip: "Save game"
                        onAction: chessConnector.saveGame()
                    }
                    ActionButton {
                        id: buttonPrev; label: "↩"
                        tip: _onlineGameId !== "" ? "Propose Takeback" : "Undo Move"
                        onAction: chessConnector.requestTakeback()
                    }
                    ActionButton {
                        id: buttonResign; label: "⚑"; tip: "Resign"
                        visible: !_showGameResult
                        onAction: _toolbarResignPending = true
                    }
                    ActionButton {
                        id: buttonNext; label: "Next"; tip: "Next move"
                        onAction: chessConnector.moveNext()
                    }
                }

                Row {
                    id: toolbarResignConfirm
                    visible: _toolbarResignPending
                    anchors { left: menuButton.right; leftMargin: 6; verticalCenter: parent.verticalCenter }
                    spacing: 8

                    Text {
                        text: "Resign?"
                        color: isDarkMode ? "#cccccc" : "#444444"
                        font.pixelSize: 13
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    ActionButton { label: "Yes"; danger: true
                        onAction: { _toolbarResignPending = false; chessConnector.resignGame() }
                    }
                    ActionButton { label: "No"
                        onAction: _toolbarResignPending = false
                    }
                }

                Rectangle {
                    id: menuButton
                    visible: gameIsInProgress
                    width: 28; height: 28
                    radius: 6
                    color:        root.isDarkMode ? "#3c3c3c" : "#e0e0e0"
                    border.color: root.isDarkMode ? "#555555" : "#bbbbbb"
                    border.width: 1
                    anchors { left: parent.left; leftMargin: 1; verticalCenter: parent.verticalCenter }

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
                        onClicked: {
                            _showGameResult = false
                            gameIsInProgress = false
                            _toolbarResignPending = false
                            screen.state = "screen_1"
                            chessConnector.endGame()
                        }
                    }
                }

                Rectangle {
                    id: settingsButton
                    width: 28; height: 28
                    radius: 6
                    color:        root.isDarkMode ? "#3c3c3c" : "#e0e0e0"
                    border.color: root.isDarkMode ? "#555555" : "#bbbbbb"
                    border.width: 1
                    anchors { right: parent.right; rightMargin: 1; verticalCenter: parent.verticalCenter }

                    Text {
                        anchors.centerIn: parent
                        text: "⚙"
                        font.pixelSize: 16
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
                            text: "Computer Levels…"
                            enabled: !_otherDialogOpen
                            onTriggered: showLevelsHelp()
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

            CapturedPanel {
                id: topPanel
                width: _boardSize
                height: _panelH
                anchors.horizontalCenter: parent.horizontalCenter
                visible: gameIsInProgress
                avatarUrl: avatarProvider.opponentUrl
                // The robot only plays in single-player games (screen_4).
                avatarTip: {
                    if (screen.state !== "screen_4") return ""
                    var info = levelInfo(chessConnector.LastLevel)
                    return "Robot · Level " + info.level + " — " + info.name + "\n"
                        + levelRatingText(info) + "\n"
                        + "Click for level info"
                }
                onAvatarClicked: showLevelsHelp()
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
                    height: _boardSize
                    gameInProgress: gameIsInProgress
                    isOnlineGame: _onlineGameId !== ""
                    rematchPending: _pendingRematch
                    anchors {
                        verticalCenter: parent.verticalCenter
                        left: parent.left; leftMargin: (parent.width + _boardSize) / 2 + 5
                    }
                }

                GameResultDialog {
                    id: gameResultPanel
                    visible: _showGameResult && _showHistoryPanel
                    winner: _checkmateWinner
                    width: _historyPanelW
                    anchors {
                        verticalCenter: parent.verticalCenter
                        right: parent.horizontalCenter
                        rightMargin: _boardSize / 2 + 5
                    }
                }

                // Narrow / portrait window: no side panels, so show the result (and Rematch)
                // on the board itself.
                Rectangle {
                    id: resultCard
                    visible: _showGameResult && !_showHistoryPanel && !_resultCardHidden
                    anchors.centerIn: parent
                    width: Math.min(_boardSize * 0.8, 320)
                    height: resultCol.implicitHeight + 28
                    radius: 12
                    z: 5
                    color:        root.isDarkMode ? "#e6252525" : "#e6f4f4f4"
                    border.color: root.isDarkMode ? "#555555" : "#cccccc"

                    MouseArea { anchors.fill: parent } // keep clicks off the board

                    Column {
                        id: resultCol
                        anchors.centerIn: parent
                        width: parent.width - 28
                        spacing: 12

                        Text {
                            width: parent.width
                            text: _checkmateWinner
                            font.pixelSize: 18
                            font.bold: true
                            color: root.isDarkMode ? "#ffffff" : "#000000"
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Row {
                            spacing: 10
                            anchors.horizontalCenter: parent.horizontalCenter

                            ActionButton {
                                label: "Rematch"
                                visible: chessConnector.GameResult !== "" && chessConnector.CanRematch
                                         && !_pendingRematch
                                onAction: chessConnector.rematch()
                            }
                            ActionButton {
                                label: "Close"
                                onAction: _resultCardHidden = true
                            }
                        }
                    }
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
                    id: difficultyBox
                    spacing: 6
                    anchors.horizontalCenter: parent.horizontalCenter
                    opacity: btnSingle.checked ? 1.0 : 0.35

                    readonly property var info: levelInfo(Math.round(difficultySlider.value))

                    Row {
                        spacing: 8
                        anchors.horizontalCenter: parent.horizontalCenter

                        Text {
                            text: "Level " + difficultyBox.info.level + " · " + difficultyBox.info.name
                            color: root.isDarkMode ? "#ffffff" : "#000000"
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Rectangle {
                            width: 18; height: 18; radius: 9
                            anchors.verticalCenter: parent.verticalCenter
                            color: levelHelpMouse.containsMouse
                                ? "#0078d4"
                                : (root.isDarkMode ? "#3c3c3c" : "#e0e0e0")
                            border.color: root.isDarkMode ? "#555555" : "#bbbbbb"
                            Text {
                                anchors.centerIn: parent
                                text: "?"
                                font.pixelSize: 12
                                font.bold: true
                                color: levelHelpMouse.containsMouse ? "#ffffff"
                                    : (root.isDarkMode ? "#ffffff" : "#09102B")
                            }
                            MouseArea {
                                id: levelHelpMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: showLevelsHelp()
                                ToolTip.visible: containsMouse
                                ToolTip.text: "What do the levels mean? (F1)"
                                ToolTip.delay: 400
                            }
                        }
                    }
                    CustomSlider {
                        id: difficultySlider
                        from: 1; to: _levels.length; value: chessConnector.LastLevel; stepSize: 1
                        width: 260
                        enabled: btnSingle.checked
                        anchors.horizontalCenter: parent.horizontalCenter
                        ToolTip.visible: pressed
                        ToolTip.text: "Level " + difficultyBox.info.level + " — "
                            + difficultyBox.info.name + " (" + levelRatingText(difficultyBox.info) + ")"
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
                _toolbarResignPending = false
                var isWhite = winner === "White Won"
                var playerWon = isWhite === chessConnector.PlayerPlaysWhite
                var name = playerWon ? avatarProvider.playerName : avatarProvider.opponentName
                _checkmateWinner = (isWhite ? "White " : "Black ") + name + " Won"
                if (chessConnector.GameResult.indexOf("resignation") >= 0)
                    chessBoard.showNotification((isWhite ? "Black" : "White") + " resigned")
                _resultCardHidden = false
                resultDialogTimer.start()
                if (appSettings.musicEnabled) 
                    sndWin.play()
            }
        }

        Connections {
            target: lichessClient

            // A challenge created while the finished game is still on screen is a rematch
            // (the Online dialog is only reachable from the start menu).
            function onChallengeCreated(gameId, joinUrl) {
                _pendingChallengeId  = gameId
                _pendingChallengeUrl = joinUrl
                _pendingRematch      = gameIsInProgress
                userManager.savePendingChallenge(gameId, joinUrl)
                _showOnlineDialog    = false
                _showChallengeDialog = true
            }

            // Not saved with savePendingChallenge: the challenge lives only while the app keeps
            // its request open, so it can't be resumed after a restart.
            function onDirectChallengeSent(challengeId, opponentName) {
                _pendingChallengeId   = challengeId
                _pendingChallengeUrl  = ""
                _pendingOpponentName  = opponentName
                _pendingRematch       = true
                _showChallengeDialog  = true
            }

            function onChallengeDeclined(opponentName) {
                clearPendingChallenge()
                networkErrorBanner.show(opponentName + " declined the rematch")
            }

            function onChallengeCanceled() {
                var wasRematch = _pendingRematch
                clearPendingChallenge()
                if (!wasRematch)
                    _showOnlineDialog = true
            }

            function onGameStarted(gameId, playingAsWhite, opponentName, opponentAvatarUrl) {
                clearPendingChallenge()
                _opponentGone = false
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
                _showDrawOffer = false
                _showTakebackOffer = false
                _toolbarResignPending = false
                _opponentGone = false
                _wasOnlineGame = true

                const winnerName = ({ "white": "White", "black": "Black" })[winner] ?? ""
                const loserName  = winner === "white" ? "Black" : "White"
                const reason     = ({ "mate": " by Checkmate", "resign": " by Resignation",
                                      "outoftime": " on Time", "timeout": " (opponent left)",
                                      "noStart": " (opponent didn't move)" })[status] ?? ""

                if      (status === "aborted")   _checkmateWinner = "Game Aborted"
                else if (winnerName !== "")      _checkmateWinner = winnerName + " Won" + reason
                else if (status === "stalemate") _checkmateWinner = "Draw by Stalemate"
                else                             _checkmateWinner = "Draw"

                chessConnector.setGameResult(_checkmateWinner)

                // Like Lichess: a red flag on the king of the side that gave up / ran out of time.
                if (winnerName !== "" && ["resign", "outoftime", "timeout", "noStart"].includes(status))
                    chessConnector.flagKing(winner !== "white")

                // Checkmate is already announced by the local game when the mating move arrives.
                const notice = ({ "resign": loserName + " resigned",
                                  "outoftime": loserName + " ran out of time",
                                  "timeout": loserName + " left the game",
                                  "noStart": loserName + " didn't move",
                                  "aborted": "Game aborted",
                                  "stalemate": "Stalemate", "draw": "Draw" })[status] ?? ""
                if (notice !== "")
                    chessBoard.showNotification(notice)

                _resultCardHidden = false
                resultDialogTimer.start()
            }

            function onOpponentGone(gone, claimWinInSeconds) {
                _opponentGone     = gone
                _claimSecondsLeft = gone ? claimWinInSeconds : -1
            }

            function onNetworkError(message) {
                if (_showChallengeDialog && _pendingRematch) {
                    clearPendingChallenge()
                    networkErrorBanner.show("Rematch failed: " + message)
                } else if (_showChallengeDialog) {
                    clearPendingChallenge()
                    _showOnlineDialog = true
                } else if (_pendingRematch || _wasOnlineGame) {
                    networkErrorBanner.show(message)
                } else if (_onlineGameId !== "") {
                    networkErrorBanner.show(message)
                }
            }
        }

        Connections {
            target: chessConnector
            function onNewGameStarted(isComputerGame) {
                chessBoard.angle = chessConnector.PlayerPlaysWhite ? 0 : 180
                _showGameResult = false
                _toolbarResignPending = false
                _wasOnlineGame = false
            }
            function onPlayerPlaysWhiteChanged() {
                if (gameIsInProgress)
                    chessBoard.angle = chessConnector.PlayerPlaysWhite ? 0 : 180
            }
            // Undo after mate / resign vs Robot resumes the game: drop the result display.
            function onGameResultChanged() {
                if (chessConnector.GameResult === "") {
                    resultDialogTimer.stop()
                    _showGameResult = false
                }
            }
            function onDrawOfferReceived()  { _showDrawOffer = true }
            function onTakebackRequested()  { _showTakebackOffer = true }
        }

        Timer {
            id: resultDialogTimer
            interval: 1600
            repeat: false
            onTriggered: _showGameResult = true
        }

        // ── Settings dialog ───────────────────────────────────────────────
        SettingsDialog {
            id: settingsDialog
            anchors.centerIn: parent
            visible: _showSettings
            z: 20
            onCloseRequested: _showSettings = false
        }

        // ── Computer levels help ─────────────────────────────────────────
        LevelsHelpDialog {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -15
            visible: _showLevelsHelp
            z: 20
            // From the start menu, rows pick a level; in a game, highlight the robot's level.
            selectable: screen.state === "screen_1"
            currentLevel: screen.state === "screen_1" ? Math.round(difficultySlider.value)
                        : screen.state === "screen_4" ? chessConnector.LastLevel
                        : 0
            onLevelPicked: function(level) {
                difficultySlider.value = level
                btnSingle.checked = true
                _showLevelsHelp = false
            }
            onCloseRequested: _showLevelsHelp = false
        }

        Shortcut {
            sequence: "F1"
            enabled: _showLevelsHelp || !_otherDialogOpen
            onActivated: _showLevelsHelp = !_showLevelsHelp
        }

        Shortcut {
            sequence: "Escape"
            enabled: _showLevelsHelp
            onActivated: _showLevelsHelp = false
        }

        Shortcut {
            sequence: "Escape"
            enabled: gameIsInProgress && !_showLevelsHelp
            onActivated: {
                _showGameResult = false
                gameIsInProgress = false
                _toolbarResignPending = false
                screen.state = "screen_1"
                chessConnector.endGame()
            }
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

        // ── Draw offer banner ────────────────────────────────────────────────
        Rectangle {
            id: drawOfferBanner
            visible: _showDrawOffer && _onlineGameId !== ""
            anchors {
                top: networkErrorBanner.bottom; topMargin: 4
                horizontalCenter: parent.horizontalCenter
            }
            width: Math.min(parent.width - 32, 380)
            height: 44
            radius: 8
            color: "#1565c0"
            z: 30

            Row {
                anchors.centerIn: parent
                spacing: 10
                Text {
                    text: "Opponent offers a draw"
                    color: "#ffffff"; font.pixelSize: 13
                    anchors.verticalCenter: parent.verticalCenter
                }
                Button {
                    text: "Accept"; implicitWidth: 70; implicitHeight: 28
                    onClicked: { chessConnector.respondDraw(true); _showDrawOffer = false }
                }
                Button {
                    text: "Decline"; implicitWidth: 70; implicitHeight: 28
                    onClicked: { chessConnector.respondDraw(false); _showDrawOffer = false }
                }
            }
        }

        // ── Takeback request banner ──────────────────────────────────────────
        Rectangle {
            id: takebackBanner
            visible: _showTakebackOffer && _onlineGameId !== ""
            anchors {
                top: drawOfferBanner.visible ? drawOfferBanner.bottom : networkErrorBanner.bottom
                topMargin: 4
                horizontalCenter: parent.horizontalCenter
            }
            width: Math.min(parent.width - 32, 380)
            height: 44
            radius: 8
            color: "#5b4a00"
            z: 30

            Row {
                anchors.centerIn: parent
                spacing: 10
                Text {
                    text: "Opponent requests a takeback"
                    color: "#ffffff"; font.pixelSize: 13
                    anchors.verticalCenter: parent.verticalCenter
                }
                Button {
                    text: "Accept"; implicitWidth: 70; implicitHeight: 28
                    onClicked: { chessConnector.respondTakeback(true); _showTakebackOffer = false }
                }
                Button {
                    text: "Decline"; implicitWidth: 70; implicitHeight: 28
                    onClicked: { chessConnector.respondTakeback(false); _showTakebackOffer = false }
                }
            }
        }

        // ── Opponent left (Lichess opponentGone) ─────────────────────────────
        Timer {
            id: claimCountdown
            interval: 1000
            repeat: true
            running: _opponentGone && _claimSecondsLeft > 0
            onTriggered: _claimSecondsLeft--
        }

        Rectangle {
            id: opponentGoneBanner
            visible: _opponentGone && _onlineGameId !== ""
            anchors {
                top: takebackBanner.visible ? takebackBanner.bottom
                   : drawOfferBanner.visible ? drawOfferBanner.bottom : networkErrorBanner.bottom
                topMargin: 4
                horizontalCenter: parent.horizontalCenter
            }
            width: Math.min(parent.width - 32, 420)
            height: 44
            radius: 8
            color: "#7a3b00"
            z: 30

            Row {
                anchors.centerIn: parent
                spacing: 10
                Text {
                    text: _claimSecondsLeft > 0
                        ? "Opponent left the game. Claim victory in " + _claimSecondsLeft + " s"
                        : "Opponent left the game"
                    color: "#ffffff"; font.pixelSize: 13
                    anchors.verticalCenter: parent.verticalCenter
                }
                Button {
                    text: "Claim victory"; implicitWidth: 100; implicitHeight: 28
                    enabled: _claimSecondsLeft <= 0
                    onClicked: lichessClient.claimVictory(_onlineGameId)
                }
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
            opponentName: _pendingOpponentName
            isRematch:    _pendingRematch
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
                PropertyChanges { target: buttonSave;   visible: false }
                PropertyChanges { target: buttonNext;   visible: false }
                PropertyChanges { target: buttonPrev;   visible: false }
                PropertyChanges { target: buttonResign; visible: false }
            },
            State {
                name: "screen_2"
                PropertyChanges { target: buttonSave;   visible: true }
                PropertyChanges { target: buttonNext;   visible: false }
                PropertyChanges { target: buttonPrev;   visible: false }
                PropertyChanges { target: buttonResign; visible: true }
            },
            State {
                name: "screen_3"
                PropertyChanges { target: buttonSave;   visible: false }
                PropertyChanges { target: buttonNext;   visible: true }
                PropertyChanges { target: buttonPrev;   visible: true }
                PropertyChanges { target: buttonResign; visible: true }
            },
            State {
                name: "screen_4"
                PropertyChanges { target: buttonSave;   visible: true }
                PropertyChanges { target: buttonNext;   visible: false }
                PropertyChanges { target: buttonPrev;   visible: true }
                PropertyChanges { target: buttonResign; visible: true }
            }
        ]
    }
}
