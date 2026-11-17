import QtQuick
import QtTest
import ChessPlusPlus
import ChessGuiTests

// The engine's analysis of a kept game (BL-19 steps 6-7), with FakeUci following the game: its score is
// the material after its move, or a mate, so the scholar's mate's 3… Nf6 is a blunder (it lets the mate
// in). Analyse from the result card, the progress, the marks in the move list, the summary and the eval
// bar; Cancel; a takeback after the mate drops the analysis, and a game going on can't be analysed; a
// kept game opened from the Games page; the arrows of a judged move (played, best).
TestCase {
    id: test
    name: "Analysis"
    when: windowShown

    AppDriver { id: app; test: test }

    readonly property string scholarsMate: "e2e4 e7e5 d1h5 b8c6 f1c4 g8f6 h5f7"

    function initTestCase() {
        app.start()
        app.createFirstProfile("Tester")
    }
    function cleanup() { app.afterTest() }
    function cleanupTestCase() { app.stop() }

    function playScholarsMate() {
        for (const [from, to] of [["e2", "e4"], ["e7", "e5"], ["d1", "h5"], ["b8", "c6"], ["f1", "c4"], ["g8", "f6"], ["h5", "f7"]])
            app.clickMove(from, to)
        tryVerify(() => ChessConnector.GameResult !== "", 3000, "the mate")
    }
    function analysed() { return ChessConnector.Analysis.depth !== undefined }

    // Two players: the result card's Analyse; "Analysing… N of 8" while it runs, then the summary, the
    // marks and the eval bar; each position's eval and judgement as it is reviewed.
    function test_1_analyseFromTheResultCard() {
        TestRun.setEngine(scholarsMate, 150)   // slow enough to see it under way
        app.startTwoPlayerGame()
        playScholarsMate()
        tryVerify(() => app.find("resultCard")?.visible, 5000, "the result card")
        verify(!analysed())
        verify(ChessConnector.CanAnalyse, "a finished kept game")
        app.click("analyseResultButton")
        tryVerify(() => ChessConnector.AnalysingThisGame, 3000)
        verify(!app.item("resultCard").visible, "the card goes: the progress is in the moves panel")
        tryVerify(() => app.item("analysisProgress").visible, 3000)
        compare(ChessConnector.AnalysisTotal, 8, "the start and seven positions")
        verify(!ChessConnector.CanAnalyse, "not twice at once")

        tryVerify(() => analysed(), 15000, "analysed")
        verify(!ChessConnector.AnalysingThisGame)
        verify(app.item("analysisSummary").visible)
        verify(!app.item("analyseButton").visible)
        verify(app.item("evalBar").visible)
        verify(GameLibrary.games[0].analysed, "kept on the record")
        compare(ChessConnector.Analysis.black.blunders, 1, "3… Nf6??")
        compare(ChessConnector.Analysis.white.blunders, 0)
        compare(app.item("ply6").mark, "??")
        compare(app.item("ply5").mark, "")
        compare(ChessConnector.ShownAnalysis.evalText, "1-0", "the mate on the board")

        app.click("ply6")   // after 3… Nf6: White mates in 1
        tryCompare(ChessConnector, "ShownPly", 6, 3000)
        compare(ChessConnector.ShownAnalysis.evalText, "#1")
        compare(ChessConnector.ShownAnalysis.judgement, "Blunder")
        tryVerify(() => app.item("shownJudgement").visible, 3000)
        compare(app.item("shownJudgement").text, "Blunder", "FakeUci's best was the move played: no Best was")
        verify(app.item("reviewBar").text.endsWith("Nf6??"), app.item("reviewBar").text)
        app.click("ply1")
        tryCompare(ChessConnector, "ShownPly", 1, 3000)
        compare(ChessConnector.ShownAnalysis.judgement, "")
        verify(!app.item("shownJudgement").visible)
        ChessConnector.reviewLast()
    }

    // More → Analyse again, then Cancel: the analysis there before stays; nothing changes.
    function test_2_cancel() {
        TestRun.setEngine(scholarsMate, 400)
        app.chooseFromMenu("moreButton", "analyseMenuItem")
        tryVerify(() => ChessConnector.AnalysingThisGame, 3000)
        app.click("cancelAnalysisButton")
        tryVerify(() => !ChessConnector.AnalysingThisGame, 3000)
        verify(analysed(), "the earlier analysis stays")
        verify(GameLibrary.games[0].analysed)
        verify(ChessConnector.CanAnalyse, "it can be asked again")
        app.leaveToHome()
    }

    // vs the robot: analysed after the mate; Ctrl+Z takes the mate back, the analysis goes (the moves
    // aren't those any more) and a game going on can't be analysed (no hints).
    function test_3_takebackDropsIt() {
        app.startComputerGame("e2e4 f7f6 d2d4 g7g5", 0)
        app.waitBoardIdle()
        app.clickMove("e2", "e4")
        tryCompare(ChessConnector, "MoveCount", 2, 5000, "the robot's f6")
        app.waitBoardIdle()
        app.clickMove("d2", "d4")
        tryCompare(ChessConnector, "MoveCount", 4, 5000, "the robot's g5")
        app.waitBoardIdle()
        app.clickMove("d1", "h5")
        tryVerify(() => ChessConnector.GameResult !== "", 3000, "the mate")
        TestRun.setEngine("e2e4 f7f6 d2d4 g7g5 d1h5", 0)
        verify(ChessConnector.analyseGame(""))
        tryVerify(() => analysed(), 15000, "analysed")
        compare(app.item("ply4").mark, "??", "2… g5?? lets the mate in")

        app.waitBoardIdle()
        keyClick(Qt.Key_Z, Qt.ControlModifier)
        tryCompare(ChessConnector, "GameResult", "", 3000)
        tryVerify(() => !analysed(), 3000, "the analysis goes with the mate")
        compare(app.item("ply4").mark, "")
        verify(!app.item("evalBar").visible)
        verify(!ChessConnector.CanAnalyse, "not while the game goes on")
        verify(!GameLibrary.games[0].analysed)
        app.leaveToHome()
    }

    // The Games page: Open the analysed two-player game (a replay): its analysis shows at once.
    function test_4_aKeptGameOpened() {
        app.laidOut()
        keyClick(Qt.Key_O, Qt.ControlModifier)
        app.waitForPage("library", 5000)
        const analysedGame = GameLibrary.games.findIndex(g => g.analysed)
        verify(analysedGame >= 0)
        app.clickIn("gameCard_" + analysedGame, "actionButton")
        app.waitForPage("game", 10000)
        compare(ChessConnector.GameMode, "replay")
        tryVerify(() => analysed(), 3000)
        tryVerify(() => app.find("analysisSummary")?.visible, 5000, "the summary (the moves panel is made after the first frame)")
        compare(app.item("ply6").mark, "??")
        app.leaveToHome()
    }

    // The moves panel's Analyse closes the result card as the card's own does; a rematch while it runs:
    // the new game shows no progress of the old one's analysis, which goes on and is kept (review of
    // #167-#170).
    function test_5_panelButtonAndRematch() {
        TestRun.setEngine("f2f3 e7e5 g2g4 d8h4", 300)
        app.startTwoPlayerGame()
        for (const [from, to] of [["f2", "f3"], ["e7", "e5"], ["g2", "g4"], ["d8", "h4"]])
            app.clickMove(from, to)
        tryVerify(() => app.find("resultCard")?.visible, 5000, "the result card")
        const foolsMate = GameLibrary.games[0].id
        app.click("analyseButton")   // the moves panel's, beside the card
        tryVerify(() => ChessConnector.AnalysingThisGame, 3000)
        verify(!app.item("resultCard").visible, "the card goes, as with its own Analyse")

        ChessConnector.rematch()
        tryCompare(ChessConnector, "MoveCount", 0, 5000, "the rematch: a new game")
        verify(ChessConnector.Analysing, "the old game's analysis goes on")
        verify(!ChessConnector.AnalysingThisGame, "not of the new game")
        verify(!app.item("analysisProgress").visible)
        tryVerify(() => !ChessConnector.Analysing, 15000, "done")
        verify(GameLibrary.game(foolsMate).analysed, "kept on its record")
        app.leaveToHome()
    }

    // A judged move whose best was another (the engine's 2… e6, not g5??): two arrows on the board, the
    // move played in its mark's colour and the engine's in green, and "Best was e6"; none on a move that
    // wasn't judged (BL-19 follow-up).
    function test_6_bestMoveArrows() {
        app.startComputerGame("e2e4 f7f6 d2d4 g7g5", 0)
        app.waitBoardIdle()
        app.clickMove("e2", "e4")
        tryCompare(ChessConnector, "MoveCount", 2, 5000, "the robot's f6")
        app.waitBoardIdle()
        app.clickMove("d2", "d4")
        tryCompare(ChessConnector, "MoveCount", 4, 5000, "the robot's g5")
        app.waitBoardIdle()
        app.clickMove("d1", "h5")
        tryVerify(() => ChessConnector.GameResult !== "", 3000, "the mate")
        TestRun.setEngine("e2e4 f7f6 d2d4 e7e6 d1h5", 0)
        verify(ChessConnector.analyseGame(""))
        tryVerify(() => analysed(), 15000, "analysed")

        ChessConnector.reviewTo(4)   // after 2… g5??
        tryCompare(ChessConnector, "ShownPly", 4, 3000)
        const shown = ChessConnector.ShownAnalysis
        compare(shown.mark, "??")
        compare(shown.best, "e6")
        compare([shown.playedFrom, shown.playedTo], [54, 38], "g7-g5")
        compare([shown.bestFrom, shown.bestTo], [52, 44], "e7-e6")
        const played = app.item("playedMoveArrow")
        const best = app.item("bestMoveArrow")
        tryVerify(() => played.visible && best.visible, 3000, "both arrows")
        verify(Qt.colorEqual(played.color, "#DF5353"), "the blunder's red: " + played.color)
        verify(Qt.colorEqual(best.color, "#2E9E41"), "the best move's green: " + best.color)
        verify(app.item("shownJudgement").text.indexOf("Best was e6") >= 0, app.item("shownJudgement").text)

        ChessConnector.reviewTo(2)   // 1… f6: the engine's too
        tryCompare(ChessConnector, "ShownPly", 2, 3000)
        tryVerify(() => !played.visible && !best.visible, 3000, "no arrows on a move not judged")
        app.leaveToHome()
    }
}
