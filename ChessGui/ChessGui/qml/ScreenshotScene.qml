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
        var gameScene = ["game", "continue", "review", "result", "ended", "moves"].includes(name)
        if ((gameScene || name === "players") && UserManager.userCount < 2) {
            var me = UserManager.activeUserId
            UserManager.createUser("Bob", "fox")
            UserManager.login(me)
        }

        if (name === "settings" || name === "about" || name === "restore") window.openSettings()
        if (name === "restore")  dialogs.restoreBackup.open()
        if (name === "about")    pages.currentItem.showAbout()
        if (name === "levels")   dialogs.levels.open()
        if (name.startsWith("profile") && name !== "profiles") window.openProfile()
        if (name === "online")   dialogs.online.open()
        if (name === "computer") dialogs.computer.openNew()
        if (name === "players")  dialogs.twoPlayer.open()
        if (name === "robot") {
            // A saved game vs the robot (level 5, the player is Black) where White mates in one,
            // then Continue: the engine finds Qh5#, played on the GUI thread (BL-09). Needs Stockfish.
            ChessConnector.startComputerGame(5, 1)   // the robot starts on move 1 ...
            ChessConnector.applyMoves(shot.moves || "e2e4 f7f6 d2d4 g7g5")
            ChessConnector.endGame()                  // ... and that answer is dropped
            ChessConnector.continueGame()
        }
        if (gameScene) {
            // A Scandinavian with captures on both sides (Black +1), and Fool's mate for the result.
            var middlegame = "e2e4 d7d5 e4d5 d8d5 b1c3 d5a5 d2d4 g8f6 g1f3 c8f5 f1c4 e7e6 c1d2 c7c6 "
                           + "c3d5 a5d8 d5f6 d8f6 d1e2 f5c2"
            var defaultMoves = ({ continue: "e2e4 e7e5", review: middlegame, moves: middlegame,
                                  result: "f2f3 e7e5 g2g4 d8h4", ended: "f2f3 e7e5 g2g4 d8h4" })[name] ?? ""
            var bob = UserManager.users.find(u => u.name === "Bob")
            ChessConnector.startLocalGame(!shot.black, bob ? bob.id : "")
            if (shot.moves || defaultMoves)
                ChessConnector.applyMoves(shot.moves || defaultMoves)
            if (name === "continue")
                ChessConnector.endGame()   // autosaved: Home offers to continue it
            if (name === "review")
                ChessConnector.reviewTo(9)
            if (name === "moves")
                pages.currentItem.sheetOpen = true   // the move sheet (narrow window)
            if (name === "ended")
                pages.currentItem._resultCardClosed = true   // Rematch / New game in the moves panel
        }

        if (ChessConnector.EngineThinking) _waitsForEngine = true   // the robot's move comes later
        else                               startReadyTimer()
    }

    function startReadyTimer() {
        // A finished game shows its result only after GamePage's resultTimer (1600 ms).
        readyTimer.interval = ChessConnector.GameResult !== "" ? 1700 : 0
        readyTimer.start()
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
}
