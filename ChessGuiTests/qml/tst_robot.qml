import QtQuick
import QtTest
import ChessPlusPlus
import "qrc:/qml"   // AppSettings

// Scenario 7 (docs/UiTestSuitePlan.md §9): a game against the robot, which is FakeUci here (the same
// moves every time, no Stockfish needed). Its line: 1. e4 e5 2. Nf3 Nc6 3. Bc4 Nf6.
TestCase {
    id: test
    name: "Robot"
    when: windowShown

    readonly property string line: "e2e4 e7e5 g1f3 b8c6 f1c4 g8f6"
    // Long enough to premove, and to leave, while it thinks: also in a slow Debug build (GitHub), where
    // the end of one test function and the start of the next take a while.
    readonly property int thinkMs: 3000
    property double robotStarted: 0   // when it began to think after the premove (test 2), for test 3

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
        robotStarted = Date.now()   // it thinks about Bc4 from now (test 3 leaves meanwhile)
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
        app.click("homeButton")
        app.waitForPage("home", 2000)
        // From when it began to think, not from the click: the end of test 2 took some of its time.
        verify(Date.now() - robotStarted < thinkMs,
               "Home before the robot would have answered (" + (Date.now() - robotStarted) + " ms after it began)")
        compare(ChessConnector.InGame, false)
        tryCompare(GameLibrary, "unfinishedCount", 1, 3000, "the game kept for Continue")
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

    // Undo on the player's turn: the robot's answer and the player's move go (3... Nf6, 3. Bc4).
    function test_5_takeback() {
        app.click("undoButton")
        tryCompare(ChessConnector, "MoveCount", 4, 3000, "two moves taken back")
        app.waitBoardIdle()
        compare(app.pieceAt("f1"), "B", "the bishop back on f1")
        compare(app.pieceAt("g8"), "n", "its knight back on g8")
        compare(app.pieceAt("c4"), "")
        compare(ChessConnector.EngineThinking, false)
        compare(ChessConnector.MovableColor, 1, "the player's move again")
    }

    // Undo while it thinks: only the player's move goes, and the robot's answer is never played.
    function test_6_takebackWhileItThinks() {
        app.clickMove("d2", "d3")
        tryCompare(ChessConnector, "MoveCount", 5, 3000, "d3 played")
        tryCompare(ChessConnector, "EngineThinking", true, 3000, "the robot thinking")
        app.click("undoButton")
        tryCompare(ChessConnector, "MoveCount", 4, 3000, "d3 taken back")
        compare(ChessConnector.EngineThinking, false, "it stopped thinking")
        wait(thinkMs + 500)   // when its answer would have come
        compare(ChessConnector.MoveCount, 4, "its answer never played")
        app.waitBoardIdle()
        compare(app.pieceAt("d2"), "P", "the pawn back on d2")
        compare(ChessConnector.MovableColor, 1)
    }

    // Rematch swaps the colours (#193): a game lost to the robot (1. f3 e5 2. g4 Qh4#), then the result
    // card's Rematch: the player takes Black, so the robot opens, with its line's first move f3.
    function test_7_rematchFromTheResultCard() {
        app.leaveToHome()
        app.startComputerGame("f2f3 e7e5 g2g4 d8h4", 300)
        app.waitBoardIdle()
        app.clickMove("f2", "f3")
        tryCompare(ChessConnector, "MoveCount", 2, 5000, "f3 and its e5")
        app.waitBoardIdle()
        app.clickMove("g2", "g4")
        tryVerify(() => ChessConnector.GameResult !== "", 5000, "its Qh4#")
        verify(ChessConnector.ResultText.indexOf("checkmate") >= 0, ChessConnector.ResultText)
        tryVerify(() => app.find("resultCard")?.visible, 5000, "the result card")
        compare(ChessConnector.PlayerPlaysWhite, true)

        app.click("rematchButton")
        tryCompare(ChessConnector, "PlayerPlaysWhite", false, 5000, "the player plays Black now")
        compare(ChessConnector.GameMode, "computer")
        tryCompare(ChessConnector, "MoveCount", 1, 5000, "the robot plays White's first move")
        compare(ChessConnector.LastMoveFrom, app.square("f2"))
        compare(ChessConnector.GameResult, "")
        app.waitBoardIdle()
        compare(ChessConnector.MovableColor, 2, "the player's move, as Black")
    }

    // BL-40: the player's 2. Qh5+ taken back while it still slides: once the board settles there is no
    // check, so neither its sound nor "Check" comes (a check waits for the board, BL-35 step 1b).
    function test_8_checkTakenBackInItsSlide() {
        app.leaveToHome()
        app.startComputerGame("e2e4 f7f6 d1h5 g7g6", thinkMs)
        const sounds = app.item("gameSounds")
        const bar = app.item("shortMessage")
        const heard = []
        const onSound = name => heard.push(name)
        const soundWasOn = AppSettings.musicEnabled
        AppSettings.musicEnabled = true
        sounds.played.connect(onSound)
        try {
            app.waitBoardIdle()
            app.clickMove("e2", "e4")
            tryCompare(ChessConnector, "MoveCount", 2, thinkMs + 5000, "its f6")
            app.clickMove("d1", "h5")
            tryCompare(ChessConnector, "MoveCount", 3, 3000)
            verify(!ChessConnector.isBoardIdle(), "Qh5+ still on its way")
            ChessConnector.requestTakeback()
            tryCompare(ChessConnector, "MoveCount", 2, 3000, "Qh5+ taken back")
            app.waitBoardIdle()
            compare(ChessConnector.CheckedKing, -1, "no check on the board")
            wait(400)   // past the gate's beat and the queue's next pass
            verify(heard.indexOf("check") < 0, "no check sound: " + heard.join())
            verify(!(bar.showing && bar.text === "Check"), "no \"Check\"")
        } finally {
            sounds.played.disconnect(onSound)
            AppSettings.musicEnabled = soundWasOn
        }
    }
}
