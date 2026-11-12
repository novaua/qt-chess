import QtQuick
import QtTest
import ChessPlusPlus

// Scenario 7 (docs/UiTestSuitePlan.md §9): a game against the robot, which is FakeUci here (the same
// moves every time, no Stockfish needed). Its line: 1. e4 e5 2. Nf3 Nc6 3. Bc4 Nf6.
TestCase {
    id: test
    name: "Robot"
    when: windowShown

    readonly property string line: "e2e4 e7e5 g1f3 b8c6 f1c4 g8f6"
    readonly property int thinkMs: 1500   // long enough to premove, and to leave, while it thinks

    AppDriver { id: app; test: test }

    function initTestCase() {
        app.start()
        app.createFirstProfile("Tester")
    }
    function cleanup() { app.afterTest() }
    function cleanupTestCase() { app.stop() }

    // 1. e4 dragged, the robot thinks, then answers e5; then it's the player's move again.
    function test_1_itAnswers() {
        app.startComputerGame(line, thinkMs)
        compare(ChessConnector.GameMode, "computer")
        app.waitBoardIdle()
        compare(ChessConnector.MovableColor, 1, "White (the player) to move")

        app.dragPiece("e2", "e4")
        tryCompare(ChessConnector, "MoveCount", 1, 3000, "e4 played")
        tryCompare(ChessConnector, "EngineThinking", true, 3000, "the robot thinking")
        tryCompare(ChessConnector, "MoveCount", 2, thinkMs + 5000, "the robot's answer")
        app.waitBoardIdle()
        compare(app.pieceAt("e5").toUpperCase(), "P", "its pawn on e5")
        compare(ChessConnector.LastMoveFrom, app.square("e7"))
        compare(ChessConnector.EngineThinking, false)
        compare(ChessConnector.MovableColor, 1, "the player's move again")
    }

    // 2. Nf3, and while the robot thinks Bc4 is premoved (red squares); its answer Nc6 comes,
    // then the premove is played at once.
    function test_2_premoveAfterItsAnswer() {
        app.clickMove("g1", "f3")
        tryCompare(ChessConnector, "MoveCount", 3, 3000, "Nf3 played")
        tryCompare(ChessConnector, "EngineThinking", true, 3000, "the robot thinking")

        app.clickSquare("f1")
        tryCompare(ChessConnector, "SelectedSquare", app.square("f1"), 2000, "the bishop selected for a premove")
        compare(ChessConnector.PremoveDots, true, "its dots are premove dots")
        app.clickSquare("c4")
        compare(ChessConnector.MoveCount, 3, "still its turn: nothing played yet")
        compare(ChessConnector.PremoveFrom, app.square("f1"), "the premove's from square")
        compare(ChessConnector.PremoveTo, app.square("c4"), "the premove's to square")

        tryCompare(ChessConnector, "MoveCount", 5, thinkMs + 5000, "its answer, then the premove")
        compare(ChessConnector.LastMoveFrom, app.square("f1"))
        compare(ChessConnector.LastMoveTo, app.square("c4"))
        compare(ChessConnector.PremoveFrom, -1, "the premove is gone")
        app.waitBoardIdle()
        compare(app.pieceAt("c6").toUpperCase(), "N", "its knight on c6")
        compare(app.pieceAt("c4").toUpperCase(), "B", "the bishop on c4")
    }

    // After Bc4 the robot thinks again: Home at once, without waiting for it (BL-16), and its
    // answer isn't played; the game is kept for Continue.
    function test_3_leaveWhileItThinks() {
        tryCompare(ChessConnector, "EngineThinking", true, 3000, "the robot thinking")
        const before = Date.now()
        app.click("homeButton")
        app.waitForPage("home", 2000)
        verify(Date.now() - before < thinkMs, "Home before the robot would have answered")
        compare(ChessConnector.InGame, false)
        tryCompare(ChessConnector, "CanContinue", true, 3000, "the game kept for Continue")
        wait(thinkMs + 500)   // when the answer would have come
        compare(ChessConnector.InGame, false, "still on Home")
    }

    // Continue: the five moves are back, the robot answers Nf6.
    function test_4_continue() {
        app.click("continueCard")
        app.waitForPage("game", 10000)
        tryCompare(ChessConnector, "MoveCount", 6, thinkMs + 8000, "the five moves and its answer")
        compare(ChessConnector.LastMoveFrom, app.square("g8"))
        compare(ChessConnector.LastMoveTo, app.square("f6"))
        app.waitBoardIdle()
        compare(app.pieceAt("f6").toUpperCase(), "N", "its knight on f6")
        compare(ChessConnector.MovableColor, 1, "the player's move")
    }
}
