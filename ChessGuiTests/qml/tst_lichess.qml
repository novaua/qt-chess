import QtQuick
import QtTest
import ChessPlusPlus
import ChessGuiTests
import "qrc:/qml"   // AppSettings

// Scenario 12 (docs/UiTestSuitePlan.md §9) and the messages from Lichess: Lichess is FakeLichess, a
// local server the test plays the other side on (TestSetup; nothing reaches lichess.org). Tester is
// linked to TesterChess there. A challenge on the bar, declined; one accepted from Home: the game,
// moves both ways, a draw offer, a challenge during it, the connection lost and back, the end. Then
// an unfinished game rejoined from Home's Continue, with its moves on the board, and Leave / Resign.
TestCase {
    id: test
    name: "Lichess"
    when: windowShown

    AppDriver { id: app; test: test }

    readonly property string me: FakeLichess.username

    function initTestCase() {
        app.start()
        app.createFirstProfile("Tester")
        TestRun.linkLichess(me)
    }
    function cleanup() { app.afterTest() }
    function cleanupTestCase() { app.stop() }

    function bar() { return app.item("challengeBar") }
    function status() { return app.item("statusMessage") }
    // A request the app sent to Lichess, within `timeout`.
    function waitRequest(request, timeout) {
        tryVerify(() => FakeLichess.received(request), timeout ?? 3000,
                  request + " (last: " + FakeLichess.requests[FakeLichess.requests.length - 1] + ")")
    }

    // Linked: the account and its rating, the ongoing games asked for, the event stream open.
    function test_01_linked() {
        tryCompare(UserManager, "lichessConnected", true, 3000)
        tryVerify(() => UserManager.lichessRatings.rapid !== undefined, 3000, "the rapid rating from /api/account")
        compare(UserManager.lichessRatings.rapid.rating, 1650)
        waitRequest("GET /api/account/playing?nb=50")
        tryCompare(FakeLichess, "eventStreamOpen", true, 3000, "the event stream")
        verify(!bar().visible, "no challenge yet")
    }

    // A challenge comes: the bar on Home says who, the clock and our side. Decline: Lichess hears
    // it, with its reason, and the bar goes.
    function test_02_challengeDeclined() {
        FakeLichess.challenge("chal0001", "Speedy", 1980, "black")
        tryVerify(() => bar().visible, 3000, "the challenge bar")
        compare(bar().text, "Speedy (1980) challenges you · 10+5 · Casual · You play White")
        verify(app.item("challengeAcceptButton").visible, "Accept")
        app.click("challengeDeclineButton")
        waitRequest("POST /api/challenge/chal0001/decline")
        compare(FakeLichess.bodyOf("POST /api/challenge/chal0001/decline"), "reason=generic")
        tryVerify(() => !bar().visible, 3000, "the bar gone")
        compare(LichessClient.incomingChallenges.length, 0)
    }

    // Another one, accepted: the game page, the board turned for Black (the challenger plays White),
    // FriendlyRival named, nothing played yet; FriendlyRival's 1. e4 comes over the stream.
    // LI-07: both cards show the Lichess players: FM FriendlyRival 1712?, and TesterChess with the
    // profile's name and the rating ("Tester · 1650").
    function test_03_challengeAccepted() {
        FakeLichess.setPlayer("FriendlyRival", "FM", 1712, true)
        FakeLichess.challenge("game0001", "FriendlyRival", 1712, "white")
        tryVerify(() => bar().visible, 3000, "the challenge bar")
        compare(bar().text, "FriendlyRival (1712) challenges you · 10+5 · Casual · You play Black")
        app.click("challengeAcceptButton")
        waitRequest("POST /api/challenge/game0001/accept")
        app.waitForPage("game", 10000)
        compare(ChessConnector.GameMode, "online")
        compare(ChessConnector.OpponentName, "FriendlyRival")
        compare(ChessConnector.GameTitle, "Lichess · vs FM FriendlyRival")
        const opponent = app.item("opponentCard"), player = app.item("playerCard")
        compare(findChild(opponent, "playerTitle").text, "FM")
        verify(findChild(opponent, "playerTitle").visible, "the opponent's title shown")
        compare(findChild(opponent, "playerName").text, "FriendlyRival")
        compare(findChild(opponent, "playerDetail").text, "1712?", "provisional, as lichess.org writes it")
        compare(findChild(player, "playerName").text, me, "the player's Lichess name, not the profile's")
        verify(!findChild(player, "playerTitle").visible, "no title")
        compare(findChild(player, "playerDetail").text, "Tester · 1650", "the profile and the rating")
        tryVerify(() => !bar().visible, 3000, "the challenge is the game now: the bar gone")
        app.waitBoardIdle()
        compare(ChessConnector.MoveCount, 0)
        compare(app.page().board.rotation, 180, "the board turned for Black")

        FakeLichess.play("game0001", "e2e4")
        tryCompare(ChessConnector, "MoveCount", 1, 3000, "1. e4 from Lichess")
        app.waitBoardIdle()
        compare(app.pieceAt("e4"), "P")
        compare(ChessConnector.MovableColor, 2, "Black (the player) to move")
    }

    // The player's move: dragged, on the board at once, and posted to Lichess, which plays it.
    function test_04_ourMovePosted() {
        app.dragPiece("e7", "e5")
        tryCompare(ChessConnector, "MoveCount", 2, 3000, "e5 on the board")
        waitRequest("POST /api/board/game/game0001/move/e7e5")
        compare(FakeLichess.moves("game0001"), "e2e4 e7e5")
        FakeLichess.play("game0001", "g1f3")
        tryCompare(ChessConnector, "MoveCount", 3, 3000, "2. Nf3 from Lichess")
        app.waitBoardIdle()
        compare(app.pieceAt("f3"), "N")
        compare(app.pieceAt("e5"), "p")
    }

    // The opponent offers a draw: over the command bar, with Accept and Decline. Declined: Lichess
    // hears it and the offer goes; the game goes on.
    function test_05_drawOfferDeclined() {
        FakeLichess.offerDraw("game0001")
        tryVerify(() => status().showing && status().text === "FriendlyRival offers a draw", 3000,
                  "the offer over the command bar (shown: \"" + status().text + "\")")
        compare(ChessConnector.PendingOffer, "draw")
        app.click("offerDeclineButton")
        waitRequest("POST /api/board/game/game0001/draw/no")
        tryCompare(ChessConnector, "PendingOffer", "", 3000, "the offer gone")
        tryVerify(() => !status().showing, 3000, "nothing over the command bar")
        compare(ChessConnector.GameResult, "", "the game goes on")
    }

    // A challenge during a running real-time game: shown, but Decline only, and why; Hide puts it away
    // (it stays on Lichess, not declined).
    function test_06_challengeDuringTheGame() {
        compare(ChessConnector.CanAcceptChallenge, false)
        FakeLichess.challenge("chal0002", "Speedy", 1980, "random")
        tryVerify(() => bar().visible, 3000, "the challenge bar")
        verify(bar().text.endsWith("Finish your game first: it waits on Lichess."), bar().text)
        verify(!app.item("challengeAcceptButton").visible, "no Accept during the game")
        verify(app.item("challengeDeclineButton").visible)
        app.click("challengeHideButton")
        tryVerify(() => !bar().visible, 3000, "the bar hidden")
        verify(!FakeLichess.received("POST /api/challenge/chal0002/decline"), "hidden, not declined")
        compare(LichessClient.incomingChallenges.length, 1, "still waiting on Lichess")
    }

    // The game's stream is cut and Lichess refuses it for a moment: "Reconnecting to Lichess…" over
    // the command bar; it comes back on its own, with "Reconnected to Lichess", and the moves are the same.
    function test_07_connectionLostAndBack() {
        FakeLichess.dropGameStreams("game0001", 1500)
        tryCompare(ChessConnector, "Reconnecting", true, 3000, "the connection lost")
        tryVerify(() => status().showing && status().text === "Reconnecting to Lichess…", 3000,
                  "over the command bar (shown: \"" + status().text + "\")")
        tryCompare(ChessConnector, "Reconnecting", false, 8000, "back")
        app.waitMessage("Reconnected to Lichess")
        compare(ChessConnector.MoveCount, 3, "the same moves")
        // Still played: the player's move reaches Lichess.
        app.dragPiece("b8", "c6")
        waitRequest("POST /api/board/game/game0001/move/b8c6")
        compare(FakeLichess.moves("game0001"), "e2e4 e7e5 g1f3 b8c6")
    }

    // The opponent resigns: the result card, the player (Black) wins; Home without a question.
    function test_08_opponentResigns() {
        FakeLichess.endGame("game0001", "resign", "black")
        tryVerify(() => app.find("resultCard")?.visible, 3000, "the result card")
        verify(ChessConnector.GameResult !== "", "a result: " + ChessConnector.GameResult)
        app.leaveToHome()
        verify(!app.item("leaveOnlineDialog").visible, "a finished game: no Leave the game?")
    }

    // An unfinished game on Lichess, 1. d4 d5 2. c4, Black to move: Home's Continue joins it with
    // its moves on the board from the first frame, then it goes on.
    function test_09_continueWithItsMoves() {
        FakeLichess.addOngoingGame("game0002", "SlowPlayer", "black", "d2d4 d7d5 c2c4",
                                   "rnbqkbnr/ppp1pppp/8/3p4/2PP4/8/PP2PPPP/RNBQKBNR b KQkq - 0 2")
        LichessClient.refreshOngoingGames()   // as the Online page's refresh does
        tryVerify(() => LichessClient.ongoingGames.length === 1, 3000, "the game listed")
        // BL-24: Continue while the list refreshes (there are several in the first seconds after a start)
        // still joins the only game, not the Continue list. Called directly, as the card's click does
        // (ChessGame.continueGames): a click could let the refresh's answer in first.
        LichessClient.refreshOngoingGames()
        verify(LichessClient.ongoingGamesLoading, "a refresh in flight")
        verify(ChessConnector.continueOnlyGame(), "the only game joined, not the list")
        waitRequest("GET /api/board/game/stream/game0002")
        app.waitForPage("game", 10000)
        compare(ChessConnector.OpponentName, "SlowPlayer")
        compare(ChessConnector.MoveCount, 3, "its three moves")
        app.waitBoardIdle()
        compare(app.pieceAt("c4"), "P")
        compare(app.pieceAt("d4"), "P")
        compare(app.pieceAt("d5"), "p")
        compare(app.pieceAt("c2"), "")
        compare(ChessConnector.MovableColor, 2, "Black (the player) to move")

        app.clickMove("e7", "e6")
        waitRequest("POST /api/board/game/game0002/move/e7e6")
    }

    // Home during the game: Leave the game? Resign: Lichess hears it, the game ends, Home.
    function test_10_leaveAndResign() {
        app.click("homeButton")
        const dialog = app.item("leaveOnlineDialog")
        tryVerify(() => dialog.opened, 3000, "Leave the game? asked")
        compare(ChessConnector.InGame, true, "nothing left before the answer")
        app.clickIn("leaveOnlineDialog", "resignButton")
        waitRequest("POST /api/board/game/game0002/resign")
        app.waitForPage("home", 5000)
        app.waitClosed(dialog)
        compare(FakeLichess.status("game0002"), "resign")
    }

    // BL-23: with Days per move chosen, its seven buttons, their label and the username field stay inside
    // the "Challenge a friend" card: in this wide window and in the narrowest one (the window's minimum).
    function test_11_challengeCardFits() {
        app.click("onlineCard")
        app.waitForPage("online", 5000)
        app.scrollIntoView("challengeTimeType")   // under Quick pairing (LI-09)
        app.clickIn("online", "challengeTimeType_2")
        app.scrollIntoView("challengeDays")
        app.clickIn("online", "challengeDays_6")
        compare(app.item("challengeDays").checkedIndex, 6, "14 days")
        const card = app.item("challengeCard")
        const wide = app.window.width
        for (const width of [wide, app.window.minimumWidth]) {
            app.window.width = width
            app.laidOut()
            for (const name of ["challengeDays", "challengeDaysLabel", "challengeOpponent"]) {
                const it = app.item(name)
                const left = it.mapToItem(card, 0, 0).x
                verify(left >= 0 && left + it.width <= card.width,
                       name + " inside the card at window width " + width + ": " + left + " + " + it.width
                       + " of " + card.width)
            }
        }
        app.window.width = wide
        app.laidOut()
        keyClick(Qt.Key_Escape)   // Back
        app.waitForPage("home", 5000)
    }

    // LI-10: a right click on a Lichess game's card, Your games on the Online page: its menu. A Crazyhouse
    // game (not playable here) has Open on lichess.org and Copy link, no Continue; Copy link puts its URL
    // on the clipboard and says "Link copied". The Menu key and Shift+F10 on its button open the menu too.
    function test_12_copyGameLink() {
        FakeLichess.addOngoingGame("zhGame01", "Housey", "white", "e2e4", "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1",
                                   "crazyhouse")
        LichessClient.refreshOngoingGames()
        tryVerify(() => LichessClient.ongoingGames.some(g => g.gameId === "zhGame01"), 3000, "the game listed")
        app.click("onlineCard")
        app.waitForPage("online", 5000)
        tryVerify(() => app.find("ongoingGame_zhGame01") !== null, 3000, "its card")
        app.scrollIntoView("ongoingGame_zhGame01")
        const card = app.item("ongoingGame_zhGame01")
        compare(card.actionText, "Open on lichess.org", "not playable here")
        mouseClick(card, card.width / 2, card.height / 2, Qt.RightButton)
        const copy = findChild(card, "gameMenuCopyLink")
        tryVerify(() => copy.menu.opened, 3000, "the card's menu")
        verify(!findChild(card, "gameMenuContinue").visible, "no Continue for a game that can't be played here")
        verify(findChild(card, "gameMenuOpen").visible)
        mouseClick(copy)
        tryVerify(() => TestRun.clipboardText() === "https://lichess.org/zhGame01", 3000, "the link on the clipboard: " + TestRun.clipboardText())
        app.waitMessage("Link copied")
        tryVerify(() => !copy.menu.visible, 3000, "the menu closed")

        // The keyboard: the Menu key, then Shift+F10, on the card's button opens the same menu.
        const button = findChild(card, "actionButton")
        button.forceActiveFocus()
        keyClick(Qt.Key_Menu)
        tryVerify(() => copy.menu.opened, 3000, "the menu by the Menu key")
        keyClick(Qt.Key_Escape)   // closes the menu, not the page
        tryVerify(() => !copy.menu.visible, 3000)
        compare(app.pageName(), "online")
        button.forceActiveFocus()
        keyClick(Qt.Key_F10, Qt.ShiftModifier)
        tryVerify(() => copy.menu.opened, 3000, "the menu by Shift+F10")
        keyClick(Qt.Key_Escape)
        tryVerify(() => !copy.menu.visible, 3000)
        keyClick(Qt.Key_Escape)   // Back
        app.waitForPage("home", 5000)
    }

    // LI-09: quick pairing on the Online page. Bullet and Blitz greyed (lichess.org only), no rating on
    // the tiles (the owner's hand tests). 10+0: a rated Standard seek, its tile and the line under the grid wait; Cancel
    // closes the seek's request. A refusal shows Lichess's reason. 15+10 Casual: someone takes it, the
    // game page opens with them; Resign ends it.
    function test_13_quickPairing() {
        app.click("onlineCard")
        app.waitForPage("online", 5000)
        const grid = app.item("quickPairingGrid")
        const tile = label => app.item("quickTile_" + label)
        app.scrollIntoView("quickPairingGrid")
        verify(tile("3+0").opacity < 1, "Blitz greyed")
        verify(tile("1+0").opacity < 1, "Bullet greyed")
        compare(tile("10+0").opacity, 1)
        verify(tile("10+0").rating === undefined, "no rating on a tile, as lichess.org's")
        compare(AppSettings.quickRated, true, "rated by default")

        app.click("quickTile_10+0")
        tryVerify(() => FakeLichess.seekOpen, 3000, "the seek's request open")
        compare(FakeLichess.bodyOf("POST /api/board/seek"), "time=10&increment=0&rated=true&variant=standard")
        compare(grid.seekingIndex, 6, "10+0 waits")
        verify(tile("10+0").isSeeking)
        verify(!tile("15+10").usable, "one seek at a time")
        verify(app.item("quickWaiting").visible, "Looking for an opponent…")
        app.click("quickCancelButton")
        tryVerify(() => FakeLichess.closedByApp("POST /api/board/seek") === 1, 3000, "Lichess sees the seek closed")
        verify(!app.item("quickWaiting").visible)
        compare(grid.seekingIndex, -1)

        FakeLichess.refuseSeeks(400, "You have too many games going on")
        app.click("quickTile_30+0")
        const errorBar = app.item("onlineErrorBar")
        tryVerify(() => errorBar.visible, 3000, "the refusal shown")
        compare(errorBar.text, "Quick pairing: You have too many games going on")
        compare(grid.seekingIndex, -1)
        FakeLichess.refuseSeeks(0)

        app.click("quickRated_1")
        compare(AppSettings.quickRated, false, "Casual remembered")
        app.click("quickTile_15+10")
        tryVerify(() => FakeLichess.seekOpen, 3000, "the second seek open")
        verify(!errorBar.visible, "the old refusal gone")
        compare(FakeLichess.bodyOf("POST /api/board/seek"), "time=15&increment=10&rated=false&variant=standard")
        verify(FakeLichess.pairSeek("seek0001", "Stranger"), "taken")
        app.waitForPage("game", 10000)
        compare(ChessConnector.GameMode, "online")
        compare(ChessConnector.OpponentName, "Stranger")
        compare(LichessClient.seek.limitSec, undefined, "the seek is over")
        compare(FakeLichess.closedByApp("POST /api/board/seek"), 1, "closed by Lichess, not the app")
        AppSettings.quickRated = true

        app.click("homeButton")
        const dialog = app.item("leaveOnlineDialog")
        tryVerify(() => dialog.opened, 3000, "Leave the game? asked")
        app.clickIn("leaveOnlineDialog", "resignButton")
        waitRequest("POST /api/board/game/seek0001/resign")
        app.waitForPage("home", 5000)
        app.waitClosed(dialog)
    }

    // LI-09 D5: the seek goes on beyond the Online page. Back says so, once a seek; every page's command
    // bar shows it, and its menu's Cancel seeking closes the request. A seek taken while on Home opens
    // the game. Switch user ends it (the profile picker keeps the token: a new token isn't what ends it).
    function test_14_seekBeyondThePage() {
        const told = "Still looking for an opponent. Cancel it from the icon at the top."
        const closed = () => FakeLichess.closedByApp("POST /api/board/seek")
        const indicator = () => findChild(app.page(), "seekIndicator")   // the page's own command bar
        const seekFrom = label => {
            app.click("onlineCard")
            app.waitForPage("online", 5000)
            app.scrollIntoView("quickPairingGrid")
            app.click("quickTile_" + label)
            tryVerify(() => FakeLichess.seekOpen, 3000, "the seek's request open")
            verify(indicator().visible, "the Online page's command bar shows it too")
            keyClick(Qt.Key_Escape)   // Back
            app.waitForPage("home", 5000)
        }
        const closedBefore = closed()
        verify(!indicator().visible, "no seek, no icon")

        seekFrom("10+0")
        app.waitMessage(told)
        verify(FakeLichess.seekOpen, "the seek goes on")
        compare(closed(), closedBefore)
        tryVerify(() => indicator().visible, 3000, "Home's command bar shows the seek")

        // Back to the page and Back again, the same seek: its tile still waits, nothing said again.
        app.click("onlineCard")
        app.waitForPage("online", 5000)
        compare(app.item("quickPairingGrid").seekingIndex, 6, "10+0 still waits")
        keyClick(Qt.Key_Escape)
        app.waitForPage("home", 5000)
        compare(app.recentMessages().filter(m => m === told).length, 1, "said once a seek")

        // The icon's menu: what is sought, Cancel seeking.
        mouseClick(indicator())
        const menuText = findChild(indicator(), "seekMenuText")
        tryVerify(() => menuText.visible, 3000, "the seek's menu open")
        verify(menuText.text.startsWith("Looking for an opponent · 10+0 Rapid · Rated · 0:"), menuText.text)
        const cancelItem = findChild(indicator(), "cancelSeekItem")
        tryVerify(() => cancelItem.menu.opened, 3000, "the menu opened")
        mouseClick(cancelItem)
        tryVerify(() => closed() === closedBefore + 1, 3000, "Lichess sees the seek closed")
        tryVerify(() => !indicator().visible, 3000, "the icon gone")
        compare(LichessClient.seek.limitSec, undefined)

        // Taken while on Home: the game opens.
        seekFrom("15+10")
        verify(FakeLichess.pairSeek("seek0002", "Wanderer"), "taken")
        app.waitForPage("game", 10000)
        compare(ChessConnector.OpponentName, "Wanderer")
        verify(!indicator().visible, "the seek is over")
        app.click("homeButton")
        const dialog = app.item("leaveOnlineDialog")
        tryVerify(() => dialog.opened, 3000, "Leave the game? asked")
        app.clickIn("leaveOnlineDialog", "resignButton")
        waitRequest("POST /api/board/game/seek0002/resign")
        app.waitForPage("home", 5000)
        app.waitClosed(dialog)

        // Switch user: the seek was this profile's.
        seekFrom("30+0")
        app.switchUserFromHome()
        tryVerify(() => closed() === closedBefore + 2, 3000, "the seek closed on Switch user")
        compare(LichessClient.seek.limitSec, undefined)
        app.pickProfile("Tester")
    }

    // LI-09 D2: the Custom tile's Seek dialog. A clock of one's own (½+12: a fraction of a minute, Rapid
    // by its increment) is sought as a tile's, and the Custom tile waits with its clock; a Blitz clock
    // can't be sent and says why; a correspondence seek (days, no time; the card's Casual and Chess960)
    // is left on Lichess, said with Open on lichess.org.
    function test_15_customSeek() {
        const seeks = () => FakeLichess.requests.filter(r => r === "POST /api/board/seek").length
        const closed = () => FakeLichess.closedByApp("POST /api/board/seek")
        app.click("onlineCard")
        app.waitForPage("online", 5000)
        app.scrollIntoView("quickPairingGrid")
        const grid = app.item("quickPairingGrid")
        const custom = app.item("quickTile_custom")
        const dialog = app.item("seekDialog")
        const inDialog = name => findChild(dialog, name)
        const setClock = (minutes, increment) => {
            inDialog("seekMinutes").value = dialog.minuteSteps.indexOf(minutes)
            inDialog("seekIncrement").value = dialog.incrementSteps.indexOf(increment)
        }
        compare(custom.opacity, 1, "Custom isn't greyed any more")

        app.click("quickTile_custom")
        tryVerify(() => dialog.opened, 3000, "the Seek dialog")
        compare(inDialog("seekChoices").text, "Rated · Standard, as chosen on the Quick pairing card.")
        compare(inDialog("seekClockText").text, "20+0 · Rapid", "the default clock")

        // 5+3 is Blitz: no Seek, by button or Enter, and the reason.
        setClock(5, 3)
        compare(inDialog("seekClockText").text, "5+3 · Blitz")
        verify(!inDialog("seekButton").enabled, "Seek off below Rapid")
        verify(inDialog("seekTooFast").visible, "the reason shown")
        const before = seeks()
        keyClick(Qt.Key_Return)
        wait(200)
        verify(dialog.opened, "Enter doesn't send it")
        compare(seeks(), before, "nothing sent")

        // ½+12: 30 + 40 × 12 = 510 s, Rapid.
        setClock(0.5, 12)
        compare(inDialog("seekClockText").text, "½+12 · Rapid")
        verify(!inDialog("seekTooFast").visible)
        app.clickIn("seekDialog", "seekButton")
        app.waitClosed(dialog)
        tryVerify(() => FakeLichess.seekOpen, 3000, "the seek's request open")
        compare(FakeLichess.bodyOf("POST /api/board/seek"), "time=0.5&increment=12&rated=true&variant=standard")
        compare(grid.seekingIndex, 11, "the Custom tile waits")
        compare(findChild(custom, "quickTileLabel").text, "½+12", "with its clock")
        verify(app.item("quickWaiting").visible)
        const closedBefore = closed()
        app.click("quickTile_custom")   // its click cancels, as a tile's
        tryVerify(() => closed() === closedBefore + 1, 3000, "Lichess sees the seek closed")
        compare(grid.seekingIndex, -1)
        compare(findChild(custom, "quickTileLabel").text, "Custom")

        // A tile's clock from the dialog (10+0) waits on Custom too, and Custom cancels it.
        app.click("quickTile_custom")
        tryVerify(() => dialog.opened, 3000, "the Seek dialog")
        setClock(10, 0)
        app.clickIn("seekDialog", "seekButton")
        app.waitClosed(dialog)
        tryVerify(() => FakeLichess.seekOpen, 3000)
        compare(grid.seekingIndex, 11, "on Custom, not on 10+0")
        compare(findChild(custom, "quickTileLabel").text, "10+0")
        // While a seek waits the dialog can't send another (the state may change while it is open).
        dialog.openWith(true, false)
        tryVerify(() => dialog.opened, 3000)
        verify(!inDialog("seekButton").enabled, "no second seek")
        verify(inDialog("seekUnavailable").visible, "and why")
        keyClick(Qt.Key_Escape)
        app.waitClosed(dialog)
        app.click("quickTile_custom")
        tryVerify(() => closed() === closedBefore + 2, 3000, "Lichess sees the seek closed")

        // Correspondence, Casual Chess960 from the card; the last clock remembered.
        app.click("quickRated_1")
        app.click("quickVariantChoice_1")
        app.click("quickTile_custom")
        tryVerify(() => dialog.opened, 3000, "the Seek dialog again")
        compare(inDialog("seekChoices").text, "Casual · Chess960, as chosen on the Quick pairing card.")
        compare(inDialog("seekClockText").text, "10+0 · Rapid", "the last clock sent")
        app.clickIn("seekKind", "seekKind_1")   // a ChoiceRow's button: looked up in its row
        verify(!inDialog("seekMinutes").visible, "no clock for correspondence")
        app.clickIn("seekDays", "seekDays_2")
        app.clickIn("seekDialog", "seekButton")
        app.waitClosed(dialog)
        const sentBar = app.item("seekSentBar")
        tryVerify(() => sentBar.visible, 3000, "the seek said to be on Lichess")
        compare(FakeLichess.bodyOf("POST /api/board/seek"), "days=3&rated=false&variant=chess960")
        compare(sentBar.text, "Your seek is on Lichess. When someone takes it, the game shows in Your games. "
                              + "Only lichess.org can cancel it.")
        compare(grid.seekingIndex, -1, "nothing waits here")
        verify(!app.item("quickWaiting").visible)

        app.click("quickTile_custom")   // Cancel in the dialog: nothing sent, the message stays
        tryVerify(() => dialog.opened, 3000)
        keyClick(Qt.Key_Escape)
        app.waitClosed(dialog)
        verify(sentBar.visible, "still said")

        app.click("quickTile_10+0")   // a new seek: the old message goes
        tryVerify(() => FakeLichess.seekOpen, 3000)
        verify(!sentBar.visible)
        compare(grid.seekingIndex, 6, "a tile's seek on its tile")
        app.click("quickCancelButton")
        tryVerify(() => closed() === closedBefore + 3, 3000)

        AppSettings.quickRated = true
        AppSettings.quickChess960 = false
        app.click("quickVariantChoice_0")
        keyClick(Qt.Key_Escape)   // Back
        app.waitForPage("home", 5000)
    }
}
