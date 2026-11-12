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
}
