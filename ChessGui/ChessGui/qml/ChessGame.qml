import QtQuick
import QtQuick.Controls
import QtCore
import QtMultimedia
import QtQuick.Dialogs
import ChessPlusPlus

// The window: one page at a time in a StackView (Welcome, the profile list, Home or the game, with
// the Profile or Settings page pushed over Home or the game, and the Online page and the Continue list (BL-17) over Home), the
// InfoBars under the command bar,
// and every dialog. UserManager and ChessConnector.InGame decide the page (route()); pages ask for
// dialogs through signals.
ApplicationWindow {
    id: root

    // A Lichess challenge waiting for the opponent (challenge setup stays in QML, see CLAUDE.md).
    property string _pendingChallengeId:  ""
    property string _pendingChallengeUrl: ""
    property string _pendingOpponentName: ""    // set for a challenge to a named player, which isn't saved
    property bool   _pendingRematch:      false // pending challenge is a rematch of the game on screen

    readonly property bool   _inGame:   ChessConnector.InGame
    readonly property string _mode:     ChessConnector.GameMode
    readonly property bool   _gameOver: ChessConnector.GameResult !== ""
    // A Lichess game that goes on without us: leaving it asks first (leaveOnlineDialog).
    readonly property bool   _onlineRunning: _inGame && _mode === "online" && !_gameOver
    // The page on top: "welcome", "login", "home", "game", "profile", "settings" or "online".
    readonly property string _page: stack.currentItem ? stack.currentItem.objectName : ""
    readonly property bool   _onGamePage: _page === "game"

    // A dialog is open: the window's shortcuts wait (Esc then closes the dialog).
    readonly property bool _dialogOpen: restoreBackupDialog.visible || levelsHelp.visible
        || computerDialog.visible || twoPlayerDialog.visible
        || challengeDialog.visible || createUserDialog.visible
        || applyMovesDialog.visible || resignConfirm.visible || switchUserConfirm.visible || leaveOnlineDialog.visible
        || pgnExportDialog.visible || profileExportDialog.visible || profileImportDialog.visible
        || importConflictDialog.visible || deleteProfileConfirm.visible || disconnectLichessConfirm.visible
        || acceptChallengeConfirm.visible || recentFlyout.visible

    // ── BL-10: Lichess challenges to the player ─────────────────────────
    // The bar shows the oldest one not hidden. The finished game's rematch offer has its own bar.
    property var  _hiddenChallenges: ({})   // id → true: "Hide" until the next challenge arrives
    // Lichess's answer to an Accept is awaited (the bar's or the Online page's: one state, BL-16)
    readonly property bool _acceptingChallenge: ChessConnector.AcceptingChallengeId !== ""
    readonly property var _challengesShown: LichessClient.incomingChallenges.filter(c =>
        !_hiddenChallenges[c.id] && c.id !== LichessClient.rematchOfferId)
    readonly property var _challenge: _challengesShown.length > 0 ? _challengesShown[0] : null

    function challengeText(c) {
        let text = c.challengerText + (c.rematchOf !== "" ? " offers a rematch" : " challenges you")
                 + " · " + c.timeControlText + " · " + (c.rated ? "Rated" : "Casual") + " · " + c.sideText
        if (!c.canPlayHere)
            text += " · Can't be played in this app"
        else if (!ChessConnector.CanAcceptChallenge)
            text += ". Finish your game first: it waits on Lichess."
        if (_challengesShown.length > 1)
            text += " (+" + (_challengesShown.length - 1) + " more)"
        return text
    }

    // A two-player game isn't saved: ask before leaving it. A robot game is autosaved, a Lichess game
    // goes on at Lichess.
    function acceptChallenge(id) {
        if (_inGame && _mode === "local" && !_gameOver) {
            acceptChallengeConfirm.challengeId = id
            acceptChallengeConfirm.open()
            return
        }
        ChessConnector.acceptChallenge(id)
    }

    function hideChallenge(id) {
        const hidden = Object.assign({}, _hiddenChallenges)
        hidden[id] = true
        _hiddenChallenges = hidden
    }

    // Green (weakest) → red (strongest), spread over however many levels there are.
    function levelColor(level) {
        var count = ChessConnector.Levels.length
        var t = count > 1 ? (Math.max(1, Math.min(count, level)) - 1) / (count - 1) : 0
        return Qt.hsla((1 - t) * 0.36, 0.65, 0.45, 1)
    }

    // BL-16: route() shows "Starting the game…" (startNotice) and waits for it to be drawn before it
    // builds the game page.
    property bool _startingGame: false
    property bool _startNoticeDrawn: false
    Connections {
        target: ChessConnector
        function onStartFrameShown() {
            if (!root._startingGame)
                return
            root._startNoticeDrawn = true
            root.route()
        }
    }

    // The page under everything follows the state; the Profile and Settings pages stay on top of it
    // until the base page changes (e.g. its profile was deleted).
    function route() {
        var page = UserManager.isLoggedIn ? (ChessConnector.InGame ? "game" : "home")
                 : UserManager.hasUsers ? "login" : "welcome"
        var base = stack.depth > 0 ? stack.get(0, StackView.DontLoad) : null
        if (page !== "game" || (base && base.objectName === page))
            _startingGame = _startNoticeDrawn = false   // no game page to build (any more)
        if (base && base.objectName === page)
            return
        // BL-16: building the game page keeps the GUI thread (150 ms here in Release, 1.5 s in a Debug
        // build, more on a slow PC or a phone). "Starting the game…" first; the page once that is on
        // the screen (ChessConnector.startFrameShown → route() again). Not in the scenes, which use
        // the page as soon as they start a game (but in "timing", which measures this).
        if (page === "game" && base && !_startNoticeDrawn
                && (!_screenshotMode || ChessConnector.ScreenshotScene.scene === "timing")) {
            if (!_startingGame) {
                _startingGame = true
                // First: whatever happens below, the frame comes and the page is built (an error after
                // _startingGame once left the notice up and no page: the game "hung").
                ChessConnector.waitForStartFrame(root)
                // A dialog still open at the start (the challenge waiting for the opponent, the
                // accept confirmation) goes at once: its fade would stay on the screen under the notice.
                for (const d of [challengeDialog, acceptChallengeConfirm])
                    d.closeAtOnce()
            }
            return
        }
        _startingGame = _startNoticeDrawn = false
        var component = ({ welcome: welcomePage, login: loginPage, home: homePage, game: gamePage })[page]
        if (stack.depth === 0) stack.push(component)
        else                   stack.replace(null, component)
        if (page !== "welcome" && page !== "login")
            createUserDialog.close()  // e.g. logged in by a script (--screenshot)
    }

    // The pages pushed over Home or the game, by objectName: Back or Esc pops them.
    readonly property var _pushedPages: ({ profile: profilePage, settings: settingsPage, online: onlinePage,
                                           games: continuePage })
    readonly property bool _onPushedPage: _pushedPages[_page] !== undefined
    function _openPage(name) {
        if (_page !== name)
            stack.push(_pushedPages[name])
    }
    function _closePage(name) {
        if (_page === name)
            stack.pop()
    }
    function openProfile()   { _openPage("profile") }
    function closeProfile()  { _closePage("profile") }
    function openSettings()  { _openPage("settings") }
    function closeSettings() { _closePage("settings") }
    function openOnline()    { _openPage("online") }
    function closeOnline()   { _closePage("online") }

    // BL-17: Home's Continue. The only unfinished game opens directly (ChessConnector decides);
    // otherwise the list of them all.
    function continueGames() {
        if (!ChessConnector.continueOnlyGame())
            _openPage("games")
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
            root.notify(info.error, "critical")
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
        if (error !== "") root.notify(error, "critical")
        else              root.notify("Profile imported", "success")
    }
    function deleteProfile(userId) {
        var user = UserManager.users.find(u => u.id === userId)
        if (!user) return
        deleteProfileConfirm.userId = userId
        deleteProfileConfirm.title = "Delete " + user.name + "'s profile?"
        deleteProfileConfirm.open()
    }

    // Home button / Escape. A running Lichess game asks first; a rematch challenge still waiting for
    // the opponent is withdrawn.
    function leaveGame() {
        if (_resigning) {                 // Home again while the resignation is on its way: now
            _quitAfterLeave = false
            ChessConnector.endGame()
        } else if (_onlineRunning) {
            leaveOnlineDialog.quitting = false
            leaveOnlineDialog.open()
        } else {
            _doLeaveGame()
        }
    }
    function _doLeaveGame() {
        if (_pendingRematch && _pendingChallengeId !== "")
            LichessClient.cancelChallenge(_pendingChallengeId)
        ChessConnector.endGame()
    }

    // The leave warning's answers: Leave / Quit, or Resign, which leaves once Lichess has ended the
    // game (ChessConnector.resignAndLeave(); the window closes then if it was Quit).
    property bool _resigning:      false
    property bool _quitAfterLeave: false
    property bool _quitConfirmed:  false   // the window closes without asking again
    function _leaveOnline(quit, resign) {
        if (resign) {
            _resigning = true
            _quitAfterLeave = quit
            ChessConnector.resignAndLeave()
        } else if (quit) {
            _quitConfirmed = true
            root.close()
        } else {
            _doLeaveGame()
        }
    }
    // The game ended while the warning was open: it no longer goes on without us.
    on_OnlineRunningChanged: if (!_onlineRunning) leaveOnlineDialog.close()

    // Closing the window during a running Lichess game asks the same as Home (Leave quits).
    onClosing: event => {
        if (_quitConfirmed || _screenshotMode || !(_onlineRunning || _resigning))
            return
        event.accepted = false
        if (_resigning) {                 // quit when the resignation is through
            _quitAfterLeave = true
            return
        }
        resignConfirm.close()             // the warning replaces a confirm about this game
        switchUserConfirm.close()
        leaveOnlineDialog.quitting = true
        leaveOnlineDialog.open()
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

    // The Online page (not signed in: it says so and leads to the profile); a challenge still waiting
    // for the friend opens over it.
    function playOnline() {
        openOnline()
        if (UserManager.lichessConnected && UserManager.hasPendingChallenge) {
            _pendingChallengeId  = UserManager.pendingChallengeId
            _pendingChallengeUrl = UserManager.pendingChallengeUrl
            challengeDialog.open()
            LichessClient.resumeOpenChallenge(_pendingChallengeId, _pendingChallengeUrl)
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

    // A move gave check, or a resumed game opened in check: the sound and the bar.
    function announceCheck() {
        if (AppSettings.musicEnabled)
            sndCheck.play()
        root.notify("Check", "caution")
    }

    function clearPendingChallenge() {
        if (_pendingOpponentName === "")   // else a saved link isn't this challenge's
            UserManager.clearPendingChallenge()
        _pendingChallengeId  = ""
        _pendingChallengeUrl = ""
        _pendingOpponentName = ""
        _pendingRematch      = false
        challengeDialog.close()
    }

    Component.onCompleted: {
        _messagesUser = UserManager.isLoggedIn ? UserManager.activeUserId : ""
        route()
    }

    // ── --screenshot mode (see main.cpp): ScreenshotScene.qml sets up the scene, then C++ grabs the
    // window. Loaded only when a scene is asked for.
    readonly property bool _screenshotMode: !!ChessConnector.ScreenshotScene.scene
    Loader {
        active: root._screenshotMode
        sourceComponent: ScreenshotScene {
            window: root
            pages: stack
            dialogs: ({ challenge: challengeDialog, createUser: createUserDialog, restoreBackup: restoreBackupDialog, levels: levelsHelp,
                        computer: computerDialog, twoPlayer: twoPlayerDialog, leaveOnline: leaveOnlineDialog })
        }
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
    SoundEffect { id: sndLowTime;   source: "qrc:/sounds/lowtime.wav";   volume: _sfxVolume }
    SoundEffect { id: sndChallenge; source: "qrc:/sounds/challenge.wav"; volume: _sfxVolume }

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
    opacity: _screenshotMode ? 0 : 1
    flags: _screenshotMode ? Qt.Tool | Qt.FramelessWindowHint | Qt.WindowTransparentForInput
                         | Qt.WindowDoesNotAcceptFocus
                       : Qt.Window

    // The glass-pieces photo, faint, behind Welcome, the profile picker and Home (their backgrounds are
    // transparent). One item for the three (BL-11): it stays decoded, so a return to Home doesn't decode
    // its 3840 × 2160 again (64 ms each time when it was the page's own: too big for the image cache).
    Image {
        anchors.fill: parent
        source: "qrc:/app/pics/ChessBackground.jpg"
        fillMode: Image.PreserveAspectCrop
        opacity: ["welcome", "login", "home"].includes(root._page) ? Theme.homeBackdropOpacity : 0
        visible: opacity > 0
        Behavior on opacity { enabled: !Theme.reduceMotion; NumberAnimation { duration: Theme.normal } }
    }

    // The pieces' images, kept decoded for the session (BL-11): Qt's image cache keeps only about 2 MB of
    // images that no item shows, less than the 12 pieces (4.3 MB), so every game page decoded them again
    // (4–7 ms each in a Debug build). Made after the window's first frame, so as not to delay it.
    Loader {
        id: pieceImages
        active: false
        sourceComponent: Item {
            visible: false
            Repeater {
                model: ["K", "Q", "R", "B", "N", "P", "k", "q", "r", "b", "n", "p"]
                Image {
                    required property string modelData
                    source: Theme.piece(modelData)
                    fillMode: Image.PreserveAspectFit   // as on the board: part of the image cache's key
                }
            }
        }
    }
    Connections {
        target: root
        enabled: !pieceImages.active
        function onFrameSwapped() { pieceImages.active = true }
    }

    // ── Pages ────────────────────────────────────────────────────────────
    StackView {
        id: stack
        anchors.fill: parent

        // WinUI page transitions: the new page fades in and rises a little; the old one fades out.
        // Reduce animations makes them instant, not disabled: a disabled popEnter wouldn't undo the
        // fade-out of the page underneath, which would come back invisible.
        readonly property int _normal: Theme.reduceMotion ? 0 : Theme.normal
        readonly property int _fast:   Theme.reduceMotion ? 0 : Theme.fast
        pushEnter: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: stack._normal; easing.type: Easing.OutCubic }
            NumberAnimation { property: "y"; from: Theme.s5; to: 0; duration: stack._normal; easing.type: Easing.OutCubic }
        }
        pushExit: Transition {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: stack._fast }
        }
        popEnter: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: stack._normal; easing.type: Easing.OutCubic }
            NumberAnimation { property: "y"; to: 0; duration: 0 }   // in case its push was cut short
        }
        popExit: Transition {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: stack._fast }
            NumberAnimation { property: "y"; from: 0; to: Theme.s5; duration: stack._fast }
        }
        replaceEnter: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: stack._normal; easing.type: Easing.OutCubic }
            NumberAnimation { property: "y"; from: Theme.s5; to: 0; duration: stack._normal; easing.type: Easing.OutCubic }
        }
        replaceExit: Transition {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: stack._fast }
        }
    }

    // BL-16: "Starting the game…" while the game page is built (route()). It takes the clicks meanwhile.
    // It fades in late, by an Animator (on the render thread, like the ring's turn: both go on while the
    // GUI thread builds the page): a start of 150 ms barely shows it, a slow one (a Debug build, a slow
    // PC or a phone) has it after about 0.3 s. Not a motion: also with Reduce animations.
    // Over the popups too (the Overlay layer): a setup dialog that started the game is still closing
    // (its fade waits for the GUI thread as well), and would cover it.
    MouseArea {
        id: startNotice
        objectName: "startNotice"
        parent: Overlay.overlay
        anchors.fill: parent
        visible: root._startingGame
        hoverEnabled: true
        acceptedButtons: Qt.AllButtons
        z: 1000
        onVisibleChanged: if (visible) noticeFade.restart(); else noticeFade.stop()

        Item {
            id: noticeLayer
            anchors.fill: parent
            opacity: 0

            Rectangle {   // the page behind, dimmed as under a dialog
                anchors.fill: parent
                color: Theme.scrim
            }
            Rectangle {
                id: noticeCard
                anchors.centerIn: parent
                width: noticeRow.implicitWidth + 2 * Theme.s5
                height: noticeRow.implicitHeight + 2 * Theme.s5
                radius: Theme.radiusCard
                color: Theme.layerBg
                border.color: Theme.cardStroke
                Accessible.role: Accessible.AlertMessage
                Accessible.name: noticeLabel.text

                Row {
                    id: noticeRow
                    anchors.centerIn: parent
                    spacing: Theme.s3
                    BusyRing {
                        width: Theme.iconSize
                        height: Theme.iconSize
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Label {
                        id: noticeLabel
                        text: "Starting the game…"
                        font: Theme.bodyStrong
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }
        OpacityAnimator {
            id: noticeFade
            target: noticeLayer
            from: 0
            to: 1
            duration: 450
            easing.type: Easing.InExpo   // ~0 for the first 250 ms
        }
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
        id: onlinePage
        OnlinePage {
            onBackRequested:    root.closeOnline()
            onProfileRequested: root.openProfile()
        }
    }

    Component {
        id: continuePage
        ContinueGamesPage {
            onBackRequested: root._closePage("games")
        }
    }

    Component {
        id: homePage
        HomePage {
            onComputerGameRequested:  computerDialog.openNew()
            onTwoPlayerGameRequested: twoPlayerDialog.open()
            onOnlineGameRequested:    root.playOnline()
            onContinueRequested:      root.continueGames()
            continueBusy:             ChessConnector.StartingOnlineGame
            onSettingsRequested:      root.openSettings()
            onProfileRequested:       root.openProfile()
            onSwitchUserRequested:    root.switchUser()
            unseenMessages:           root.unseenMessages
            onRecentMessagesRequested: (anchor) => root.openRecentMessages(anchor)
        }
    }

    Component {
        id: gamePage
        GamePage {
            rematchPending: root._pendingRematch
            infoBarsHeight: infoBars.height
            unseenMessages: root.unseenMessages
            onRecentMessagesRequested: (anchor) => root.openRecentMessages(anchor)
            onHomeRequested:        root.leaveGame()
            onNewGameRequested:     root.newGameLikeThis()
            onExportPgnRequested:   root.exportPgn()
            onResignRequested:      root.resign()
            onLevelsHelpRequested:  levelsHelp.open()
            onSettingsRequested:    root.openSettings()
            onProfileRequested:     root.openProfile()
            onSwitchUserRequested:  root.switchUser()
            onCheckShown:           root.announceCheck()
            onLowTimeShown:         if (AppSettings.musicEnabled) sndLowTime.play()
        }
    }

    Connections {
        target: UserManager
        function onActiveUserChanged() { root.route(); root._userChanged() }
        function onUsersChanged()      { root.route() }   // the first profile imported, the last one deleted
    }

    // ── Messages over the command bar (BL-13 step 3) ──────────────────────
    // They take no room, so the board never moves for them: centred on the board on the game page, kept
    // inside the command bar's free middle (CommandBar.messageArea). Two of them, one shown at a time:
    // the game's state (an offer, the opponent left, the lost connection) before a short message ("Check",
    // "Game saved", an error); a short message takes the place for its time (an error until it is closed),
    // then the state comes back. Only what doesn't fit there shows in the InfoBars below (a narrow window,
    // pages without a command bar).
    readonly property var _header: stack.currentItem ? stack.currentItem.header ?? null : null
    readonly property bool _hasMessageArea: _header !== null && (_header.messageArea ?? null) !== null

    // The game's state, by priority: an offer or the opponent left (they wait for an answer), then the
    // lost connection.
    readonly property string _offer: ChessConnector.PendingOffer   // "", "draw", "takeback", "rematch"
    readonly property bool _gone: ChessConnector.OpponentGone
    readonly property bool _reconnecting: ChessConnector.Reconnecting && ChessConnector.GameResult === ""
    readonly property string _statusKind: _offer !== "" ? "offer" : _gone && !_goneDismissed ? "gone" : _reconnecting ? "connection" : ""
    function _offerTextFor(offer) {
        return offer === "draw"     ? ChessConnector.OpponentName + " offers a draw"
             : offer === "takeback" ? ChessConnector.OpponentName + " asks to take back a move"
             : offer === "rematch"  ? ChessConnector.OpponentName + " offers a rematch" : ""
    }
    readonly property string _offerText: _offerTextFor(_offer)
    // The opponent left: the seconds until Lichess allows the claim (<= 0: now), and whether it takes
    // it now (only on the opponent's turn: on the player's, a move first). ChessConnector counts down.
    readonly property int _claimSeconds: ChessConnector.ClaimWinSeconds
    readonly property bool _canClaim: ChessConnector.CanClaimVictory
    readonly property string _goneText: _claimSeconds > 0 ? "Opponent left · claim victory in " + _claimSeconds + " s"
                                      : _canClaim ? "Opponent left the game" : "Opponent left · your move, then claim victory"
    // Closed with its ×: hidden until the claim becomes possible or the opponent comes back or leaves
    // again (the recent messages keep Claim victory).
    property bool _goneDismissed: false
    on_CanClaimChanged: if (_canClaim) _goneDismissed = false
    // Over the command bar, or waiting there behind a short message: not in the InfoBars.
    function _statusShows(kind) { return statusMessage.fits && root._statusKind === kind }
    // The state waits for the player's answer (an offer, Claim victory): a short message never hides it
    // for longer than its 4 s (an error too: it stays in the recent messages), "Check" not at all (the
    // board and the sound tell it), and a message shown when it comes gives way at once.
    readonly property bool _answerable: _statusKind === "offer" || (_statusKind === "gone" && _canClaim)
    on_AnswerableChanged: if (_answerable) _closeMessage(_message)

    // A short message: the newest replaces the one shown, except that an error stays until it is closed
    // (a newer message meanwhile goes only to the recent messages, with the dot).
    property var _message: null        // { text, severity }
    Timer { id: messageTimer; interval: 4000; onTriggered: root._message = null }
    function notify(text, severity, external) {
        severity = severity ?? "info"
        const behindError = _message !== null && _message.severity === "critical" && severity !== "critical"
        if (text !== "Check")   // the board, the sound and the move list ("+") tell it already
            root._addRecent(text, severity, external || severity === "critical" || behindError, "message", "")
        if (behindError || (text === "Check" && _answerable))
            return
        _message = { text: text, severity: severity }
        if (severity === "critical" && !_answerable) messageTimer.stop()
        else                                         messageTimer.restart()
    }
    function _closeMessage(m) {
        if (m === _message) { _message = null; messageTimer.stop() }
    }
    // The InfoBar's one: a message that doesn't fit the command bar.
    readonly property var _barMessage: _message !== null && !shortMessage.showing ? _message : null

    component BarMessage: CommandBarMessage {
        room: root._hasMessageArea ? root._header.messageWidth : 0
        titleWidth: root._hasMessageArea ? root._header.messageTextWidth : 0
        // On the game page over the board's middle (right of the window's centre with the player column).
        centerX: root._onGamePage ? stack.currentItem.boardCenterX - root._header.x - root._header.messageX : room / 2
        x: root._hasMessageArea ? stack.currentItem.x + root._header.x + root._header.messageX : 0
        y: root._hasMessageArea ? stack.currentItem.y + root._header.y : 0
        width: room
        height: Theme.commandBarHeight - 1   // the bar's bottom divider stays visible
        z: 11
    }

    BarMessage {
        id: statusMessage
        active: root._statusKind !== "" && !shortMessage.showing   // a short message has the place for its time
        severity: root._statusKind === "offer" ? "info" : "caution"
        text: root._statusKind === "offer" ? root._offerText
            : root._statusKind === "gone" ? root._goneText
            : "Reconnecting to Lichess…"
        closable: root._statusKind === "gone"
        onCloseRequested: root._goneDismissed = true
        tooltip: root._statusKind === "connection" ? "The connection to Lichess was lost. Your clock keeps running on Lichess." : ""
        Button {
            visible: root._statusKind === "offer"
            text: "Accept"; highlighted: true
            onClicked: root.respondOffer(true)
        }
        Button {
            visible: root._statusKind === "offer"
            text: "Decline"
            onClicked: root.respondOffer(false)
        }
        Button {
            visible: root._statusKind === "gone" && root._canClaim
            text: "Claim victory"; highlighted: true
            onClicked: ChessConnector.claimVictory()
        }
    }

    BarMessage {
        id: shortMessage
        active: root._message !== null
        text: root._message ? root._message.text : ""
        severity: root._message ? root._message.severity : "info"
        closable: severity === "critical"
        clickToClose: true
        onCloseRequested: root._closeMessage(root._message)
    }

    // ── Recent messages (BL-13 step 3, §9.9): for a message the player missed ──
    // The last 20 of this session, newest first, all but "Check". An offer keeps its Accept / Decline in
    // the list while it is open. The More button (Home: the account button) shows a dot for new ones the
    // player didn't cause: an offer, the opponent left, the connection, a decline, an error.
    ListModel { id: recentMessages }
    property int _recentId: 0
    property int unseenMessages: 0
    // They are the profile's: another one (or the profile list) starts without them.
    property string _messagesUser: ""
    function _userChanged() {
        const user = UserManager.isLoggedIn ? UserManager.activeUserId : ""
        if (user === _messagesUser)
            return
        _messagesUser = user
        recentFlyout.close()
        recentMessages.clear()
        unseenMessages = 0
        _offerRid = 0
        _goneRid = 0
        _closeMessage(_message)
    }
    function _addRecent(text, severity, external, kind, offer) {
        var rid = ++_recentId
        recentMessages.insert(0, { rid: rid, text: text, severity: severity, kind: kind, offer: offer,
                                   status: kind === "message" ? "" : "open",
                                   time: root._screenshotMode ? "12:41" : Qt.formatTime(new Date(), "HH:mm"),   // stable pictures
                                   source: ChessConnector.InGame ? ChessConnector.GameTitle : "" })
        if (recentMessages.count > 20)
            recentMessages.remove(20, recentMessages.count - 20)
        if (external && !recentFlyout.visible)
            unseenMessages++
        return rid
    }
    function _setRecentStatus(rid, status) {
        for (var i = 0; i < recentMessages.count; ++i)
            if (recentMessages.get(i).rid === rid) { recentMessages.setProperty(i, "status", status); return }
    }
    property int _offerRid: 0
    property string _offerAnswer: ""   // how the open offer ended: "accepted", "declined"; else it was withdrawn
    property int _goneRid: 0
    function respondOffer(accept) {
        _offerAnswer = accept ? "accepted" : "declined"
        if (_offer === "draw")          ChessConnector.respondDraw(accept)
        else if (_offer === "takeback") ChessConnector.respondTakeback(accept)
        else if (_offer === "rematch")  ChessConnector.respondRematch(accept)
    }
    Connections {
        target: ChessConnector
        // ChessConnector's own values: a binding on them (root._offer) may not be updated yet here.
        function onPendingOfferChanged() {
            const offer = ChessConnector.PendingOffer
            if (root._offerRid) root._setRecentStatus(root._offerRid, root._offerAnswer || "withdrawn")
            root._offerRid = 0
            root._offerAnswer = ""
            if (offer !== "")
                root._offerRid = root._addRecent(root._offerTextFor(offer), "info", true, "offer", offer)
        }
        // Only when the opponent leaves or comes back (not for a new countdown).
        function onOpponentGoneChanged() {
            root._goneDismissed = false
            if (ChessConnector.OpponentGone && !root._goneRid)
                root._goneRid = root._addRecent("Your opponent left the game", "caution", true, "gone", "")
            else if (!ChessConnector.OpponentGone && root._goneRid) {
                root._setRecentStatus(root._goneRid, "")
                root._goneRid = 0
            }
        }
        function onReconnectingChanged() {
            if (ChessConnector.Reconnecting && ChessConnector.GameResult === "")
                root._addRecent("Connection to Lichess lost", "caution", true, "message", "")
        }
    }
    // anchor: the button whose menu opened it (the list opens under it); null: Ctrl+M, under the
    // command bar's right end.
    function openRecentMessages(anchor) {
        var w = recentFlyout.panelWidth
        if (anchor) {
            var p = anchor.mapToItem(root.contentItem, 0, anchor.height)
            var right = p.x + anchor.width
            recentFlyout.panelX = Math.max(Theme.s2, Math.min(right > root.width / 2 ? right - w : p.x, root.width - w - Theme.s2))
            recentFlyout.panelY = p.y + Theme.s1
        } else {
            recentFlyout.panelX = root.width - w - Theme.s2
            recentFlyout.panelY = Theme.commandBarHeight + Theme.s1
        }
        unseenMessages = 0
        recentFlyout.open()
    }
    MessagesFlyout {
        id: recentFlyout
        anchors.fill: parent
        model: recentMessages
        canClaim: root._canClaim
        onOfferAnswered: (accept) => root.respondOffer(accept)
        onClearRequested: { recentMessages.clear(); root.unseenMessages = 0 }
    }

    // ── InfoBars: messages and Lichess offers, under the command bar ────────
    // The game page makes room for them (GamePage.infoBarsHeight); the other pages centre or scroll their
    // content. With the game page's player column (BL-13), they go between its cards (GamePage.barsArea),
    // and scroll there when they don't fit.
    readonly property rect _barsArea: _onGamePage ? stack.currentItem.barsArea : Qt.rect(0, 0, 0, 0)
    readonly property bool _barsInColumn: _barsArea.width > 0
    Flickable {
        id: barsFlick
        x: root._barsInColumn ? stack.currentItem.x + root._barsArea.x : Math.round((parent.width - width) / 2)
        y: root._barsInColumn ? stack.currentItem.y + root._barsArea.y : Theme.commandBarHeight + Theme.s2
        width: root._barsInColumn ? root._barsArea.width : Math.min(parent.width - 2 * Theme.s4, 520)
        height: root._barsInColumn ? Math.min(contentHeight, root._barsArea.height) : contentHeight
        contentHeight: infoBars.height
        interactive: contentHeight > height
        clip: interactive
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: barsFlick.interactive ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff }
        z: 10

        Column {
            id: infoBars
            width: parent.width
            spacing: Theme.s1

            // A short message that doesn't fit over the command bar, waits behind an error there, or a
            // page without a command bar.
            InfoBar {
                width: parent.width
                visible: root._barMessage !== null
                text: root._barMessage ? root._barMessage.text : ""
                severity: root._barMessage ? root._barMessage.severity : "info"
                hideOnClose: false
                onClosed: root._closeMessage(root._barMessage)
            }

            // BL-10: a Lichess challenge to the player, over any page.
            InfoBar {
                id: challengeBar
                objectName: "challengeBar"
                width: parent.width
                visible: root._challenge !== null && UserManager.isLoggedIn && root._page !== "online"   // the page lists them all
                closable: false
                actionsBelow: true
                text: root._challenge ? root.challengeText(root._challenge) : ""
                // During a running real-time Lichess game: Decline only (the text says why).
                BusyButton {
                    visible: root._challenge !== null && root._challenge.canPlayHere && ChessConnector.CanAcceptChallenge
                    idleText: "Accept"
                    busy: root._challenge !== null && ChessConnector.AcceptingChallengeId === root._challenge.id
                    busyText: "Accepting…"
                    highlighted: true
                    enabled: !root._acceptingChallenge
                    onClicked: root.acceptChallenge(root._challenge.id)
                }
                Button {
                    visible: root._challenge !== null && !root._challenge.canPlayHere
                    text: "Open on lichess.org"
                    onClicked: Qt.openUrlExternally(root._challenge.url)
                }
                Button {
                    text: "Decline"
                    onClicked: LichessClient.declineChallenge(root._challenge.id)
                }
                Button {
                    text: "Hide"
                    flat: true
                    onClicked: root.hideChallenge(root._challenge.id)
                }
            }

            // users.json didn't parse at start: nothing was loaded, and nothing is lost yet.
            InfoBar {
                width: parent.width
                visible: UserManager.dataDamaged
                actionsBelow: root._barsInColumn
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
                        if (UserManager.restoreFromBackup()) root.notify("Profiles restored", "success")
                        else                                 root.notify("The backups couldn't be read either", "critical")
                    }
                }
            }

            // The game's state when it isn't over the command bar (it doesn't fit, or waits behind
            // another): an offer, the lost connection, the opponent left.
            InfoBar {
                width: parent.width
                visible: root._offer !== "" && !root._statusShows("offer")
                actionsBelow: root._barsInColumn
                closable: false
                text: root._offerText
                Button {
                    text: "Accept"; highlighted: true
                    onClicked: root.respondOffer(true)
                }
                Button {
                    text: "Decline"
                    onClicked: root.respondOffer(false)
                }
            }

            // LI-06: the Lichess game's stream was lost; LichessClient reconnects on its own.
            InfoBar {
                width: parent.width
                visible: root._reconnecting && !root._statusShows("connection")
                closable: false
                severity: "caution"
                text: "Connection to Lichess lost. Reconnecting… Your clock keeps running on Lichess."
            }

            // Lichess "opponentGone": counts down until Lichess allows claiming the win.
            InfoBar {
                width: parent.width
                visible: root._gone && !root._goneDismissed && !root._statusShows("gone")
                actionsBelow: root._barsInColumn
                hideOnClose: false
                onClosed: root._goneDismissed = true
                severity: "caution"
                text: root._claimSeconds > 0 ? "Your opponent left the game. You can claim victory in " + root._claimSeconds + " s."
                    : root._canClaim ? "Your opponent left the game."
                    : "Your opponent left the game. Make your move, then you can claim victory."
                Button {
                    visible: root._canClaim
                    text: "Claim victory"
                    highlighted: true
                    onClicked: ChessConnector.claimVictory()
                }
            }
        }
    }   // Flickable

    Connections {
        target: ChessConnector
        function onGameInfoChanged() {
            root.route()
            if (!ChessConnector.InGame && root._resigning) {   // resignAndLeave() is through
                root._resigning = false
                if (root._quitAfterLeave) {
                    root._quitAfterLeave = false
                    root._quitConfirmed = true
                    root.close()
                }
            }
        }

        function onMoveCountChanged() {
            if (!AppSettings.musicEnabled || !ChessConnector.InGame || ChessConnector.MoveCount === 0)
                return
            var snd = (ChessConnector.MoveCount % 2 === 1) ? sndMove : sndMoveOpponent
            if (snd.status === SoundEffect.Ready)
                snd.play()
        }
        function onCheckNotify() { root.announceCheck() }
        function onCheckMateNotify() {
            if (AppSettings.musicEnabled)
                sndCheckmate.play()
        }
        function onCheckMateResult(winner) {
            if (AppSettings.musicEnabled)
                sndWin.play()
        }
        function onSavedOk()           { root.notify("Game saved", "success") }
        function onNoSavedGame()       { root.notify("There is no saved game yet. Use Save during a game to keep one.") }
        function onNoMoreMovesNotify() { root.notify("No more moves") }
        function onPgnCopied()         { root.notify("PGN copied to the clipboard", "success") }
        function onMovesTakenBack(plies) { root.notify(plies === 1 ? "Move taken back" : plies + " moves taken back") }
        function onReconnected()       { root.notify("Reconnected to Lichess", "success", true) }
        function onPgnExported(ok, path) {
            if (ok) root.notify("Saved " + path, "success")
            else    root.notify("Couldn't save " + path, "critical")
        }
        // BL-17: an unfinished Lichess game couldn't be rejoined (e.g. it ended meanwhile), from Home,
        // the Continue list or the Online page.
        function onGameStartFailed(message) { root.notify("Couldn't continue the game: " + message, "critical") }
    }

    // Lichess challenge setup: the dialogs and the pending challenge. The game itself starts and
    // ends in ChessConnector.
    Connections {
        target: LichessClient

        // A challenge created while the finished game is still on screen is a rematch
        // (the Online page is only reachable from Home).
        function onChallengeCreated(gameId, joinUrl) {
            _pendingChallengeId  = gameId
            _pendingChallengeUrl = joinUrl
            _pendingRematch      = ChessConnector.InGame
            UserManager.savePendingChallenge(gameId, joinUrl)
            challengeDialog.open()
        }

        // Not saved with savePendingChallenge: the challenge lives only while the app keeps
        // its request open, so it can't be resumed after a restart.
        function onDirectChallengeSent(challengeId, opponentName) {
            _pendingChallengeId   = challengeId
            _pendingChallengeUrl  = ""
            _pendingOpponentName  = opponentName
            _pendingRematch       = ChessConnector.InGame
            challengeDialog.open()
        }

        function onDirectChallengeLeft(opponentName) {
            root.notify((ChessConnector.InGame ? "Rematch" : "Challenge") + " sent to " + opponentName
                            + ". Once accepted, the game is under Your games on the Online page.", "success")
        }

        function onChallengeDeclined(opponentName) {
            const what = _pendingRematch ? "rematch" : "challenge"
            clearPendingChallenge()
            root.notify(opponentName + " declined the " + what, "caution", true)
        }

        // Back to the Online page, which is under the dialog (a rematch: the game page).
        function onChallengeCanceled() { clearPendingChallenge() }

        function onGameStarted(gameId, playingAsWhite, opponentName, opponentAvatarUrl) {
            challengeDialog.closeAtOnce()   // BL-16: not a fade under "Starting the game…"
            clearPendingChallenge()
        }

        // BL-10: a new challenge to the player; one resent after a reconnect isn't new.
        function onChallengeReceived(challengeId) {
            if (AppSettings.musicEnabled)
                sndChallenge.play()
            if (!root.active)
                root.alert(0)   // the taskbar button flashes until the window is activated
        }

        // e.g. the challenger canceled meanwhile. The Online page shows its own error.
        function onChallengeAcceptFailed(challengeId, message) {
            if (_page !== "online")
                root.notify("Couldn't accept the challenge: " + message, "critical")
        }

        function onNetworkError(message) {
            if (challengeDialog.visible && _pendingRematch) {
                clearPendingChallenge()
                root.notify("Rematch failed: " + message, "critical")
            } else if (challengeDialog.visible) {
                clearPendingChallenge()   // the Online page under it shows the error
            } else if (_mode === "online") {
                root.notify(message, "critical")
            }
        }
    }

    // ── Dialogs ──────────────────────────────────────────────────────────
    RestoreBackupDialog {
        id: restoreBackupDialog
        onRestoreRequested: number => {
            if (UserManager.restoreBackup(number)) root.notify("Profiles restored from the backup", "success")
            else                                   root.notify("Couldn't restore the backup", "critical")
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
            if (error !== "") root.notify(error, "critical")
            else              root.notify("Profile exported", "success")
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
        id: acceptChallengeConfirm
        property string challengeId
        title: "Leave this game?"
        text: "A two-player game isn't saved. The Lichess game starts instead."
        acceptText: "Accept challenge"
        onAccepted: ChessConnector.acceptChallenge(challengeId)
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
        text: _mode === "online" ? "You leave the Lichess game; it keeps running on lichess.org. You can come back from Continue or Play online."
                                 : "The game is saved, so you can continue it later."
        acceptText: "Switch user"
        onAccepted: root._doSwitchUser()
    }

    LeaveOnlineDialog {
        id: leaveOnlineDialog
        onAccepted:        root._leaveOnline(quitting, false)
        onResignRequested: root._leaveOnline(quitting, true)
    }

    // ── Shortcuts (not while a dialog is open) ───────────────────────────
    Shortcut {
        sequence: "F1"
        enabled: !_dialogOpen && UserManager.isLoggedIn
        onActivated: levelsHelp.open()
    }
    Shortcut {
        sequence: "Ctrl+M"
        enabled: !_dialogOpen && UserManager.isLoggedIn
        onActivated: root.openRecentMessages(null)
    }
    Shortcut {
        sequence: "Ctrl+,"
        enabled: !_dialogOpen && UserManager.isLoggedIn
        onActivated: root.openSettings()
    }
    Shortcut {
        sequence: "Escape"
        // Not while a piece is dragged: then Esc puts it back (ChessBoard).
        enabled: (_onGamePage || _onPushedPage) && !_dialogOpen
                 && !(_onGamePage && stack.currentItem.pieceDragging)
        onActivated: {
            if (_onPushedPage)                         _closePage(_page)
            else if (stack.currentItem.sheetOpen)      stack.currentItem.sheetOpen = false   // an open move sheet first
            else if (stack.currentItem.premovePending) ChessConnector.cancelPremove()       // then a premove (LI-05)
            else                                       leaveGame()
        }
    }
    // New game: on Home the game vs the computer; in a finished game the result card's New game
    // (a running game isn't dropped).
    Shortcut {
        sequence: "Ctrl+N"
        enabled: (_page === "home" || (_onGamePage && _gameOver)) && !_dialogOpen
        onActivated: _page === "home" ? computerDialog.openNew() : root.newGameLikeThis()
    }
    Shortcut {
        sequence: "Ctrl+O"
        enabled: _page === "home" && !_dialogOpen
        onActivated: ChessConnector.loadGame()   // says so itself when there is none (onNoSavedGame)
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
