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

    // Home, then Games (BL-19): the game is kept, unfinished, with its three moves; Open from its card
    // is a replay from the start position, and the › button steps through them to the kept position (in check).
    function test_4_keptGameReplay() {
        app.leaveToHome()
        compare(GameLibrary.count, 1, "the game kept")
        compare(GameLibrary.unfinishedCount, 1, "unfinished")
        compare(GameLibrary.games[0].plyCount, 3)
        app.click("gamesCard")
        app.waitForPage("library", 5000)
        tryVerify(() => app.find("gameCard_0") !== null, 3000, "its card")
        compare(app.item("gameCard_0").actionText, "Continue", "unfinished: Continue")
        // A replay of an unfinished game: the connector's openGame, as the Games page offers for a finished one.
        verify(ChessConnector.openGame(GameLibrary.games[0].id))
        app.waitForPage("game", 10000)
        tryCompare(ChessConnector, "PlyCount", 3, 3000, "the kept moves in the list")
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

    // BL-21: a square the selected piece takes on shows the capture mark (corners and the red glow, under
    // the pieces), not a dot; the other targets keep their dots. Shown or not as the move layers show it.
    function markShown(square) {
        const mark = findChild(app.item("captureLayer"), "captureMark_" + app.square(square))
        return mark !== null && mark.visible && mark.status === Loader.Ready && mark.parent.visible
    }
    function dotShown(square) {
        const dot = findChild(app.item("navigationLayerGrid"), "moveDot_" + app.square(square))
        return dot !== null && dot.visible && dot.parent.visible
    }
    function test_5_captureMarks() {
        app.leaveToHome()
        app.startTwoPlayerGame()
        for (const [from, to] of [["e2", "e4"], ["e7", "e5"], ["d1", "h5"], ["b8", "c6"]])
            app.clickMove(from, to)
        app.waitBoardIdle()
        app.clickSquare("h5")   // the queen: takes on e5 (a dark square), f7 and h7 (light ones)
        tryCompare(ChessConnector, "SelectedSquare", app.square("h5"), 2000)
        for (const sq of ["e5", "f7", "h7"]) {
            tryVerify(() => markShown(sq), 2000, "the capture mark on " + sq)
            verify(!dotShown(sq), "no dot on " + sq)
        }
        for (const sq of ["g5", "f3", "h6"]) {
            verify(dotShown(sq), "a dot on " + sq)
            verify(!markShown(sq), "no capture mark on " + sq)
        }
        app.clickSquare("a6")   // an empty square, not a target: deselected, nothing shows
        tryCompare(ChessConnector, "SelectedSquare", -1, 2000)
        tryVerify(() => !markShown("e5") && !dotShown("g5"), 2000, "the marks gone")
        app.leaveToHome()

        // En passant: Black's d4 pawn takes the c3 pawn, or on e3 (an empty square); d3 is a dot.
        app.startTwoPlayerGame()
        for (const [from, to] of [["c2", "c3"], ["d7", "d5"], ["a2", "a3"], ["d5", "d4"], ["e2", "e4"]])
            app.clickMove(from, to)
        app.waitBoardIdle()
        app.clickSquare("d4")
        tryCompare(ChessConnector, "SelectedSquare", app.square("d4"), 2000)
        tryVerify(() => markShown("c3") && markShown("e3"), 2000, "c3, and e3 en passant")
        verify(dotShown("d3"), "d3: a move, not a capture")
        verify(!markShown("d3"))
        app.leaveToHome()

        // A premove (the robot thinks): the f3 knight's premove onto e5's pawn gets the corners; Esc drops it.
        app.startComputerGame("e2e4 e7e5 g1f3 b8c6", 1500)
        app.waitBoardIdle()
        app.clickMove("e2", "e4")
        tryCompare(ChessConnector, "MoveCount", 2, 5000, "the robot's e5")
        app.waitBoardIdle()
        app.clickMove("g1", "f3")
        tryVerify(() => ChessConnector.EngineThinking, 3000, "the robot thinks")
        app.clickSquare("f3")
        tryVerify(() => ChessConnector.PremoveDots, 2000, "premove targets")
        tryVerify(() => markShown("e5"), 2000, "the premove capture's corners on e5")
        verify(dotShown("g5"), "g5: a premove, not a capture")
        app.clickSquare("a6")   // deselected, no premove set
        tryCompare(ChessConnector, "MoveCount", 4, 5000, "the robot's Nc6")
        app.leaveToHome()
    }
}
