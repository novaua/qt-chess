import QtQuick
import QtTest
import ChessPlusPlus

// Scenario 5 (docs/UiTestSuitePlan.md §9): a pawn on the last rank, the choice of its piece, for White
// and for Black. The moves before are put on the board as "Apply moves" does (ChessConnector.applyMoves).
TestCase {
    id: test
    name: "Promotion"
    when: windowShown

    AppDriver { id: app; test: test }

    function initTestCase() {
        app.start()
        app.createFirstProfile("Tester")
    }
    function cleanup() { app.afterTest() }
    function cleanupTestCase() { app.stop() }

    // Drop the pawn on the last rank, then choose `piece` (Q, R, B or N) in the promotion box. The move
    // counts at the drop; the piece is chosen after it, and the side to move waits for that.
    function promote(from, to, piece, movesBefore) {
        app.dragPiece(from, to)
        const choice = "promote" + piece
        tryVerify(() => app.find(choice) !== null && app.find(choice).visible, 3000, "the promotion choices")
        compare(ChessConnector.MoveCount, movesBefore + 1, "the pawn's move")
        compare(ChessConnector.MovableColor, 0, "nobody moves while the piece is chosen")
        mouseClick(app.item(choice))
        tryVerify(() => app.find("promotionOptions") === null, 3000, "the promotion choices gone")
        app.waitBoardIdle()
    }

    // 1. a4 b5 2. axb5 a6 3. bxa6 Bb7 4. axb7 Nc6 5. bxa8=N
    function test_1_whiteChoosesAKnight() {
        app.startTwoPlayerGame()
        ChessConnector.applyMoves("a2a4 b7b5 a4b5 a7a6 b5a6 c8b7 a6b7 b8c6")
        tryCompare(ChessConnector, "MoveCount", 8, 3000)
        app.waitBoardIdle()

        promote("b7", "a8", "N", 8)
        compare(app.pieceAt("a8"), "N", "a white knight on a8")
        compare(app.pieceAt("b7"), "", "the pawn gone")
        compare(ChessConnector.MovableColor, 2, "Black to move")
    }

    // A new game: 1. a3 h5 2. b3 h4 3. c3 h3 4. d3 hxg2 5. e3 gxh1=N
    function test_2_blackChoosesAKnight() {
        app.leaveToHome()
        app.startTwoPlayerGame()
        ChessConnector.applyMoves("a2a3 h7h5 b2b3 h5h4 c2c3 h4h3 d2d3 h3g2 e2e3")
        tryCompare(ChessConnector, "MoveCount", 9, 3000)
        app.waitBoardIdle()

        promote("g2", "h1", "N", 9)
        compare(app.pieceAt("h1"), "n", "a black knight on h1")
        compare(app.pieceAt("g2"), "")
        compare(ChessConnector.MovableColor, 1, "White to move")
    }
}
