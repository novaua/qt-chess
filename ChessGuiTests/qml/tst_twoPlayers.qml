import QtQuick
import QtTest
import ChessPlusPlus
import "qrc:/qml"   // AppSettings

// Scenarios 2 and 3 (docs/UiTestSuitePlan.md §9): a two-player game played with the mouse.
TestCase {
    id: test
    name: "TwoPlayers"
    when: windowShown

    AppDriver { id: app; test: test }

    function initTestCase() {
        app.start()
        app.createFirstProfile("Tester")
    }
    function cleanup() {
        AppSettings.rematchSwapsColours = true   // test 4 turns it off: never left so for the next test
        app.afterTest()
    }
    function cleanupTestCase() { app.stop() }

    // Fool's mate by clicks: 1. f3 e5 2. g4 Qh4#, then the result card. The sounds (GameSounds, BL-35):
    // each move's own, and the mate's only once the board has settled (BL-34).
    function test_1_foolsMateByClicks() {
        const sounds = app.item("gameSounds")
        const heard = []   // "name@idle" or "name@busy": the board's state when the sound was asked for
        const record = name => heard.push(name + (ChessConnector.isBoardIdle() ? "@idle" : "@busy"))
        const soundWasOn = AppSettings.musicEnabled
        AppSettings.musicEnabled = true
        sounds.played.connect(record)
        try {
            app.startTwoPlayerGame()
            compare(ChessConnector.MoveCount, 0)
            const moves = [["f2", "f3"], ["e7", "e5"], ["g2", "g4"], ["d8", "h4"]]
            for (let i = 0; i < moves.length; ++i) {
                app.clickMove(moves[i][0], moves[i][1])
                tryCompare(ChessConnector, "MoveCount", i + 1, 3000, "move " + (i + 1))
            }
            app.waitBoardIdle()
            compare(app.pieceAt("h4").toUpperCase(), "Q", "the queen on h4")
            compare(ChessConnector.CheckedKing, app.square("e1"), "White's king in check")

            tryVerify(() => ChessConnector.GameResult !== "", 3000, "the game over")
            verify(ChessConnector.ResultText.indexOf("checkmate") >= 0, ChessConnector.ResultText)
            const card = app.item("resultCard")
            tryVerify(() => card.visible, 5000, "the result card")
            verify(app.item("newGameButton").visible)
            verify(app.item("reviewButton").visible)

            tryVerify(() => heard.some(h => h.startsWith("checkmate@")), 3000, "the mate's sound: " + heard)
            const moveSounds = heard.filter(h => h.startsWith("move"))
                                    .map(h => h.substring(0, h.indexOf("@")))
            compare(moveSounds.join(), "move,moveOpponent,move,moveOpponent", "each move's own sound")
            const mates = heard.filter(h => h.startsWith("checkmate@"))
            compare(mates.join(), "checkmate@idle", "the mate's sound once, with the board settled: " + heard)
            const mateAt = heard.indexOf("checkmate@idle")
            verify(heard.slice(0, mateAt).filter(h => h.startsWith("move")).length === 4,
                   "the mate heard after the mating move's own sound: " + heard)
        } finally {
            sounds.played.disconnect(record)
            AppSettings.musicEnabled = soundWasOn
        }
    }

    // New game from the result card, then 1. e4 dragged: the pawn on e4, the last move marked,
    // "e4" in the move list, Black to move.
    function test_2_newGameAndDrag() {
        app.click("newGameButton")
        const dialog = app.item("twoPlayerDialog")
        tryVerify(() => dialog.opened, 3000, "the Two players dialog open")
        app.click("startGameButton")
        app.waitClosed(dialog)
        tryCompare(ChessConnector, "MoveCount", 0, 5000, "a new game")
        tryCompare(ChessConnector, "GameResult", "", 3000)
        app.waitForPage("game")
        app.waitBoardIdle()
        compare(ChessConnector.MovableColor, 1, "White to move")

        app.dragPiece("e2", "e4")
        tryCompare(ChessConnector, "MoveCount", 1, 3000, "the drag played e4")
        compare(ChessConnector.LastMoveFrom, app.square("e2"))
        compare(ChessConnector.LastMoveTo, app.square("e4"))
        app.waitBoardIdle()
        compare(app.pieceAt("e4").toUpperCase(), "P", "the pawn on e4")
        compare(app.pieceAt("e2"), "", "e2 empty")
        tryCompare(ChessConnector, "MovableColor", 2, 2000, "Black to move")
        tryVerify(() => app.find("ply1") !== null, 2000, "the move list's first move")
        compare(app.item("ply1").san, "e4")
    }

    // Leaving a two-player game keeps it (BL-19): Esc goes Home at once, the game is in the library
    // and Continue brings it back as it was; a game without moves isn't kept.
    function test_3_leavingKeepsTheGame() {
        compare(ChessConnector.MoveCount, 1, "e4 from test 2")
        app.laidOut()
        keyClick(Qt.Key_Escape)
        app.waitForPage("home", 5000)
        compare(GameLibrary.count, 2, "the game kept (with test 1's finished one)")
        compare(GameLibrary.unfinishedCount, 1, "unfinished")
        compare(GameLibrary.games[1].finished, true, "the Fool's mate of test 1, finished")
        compare(GameLibrary.games[1].result, "0-1")
        const kept = GameLibrary.games[0]
        compare(kept.kind, "local")
        compare(kept.plyCount, 1)
        compare(kept.lastMove, "e2e4")
        compare(kept.opponentName, "Guest")
        verify(app.item("continueCard").visible, "Continue offers it")

        // A new game without moves: not kept.
        app.startTwoPlayerGame()
        app.click("homeButton")
        app.waitForPage("home", 5000)
        compare(GameLibrary.count, 2, "nothing new")

        // Continue: the position as it was, Black to move; then it goes on.
        app.click("continueCard")
        app.waitForPage("game", 10000)
        tryCompare(ChessConnector, "MoveCount", 1, 3000)
        compare(ChessConnector.GameMode, "local")
        app.waitBoardIdle()
        compare(app.pieceAt("e4").toUpperCase(), "P", "the pawn on e4")
        tryCompare(ChessConnector, "MovableColor", 2, 2000, "Black to move")
        app.clickMove("e7", "e5")
        tryCompare(ChessConnector, "MoveCount", 2, 3000, "e5")
        compare(GameLibrary.games[0].plyCount, 2, "the record follows the game")
        compare(GameLibrary.count, 2, "the same record, not a new one")
    }

    // Rematch swaps the colours (#193): the result card's Rematch after a finished game gives the player
    // Black and turns the board; White still moves first. Each rematch below starts after a move, so
    // "no moves" shows a new game began. With Settings → Gameplay "Rematch swaps colours" off, the colours
    // stay. The Two players dialog still offers the side chosen in it, not the swapped one.
    function test_4_rematchSwapsColours() {
        compare(AppSettings.rematchSwapsColours, true, "on by default")
        app.leaveToHome()
        app.startTwoPlayerGame()
        compare(ChessConnector.PlayerPlaysWhite, true)
        const moves = [["f2", "f3"], ["e7", "e5"], ["g2", "g4"], ["d8", "h4"]]
        for (let i = 0; i < moves.length; ++i) {
            app.clickMove(moves[i][0], moves[i][1])
            tryCompare(ChessConnector, "MoveCount", i + 1, 3000, "move " + (i + 1))
        }
        tryVerify(() => app.find("resultCard")?.visible, 5000, "the result card")
        app.click("rematchButton")
        tryCompare(ChessConnector, "MoveCount", 0, 5000, "a new game")
        compare(ChessConnector.GameResult, "")
        compare(ChessConnector.PlayerPlaysWhite, false, "the player plays Black now")
        tryCompare(app.page().board, "angle", 180, 3000, "the board turned to Black")
        app.waitBoardIdle()
        compare(ChessConnector.MovableColor, 1, "White to move")
        app.clickMove("e2", "e4")
        tryCompare(ChessConnector, "MoveCount", 1, 3000, "e4 on the turned board")

        ChessConnector.rematch()
        tryCompare(ChessConnector, "MoveCount", 0, 5000, "the second rematch: a new game")
        compare(ChessConnector.PlayerPlaysWhite, true, "swapped back")
        tryCompare(app.page().board, "angle", 0, 3000)
        app.waitBoardIdle()
        app.clickMove("d2", "d4")
        tryCompare(ChessConnector, "MoveCount", 1, 3000)

        AppSettings.rematchSwapsColours = false
        compare(ChessConnector.RematchSwapsColours, false, "the setting reaches C++")
        ChessConnector.rematch()
        tryCompare(ChessConnector, "MoveCount", 0, 5000, "the third rematch: a new game")
        compare(ChessConnector.PlayerPlaysWhite, true, "kept with the setting off")
        AppSettings.rematchSwapsColours = true

        // Swapped to Black once more, then New game: the dialog offers White, as chosen in it.
        app.waitBoardIdle()
        app.clickMove("c2", "c4")
        tryCompare(ChessConnector, "MoveCount", 1, 3000)
        ChessConnector.rematch()
        tryCompare(ChessConnector, "MoveCount", 0, 5000)
        compare(ChessConnector.PlayerPlaysWhite, false)
        app.leaveToHome()
        app.click("twoPlayersCard")
        const dialog = app.item("twoPlayerDialog")
        tryVerify(() => dialog.opened, 3000, "the Two players dialog open")
        compare(app.item("twoPlayerSide").checkedIndex, 0, "White, as chosen in the dialog, not the rematch's Black")
        dialog.close()
        app.waitClosed(dialog)
    }
}
