import QtQuick
import QtTest
import ChessPlusPlus
import ChessGuiTests
import "qrc:/qml"   // AppSettings

// LI-01A Chess960 (docs/LI-01A-Chess960Plan.md §3.7, §3.8): a game from a given position, castling as on
// lichess.org (the king onto its own rook, or onto its square when no move of its own goes there), the
// takeback (vs the robot), Rematch with the same position, the game kept and continued, its replay, Play from here,
// its PGN, and the robot (FakeUci) castling, also as a premove. Then the setup dialogs' Variant row: a
// random position, "Chess960 #N" in the title, New game offers Chess960 again, the choice remembered,
// no row in Play from here.
// #3 BQNNRKRB: king f1, rook g1 (they swap on O-O, at once). #518 is the standard set-up.
// Tests 1-6 start their games at a fixed position by ChessConnector's own calls, as the dialogs do.
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

    // Two players, Chess960 chosen in the dialog: one of the 960 at random, named in the title; the
    // choice is remembered. The result card's New game offers Chess960 again.
    function test_7_variantRowTwoPlayers() {
        app.waitForPage("home")
        compare(AppSettings.chess960, false, "a new profile: standard")
        app.click("twoPlayersCard")
        const dialog = app.item("twoPlayerDialog")
        tryVerify(() => dialog.opened, 3000, "the Two players dialog open")
        const variant = app.item("playersVariant")
        verify(variant.visible)
        compare(variant.chess960, false)
        app.clickIn("playersVariant", "variantChoice_1")
        compare(variant.chess960, true, "Chess960 chosen")
        app.click("startGameButton")
        app.waitForPage("game", 10000)
        app.waitClosed(dialog)
        app.waitBoardIdle()

        compare(ChessConnector.Chess960, true)
        const number = ChessConnector.StartPositionNumber
        verify(number >= 0 && number <= 959, "a position: " + number)
        verify(ChessConnector.GameTitle.endsWith(" · Chess960 #" + number), ChessConnector.GameTitle)
        compare(AppSettings.chess960, true, "remembered")

        ChessConnector.resignGame()
        tryVerify(() => app.item("resultCard").visible, 5000, "the result card")
        app.click("newGameButton")
        tryVerify(() => dialog.opened, 3000, "the dialog again")
        compare(variant.chess960, true, "New game: Chess960 again")
        app.click("startGameButton")
        app.waitClosed(dialog)
        tryCompare(ChessConnector, "GameResult", "", 5000, "a new game")
        compare(ChessConnector.Chess960, true)
        app.leaveToHome()
    }

    // vs the robot: the dialog opens with the last choice; Standard chosen again is a standard game, and
    // remembered too.
    function test_8_variantRowComputer() {
        TestRun.setEngine("", 0)
        app.waitForPage("home")
        app.click("computerCard")
        const dialog = app.item("computerDialog")
        tryVerify(() => dialog.opened, 3000, "the vs computer dialog open")
        const variant = app.item("computerVariant")
        compare(variant.chess960, true, "the last choice: Chess960")
        app.clickIn("computerVariant", "variantChoice_0")
        app.click("startComputerGameButton")
        app.waitForPage("game", 10000)
        app.waitClosed(dialog)
        compare(ChessConnector.Chess960, false, "a standard game")
        compare(ChessConnector.StartPositionNumber, -1)
        verify(ChessConnector.GameTitle.indexOf("Chess960") < 0, ChessConnector.GameTitle)
        compare(AppSettings.chess960, false)
        app.leaveToHome()
    }

    // Play from here keeps the game's start: no Variant row (the kept #3 game of test 3, replayed).
    function test_9_noVariantRowFromHere() {
        verify(ChessConnector.openGame(keptId))
        app.waitForPage("game", 10000)
        app.waitBoardIdle()
        ChessConnector.reviewTo(1)
        tryVerify(() => ChessConnector.PlayFromHere.gameId === keptId, 3000, "a position to play from")
        const dialog = app.item("computerDialog")
        dialog.openFrom(ChessConnector.PlayFromHere)
        tryVerify(() => dialog.opened, 3000)
        compare(app.item("computerVariant").visible, false, "no Variant row")
        dialog.reject()
        app.waitClosed(dialog)
        app.leaveToHome()
    }
}
