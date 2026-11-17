import QtQuick
import QtTest
import ChessPlusPlus
import ChessGuiTests

// LI-01A Chess960 (docs/LI-01A-Chess960Plan.md §3.7): a game from a given position, castling as on
// lichess.org (the king onto its own rook, or onto its square when no move of its own goes there), the
// takeback (vs the robot), Rematch with the same position, the game kept and continued, its replay, Play from here,
// its PGN, and the robot (FakeUci) castling, also as a premove.
// #3 BQNNRKRB: king f1, rook g1 (they swap on O-O, at once). #518 is the standard set-up.
// The games start as the setup dialogs start them, by ChessConnector's own calls (the dialogs' Variant row
// is step 4).
TestCase {
    id: test
    name: "Chess960"
    when: windowShown

    AppDriver { id: app; test: test }

    property string keptId: ""

    function initTestCase() {
        app.start()
        app.createFirstProfile("Tester")
    }
    function cleanup() { app.afterTest() }
    function cleanupTestCase() { app.stop() }

    function startTwoPlayers960(number) {
        app.waitForPage("home")
        ChessConnector.startLocalGame(true, "", 0, 0, number)
        app.waitForPage("game", 10000)
        app.waitBoardIdle()
    }
    function dotOn(name) { return ChessConnector.PossibleMoves[app.square(name)] !== "" }

    // #3: White castles by dropping the king on its rook, Black by clicking the king, then the rook.
    function test_1_castleOntoTheRook() {
        startTwoPlayers960(3)
        compare(ChessConnector.Chess960, true)
        compare(ChessConnector.StartPositionNumber, 3)
        compare(app.pieceAt("a1"), "B", "#3 begins with a bishop on a1")
        compare(app.pieceAt("f1"), "K")
        compare(app.pieceAt("g1"), "R")

        app.dragPiece("f1", "g1")
        tryCompare(ChessConnector, "MoveCount", 1, 3000, "O-O by a drop on the rook")
        app.waitBoardIdle()
        compare(app.pieceAt("g1"), "K", "the king on g1")
        compare(app.pieceAt("f1"), "R", "the rook on f1")
        tryVerify(() => app.find("ply1") !== null, 2000)
        compare(app.item("ply1").san, "O-O")

        app.clickSquare("f8")
        tryCompare(ChessConnector, "SelectedSquare", app.square("f8"), 2000)
        verify(dotOn("g8"), "a dot on the rook")
        app.clickSquare("g8")
        tryCompare(ChessConnector, "MoveCount", 2, 3000, "O-O by a click on the rook")
        app.waitBoardIdle()
        compare(app.pieceAt("g8"), "k")
        compare(app.pieceAt("f8"), "r")
        compare(app.item("ply2").san, "O-O")
    }

    // Rematch: the same position again, no move.
    function test_2_rematchKeepsThePosition() {
        ChessConnector.rematch()
        tryCompare(ChessConnector, "MoveCount", 0, 5000, "a new game")
        compare(ChessConnector.Chess960, true)
        compare(ChessConnector.StartPositionNumber, 3)
        app.waitBoardIdle()
        compare(app.pieceAt("f1"), "K")
        app.clickMove("f1", "g1")
        tryCompare(ChessConnector, "MoveCount", 1, 3000)
    }

    // Left for Home, the game is kept with its position; Continue brings it back; its PGN is Chess960's.
    function test_3_keptAndContinued() {
        app.leaveToHome()
        const kept = GameLibrary.games[0]
        keptId = kept.id
        compare(kept.chess960, true)
        compare(kept.startPosition, 3)
        compare(kept.lastMove, "f1g1", "castling kept as the king onto its rook")

        const pgn = ChessConnector.pgn(keptId)
        verify(pgn.indexOf('[Variant "Chess960"]') >= 0, pgn)
        verify(pgn.indexOf('[SetUp "1"]') >= 0, pgn)
        verify(pgn.indexOf('[FEN "bqnnrkrb/pppppppp/8/8/8/8/PPPPPPPP/BQNNRKRB w KQkq - 0 1"]') >= 0, pgn)
        verify(pgn.indexOf("1. O-O") >= 0, pgn)

        verify(ChessConnector.continueGame(keptId))
        app.waitForPage("game", 10000)
        tryCompare(ChessConnector, "MoveCount", 1, 3000)
        compare(ChessConnector.Chess960, true)
        compare(ChessConnector.StartPositionNumber, 3)
        app.waitBoardIdle()
        compare(app.pieceAt("g1"), "K")
        compare(app.pieceAt("f1"), "R")
        app.leaveToHome()
        compare(ChessConnector.Chess960, false, "Home: no game")
    }

    // The replay starts from the game's position; Play from here keeps it.
    function test_4_replayAndPlayFromHere() {
        verify(ChessConnector.openGame(keptId))
        app.waitForPage("game", 10000)
        compare(ChessConnector.GameMode, "replay")
        compare(ChessConnector.StartPositionNumber, 3)
        app.waitBoardIdle()
        compare(app.pieceAt("f1"), "K", "the replay begins at #3's start")

        verify(ChessConnector.startLocalGameFrom(keptId, 1, true, ""))
        tryCompare(ChessConnector, "MoveCount", 1, 5000, "the game's first move")
        compare(ChessConnector.GameMode, "local")
        compare(ChessConnector.StartPositionNumber, 3, "a fork keeps the position")
        app.waitBoardIdle()
        compare(app.pieceAt("g1"), "K")
        app.clickMove("f8", "g8")
        tryCompare(ChessConnector, "MoveCount", 2, 3000, "Black castles in the fork")
        app.leaveToHome()
    }

    // #518 as Chess960: e1 → g1 isn't a king move, so a click there castles (the rook's square too).
    function test_5_castleOnTheKingsSquare() {
        startTwoPlayers960(518)
        for (const [from, to] of [["e2", "e4"], ["e7", "e5"], ["g1", "f3"], ["b8", "c6"], ["f1", "c4"], ["g8", "f6"]])
            app.clickMove(from, to)
        tryCompare(ChessConnector, "MoveCount", 6, 3000)
        app.clickSquare("e1")
        tryCompare(ChessConnector, "SelectedSquare", app.square("e1"), 2000)
        verify(dotOn("h1"), "a dot on the rook")
        verify(dotOn("g1"), "a dot on the king's square")
        verify(dotOn("f1"), "the king's own move")
        app.clickSquare("g1")
        tryCompare(ChessConnector, "MoveCount", 7, 3000, "O-O by a click on g1")
        app.waitBoardIdle()
        compare(app.pieceAt("g1"), "K")
        compare(app.pieceAt("f1"), "R")
        compare(app.pieceAt("h1"), "")
        compare(app.item("ply7").san, "O-O")

        // ← reviews the position before it, replayed from #518's start
        app.laidOut()
        keyClick(Qt.Key_Left)
        tryCompare(ChessConnector, "ReviewMode", true, 3000)
        app.waitPiece("e1", "K")
        app.waitPiece("h1", "R")
        compare(app.pieceAt("g1"), "")
        app.leaveToHome()
    }

    // vs the robot (FakeUci, told the position): while it thinks the player premoves O-O by clicking the
    // king and its rook; the robot answers 1… e5, the castling is played at once; then the robot castles,
// and Undo takes both castlings back.
    function test_6_robotAndPremove() {
        TestRun.setEngine("e2e4 e7e5 f1g1 f8g8", 1500)
        ChessConnector.startComputerGame(1, 0, 0, 0, 3)
        app.waitForPage("game", 10000)
        app.waitBoardIdle()
        compare(ChessConnector.StartPositionNumber, 3)

        app.clickMove("e2", "e4")
        tryCompare(ChessConnector, "EngineThinking", true, 3000, "the robot thinking")
        app.clickSquare("f1")
        tryCompare(ChessConnector, "SelectedSquare", app.square("f1"), 2000, "the king selected for a premove")
        app.clickSquare("g1")
        compare(ChessConnector.PremoveFrom, app.square("f1"), "a premove onto the rook")
        compare(ChessConnector.PremoveTo, app.square("g1"))

        tryCompare(ChessConnector, "MoveCount", 3, 8000, "its answer, then the castling")
        app.waitBoardIdle()
        compare(app.pieceAt("e5"), "p", "its e5")
        compare(app.pieceAt("g1"), "K", "the premoved O-O")
        compare(app.pieceAt("f1"), "R")

        tryCompare(ChessConnector, "MoveCount", 4, 8000, "its O-O")
        app.waitBoardIdle()
        compare(app.pieceAt("g8"), "k", "the robot castled")
        compare(app.pieceAt("f8"), "r")
        compare(ChessConnector.MovableColor, 1, "the player's move")

        // Undo on the player's turn: both castlings go, each king and rook back on its own square.
        app.click("undoButton")
        tryCompare(ChessConnector, "MoveCount", 2, 3000, "two moves taken back")
        app.waitBoardIdle()
        compare(app.pieceAt("f8"), "k", "Black's king back on f8")
        compare(app.pieceAt("g8"), "r", "Black's rook back on g8")
        compare(app.pieceAt("f1"), "K", "White's king back on f1")
        compare(app.pieceAt("g1"), "R", "White's rook back on g1")
        app.leaveToHome()
        const kept = GameLibrary.games[0]
        compare(kept.kind, "computer")
        compare(kept.chess960, true)
    }
}
