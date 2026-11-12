import QtQuick
import ChessPlusPlus

// --screenshot mode only (see main.cpp, scripts\Take-Screenshot.ps1 and the run-chess-app skill): a
// tool for looking at the app's views without clicking through it. It sets up the scene, waits until
// it is ready, then C++ grabs the window. ChessGame.qml loads it only when a scene is asked for, so a
// normal run never creates it.
Item {
    id: scene

    required property var window       // ChessGame.qml's root: its size and open/inspect functions
    required property var pages        // the window's StackView: the page on top
    required property var dialogs      // the window's dialogs, by name (see ChessGame.qml)

    readonly property var shot: ChessConnector.ScreenshotScene
    property bool _waitsForEngine: false

    // After ChessGame.qml's route(): the first page is there.
    Component.onCompleted: Qt.callLater(setUp)

    function setUp() {
        if (shot.width)  window.width  = shot.width
        if (shot.height) window.height = shot.height
        AppSettings.theme = shot.dark ? 2 : 1   // not "system": stable screenshots
        Theme.forceHighContrast = shot.highContrast
        Theme.forceReduceMotion = true   // repeatable pictures for Compare-Screenshots.ps1
        var name = ({ menu: "home", board: "game", login: "profiles" })[shot.scene] ?? shot.scene

        // welcome and create-user: first run. profiles and import: profiles made by main.cpp, nobody
        // logged in. The profile scenes: main.cpp's profiles, logged in as Tester.
        if (name === "create-user") {
            dialogs.createUser.open()
        } else if (name === "import") {
            window.inspectImport(shot.importFile)   // Bob again: the conflict dialog
        } else if (name !== "welcome" && name !== "profiles" && !UserManager.isLoggedIn) {
            if (UserManager.hasUsers) UserManager.login(UserManager.users[0].id)
            else                      UserManager.createUser("Tester", "wizard")
        }
        // A second profile to play against: same names and avatars every time.
        var gameScene = ["game", "leave-online", "leave-local", "drag-off", "continue", "continue-list", "continue-list-robot", "continue-list-starting", "review", "result", "ended", "moves", "select", "drag", "slides",
                         "clock", "clock-low", "clock-flag", "clock-flag-draw", "clock-promotion", "clock-continue", "stalemate",
                         "captures", "check", "saved", "more-menu"].includes(name)
        if ((gameScene || name.startsWith("players")) && UserManager.userCount < 2) {
            var me = UserManager.activeUserId
            UserManager.createUser("Bob", "fox")
            if (name === "players-many")   // the opponents wrap: with -Size 640x560 the dialog scrolls
                for (const other of ["Alice", "Carol", "Dan", "Eve", "Frank"])
                    UserManager.createUser(other, "girl")
            UserManager.login(me)
        }

        if (name === "settings" || name === "about" || name === "restore") window.openSettings()
        if (name === "restore")  dialogs.restoreBackup.open()
        if (name === "about")    pages.currentItem.showAbout()
        if (name === "levels")   dialogs.levels.open()
        if (name.startsWith("profile") && name !== "profiles") window.openProfile()
        if (name.startsWith("online")) {
            // main.cpp: Tester with a Lichess account for online and online-empty, the games for online.
            window.openOnline()
            if (name === "online-blitz" || name === "online-challenge") {
                findItem(pages.currentItem, "challengeOpponent").text = "Friend"
                findItem(pages.currentItem, "challengeClock").select(3)   // 5+3
            }
            if (name === "online-challenges-decline")   // the first challenge's reasons
                Qt.callLater(() => findItem(pages.currentItem, "declineButton").clicked())
            if (name === "online-challenge") {   // sent to Friend: no link, Lichess isn't asked
                window._pendingChallengeId = "scene"
                window._pendingOpponentName = "Friend"
                dialogs.challenge.open()
            }
        }
        if (name === "computer") dialogs.computer.openNew()
        if (name.startsWith("players")) dialogs.twoPlayer.open()
        if (name === "robot") {
            // A saved game vs the robot (level 5, the player is Black) where White mates in one,
            // then Continue: the engine finds Qh5#, played on the GUI thread (BL-09). Needs Stockfish.
            ChessConnector.startComputerGame(5, 1)   // the robot starts on move 1 ...
            ChessConnector.applyMoves(shot.moves || "e2e4 f7f6 d2d4 g7g5")
            ChessConnector.endGame()                  // ... and that answer is dropped
            ChessConnector.continueGame()
        }
        if (name === "timing") {
            // BL-11 measurement, not a screenshot (not in all; run with -KeepLog and read the
            // [Timing] lines): game starts as clicks in a running app make them, 1.5 s apart, from
            // Home: vs Robot, leave, Continue, leave, two players, leave, vs Robot as Black (the
            // engine starts thinking at once) and leave while it thinks (the engine's stop); then
            // Settings and the Online page opened and closed (BL-16).
            timingSteps.start()
            return
        }
        if (name === "play") {
            // BL-11 measurement, not a screenshot (not in all; needs Stockfish): a game vs the robot
            // (level 1) played as a user plays it, animations on: clicks and a drag are mouse events
            // (SceneInput), through the board's TapHandler and DragHandler. Nf3 by two clicks, the
            // robot's answer, e4 by a drag, a takeback (Ctrl+Z's), then the review (←, ←, →, →, Home,
            // End). Run with -KeepLog and QT_LOGGING_RULES="chess.frames.debug=true", or under
            // qmlprofiler; "[Play]" lines in the log mark the steps.
            Theme.forceReduceMotion = false
            ChessConnector.startComputerGame(1, 0)
            playSteps.start()
            return
        }
        if (name === "premove-robot") {
            // LI-05 check, needs Stockfish: 1. e4 against the robot (level 1), and Ng1-f3 premoved
            // while it thinks. Its answer comes, then Nf3 is played (by the shot, a second later, the
            // robot has answered that too).
            ChessConnector.startComputerGame(1, 0)
            ChessConnector.movePiece(12, 28)
            ChessConnector.figureSelected(6)
            ChessConnector.figureSelected(21)
            premoveRobotTimer.start()
            return
        }
        if (gameScene) {
            // A Scandinavian with captures on both sides (Black +1), and Fool's mate for the result.
            var middlegame = "e2e4 d7d5 e4d5 d8d5 b1c3 d5a5 d2d4 g8f6 g1f3 c8f5 f1c4 e7e6 c1d2 c7c6 "
                           + "c3d5 a5d8 d5f6 d8f6 d1e2 f5c2"
            var defaultMoves = ({ continue: "e2e4 e7e5", "continue-list": "e2e4 e7e5", "continue-list-robot": "e2e4 e7e5", "continue-list-starting": "e2e4 e7e5", review: middlegame, moves: middlegame, select: "e2e4 e7e5", drag: "e2e4 e7e5",
                                  slides: "e2e4 e7e5", "leave-online": "e2e4 e7e5", "leave-local": "e2e4 e7e5", "drag-off": "e2e4 e7e5",
                                  result: "f2f3 e7e5 g2g4 d8h4", ended: "f2f3 e7e5 g2g4 d8h4",
                                  clock: "e2e4 e7e5 g1f3", "clock-low": "e2e4 e7e5 g1f3", "clock-flag": "e2e4 e7e5",
                                  "clock-continue": "e2e4 e7e5", "clock-promotion": "a2a4 b7b5 a4b5 a7a6 b5a6 c8b7 a6b7 d8c8",
                                  // White takes everything: Black has a lone king, so White's flag is a draw.
                                  "clock-flag-draw": "e2e4 b7b5 f1b5 h7h5 d1h5 c8a6 b5a6 d8c8 h5h8 f7f6 h8g7 c7c6 a6c8 e8d8 c8d7 a7a6 "
                                                   + "g7f8 d8c7 f8f6 c7b7 f6e7 a6a5 d7c6 b7c8 c6a8 b8c6 a8c6 g8f6 e7f6 a5a4 c6a4 c8b7",
                                  // BL-13: Black is down to a lone king, White lost six: White's captured
                                  // pieces wrap in the player column, "+N" on both strips' lines.
                                  captures: "h2h3 b7b5 b1c3 f7f6 c3b5 e7e5 b5a7 b8c6 a7c8 d8c8 f2f4 g8e7 f4e5 a8a2 a1a2 h8g8 e5f6 g7f6 e2e3 c8b7 g2g3 g8g3 d2d3 g3g2 f1g2 c6e5 g2b7 e7d5 b7d5 c7c6 d5c6 "
                                          + "d7c6 g1e2 e5d3 c2d3 e8f7 d3d4 f8a3 b2a3 f7e7 d1c2 h7h6 c2c6 f6f5 c6h6 e7d7 e2f4 d7e7 a2c2 e7d8 c2c6 d8d7 a3a4 d7e7 f4g6 e7e8 e1f2 e8d8 g6e7 d8d7 e7f5 d7d8",
                                  // The page opens in check: "Check" a second later (in the column, wide).
                                  check: "e2e4 f7f6 d1h5",
                                  saved: "e2e4 e7e5",
                                  stalemate: "e2e3 a7a5 d1h5 a8a6 h5a5 h7h5 h2h4 a6h6 a5c7 f7f6 c7d7 e8f7 d7b7 d8d3 "
                                           + "b7b8 d3h7 b8c8 f7g6" })[name] ?? ""
            // White's and Black's time: time stands still in this mode, so a scene sets what it shows.
            var clockTimes = ({ clock: [171000, 163000], "clock-low": [41000, 8450], "clock-flag": [0, 120000], "clock-flag-draw": [0, 120000],
                                "clock-promotion": [30000, 120000], "clock-continue": [95000, 62000] })[name]
            var limitSec = clockTimes ? 180 : 0
            var incSec = clockTimes ? 2 : 0
            var bob = UserManager.users.find(u => u.name === "Bob")
            var robot = ["continue", "continue-list", "continue-list-robot", "continue-list-starting", "clock-continue"].includes(name)
            if (robot)   // only a game vs the robot is autosaved; White: no engine needed
                ChessConnector.startComputerGame(3, 0, limitSec, incSec)
            else
                ChessConnector.startLocalGame(!shot.black, bob ? bob.id : "", limitSec, incSec)
            if (shot.moves || defaultMoves)
                ChessConnector.applyMoves(shot.moves || defaultMoves)
            if (clockTimes)
                ChessConnector.setScreenshotClock(clockTimes[0], clockTimes[1])
            if (robot)
                ChessConnector.endGame()   // autosaved: Home offers to continue it
            if (name === "clock-continue")
                ChessConnector.continueGame()
            if (name === "stalemate")
                ChessConnector.movePiece(58, 44)   // Qe6: played like the player's move, so the game sees it
            if (name === "clock-promotion") {
                ChessConnector.movePiece(49, 56)   // bxa8: the piece picker opens ...
                ChessConnector.setScreenshotClock(0, 120000)   // ... and the time runs out: the pawn goes back
            }
            if (name === "review")
                ChessConnector.reviewTo(9)
            if (name === "moves")
                pages.currentItem.sheetOpen = true   // the move sheet (narrow window)
            if (name === "ended")
                pages.currentItem._resultCardClosed = true   // Rematch / New game in the moves panel
            if (name === "leave-online")
                dialogs.leaveOnline.open()   // the dialog only: an online game needs Lichess
            if (name === "saved")
                ChessConnector.saveGame()   // "Game saved" over the command bar, centred on the board
            if (name === "more-menu")
                pages.currentItem.openMoreMenu()   // made on its first opening (BL-11)
            if (name === "select")
                ChessConnector.figureSelected(6)   // the g1 knight: tinted square, dots on its moves
            if (name === "leave-local")
                window.leaveGame()   // a two-player game with unsaved moves: "Leave without saving?"
            if (name === "drag-off") {
                // The g1 knight dragged off the board, over the bottom player card: over it, not under.
                ChessConnector.figureSelected(6)
                var offBoard = pages.currentItem.board
                offBoard.dragPiece = "N"
                offBoard.dragPoint = Qt.point(offBoard.squareX(4) + 0.5 * offBoard.squareSize,
                                              8.45 * offBoard.squareSize)
                offBoard.dragFrom = 6
            }
            if (name === "drag") {
                // The g1 knight held a little off f3's centre: faded on g1, dots, f3 outlined.
                ChessConnector.figureSelected(6)
                var board = pages.currentItem.board
                board.dragPiece = "N"
                board.dragPoint = Qt.point(board.squareX(21) + 0.65 * board.squareSize,
                                           board.squareY(21) + 0.35 * board.squareSize)
                board.dragFrom = 6
            }
            if (name === "slides") {
                // BL-14 measurement, not a screenshot: animations on (unlike every other scene),
                // a selection, then four moves. Run with --delay 5200 and
                // QT_LOGGING_RULES="chess.frames.debug=true", then read the .png.log for the
                // FrameTrace lines (frametrace.h). The saved picture is an arbitrary frame.
                Theme.forceReduceMotion = false
                slideSelectTimer.start()
                slideMove1Timer.start()
                slideMove2Timer.start()
                slideMove3Timer.start()
                slideMove4Timer.start()
            }
        }
        // BL-17: the Continue list over Home (the robot's game, if any, autosaved by endGame above).
        if (name.startsWith("continue-list"))
            window._openPage("games")
        // BL-16: Lichess (main.cpp: a local server that never answers) hasn't answered yet: the
        // rejoined game's Continue (or Home's card) and the challenge bar's Accept say so.
        if (name === "continue-list-starting" || name === "continue-starting")
            ChessConnector.continueOnlineGame("aEPqtBMc")
        if (name === "challenge-accepting")
            window.acceptChallenge("chal0001")
        // BL-16: "Starting the game…" as while a game page is built (no game here), over the setup
        // dialog that started it (in the app the dialog is closing then): the notice must be on top.
        if (name === "starting") {
            dialogs.computer.openNew()
            window._startingGame = true
        }

        if (name === "home-message")
            window.notify("Profile imported", "success")   // over Home's command bar
        if (name === "messages") {
            // Earlier messages (oldest first), then main.cpp's draw offer; the list opens once it is there.
            window._addRecent("Game saved", "success", false, "message", "")
            window._addRecent("Speedy declined the challenge", "caution", true, "message", "")
            window._addRecent("Connection to Lichess lost", "caution", true, "message", "")
            window._addRecent("Reconnected to Lichess", "success", true, "message", "")
            window._addRecent("Couldn't save C:\\Users\\Tester\\Documents\\Chess\\game.pgn", "critical", true, "message", "")
            messagesOpener.enabled = true
        }
        if (["takeback", "clock-online", "clock-correspondence", "premove", "premove-played", "premove-dropped", "reconnecting", "rejoin",
             "challenge-ingame", "draw-offer", "messages", "opponent-left", "opponent-left-claim", "opponent-left-turn",
             "opponent-left-error"].includes(name)) {
            ChessConnector.screenshotSceneSetUp()   // main.cpp feeds the Lichess game and says when the board is ready
            return
        }
        if (ChessConnector.EngineThinking) _waitsForEngine = true   // the robot's move comes later
        else                               startReadyTimer()
    }

    Connections {
        id: messagesOpener
        enabled: false
        target: ChessConnector
        function onPendingOfferChanged() {
            if (ChessConnector.PendingOffer === "draw") Qt.callLater(() => scene.window.openRecentMessages(null))
        }
    }

    function findItem(item, objectName) {
        if (item.objectName === objectName)
            return item
        for (const child of item.children) {
            const found = findItem(child, objectName)
            if (found)
                return found
        }
        return null
    }

    function startReadyTimer() {
        // A finished game: wait until the result card is actually up, not for a guessed delay.
        if (ChessConnector.GameResult !== "") {
            resultCardTimer.start()
        } else {
            // clock-continue: past the second the clock waits for the page, or the shot races with it.
            // check: "Check" shows once the page has been open a second. online-challenges-decline: the menu has opened.
            readyTimer.interval = shot.scene === "clock-continue" || shot.scene === "check" ? 1500 : shot.scene === "online-challenges-decline" ? 500 : 0
            readyTimer.start()
        }
    }

    // The card comes after the page settles and the board idles (GamePage's chain); poll for it.
    Timer {
        id: resultCardTimer
        interval: 50
        repeat: true
        onTriggered: {
            const page = pages.currentItem
            if (page && page._resultShown) {
                stop()
                readyTimer.interval = 0
                readyTimer.start()
            }
        }
    }

    Connections {
        target: ChessConnector
        enabled: scene._waitsForEngine
        function onEngineThinkingChanged() {
            if (ChessConnector.EngineThinking) return
            scene._waitsForEngine = false
            scene.startReadyTimer()
        }
    }

    Timer {
        id: readyTimer
        onTriggered: ChessConnector.screenshotSceneReady()
    }

    // Scene "timing": how long opening a page takes to its first frame (its creation and push, then
    // what is left for the frame: deferred items, polish, the scene graph's sync), in the log like
    // ChessConnector's game starts. The frame: the one synchronized after the push, as for a game.
    function timePage(name, open) {
        const t0 = Date.now()
        open()
        const created = Date.now() - t0
        const shown = function() {
            ChessConnector.startFrameShown.disconnect(shown)
            console.info("[Timing] " + name + ": " + (Date.now() - t0) + " ms to the first frame (open "
                         + created + ")")
        }
        ChessConnector.startFrameShown.connect(shown)
        ChessConnector.waitForStartFrame(window)
    }

    // Scene "timing" (BL-11): one step every 1.5 s, as clicks in a running app.
    Timer {
        id: timingSteps
        interval: 1500
        repeat: true
        property int step: 0
        onTriggered: {
            const bob = UserManager.users.find(u => u.name === "Bob")
            switch (step++) {
            case 0: ChessConnector.startComputerGame(3, 0); break
            // The player's 1. e4, and leaving before the robot answers (its answer is dropped): the
            // same autosave every run, the robot to move when it is continued.
            case 1: ChessConnector.movePiece(12, 28); ChessConnector.endGame(); break
            case 2: ChessConnector.continueGame(); break
            case 3: ChessConnector.endGame(); break
            case 4: ChessConnector.startLocalGame(true, bob ? bob.id : ""); break
            case 5: ChessConnector.endGame(); break
            case 6: ChessConnector.startComputerGame(5, 1); break   // the robot (White) thinks at once ...
            case 7: ChessConnector.endGame(); break                  // ... and is stopped
            // BL-16: the other pages pushed over Home, created on each push.
            case 8: timePage("Settings", window.openSettings); break
            case 9: window.closeSettings(); break
            case 10: timePage("Online page", window.openOnline); break
            case 11: window.closeOnline(); break
            default:
                stop()
                readyTimer.interval = 0
                readyTimer.start()
            }
        }
    }

    // Scene "play" (BL-11): the steps, each after the previous one's wait.
    function squarePoint(square) {   // a square's middle, in the window
        const board = pages.currentItem.board
        const grid = findItem(pages.currentItem, "chessPiecesGrid")
        return grid.mapToItem(null, board.squareX(square) + board.squareSize / 2,
                              board.squareY(square) + board.squareSize / 2)
    }
    // The player may move now, after `moves` moves (the robot's answer is on the board, its slide done).
    function playerToMove(moves) {
        return ChessConnector.MoveCount === moves && !ChessConnector.EngineThinking && ChessConnector.isBoardIdle()
            && ChessConnector.MovableColor === 1 && findItem(pages.currentItem, "chessPiecesGrid") !== null
    }
    Timer {
        id: playSteps
        interval: 1500   // the page has opened and settled (1 s)
        property int step: 0
        readonly property var steps: [
            // [what, when it can be done, its action, the pause after it (ms): its slide]
            ["select g1 (click)", () => playerToMove(0), () => SceneInput.click(window.contentItem, squarePoint(6)), 400],
            ["Nf3 (click)", () => true, () => SceneInput.click(window.contentItem, squarePoint(21)), 600],
            ["e4 (drag)", () => playerToMove(2), () => SceneInput.drag(window.contentItem, squarePoint(12), squarePoint(28), 300), 600],
            ["takeback", () => playerToMove(4), () => ChessConnector.requestTakeback(), 1000],
            ["review ←", () => true, () => ChessConnector.reviewPrev(), 500],
            ["review ←", () => true, () => ChessConnector.reviewPrev(), 500],
            ["review →", () => true, () => ChessConnector.reviewNext(), 500],
            ["review →", () => true, () => ChessConnector.reviewNext(), 500],
            ["review Home", () => true, () => ChessConnector.reviewFirst(), 500],
            ["review End", () => true, () => ChessConnector.reviewLast(), 1000],
        ]
        onTriggered: scene.playStep()
    }

    function playStep() {
        const steps = playSteps.steps
        if (playSteps.step >= steps.length) {
            readyTimer.interval = 0
            readyTimer.start()
            return
        }
        const [what, ready, action, pause] = steps[playSteps.step]
        if (!ready()) {   // the page isn't up yet, or the robot hasn't answered: look again soon
            playSteps.interval = 50
            playSteps.start()
            return
        }
        playSteps.step++
        console.log("[Play] " + what + ", move " + ChessConnector.MoveCount)
        action()
        playSteps.interval = pause
        playSteps.start()
    }

    // Scene "premove-robot": wait for the robot's answer and the premove after it.
    Timer {
        id: premoveRobotTimer
        interval: 50
        repeat: true
        onTriggered: {
            if (ChessConnector.MoveCount < 3)
                return
            stop()
            readyTimer.interval = 0
            readyTimer.start()
        }
    }

    // Scene "slides" (BL-14): select the g1 knight, then Nf3, Nf6, Bc4, Nc6, 0.4–1 s apart.
    Timer { id: slideSelectTimer; interval: 900;  onTriggered: ChessConnector.figureSelected(6) }
    Timer { id: slideMove1Timer;  interval: 1300; onTriggered: ChessConnector.movePiece(6, 21) }
    Timer { id: slideMove2Timer;  interval: 2300; onTriggered: ChessConnector.movePiece(62, 45) }
    Timer { id: slideMove3Timer;  interval: 3300; onTriggered: ChessConnector.movePiece(5, 26) }
    Timer { id: slideMove4Timer;  interval: 4300; onTriggered: ChessConnector.movePiece(58, 42) }
}
