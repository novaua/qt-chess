import QtQuick
import QtTest
import ChessPlusPlus
import ChessGuiTests
import "qrc:/qml"   // AppSettings

// LI-01A stage 2 (docs/LI-01A-Chess960LichessPlan.md §5): Chess960 on Lichess, played against FakeLichess
// (TestSetup; nothing reaches lichess.org). A 960 challenge accepted: the board at its position, a
// premoved castling posted as the king onto its rook, the opponent's castling from the stream. An
// ongoing 960 game continued from Home. Other variants: no Accept, and a game of one refused before it
// replaces anything. The Online page's Variant row: the challenge's variant, the Chess960 rating,
// the choice remembered apart from the local games'.
// #3 BQNNRKRB: king f1, rook g1; O-O swaps them (f8 / g8 for Black).
TestCase {
    id: test
    name: "Lichess960"
    when: windowShown

    AppDriver { id: app; test: test }

    readonly property string me: FakeLichess.username
    readonly property string start3: "bqnnrkrb/pppppppp/8/8/8/8/PPPPPPPP/BQNNRKRB w KQkq - 0 1"

    SignalSpy { id: startFailed; target: ChessConnector; signalName: "gameStartFailed" }

    function initTestCase() {
        app.start()
        app.createFirstProfile("Tester")
        TestRun.linkLichess(me)
        tryCompare(UserManager, "lichessConnected", true, 3000)
        tryCompare(FakeLichess, "eventStreamOpen", true, 3000, "the event stream")
    }
    function cleanup() { app.afterTest() }
    function cleanupTestCase() { app.stop() }

    function bar() { return app.item("challengeBar") }
    function waitRequest(request, timeout) {
        tryVerify(() => FakeLichess.received(request), timeout ?? 3000,
                  request + " (last: " + FakeLichess.requests[FakeLichess.requests.length - 1] + ")")
    }

    // A 960 challenge: the bar names the variant, Accept; the game at #3, the player Black. A castling
    // premoved during White's first move goes as soon as it is played, posted as f8g8; White's O-O
    // (f1g1) from the stream swaps king and rook.
    function test_1_challengeAcceptedAndCastling() {
        FakeLichess.challenge("c9600001", "Rival960", 1700, "white", "chess960", start3)
        tryVerify(() => bar().visible, 3000, "the challenge bar")
        compare(bar().text, "Rival960 (1700) challenges you · 10+5 · Chess960 · Casual · You play Black")
        app.click("challengeAcceptButton")
        waitRequest("POST /api/challenge/c9600001/accept")
        app.waitForPage("game", 10000)
        app.waitBoardIdle()
        compare(ChessConnector.GameMode, "online")
        compare(ChessConnector.Chess960, true)
        compare(ChessConnector.StartPositionNumber, 3)
        verify(ChessConnector.GameTitle.endsWith("Chess960 #3"), ChessConnector.GameTitle)
        compare(app.pieceAt("a1"), "B", "#3, not the standard set-up")
        compare(app.pieceAt("f8"), "k")
        compare(app.pieceAt("g8"), "r")

        app.clickSquare("f8")
        tryCompare(ChessConnector, "SelectedSquare", app.square("f8"), 2000, "the king selected for a premove")
        app.clickSquare("g8")
        compare(ChessConnector.PremoveFrom, app.square("f8"), "a premove onto the rook")
        compare(ChessConnector.PremoveTo, app.square("g8"))

        FakeLichess.play("c9600001", "g2g3")
        waitRequest("POST /api/board/game/c9600001/move/f8g8")
        tryCompare(ChessConnector, "MoveCount", 2, 3000)
        app.waitBoardIdle()
        compare(app.pieceAt("g8"), "k", "the premoved O-O")
        compare(app.pieceAt("f8"), "r")
        tryVerify(() => app.find("ply2") !== null, 2000)
        compare(app.item("ply2").san, "O-O")

        FakeLichess.play("c9600001", "f1g1")
        tryCompare(ChessConnector, "MoveCount", 3, 3000, "White's O-O from Lichess")
        app.waitBoardIdle()
        compare(app.pieceAt("g1"), "K")
        compare(app.pieceAt("f1"), "R")
        compare(app.item("ply3").san, "O-O")

        FakeLichess.endGame("c9600001", "resign", "black")
        tryVerify(() => ChessConnector.GameResult !== "", 3000, "the game over")
        app.leaveToHome()
        app.waitForPage("home", 5000)
    }

    // An unfinished 960 game on Lichess (1. g3 g6, White to move): its card says Chess960, Home's
    // Continue puts it on the board from its own start, and a castling by clicks is posted as f1g1.
    function test_2_continueOngoing960() {
        FakeLichess.addOngoingGame("c9600002", "Slow960", "white", "g2g3 g7g6",
                                   "bqnnrkrb/pppppp1p/6p1/8/8/6P1/PPPPPP1P/BQNNRKRB w KQkq - 0 2", "chess960", start3)
        LichessClient.refreshOngoingGames()
        tryVerify(() => LichessClient.ongoingGames.length === 1, 3000, "the game listed")
        const listed = LichessClient.ongoingGames[0]
        compare(listed.canPlayHere, true, "Chess960 can be played here")
        compare(listed.variantName, "Chess960")

        app.click("continueCard")
        waitRequest("GET /api/board/game/stream/c9600002")
        app.waitForPage("game", 10000)
        app.waitBoardIdle()
        compare(ChessConnector.StartPositionNumber, 3)
        compare(ChessConnector.MoveCount, 2, "its two moves")
        compare(app.pieceAt("g3"), "P")
        compare(app.pieceAt("g6"), "p")
        compare(app.pieceAt("a1"), "B")

        app.clickSquare("f1")
        tryCompare(ChessConnector, "SelectedSquare", app.square("f1"), 2000)
        app.clickSquare("g1")
        waitRequest("POST /api/board/game/c9600002/move/f1g1")
        app.waitBoardIdle()
        compare(app.pieceAt("g1"), "K")
        compare(app.pieceAt("f1"), "R")

        FakeLichess.endGame("c9600002", "resign", "white")
        tryVerify(() => ChessConnector.GameResult !== "", 3000, "the game over")
        app.leaveToHome()
        app.waitForPage("home", 5000)
    }

    // Crazyhouse: the bar says so, Decline only. A Crazyhouse game rejoined anyway is refused with its
    // reason; Home stays, no game.
    function test_3_otherVariantsRefused() {
        FakeLichess.challenge("zh000001", "Housey", 1600, "white", "crazyhouse")
        tryVerify(() => bar().visible, 3000, "the challenge bar")
        verify(bar().text.indexOf("Crazyhouse") >= 0, bar().text)
        verify(bar().text.endsWith("Can't be played in this app"), bar().text)
        verify(!app.item("challengeAcceptButton").visible, "no Accept")
        app.click("challengeDeclineButton")
        waitRequest("POST /api/challenge/zh000001/decline")

        FakeLichess.addOngoingGame("zh000002", "Housey", "white", "e2e4", "", "crazyhouse")
        LichessClient.refreshOngoingGames()
        tryVerify(() => LichessClient.ongoingGames.some(g => g.gameId === "zh000002"), 3000, "the game listed")
        compare(LichessClient.ongoingGames.find(g => g.gameId === "zh000002").canPlayHere, false)

        startFailed.clear()
        verify(ChessConnector.continueOnlineGame("zh000002"))
        waitRequest("GET /api/board/game/stream/zh000002")
        tryCompare(startFailed, "count", 1, 3000, "refused")
        compare(startFailed.signalArguments[0][0],
                "it is Crazyhouse, and Chess++ plays standard chess and Chess960. Open it on lichess.org.")
        compare(ChessConnector.InGame, false, "no game")
        compare(app.pageName(), "home")
        FakeLichess.endGame("zh000002", "aborted", "")
    }

    // A Chess960 game Lichess starts as "startpos" (#518, the standard set-up): played as Chess960 #518,
    // not refused.
    function test_4_chess960AtTheStandardSetUp() {
        FakeLichess.challenge("c9600003", "Classic960", 1700, "black", "chess960")
        tryVerify(() => bar().visible, 3000, "the challenge bar")
        app.click("challengeAcceptButton")
        app.waitForPage("game", 10000)
        app.waitBoardIdle()
        compare(ChessConnector.Chess960, true)
        compare(ChessConnector.StartPositionNumber, 518)
        compare(app.pieceAt("e1"), "K")
        compare(app.pieceAt("h1"), "R")
        FakeLichess.endGame("c9600003", "aborted", "")
        tryVerify(() => ChessConnector.GameResult !== "", 3000, "the game over")
        app.leaveToHome()
        app.waitForPage("home", 5000)
    }

    // A Lichess game refused while a local game with a clock is on the board: nothing of it (its clock,
    // 10+5 in FakeLichess) reaches the local game, which goes on as it was.
    function test_5_refusedKeepsTheLocalGame() {
        ChessConnector.startLocalGame(true, "", 300, 0)
        app.waitForPage("game", 10000)
        app.waitBoardIdle()
        const clock = ChessConnector.Clock
        verify(clock.enabled)
        const white = clock.whiteText
        const black = clock.blackText

        FakeLichess.addOngoingGame("zh000003", "Housey", "white", "", "", "crazyhouse")
        startFailed.clear()
        verify(ChessConnector.continueOnlineGame("zh000003"))
        waitRequest("GET /api/board/game/stream/zh000003")
        tryCompare(startFailed, "count", 1, 3000, "refused")
        wait(300)   // a clock line of the refused game would have come by now
        compare(ChessConnector.GameMode, "local")
        compare(ChessConnector.InGame, true)
        compare(clock.whiteText, white, "the local clock, not Lichess's")
        compare(clock.blackText, black)
        compare(ChessConnector.GameResult, "")
        FakeLichess.endGame("zh000003", "aborted", "")
        app.leaveToHome()
        app.waitForPage("home", 5000)
    }

    // The Online page's Variant row: Chess960 names the Chess960 rating; a challenge to a player and a
    // link both ask Lichess for chess960; the choice is kept apart from the local games'.
    function test_6_onlineVariantRow() {
        compare(AppSettings.onlineChess960, false)
        const localChoice = AppSettings.chess960
        app.click("onlineCard")
        app.waitForPage("online", 5000)

        // LI-09: Quick pairing's own Variant row: a Chess960 seek, the tiles with the Chess960 rating,
        // remembered apart from the challenge's.
        app.scrollIntoView("quickPairingGrid")
        app.clickIn("online", "quickVariantChoice_1")
        compare(app.item("quickTile_10+0").rating, "1480?", "the Chess960 pool, provisional")
        app.click("quickTile_10+0")
        tryVerify(() => FakeLichess.seekOpen, 3000, "the seek's request open")
        compare(FakeLichess.bodyOf("POST /api/board/seek"), "time=10&increment=0&rated=true&variant=chess960")
        compare(AppSettings.quickChess960, true, "remembered")
        compare(AppSettings.onlineChess960, false, "the challenge's choice untouched")
        app.click("quickCancelButton")
        tryVerify(() => FakeLichess.closedByApp("POST /api/board/seek") === 1, 3000, "the seek closed")

        app.scrollIntoView("challengeRating")
        app.clickIn("online", "variantChoice_1")
        tryCompare(app.item("challengeRating"), "text", "Your Chess960 rating: 1480?", 2000)

        app.item("challengeOpponent").text = "Friend960"
        app.scrollIntoView("challengeButton")
        app.click("challengeButton")
        waitRequest("POST /api/challenge/Friend960")
        verify(FakeLichess.bodyOf("POST /api/challenge/Friend960").indexOf("variant=chess960") >= 0,
               FakeLichess.bodyOf("POST /api/challenge/Friend960"))
        compare(AppSettings.onlineChess960, true, "remembered")
        compare(AppSettings.chess960, localChoice, "the local games' choice untouched")

        // No such player here (FakeLichess answers 404): the page is free again for a link.
        app.item("challengeOpponent").text = ""
        tryVerify(() => app.item("challengeButton").enabled, 3000, "the button free again")
        app.click("challengeButton")
        waitRequest("POST /api/challenge/open")
        verify(FakeLichess.bodyOf("POST /api/challenge/open").indexOf("variant=chess960") >= 0,
               FakeLichess.bodyOf("POST /api/challenge/open"))
    }
}
