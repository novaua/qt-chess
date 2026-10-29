import QtQuick
import QtQuick.Controls
import QtCore
import QtMultimedia
import QtQuick.Dialogs
import ChessPlusPlus

// The window: one page at a time in a StackView (Welcome, the profile list, Home or the game, with
// the Profile or Settings page pushed over Home or the game), the InfoBars under the command bar,
// and every dialog. UserManager and ChessConnector.InGame decide the page (route()); pages ask for
// dialogs through signals.
ApplicationWindow {
    id: root

    // A Lichess challenge waiting for the opponent (challenge setup stays in QML, see CLAUDE.md).
    property string _pendingChallengeId:  ""
    property string _pendingChallengeUrl: ""
    property string _pendingOpponentName: ""    // set for a direct (rematch) challenge
    property bool   _pendingRematch:      false // pending challenge is a rematch of the game on screen

    readonly property bool   _inGame:   ChessConnector.InGame
    readonly property string _mode:     ChessConnector.GameMode
    readonly property bool   _gameOver: ChessConnector.GameResult !== ""
    // The page on top: "welcome", "login", "home", "game", "profile" or "settings".
    readonly property string _page: stack.currentItem ? stack.currentItem.objectName : ""
    readonly property bool   _onGamePage: _page === "game"

    // A dialog is open: the window's shortcuts wait (Esc then closes the dialog).
    readonly property bool _dialogOpen: restoreBackupDialog.visible || levelsHelp.visible
        || computerDialog.visible || twoPlayerDialog.visible || onlineDialog.visible
        || challengeDialog.visible || createUserDialog.visible
        || applyMovesDialog.visible || resignConfirm.visible || switchUserConfirm.visible
        || pgnExportDialog.visible || profileExportDialog.visible || profileImportDialog.visible
        || importConflictDialog.visible || deleteProfileConfirm.visible || disconnectLichessConfirm.visible

    // Green (weakest) → red (strongest), spread over however many levels there are.
    function levelColor(level) {
        var count = ChessConnector.Levels.length
        var t = count > 1 ? (Math.max(1, Math.min(count, level)) - 1) / (count - 1) : 0
        return Qt.hsla((1 - t) * 0.36, 0.65, 0.45, 1)
    }

    // The page under everything follows the state; the Profile and Settings pages stay on top of it
    // until the base page changes (e.g. its profile was deleted).
    function route() {
        var page = UserManager.isLoggedIn ? (ChessConnector.InGame ? "game" : "home")
                 : UserManager.hasUsers ? "login" : "welcome"
        var base = stack.depth > 0 ? stack.get(0, StackView.DontLoad) : null
        if (base && base.objectName === page)
            return
        var component = ({ welcome: welcomePage, login: loginPage, home: homePage, game: gamePage })[page]
        if (stack.depth === 0) stack.push(component)
        else                   stack.replace(null, component)
        if (page !== "welcome" && page !== "login")
            createUserDialog.close()  // e.g. logged in by a script (--screenshot)
    }

    function openProfile() {
        if (_page !== "profile")
            stack.push(profilePage)
    }
    function closeProfile() {
        if (_page === "profile")
            stack.pop()
    }
    function openSettings() {
        if (_page !== "settings")
            stack.push(settingsPage)
    }
    function closeSettings() {
        if (_page === "settings")
            stack.pop()
    }
    function resign() {
        if (AppSettings.confirmResign) resignConfirm.open()
        else                           ChessConnector.resignGame()
    }

    // ── Profiles: export, import, delete (the dialogs are below) ──
    function exportProfile(userId) {
        profileExportDialog.userId = userId
        // Encoded: a '#' or '%' in a name is not URL syntax.
        profileExportDialog.selectedFile = profileExportDialog.currentFolder + "/"
                                         + encodeURIComponent(UserManager.exportFileName(userId))
        profileExportDialog.open()
    }
    function inspectImport(file) {
        var info = UserManager.inspectImport(file)
        if (info.error !== "") {
            messageBar.show(info.error, "critical")
        } else if (info.conflictId !== "") {
            importConflictDialog.file = file
            importConflictDialog.importName = info.name
            importConflictDialog.existingName = info.conflictName
            importConflictDialog.open()
        } else {
            importProfile(file, false)
        }
    }
    function importProfile(file, replace) {
        var error = UserManager.importProfile(file, replace)
        if (error !== "") messageBar.show(error, "critical")
        else              messageBar.show("Profile imported", "success")
    }
    function deleteProfile(userId) {
        var user = UserManager.users.find(u => u.id === userId)
        if (!user) return
        deleteProfileConfirm.userId = userId
        deleteProfileConfirm.title = "Delete " + user.name + "'s profile?"
        deleteProfileConfirm.open()
    }

    // Home button / Escape. A rematch challenge still waiting for the opponent is withdrawn.
    function leaveGame() {
        if (_pendingRematch && _pendingChallengeId !== "")
            LichessClient.cancelChallenge(_pendingChallengeId)
        ChessConnector.endGame()
    }

    function switchUser() {
        if (_inGame && !_gameOver && _mode !== "replay")
            switchUserConfirm.open()
        else
            _doSwitchUser()
    }
    function _doSwitchUser() {
        if (_pendingRematch && _pendingChallengeId !== "")
            LichessClient.cancelChallenge(_pendingChallengeId)
        ChessConnector.switchUser()
    }

    function playOnline() {
        if (!UserManager.lichessConnected) {
            openProfile()   // link or reconnect the Lichess account there
        } else if (UserManager.hasPendingChallenge) {
            _pendingChallengeId  = UserManager.pendingChallengeId
            _pendingChallengeUrl = UserManager.pendingChallengeUrl
            challengeDialog.open()
            LichessClient.waitForGameStart(_pendingChallengeId)
        } else {
            onlineDialog.open()
        }
    }

    // The result card's "New game": the setup of the same kind of game. The online setup is
    // reached from Home (a challenge made in a game would count as a rematch).
    function newGameLikeThis() {
        if (_mode === "computer") {
            computerDialog.openNew()
        } else if (_mode === "local") {
            twoPlayerDialog.open()
        } else {
            var wasOnline = _mode === "online"
            leaveGame()
            if (wasOnline) playOnline()
        }
    }

    function exportPgn() {
        // Encoded: a '#' or '%' in a player's name is not URL syntax.
        pgnExportDialog.selectedFile = pgnExportDialog.currentFolder + "/"
                                     + encodeURIComponent(ChessConnector.pgnFileName())
        pgnExportDialog.open()
    }

    function clearPendingChallenge() {
        UserManager.clearPendingChallenge()
        _pendingChallengeId  = ""
        _pendingChallengeUrl = ""
        _pendingOpponentName = ""
        _pendingRematch      = false
        challengeDialog.close()
    }

    // ── --screenshot mode (see main.cpp): set up the scene, then C++ grabs the window ──
    readonly property var _shot: ChessConnector.ScreenshotScene
    Component.onCompleted: {
        route()
        if (_shot.scene)
            Qt.callLater(setUpScreenshotScene)
    }

    function setUpScreenshotScene() {
        if (_shot.width)  root.width  = _shot.width
        if (_shot.height) root.height = _shot.height
        AppSettings.theme = _shot.dark ? 2 : 1   // not "system": stable screenshots
        var scene = ({ menu: "home", board: "game", login: "profiles" })[_shot.scene] ?? _shot.scene

        // welcome and create-user: first run. profiles and import: profiles made by main.cpp, nobody
        // logged in. The profile scenes: main.cpp's profiles, logged in as Tester.
        if (scene === "create-user") {
            createUserDialog.open()
        } else if (scene === "import") {
            inspectImport(_shot.importFile)   // Bob again: the conflict dialog
        } else if (scene !== "welcome" && scene !== "profiles" && !UserManager.isLoggedIn) {
            if (UserManager.hasUsers) UserManager.login(UserManager.users[0].id)
            else                      UserManager.createUser("Tester", "wizard")
        }
        // A second profile to play against: same names and avatars every time.
        var gameScene = ["game", "continue", "review", "result", "ended", "moves"].includes(scene)
        if ((gameScene || scene === "players") && UserManager.userCount < 2) {
            var me = UserManager.activeUserId
            UserManager.createUser("Bob", "fox")
            UserManager.login(me)
        }

        if (scene === "settings" || scene === "about" || scene === "restore") openSettings()
        if (scene === "restore")  restoreBackupDialog.open()
        if (scene === "about")    stack.currentItem.showAbout()
        if (scene === "levels")   levelsHelp.open()
        if (scene.startsWith("profile") && scene !== "profiles") openProfile()
        if (scene === "online")   onlineDialog.open()
        if (scene === "computer") computerDialog.openNew()
        if (scene === "players")  twoPlayerDialog.open()
        if (gameScene) {
            // A Scandinavian with captures on both sides (Black +1), and Fool's mate for the result.
            var middlegame = "e2e4 d7d5 e4d5 d8d5 b1c3 d5a5 d2d4 g8f6 g1f3 c8f5 f1c4 e7e6 c1d2 c7c6 "
                           + "c3d5 a5d8 d5f6 d8f6 d1e2 f5c2"
            var defaultMoves = ({ continue: "e2e4 e7e5", review: middlegame, moves: middlegame,
                                  result: "f2f3 e7e5 g2g4 d8h4", ended: "f2f3 e7e5 g2g4 d8h4" })[scene] ?? ""
            var bob = UserManager.users.find(u => u.name === "Bob")
            ChessConnector.startLocalGame(!_shot.black, bob ? bob.id : "")
            if (_shot.moves || defaultMoves)
                ChessConnector.applyMoves(_shot.moves || defaultMoves)
            if (scene === "continue")
                ChessConnector.endGame()   // autosaved: Home offers to continue it
            if (scene === "review")
                ChessConnector.reviewTo(9)
            if (scene === "moves")
                stack.currentItem.sheetOpen = true   // the move sheet (narrow window)
            if (scene === "ended")
                stack.currentItem._resultCardClosed = true   // Rematch / New game in the moves panel
        }

        // A finished game shows its result only after GamePage's resultTimer (1600 ms).
        screenshotReadyTimer.interval = ChessConnector.GameResult !== "" ? 1700 : 0
        screenshotReadyTimer.start()
    }

    Timer {
        id: screenshotReadyTimer
        onTriggered: ChessConnector.screenshotSceneReady()
    }

    Settings {
        property alias width:  root.width
        property alias height: root.height
        property alias x:      root.x
        property alias y:      root.y
    }

    Binding { target: Theme; property: "systemAccent"; value: root.palette.accent }

    readonly property real _sfxVolume: AppSettings.effectsVolume()

    SoundEffect { id: sndMove;         source: "qrc:/sounds/move.wav";          volume: _sfxVolume }
    SoundEffect { id: sndMoveOpponent; source: "qrc:/sounds/move_opponent.wav"; volume: _sfxVolume }
    SoundEffect { id: sndCheck;     source: "qrc:/sounds/check.wav";     volume: _sfxVolume }
    SoundEffect { id: sndCheckmate; source: "qrc:/sounds/checkmate.wav"; volume: _sfxVolume }
    SoundEffect { id: sndWin;       source: "qrc:/sounds/win.wav";       volume: _sfxVolume }

    title: _inGame ? "Chess++ — " + ChessConnector.GameTitle : "Chess++"
    color: Theme.windowBg

    width: 768
    height: 1054
    minimumWidth: 640
    minimumHeight: 560
    visible: true
    // --screenshot: a real window (so the Fluent style gets the forced light/dark palette, which
    // -platform offscreen can't do) that the user never sees or clicks; grabWindow() still
    // captures the rendered scene.
    opacity: _shot.scene ? 0 : 1
    flags: _shot.scene ? Qt.Tool | Qt.FramelessWindowHint | Qt.WindowTransparentForInput
                         | Qt.WindowDoesNotAcceptFocus
                       : Qt.Window

    // ── Pages ────────────────────────────────────────────────────────────
    StackView {
        id: stack
        anchors.fill: parent
    }

    Component {
        id: welcomePage
        WelcomePage {
            onCreateProfileRequested: createUserDialog.open()
            onImportRequested:        profileImportDialog.open()
        }
    }

    Component {
        id: loginPage
        ProfilePickerPage {
            onAddUserRequested:  createUserDialog.open()
            onImportRequested:   profileImportDialog.open()
            onExportRequested:   userId => root.exportProfile(userId)
            onDeleteRequested:   userId => root.deleteProfile(userId)
        }
    }

    Component {
        id: profilePage
        ProfilePage {
            onBackRequested:     root.closeProfile()
            onExportRequested:   userId => root.exportProfile(userId)
            onDeleteRequested:   userId => root.deleteProfile(userId)
            onDisconnectLichessRequested: disconnectLichessConfirm.open()
        }
    }

    Component {
        id: settingsPage
        SettingsPage {
            onBackRequested:        root.closeSettings()
            onLevelsHelpRequested:  levelsHelp.open()
            onExportRequested:      userId => root.exportProfile(userId)
            onImportRequested:      profileImportDialog.open()
            onRestoreRequested:     restoreBackupDialog.open()
        }
    }

    Component {
        id: homePage
        HomePage {
            onComputerGameRequested:  computerDialog.openNew()
            onTwoPlayerGameRequested: twoPlayerDialog.open()
            onOnlineGameRequested:    root.playOnline()
            onSettingsRequested:      root.openSettings()
            onProfileRequested:       root.openProfile()
            onSwitchUserRequested:    root.switchUser()
        }
    }

    Component {
        id: gamePage
        GamePage {
            rematchPending: root._pendingRematch
            onHomeRequested:        root.leaveGame()
            onNewGameRequested:     root.newGameLikeThis()
            onExportPgnRequested:   root.exportPgn()
            onResignRequested:      root.resign()
            onLevelsHelpRequested:  levelsHelp.open()
            onSettingsRequested:    root.openSettings()
            onProfileRequested:     root.openProfile()
            onSwitchUserRequested:  root.switchUser()
        }
    }

    Connections {
        target: UserManager
        function onActiveUserChanged() { root.route() }
        function onUsersChanged()      { root.route() }   // the first profile imported, the last one deleted
    }

    // ── InfoBars: messages and Lichess offers, under the command bar ────────
    Column {
        anchors {
            top: parent.top; topMargin: Theme.commandBarHeight + Theme.s2
            horizontalCenter: parent.horizontalCenter
        }
        width: Math.min(parent.width - 2 * Theme.s4, 520)
        spacing: Theme.s1
        z: 10

        InfoBar {
            id: messageBar
            width: parent.width
            timeout: 4000
        }

        // users.json didn't parse at start: nothing was loaded, and nothing is lost yet.
        InfoBar {
            width: parent.width
            visible: UserManager.dataDamaged
            closable: false
            severity: "critical"
            text: UserManager.canRestoreBackup
                  ? "Your profiles couldn't be read. Restore them from the latest backup?"
                  : "Your profiles couldn't be read and there is no backup. A new profile starts over; the damaged file is kept in the data folder."
            Button {
                visible: UserManager.canRestoreBackup
                text: "Restore"
                highlighted: true
                onClicked: {
                    if (UserManager.restoreFromBackup()) messageBar.show("Profiles restored", "success")
                    else                                 messageBar.show("The backups couldn't be read either", "critical")
                }
            }
        }

        InfoBar {
            width: parent.width
            visible: ChessConnector.PendingOffer === "draw"
            closable: false
            text: "Your opponent offers a draw"
            Button {
                text: "Accept"; highlighted: true
                onClicked: ChessConnector.respondDraw(true)
            }
            Button {
                text: "Decline"
                onClicked: ChessConnector.respondDraw(false)
            }
        }

        InfoBar {
            width: parent.width
            visible: ChessConnector.PendingOffer === "takeback"
            closable: false
            text: "Your opponent asks to take back a move"
            Button {
                text: "Accept"; highlighted: true
                onClicked: ChessConnector.respondTakeback(true)
            }
            Button {
                text: "Decline"
                onClicked: ChessConnector.respondTakeback(false)
            }
        }

        // Lichess "opponentGone": counts down until Lichess allows claiming the win.
        InfoBar {
            id: opponentGoneBar
            property int secondsLeft: -1   // <= 0: the win can be claimed now

            width: parent.width
            visible: ChessConnector.OpponentGone
            closable: false
            severity: "caution"
            text: secondsLeft > 0 ? "Your opponent left the game. You can claim victory in " + secondsLeft + " s."
                                  : "Your opponent left the game."

            Connections {
                target: ChessConnector
                function onOpponentGoneChanged() { opponentGoneBar.secondsLeft = ChessConnector.ClaimWinSeconds }
            }
            Timer {
                interval: 1000
                repeat: true
                running: opponentGoneBar.visible && opponentGoneBar.secondsLeft > 0
                onTriggered: opponentGoneBar.secondsLeft--
            }
            Button {
                text: "Claim victory"
                highlighted: true
                enabled: opponentGoneBar.secondsLeft <= 0
                onClicked: ChessConnector.claimVictory()
            }
        }
    }

    Connections {
        target: ChessConnector
        function onGameInfoChanged() { root.route() }

        function onMoveCountChanged() {
            if (!AppSettings.musicEnabled || !ChessConnector.InGame || ChessConnector.MoveCount === 0)
                return
            var snd = (ChessConnector.MoveCount % 2 === 1) ? sndMove : sndMoveOpponent
            if (snd.status === SoundEffect.Ready)
                snd.play()
        }
        function onCheckNotify() {
            if (AppSettings.musicEnabled)
                sndCheck.play()
            messageBar.show("Check", "caution")
        }
        function onCheckMateNotify() {
            if (AppSettings.musicEnabled)
                sndCheckmate.play()
        }
        function onCheckMateResult(winner) {
            if (AppSettings.musicEnabled)
                sndWin.play()
        }
        function onSavedOk()           { messageBar.show("Game saved", "success") }
        function onNoSavedGame()       { messageBar.show("There is no saved game yet. Use Save during a game to keep one.") }
        function onNoMoreMovesNotify() { messageBar.show("No more moves") }
        function onPgnCopied()         { messageBar.show("PGN copied to the clipboard", "success") }
        function onPgnExported(ok, path) {
            if (ok) messageBar.show("Saved " + path, "success")
            else    messageBar.show("Couldn't save " + path, "critical")
        }
    }

    // Lichess challenge setup: the dialogs and the pending challenge. The game itself starts and
    // ends in ChessConnector.
    Connections {
        target: LichessClient

        // A challenge created while the finished game is still on screen is a rematch
        // (the Online dialog is only reachable from Home).
        function onChallengeCreated(gameId, joinUrl) {
            _pendingChallengeId  = gameId
            _pendingChallengeUrl = joinUrl
            _pendingRematch      = ChessConnector.InGame
            UserManager.savePendingChallenge(gameId, joinUrl)
            onlineDialog.close()
            challengeDialog.open()
        }

        // Not saved with savePendingChallenge: the challenge lives only while the app keeps
        // its request open, so it can't be resumed after a restart.
        function onDirectChallengeSent(challengeId, opponentName) {
            _pendingChallengeId   = challengeId
            _pendingChallengeUrl  = ""
            _pendingOpponentName  = opponentName
            _pendingRematch       = true
            challengeDialog.open()
        }

        function onChallengeDeclined(opponentName) {
            clearPendingChallenge()
            messageBar.show(opponentName + " declined the rematch", "caution")
        }

        function onChallengeCanceled() {
            var wasRematch = _pendingRematch
            clearPendingChallenge()
            if (!wasRematch)
                onlineDialog.open()
        }

        function onGameStarted(gameId, playingAsWhite, opponentName, opponentAvatarUrl) {
            clearPendingChallenge()
            onlineDialog.close()
        }

        function onNetworkError(message) {
            if (challengeDialog.visible && _pendingRematch) {
                clearPendingChallenge()
                messageBar.show("Rematch failed: " + message, "critical")
            } else if (challengeDialog.visible) {
                clearPendingChallenge()
                onlineDialog.open()
            } else if (_mode === "online") {
                messageBar.show(message, "critical")
            }
        }
    }

    // ── Dialogs ──────────────────────────────────────────────────────────
    RestoreBackupDialog {
        id: restoreBackupDialog
        onRestoreRequested: number => {
            if (UserManager.restoreBackup(number)) messageBar.show("Profiles restored from the backup", "success")
            else                                   messageBar.show("Couldn't restore the backup", "critical")
        }
    }

    ComputerGameDialog {
        id: computerDialog
        // One dialog at a time: the levels help replaces it and brings it back.
        onLevelsHelpRequested: {
            levelsHelp.fromSetup = true
            close()
            levelsHelp.open()
        }
    }

    // Computer levels. From the new-game setup, rows pick a level; in a game, the robot's level
    // is highlighted.
    LevelsHelpDialog {
        id: levelsHelp
        property bool fromSetup: false
        selectable: fromSetup
        currentLevel: fromSetup ? Math.round(computerDialog.level)
                    : _mode === "computer" ? ChessConnector.LastLevel
                    : 0
        onLevelPicked: function(level) {
            computerDialog.level = level
            close()
        }
        onClosed: {
            if (fromSetup) {
                fromSetup = false
                computerDialog.open()
            }
        }
    }

    TwoPlayerDialog { id: twoPlayerDialog }

    LichessGameDialog {
        id: onlineDialog
        onProfileRequested: {
            close()
            root.openProfile()
        }
    }

    // Waiting for the opponent to accept a Lichess challenge.
    ChallengeDialog {
        id: challengeDialog
        challengeId:  _pendingChallengeId
        challengeUrl: _pendingChallengeUrl
        opponentName: _pendingOpponentName
        isRematch:    _pendingRematch
    }

    CreateUserDialog {
        id: createUserDialog
        onConfirmed: function(name, avatar) {
            UserManager.createUser(name, avatar)
        }
    }

    // ── Profiles: export, import, delete, Lichess disconnect ──
    FileDialog {
        id: profileExportDialog
        property string userId
        title: "Export profile"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Chess++ profiles (*.chessprofile)", "All files (*)"]
        defaultSuffix: "chessprofile"
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: {
            var error = UserManager.exportProfile(userId, selectedFile)
            if (error !== "") messageBar.show(error, "critical")
            else              messageBar.show("Profile exported", "success")
        }
    }

    FileDialog {
        id: profileImportDialog
        title: "Import profile"
        fileMode: FileDialog.OpenFile
        nameFilters: ["Chess++ profiles (*.chessprofile)", "All files (*)"]
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: root.inspectImport(selectedFile)
    }

    ImportConflictDialog {
        id: importConflictDialog
        property url file
        onImportAsNew:      root.importProfile(file, false)
        onReplaceExisting:  root.importProfile(file, true)
    }

    ConfirmDialog {
        id: deleteProfileConfirm
        property string userId
        text: "Its statistics and saved games are deleted too. This can't be undone; export the profile first to keep a copy."
        acceptText: "Delete"
        onAccepted: {
            if (userId === UserManager.activeUserId)
                root.closeProfile()
            UserManager.deleteUser(userId)
        }
    }

    ConfirmDialog {
        id: disconnectLichessConfirm
        title: "Disconnect Lichess?"
        text: "This PC forgets the Lichess sign-in of " + UserManager.lichessUsername
              + ". You can link the account again at any time."
        acceptText: "Disconnect"
        onAccepted: UserManager.clearLichessToken()
    }

    // Move-input cheat (Alt+S in a two-player game).
    ApplyMovesDialog { id: applyMovesDialog }

    FileDialog {
        id: pgnExportDialog
        title: "Export PGN"
        fileMode: FileDialog.SaveFile
        nameFilters: ["PGN files (*.pgn)", "All files (*)"]
        defaultSuffix: "pgn"
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: ChessConnector.exportPgn(selectedFile)
    }

    ConfirmDialog {
        id: resignConfirm
        title: "Resign this game?"
        text: _mode === "online" ? "The game ends as a loss on Lichess." : ""
        acceptText: "Resign"
        onAccepted: ChessConnector.resignGame()
    }

    ConfirmDialog {
        id: switchUserConfirm
        title: "Switch user?"
        text: _mode === "online" ? "You leave the Lichess game; it keeps running on lichess.org."
                                 : "The game is saved, so you can continue it later."
        acceptText: "Switch user"
        onAccepted: root._doSwitchUser()
    }

    // ── Shortcuts (not while a dialog is open) ───────────────────────────
    Shortcut {
        sequence: "F1"
        enabled: !_dialogOpen && UserManager.isLoggedIn
        onActivated: levelsHelp.open()
    }
    Shortcut {
        sequence: "Ctrl+,"
        enabled: !_dialogOpen && UserManager.isLoggedIn
        onActivated: root.openSettings()
    }
    Shortcut {
        sequence: "Escape"
        enabled: (_onGamePage || _page === "profile" || _page === "settings") && !_dialogOpen
        onActivated: {
            if (_page === "profile")              closeProfile()
            else if (_page === "settings")        closeSettings()
            else if (stack.currentItem.sheetOpen) stack.currentItem.sheetOpen = false   // an open move sheet first
            else                                  leaveGame()
        }
    }
    Shortcut {
        sequence: "F"
        enabled: _onGamePage && !_dialogOpen
        onActivated: stack.currentItem.flipBoard()
    }
    Shortcut {
        sequence: "Ctrl+S"
        enabled: _onGamePage && (_mode === "computer" || _mode === "local") && !_dialogOpen
        onActivated: ChessConnector.saveGame()
    }
    Shortcut {
        sequence: "Ctrl+Z"
        enabled: _onGamePage && (_mode === "computer" || (_mode === "online" && !_gameOver)) && !_dialogOpen
        onActivated: ChessConnector.requestTakeback()
    }
    Shortcut {
        sequence: "Left"
        enabled: _onGamePage && !_dialogOpen
        onActivated: ChessConnector.reviewPrev()
    }
    Shortcut {
        sequence: "Right"
        enabled: _onGamePage && !_dialogOpen
        onActivated: ChessConnector.reviewNext()
    }
    Shortcut {
        sequence: "Home"
        enabled: _onGamePage && !_dialogOpen
        onActivated: ChessConnector.reviewFirst()
    }
    Shortcut {
        sequence: "End"
        enabled: _onGamePage && !_dialogOpen
        onActivated: ChessConnector.reviewLast()
    }
    Shortcut {
        sequence: "Alt+S"
        enabled: _onGamePage && _mode === "local" && !_dialogOpen
        onActivated: applyMovesDialog.open()
    }
}
