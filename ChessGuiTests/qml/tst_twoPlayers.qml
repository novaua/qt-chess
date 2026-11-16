import QtQuick
import QtTest
import ChessPlusPlus

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
    function cleanup() { app.afterTest() }
    function cleanupTestCase() { app.stop() }

    // Fool's mate by clicks: 1. f3 e5 2. g4 Qh4#, then the result card.
    function test_1_foolsMateByClicks() {
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
}
