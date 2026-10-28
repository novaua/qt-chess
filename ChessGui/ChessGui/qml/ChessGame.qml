import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import QtMultimedia

ApplicationWindow {
    id: root

    property bool   gameIsInProgress: false
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
    property bool   _resultCardHidden:        false
    property bool   _toolbarResignPending:    false
    property bool   _wasOnlineGame:           false
    property bool   _showLevelsHelp:          false
    readonly property int _toolbarH: 48
    property real   _boardSize: Math.min(chessBoard.width, chessBoard.height) * 0.95
    // _panelH is computed analytically to avoid a binding loop:
    //   _boardSize → chessBoard.height → centralItem.height → _panelH → _boardSize
    // Portrait (width-limited):  panelH = rootW * 0.95 / 16
    // Landscape (height-limited): solving panelH = (rootH-toolbarH-2*panelH)*0.95/16
    //   gives panelH = (rootH-toolbarH)*0.95 / (16*(1+0.95/8)) = (rootH-toolbarH)*0.95/17.9
    property real   _panelH: {
        if (!gameIsInProgress) return 0
        var effH = root.height - _toolbarH
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

    // Leaving the game (menu button / Escape): withdraw a pending rematch, drop online banners.
    onGameIsInProgressChanged: {
        if (gameIsInProgress)
            return
        opponentGoneBanner.hide()
        if (_pendingRematch && _pendingChallengeId !== "")
            lichessClient.cancelChallenge(_pendingChallengeId)
    }

    // ── --screenshot mode (see main.cpp): set up the scene, then C++ grabs the window ──
    readonly property var _shot: chessConnector.ScreenshotScene
    Component.onCompleted: if (_shot.scene) Qt.callLater(setUpScreenshotScene)

    function setUpScreenshotScene() {
        if (_shot.width)  root.width  = _shot.width
        if (_shot.height) root.height = _shot.height
        appSettings.theme = _shot.dark ? 2 : 1   // not "system": stable screenshots

        if (_shot.scene === "create-user" || _shot.scene === "login") {
            if (_shot.scene === "login" && !userManager.hasUsers)
                userManager.createUser("Tester", "wizard")
            userManager.logout()
            authOverlay._showCreate = _shot.scene === "create-user"
        } else if (!userManager.isLoggedIn) {
            if (userManager.hasUsers) userManager.login(userManager.users[0].id)
            else                      userManager.createUser("Tester", "wizard")
        }

        if (_shot.scene === "settings") _showSettings = true
        if (_shot.scene === "levels")   _showLevelsHelp = true
        if (_shot.scene === "profile")  _showProfile = true
        if (_shot.scene === "online")   _showOnlineDialog = true
        if (_shot.scene === "players") {
            if (userManager.userCount < 2) {
                userManager.createUser("Bob", "fox")
                userManager.login(userManager.users[0].id)
            }
            _showPlayerPicker = true
        }
        if (_shot.scene === "board") {
            chessConnector.setPlayerPlaysWhite(!_shot.black)
            screen.state = "screen_2"
            chessConnector.startNewGame()
            avatarProvider.setOpponentFromUser("unicorn") // not the random one: stable screenshots
            gameIsInProgress = true
            chessBoard.angle = chessConnector.PlayerPlaysWhite ? 0 : 180
            if (_shot.moves)
                chessConnector.applyMoves(_shot.moves)
        }

        // A finished game shows its result only after resultDialogTimer.
        screenshotReadyTimer.interval = chessConnector.GameResult !== "" ? resultDialogTimer.interval + 100 : 0
        screenshotReadyTimer.start()
    }

    Timer {
        id: screenshotReadyTimer
        onTriggered: chessConnector.screenshotSceneReady()
    }

    Settings {
        property alias width:  root.width
        property alias height: root.height
        property alias x:      root.x
        property alias y:      root.y
    }

    Settings {
        id: appSettings
        property int  theme:           -1  // 0 system, 1 light, 2 dark; -1 = not set yet, see darkMode
        property bool darkMode:        false // before the theme setting: only read, to migrate it
        property bool musicEnabled:    true
        property int  soundVolume:     3   // 0-5; 0 = muted
        property int  lastSoundVolume: 3   // remembers last non-zero level for restore
    }

    Binding {
        target: Theme; property: "scheme"
        value: appSettings.theme >= 0 ? appSettings.theme : (appSettings.darkMode ? 2 : 0)
    }
    Binding { target: Theme; property: "systemAccent"; value: root.palette.accent }

    // Perceptual loudness curve: linear 0-5 feels far too loud at level 1,
    // since human hearing perceives volume roughly logarithmically.
    readonly property real _sfxVolume: Math.pow(appSettings.soundVolume / 5.0, 2)

    SoundEffect { id: sndMove;         source: "qrc:/sounds/move.wav";          volume: _sfxVolume }
    SoundEffect { id: sndMoveOpponent; source: "qrc:/sounds/move_opponent.wav"; volume: _sfxVolume }
    SoundEffect { id: sndCheck;     source: "qrc:/sounds/check.wav";     volume: _sfxVolume }
    SoundEffect { id: sndCheckmate; source: "qrc:/sounds/checkmate.wav"; volume: _sfxVolume }
    SoundEffect { id: sndWin;       source: "qrc:/sounds/win.wav";       volume: _sfxVolume }

    // Leave the game for the start menu (Home button / Escape).
    function leaveGame() {
        _showGameResult = false
        gameIsInProgress = false
        _toolbarResignPending = false
        screen.state = "screen_1"
        chessConnector.endGame()
    }

    // Start menu: game types and actions, one checked at a time.
    component ModeButton: Button {
        checkable: true
        Layout.fillWidth: true
        Layout.preferredWidth: 1   // equal columns
    }

    title: qsTr("Chess++")
    color: Theme.windowBg

    width: 768
    height: 1054
    visible: true
    // --screenshot: a real window (so the Fluent style gets the forced light/dark palette, which
    // -platform offscreen can't do) that the user never sees or clicks; grabWindow() still
    // captures the rendered scene.
    opacity: _shot.scene ? 0 : 1
    flags: _shot.scene ? Qt.Tool | Qt.FramelessWindowHint | Qt.WindowTransparentForInput
                         | Qt.WindowDoesNotAcceptFocus
                       : Qt.Window

    Rectangle {
        id: screen
        state: "screen_1"
        anchors.fill: parent
        color: Theme.windowBg

        // The glass-pieces photo, faint, behind the start menu only.
        Image {
            id: background
            anchors.fill: parent
            source: "qrc:/app/pics/ChessBackground.jpg"
            fillMode: Image.PreserveAspectCrop
            opacity: Theme.homeBackdropOpacity
            visible: screen.state === "screen_1"
        }

        Column {
            anchors.fill: parent

            // ── Command bar ─────────────────────────────────────────────────
            Rectangle {
                id: toolBar
                width: parent.width
                height: _toolbarH
                color: Theme.layerBg

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width; height: 1
                    color: Theme.divider
                }

                RowLayout {
                    anchors { fill: parent; leftMargin: Theme.s1; rightMargin: Theme.s1 }
                    spacing: Theme.s1

                    IconButton {
                        id: menuButton
                        visible: gameIsInProgress
                        icon.source: Theme.icon("home")
                        tip: "Back to the menu (Esc)"
                        onClicked: leaveGame()
                    }

                    Rectangle {
                        visible: menuButton.visible
                        Layout.preferredWidth: 1; Layout.preferredHeight: 24
                        color: Theme.divider
                    }

                    RowLayout {
                        id: controlButtons
                        visible: !_toolbarResignPending
                        spacing: Theme.s1

                        IconButton {
                            id: buttonSave
                            icon.source: Theme.icon("save"); tip: "Save game"
                            onClicked: chessConnector.saveGame()
                        }
                        IconButton {
                            id: buttonPrev
                            icon.source: Theme.icon("undo")
                            tip: _onlineGameId !== "" ? "Propose takeback" : "Undo move"
                            onClicked: chessConnector.requestTakeback()
                        }
                        IconButton {
                            id: buttonResign
                            icon.source: Theme.icon("flag"); tip: "Resign"
                            visible: !_showGameResult
                            onClicked: _toolbarResignPending = true
                        }
                        IconButton {
                            id: buttonNext
                            icon.source: Theme.icon("next"); tip: "Next move"
                            onClicked: chessConnector.moveNext()
                        }
                    }

                    RowLayout {
                        id: toolbarResignConfirm
                        visible: _toolbarResignPending
                        spacing: Theme.s2

                        Label {
                            text: "Resign this game?"
                            Layout.leftMargin: Theme.s2
                        }
                        Button {
                            text: "Resign"
                            highlighted: true
                            onClicked: { _toolbarResignPending = false; chessConnector.resignGame() }
                        }
                        Button {
                            text: "Cancel"
                            onClicked: _toolbarResignPending = false
                        }
                    }

                    Item { Layout.fillWidth: true }

                    IconButton {
                        id: settingsButton
                        icon.source: Theme.icon("settings")
                        tip: "Settings and profile"
                        onClicked: appMenu.popup(settingsButton, settingsButton.width - appMenu.width,
                                                 settingsButton.height)

                        Menu {
                            id: appMenu
                            MenuItem {
                                text: "Settings"
                                icon.source: Theme.icon("settings")
                                onTriggered: _showSettings = true
                            }
                            MenuItem {
                                text: "Profile"
                                icon.source: Theme.icon("person")
                                onTriggered: _showProfile = true
                            }
                            MenuItem {
                                text: "Computer levels"
                                icon.source: Theme.icon("robot")
                                enabled: !_otherDialogOpen
                                onTriggered: showLevelsHelp()
                            }
                            MenuSeparator {}
                            MenuItem {
                                text: "Switch user"
                                icon.source: Theme.icon("sign-out")
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
                height: parent.height - _toolbarH - (gameIsInProgress ? 2 * _panelH : 0)

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
                GameResultCard {
                    visible: _showGameResult && !_showHistoryPanel && !_resultCardHidden
                    anchors.centerIn: parent
                    width: Math.min(_boardSize * 0.8, 320)
                    z: 5
                    resultText: _checkmateWinner
                    canRematch: chessConnector.GameResult !== "" && chessConnector.CanRematch
                                && !_pendingRematch
                    onRematchRequested: chessConnector.rematch()
                    onCloseRequested:   _resultCardHidden = true
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
            height: menuContent.implicitHeight + 2 * Theme.s5
            visible: screen.state === "screen_1"
            color: Theme.cardBg
            border.color: Theme.cardStroke
            border.width: 1
            radius: Theme.radiusCard

            ColumnLayout {
                id: menuContent
                anchors.centerIn: parent
                spacing: Theme.s4
                width: parent.width - 2 * Theme.s5

                Text {
                    text: "Chess++"
                    font: Theme.title
                    color: Theme.textPrimary
                    Layout.alignment: Qt.AlignHCenter
                }

                // Game mode radio group
                ButtonGroup { id: gameModeGroup; buttons: modeGrid.children }

                GridLayout {
                    id: modeGrid
                    columns: 2
                    columnSpacing: Theme.s2
                    rowSpacing: Theme.s2
                    Layout.fillWidth: true

                    ModeButton {
                        id: btnSingle
                        text: "Single player"
                        icon.source: Theme.icon("robot")
                        checked: true
                    }
                    ModeButton {
                        id: btnTwo
                        text: "Two players"
                        icon.source: Theme.icon("two-players")
                    }
                    ModeButton {
                        id: btnOnline
                        text: "Online"
                        icon.source: Theme.icon("globe")
                        Layout.columnSpan: 2
                    }
                    ModeButton {
                        id: btnContinue
                        text: "Continue"
                        icon.source: Theme.icon("play")
                        enabled: chessConnector.CanContinue
                    }
                    ModeButton {
                        id: btnLoad
                        text: "Load"
                        icon.source: Theme.icon("folder-open")
                        enabled: chessConnector.CanLoad
                    }
                }

                // Color selection
                RowLayout {
                    spacing: Theme.s2
                    Layout.fillWidth: true
                    visible: btnSingle.checked || btnTwo.checked

                    ButtonGroup { id: colorGroup }

                    Button {
                        id: btnColorWhite
                        text: "White ♔"
                        checkable: true
                        ButtonGroup.group: colorGroup
                        checked: chessConnector.PlayerPlaysWhite
                        Layout.fillWidth: true; Layout.preferredWidth: 1
                    }
                    Button {
                        id: btnColorBlack
                        text: "Black ♚"
                        checkable: true
                        ButtonGroup.group: colorGroup
                        checked: !chessConnector.PlayerPlaysWhite
                        Layout.fillWidth: true; Layout.preferredWidth: 1
                    }
                }

                // Difficulty (single player only)
                ColumnLayout {
                    id: difficultyBox
                    spacing: 0
                    Layout.fillWidth: true
                    visible: btnSingle.checked

                    readonly property var info: levelInfo(Math.round(difficultySlider.value))

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.s1

                        Label {
                            text: "Level " + difficultyBox.info.level + " · " + difficultyBox.info.name
                            font: Theme.bodyStrong
                        }
                        Label {
                            text: levelRatingText(difficultyBox.info)
                            color: Theme.textSecondary
                            Layout.fillWidth: true
                        }
                        IconButton {
                            dense: true
                            icon.source: Theme.icon("info")
                            tip: "What do the levels mean? (F1)"
                            onClicked: showLevelsHelp()
                        }
                    }
                    Slider {
                        id: difficultySlider
                        from: 1; to: _levels.length; value: chessConnector.LastLevel; stepSize: 1
                        snapMode: Slider.SnapAlways
                        Layout.fillWidth: true
                        Accessible.name: "Computer level"
                    }
                }

                // Action buttons
                RowLayout {
                    spacing: Theme.s2
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: Theme.s2

                    Button {
                        text: "Start game"
                        highlighted: true
                        focus: true
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
                opponentGoneBanner.hide()
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
                opponentGoneBanner.hide()
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
                // Without a winner (e.g. time ran out against a bare king) it is a draw, and
                // we don't know who lost.
                const loserNotice = ({ "resign": loserName + " resigned",
                                       "outoftime": loserName + " ran out of time",
                                       "timeout": loserName + " left the game",
                                       "noStart": loserName + " didn't move" })[status] ?? ""
                const notice = winnerName !== "" ? loserNotice
                             : ({ "aborted": "Game aborted", "stalemate": "Stalemate",
                                  "mate": "" })[status] ?? "Draw"
                if (notice !== "")
                    chessBoard.showNotification(notice)

                _resultCardHidden = false
                resultDialogTimer.start()
            }

            function onOpponentGone(gone, claimWinInSeconds) {
                if (gone) opponentGoneBanner.show(claimWinInSeconds)
                else      opponentGoneBanner.hide()
            }

            function onNetworkError(message) {
                if (_showChallengeDialog && _pendingRematch) {
                    clearPendingChallenge()
                    networkErrorBanner.show("Rematch failed: " + message)
                } else if (_showChallengeDialog) {
                    clearPendingChallenge()
                    _showOnlineDialog = true
                } else if (_onlineGameId !== "" || _wasOnlineGame) {
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
            visible: _showSettings
            onClosed: _showSettings = false
        }

        // ── Computer levels help ─────────────────────────────────────────
        LevelsHelpDialog {
            visible: _showLevelsHelp
            onClosed: _showLevelsHelp = false
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
        }

        // An open dialog blocks these (Esc then closes the dialog instead).
        Shortcut {
            sequence: "F1"
            enabled: !_otherDialogOpen
            onActivated: showLevelsHelp()
        }

        Shortcut {
            sequence: "Escape"
            enabled: gameIsInProgress && !_showLevelsHelp && !_otherDialogOpen
            onActivated: leaveGame()
        }

        // ── Move-input cheat dialog (Alt+S in two-player mode) ────────────
        Shortcut {
            sequence: "Alt+S"
            enabled: screen.state === "screen_2"
            onActivated: _showMoveInput = true
        }

        ApplyMovesDialog {
            id: applyMovesDialog
            visible: _showMoveInput
            onClosed: _showMoveInput = false
        }

        // ── Lichess network error banner ─────────────────────────────────────
        Rectangle {
            id: networkErrorBanner
            anchors { top: parent.top; topMargin: 8; horizontalCenter: parent.horizontalCenter }
            width: Math.min(parent.width - 32, 420)
            height: bannerText.implicitHeight + 2 * Theme.s3
            radius: Theme.radiusCard
            color: Theme.criticalBg
            border.color: Theme.critical
            visible: false
            z: 30

            function show(msg) {
                bannerText.text = "⚠ " + msg
                visible = true
                bannerTimer.restart()
            }

            Text {
                id: bannerText
                anchors.centerIn: parent
                width: parent.width - 2 * Theme.s4
                color: Theme.textPrimary
                font: Theme.body
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
            width: Math.min(parent.width - 32, 420)
            height: drawOfferRow.implicitHeight + 2 * Theme.s2
            radius: Theme.radiusCard
            color: Theme.cardBg
            border.color: Theme.accent
            z: 30

            Row {
                id: drawOfferRow
                anchors.centerIn: parent
                spacing: Theme.s3
                Text {
                    text: "Your opponent offers a draw"
                    color: Theme.textPrimary; font: Theme.body
                    anchors.verticalCenter: parent.verticalCenter
                }
                Button {
                    text: "Accept"; highlighted: true
                    onClicked: { chessConnector.respondDraw(true); _showDrawOffer = false }
                }
                Button {
                    text: "Decline"
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
            width: Math.min(parent.width - 32, 420)
            height: takebackRow.implicitHeight + 2 * Theme.s2
            radius: Theme.radiusCard
            color: Theme.cardBg
            border.color: Theme.accent
            z: 30

            Row {
                id: takebackRow
                anchors.centerIn: parent
                spacing: Theme.s3
                Text {
                    text: "Your opponent asks to take back a move"
                    color: Theme.textPrimary; font: Theme.body
                    anchors.verticalCenter: parent.verticalCenter
                }
                Button {
                    text: "Accept"; highlighted: true
                    onClicked: { chessConnector.respondTakeback(true); _showTakebackOffer = false }
                }
                Button {
                    text: "Decline"
                    onClicked: { chessConnector.respondTakeback(false); _showTakebackOffer = false }
                }
            }
        }

        // ── Opponent left (Lichess opponentGone) ─────────────────────────────
        OpponentGoneBanner {
            id: opponentGoneBanner
            anchors {
                top: takebackBanner.visible ? takebackBanner.bottom
                   : drawOfferBanner.visible ? drawOfferBanner.bottom : networkErrorBanner.bottom
                topMargin: 4
                horizontalCenter: parent.horizontalCenter
            }
            width: Math.min(parent.width - 32, 420)
            z: 30
            onClaimVictoryRequested: chessConnector.claimVictory()
        }

        // ── Lichess online dialog ────────────────────────────────────────────
        LichessGameDialog {
            visible: _showOnlineDialog
            onClosed: _showOnlineDialog = false
            onCloseRequested: _showOnlineDialog = false
        }

        // ── Pending challenge dialog ─────────────────────────────────────────
        ChallengeDialog {
            visible: _showChallengeDialog
            onClosed: _showChallengeDialog = false
            challengeId:  _pendingChallengeId
            challengeUrl: _pendingChallengeUrl
            opponentName: _pendingOpponentName
            isRematch:    _pendingRematch
        }

        // ── Profile dialog ───────────────────────────────────────────────────
        CreateUserDialog {
            visible: _showProfile
            onClosed: _showProfile = false
            editMode: true
            initialName:   userManager.activeUserName
            initialAvatar: userManager.activeUserAvatar
            onConfirmed: function(name, avatar) {
                userManager.updateProfile(name, avatar)
            }
        }

        // ── Second-player picker (two-player with ≥2 users) ─────────────────
        SelectPlayerDialog {
            visible: _showPlayerPicker
            onClosed: _showPlayerPicker = false
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

            Rectangle {
                anchors.fill: parent
                visible: authOverlay._showCreate
                color: Theme.windowBg
                Image {
                    anchors.fill: parent
                    source: "qrc:/app/pics/ChessBackground.jpg"
                    fillMode: Image.PreserveAspectCrop
                    opacity: Theme.homeBackdropOpacity
                }
            }

            CreateUserDialog {
                visible: authOverlay.visible && authOverlay._showCreate
                editMode: false
                canCancel: userManager.hasUsers   // back to the profile list
                onConfirmed: function(name, avatar) {
                    userManager.createUser(name, avatar)
                    authOverlay._showCreate = false
                }
                onCancelled: authOverlay._showCreate = false
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
