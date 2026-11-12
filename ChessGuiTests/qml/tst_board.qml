import QtQuick
import QtTest
import ChessPlusPlus

// Scenarios 4, 6, 8 and 11 (docs/UiTestSuitePlan.md §9) in a two-player game: a drop on an
// illegal square, "Check", the review, and a saved game replayed. (The takeback: tst_robot, as
// a two-player game has no Undo button.)
TestCase {
    id: test
    name: "Board"
    when: windowShown

    AppDriver { id: app; test: test }

    function initTestCase() {
        app.start()
        app.createFirstProfile("Tester")
        app.startTwoPlayerGame()
    }
    function cleanup() { app.afterTest() }
    function cleanupTestCase() { app.stop() }

    // e2 dropped on e5 (not a pawn's move): it goes back, nothing is played. A black piece can't be
    // dragged on White's move.
    function test_1_illegalDropGoesBack() {
        app.dragPiece("e2", "e5")
        app.waitBoardIdle()
        compare(ChessConnector.MoveCount, 0, "nothing played")
        compare(app.pieceAt("e2").toUpperCase(), "P", "the pawn back on e2")
        compare(app.pieceAt("e5"), "", "e5 empty")

        app.dragPiece("e7", "e5")
        app.waitBoardIdle()
        compare(ChessConnector.MoveCount, 0, "Black can't move on White's turn")
        compare(app.pieceAt("e7").toUpperCase(), "P")
    }

    // 1. e4 f6 2. Qh5+: "Check" over the command bar, Black's king marked.
    function test_2_check() {
        app.clickMove("e2", "e4")
        app.clickMove("f7", "f6")
        app.clickMove("d1", "h5")
        tryCompare(ChessConnector, "MoveCount", 3, 3000)
        compare(ChessConnector.CheckedKing, app.square("e8"), "Black's king in check")
        app.waitMessage("Check")
    }

    // The review: the first move from the move list, back to the start, forward again, and back to
    // the game (the check still on).
    function test_3_review() {
        app.click("ply1")   // 1. e4 in the move list
        tryCompare(ChessConnector, "ReviewMode", true, 2000)
        compare(ChessConnector.ShownPly, 1)
        app.waitPiece("e4", "P")      // after 1. e4
        app.waitPiece("f7", "p")      // before 1... f6
        app.waitPiece("h5", "")       // before 2. Qh5+
        compare(ChessConnector.CheckedKing, -1, "no check shown at move 1")

        app.click("review_prev")
        tryCompare(ChessConnector, "ShownPly", 0, 2000)
        app.waitPiece("e2", "P")      // the start position

        app.click("review_next")
        tryCompare(ChessConnector, "ShownPly", 1, 2000)

        app.click("review_last")
        tryCompare(ChessConnector, "ReviewMode", false, 2000, "back to the game")
        compare(ChessConnector.ShownPly, 3)
        app.waitBoardIdle()
        compare(app.pieceAt("h5"), "Q", "the queen on h5 again")
        compare(ChessConnector.CheckedKing, app.square("e8"), "and the check")
        compare(ChessConnector.MovableColor, 2, "Black to move")
    }

    // Save, Home, "Open saved game": a replay of the three moves, from the start position; the ›
    // button steps through them to the saved position (in check).
    function test_4_saveAndReplay() {
        app.click("saveButton")
        app.waitMessage("Game saved")
        tryCompare(ChessConnector, "CanLoad", true, 2000)

        app.leaveToHome()
        app.click("openSavedCard")
        app.waitForPage("game", 10000)
        tryCompare(ChessConnector, "PlyCount", 3, 3000, "the saved moves in the list")
        compare(ChessConnector.ShownPly, 0, "the replay starts at the start position")
        app.waitPiece("e2", "P")
        app.waitPiece("d1", "Q")

        for (let ply = 1; ply <= 3; ++ply) {
            app.click("nextButton")
            tryCompare(ChessConnector, "ShownPly", ply, 2000, "move " + ply)
        }
        app.waitPiece("e4", "P")
        app.waitPiece("f6", "p")
        app.waitPiece("h5", "Q")
        app.waitPiece("d1", "")
        compare(ChessConnector.CheckedKing, app.square("e8"), "the check at the end")
    }
}
