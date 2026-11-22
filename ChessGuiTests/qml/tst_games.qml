import QtQuick
import QtTest
import ChessPlusPlus
import ChessGuiTests

// The games library (BL-19 step 3): the kept games on the Games page, its filters, a game's PGN (the
// file Export PGN writes), Delete, and Settings → Data "Games kept" (a smaller number asks first).
TestCase {
    id: test
    name: "Games"
    when: windowShown

    AppDriver { id: app; test: test }

    SignalSpy { id: exported; target: ChessConnector; signalName: "pgnExported" }

    function initTestCase() {
        app.start()
        app.createFirstProfile("Tester")
    }
    function cleanup() { app.afterTest() }
    function cleanupTestCase() { app.stop() }

    function keptGame(test) { return GameLibrary.games.find(test) }
    function foolsMate() { return keptGame(g => g.kind === "local" && g.finished) }

    // Three games, each left for Home: Fool's mate (two players, finished), 1. e4 (two players,
    // unfinished), 1. e4 e5 vs the robot (unfinished).
    function test_01_threeGames() {
        app.startTwoPlayerGame()
        for (const [from, to] of [["f2", "f3"], ["e7", "e5"], ["g2", "g4"], ["d8", "h4"]])
            app.clickMove(from, to)
        tryVerify(() => ChessConnector.GameResult !== "", 3000, "the mate")
        app.leaveToHome()

        app.startTwoPlayerGame()
        app.clickMove("e2", "e4")
        tryCompare(ChessConnector, "MoveCount", 1, 3000)
        app.leaveToHome()

        app.startComputerGame("e2e4 e7e5", 0)
        app.waitBoardIdle()
        app.clickMove("e2", "e4")
        tryCompare(ChessConnector, "MoveCount", 2, 5000, "the robot's e5")
        app.leaveToHome()

        compare(GameLibrary.count, 3)
        compare(GameLibrary.unfinishedCount, 2)
        compare(GameLibrary.finishedCount, 1)
    }

    // Ctrl+O: the Games page, newest first; each filter shows its kind; Back to Home.
    function test_02_filters() {
        app.laidOut()
        keyClick(Qt.Key_O, Qt.ControlModifier)
        app.waitForPage("library", 5000)
        tryVerify(() => app.find("gameCard_2") !== null, 3000, "three cards")
        compare(app.item("gameCard_0").title.indexOf("Robot"), 0, "the robot's game first (newest)")
        compare(app.item("gameCard_2").actionText, "Open", "Fool's mate: finished")

        const shown = () => app.page()._games
        app.click("gamesFilter_1")   // vs Robot
        tryCompare(app.page(), "filter", 1)
        compare(shown().length, 1)
        tryVerify(() => app.find("gameCard_1") === null, 2000, "one card")
        verify(shown().every(g => g.kind === "computer"))

        app.click("gamesFilter_2")   // Two players
        tryVerify(() => shown().length === 2 && shown().every(g => g.kind === "local"), 2000)

        app.click("gamesFilter_3")   // Lichess (LI-14): none kept yet
        tryVerify(() => shown().length === 0, 2000)
        verify(app.item("gamesFilterEmpty").visible)
        compare(app.item("gamesFilterEmpty").text, "No Lichess games yet. A Lichess game you finish here is kept.")

        app.click("gamesFilter_4")   // Unfinished
        tryVerify(() => shown().length === 2 && shown().every(g => !g.finished), 2000)
        compare(app.item("gameCard_0").actionText, "Continue")
        verify(!app.item("gamesFilterEmpty").visible)

        app.click("gamesFilter_0")   // All
        tryVerify(() => app.find("gameCard_2") !== null, 2000, "all three again")
        verify(app.item("gameCard_0").exportable, "Export PGN on the cards")
    }

    // A kept game's PGN: the tags for a two-player game and a robot game, and the file Export PGN
    // writes (the card's button opens the file dialog, a native one: not in a test).
    function test_03_exportPgn() {
        const mate = foolsMate()
        const pgn = ChessConnector.pgn(mate.id)
        for (const tag of ['[Event "Casual two-player game"]', '[Site "Chess++"]', '[White "Tester"]',
                           '[Black "Guest"]', '[Result "0-1"]', '[TimeControl "-"]'])
            verify(pgn.indexOf(tag) >= 0, tag + " in\n" + pgn)
        verify(pgn.indexOf("Elo") < 0, "no Elo in a two-player game")
        verify(pgn.indexOf("1. f3 e5 2. g4 Qh4# 0-1") >= 0, pgn)
        compare(ChessConnector.pgnFileName(mate.id).indexOf("Tester vs Guest "), 0)

        const robot = keptGame(g => g.kind === "computer")
        const robotPgn = ChessConnector.pgn(robot.id)
        const level = ChessConnector.Levels.find(l => l.level === robot.level)
        verify(robotPgn.indexOf('[Event "Casual game vs the robot, level ' + robot.level + " (" + level.name + ')"]') >= 0, robotPgn)
        const robotSide = robot.playerPlaysWhite ? "Black" : "White"
        verify(robotPgn.indexOf("[" + robotSide + 'Elo "' + level.elo + '"]') >= 0, "the robot's Elo in\n" + robotPgn)
        verify(robotPgn.indexOf('[Result "*"]') >= 0, "unfinished")
        verify(robotPgn.indexOf("1. e4 e5 *") >= 0, robotPgn)

        const file = TestRun.fileUrl("kept.pgn")
        exported.clear()
        ChessConnector.exportPgn(file, mate.id)
        compare(exported.count, 1)
        compare(exported.signalArguments[0][0], true, "written")
        compare(TestRun.readTextFile(file), pgn)
        compare(ChessConnector.pgn("no-such-game"), "")
    }

    // Delete on Fool's mate's card: the confirm (the window's shortcuts wait while it is open: Esc is the
    // dialog's), then the game is gone, the others stay.
    function test_04_delete() {
        const id = foolsMate().id
        const index = GameLibrary.games.findIndex(g => g.id === id)
        app.clickIn("gameCard_" + index, "deleteButton")
        const confirm = app.item("deleteGameConfirm")
        tryVerify(() => confirm.opened, 3000, "asked first")
        verify(app.window._dialogOpen, "the page's dialog counts as the window's")
        app.clickIn("deleteGameConfirm", "acceptButton")
        app.waitClosed(confirm)
        verify(!app.window._dialogOpen)
        tryCompare(GameLibrary, "count", 2, 2000)
        verify(!GameLibrary.games.some(g => g.id === id))
        compare(GameLibrary.unfinishedCount, 2)
        app.clickIn("library", "backButton")
        app.waitForPage("home", 5000)
    }

    // Settings → Data "Games kept" with 52 finished games: 50 asks (Cancel keeps all of them; Delete
    // removes the 2 oldest, the unfinished ones stay); 200 doesn't ask.
    function test_05_gamesKept() {
        TestRun.addFinishedGames(52)
        compare(GameLibrary.finishedCount, 52)
        app.click("settingsButton")
        app.waitForPage("settings", 5000)
        const box = app.item("gamesKeptBox")
        compare(box.displayText, "100 games")

        const choose = count => {
            app.scrollIntoView("gamesKeptBox")
            app.click("gamesKeptBox")
            tryVerify(() => box.popup.opened, 3000, "the list open")
            const row = box.popup.contentItem.itemAtIndex(box.counts.indexOf(count))
            verify(row !== null, "the row for " + count)
            mouseClick(row)
        }
        choose(50)
        const confirm = app.item("gamesKeptConfirm")
        tryVerify(() => confirm.opened, 3000, "asked first")
        compare(confirm.gone, 2)
        verify(app.window._dialogOpen, "the page's dialog counts as the window's")
        app.clickIn("gamesKeptConfirm", "cancelButton")
        app.waitClosed(confirm)
        verify(!app.window._dialogOpen)
        compare(GameLibrary.keepCount, 100, "kept as it was (it follows the setting)")
        compare(GameLibrary.finishedCount, 52, "nothing deleted")
        tryCompare(box, "displayText", "100 games")

        choose(50)
        tryVerify(() => confirm.opened, 3000, "asked again")
        app.clickIn("gamesKeptConfirm", "acceptButton")
        app.waitClosed(confirm)
        compare(GameLibrary.keepCount, 50)
        tryCompare(GameLibrary, "finishedCount", 50, 2000, "the 2 oldest gone")
        compare(GameLibrary.unfinishedCount, 2, "unfinished games stay")
        tryCompare(box, "displayText", "50 games")

        choose(200)
        tryCompare(GameLibrary, "keepCount", 200, 2000, "more: no question")
        verify(!confirm.visible)
        tryCompare(box, "displayText", "200 games")
        compare(GameLibrary.finishedCount, 50)
        app.clickIn("settings", "backButton")
        app.waitForPage("home", 5000)
    }

    // Play from here (BL-19 step 4) vs the robot: a two-player game 1. e4 e5 2. Nf3 Nc6, reviewed back
    // to 1... e5: not while the game goes on; once White resigns, Play from here vs computer: the dialog
    // says where, the player takes White (to move), the board has the two moves; Nf3 and the robot's Nc6
    // (FakeUci follows the same game). The fork is a record of its own (forkOfId, forkPly 2); a takeback
    // stops at the moves it began with.
    property string resignedId   // test 6's game, resigned: not a Fool's mate for test 7
    function test_06_playFromHereVsRobot() {
        app.startTwoPlayerGame()
        for (const [from, to] of [["e2", "e4"], ["e7", "e5"], ["g1", "f3"], ["b8", "c6"]])
            app.clickMove(from, to)
        tryCompare(ChessConnector, "MoveCount", 4, 3000)
        app.waitBoardIdle()
        // Copied: a list property's element read again later is the list's element then.
        const sourceId = GameLibrary.games[0].id
        resignedId = sourceId
        compare(GameLibrary.games[0].plyCount, 4, "the two-player game kept")
        // Step 4b: only while reviewing; the More menu doesn't offer it.
        compare(Object.keys(ChessConnector.PlayFromHere).length, 0, "not at the live position")
        verify(!app.find("playFromHereButton").visible)
        app.click("moreButton")
        const more = app.item("switchUserItem").menu
        tryVerify(() => more.opened, 3000)
        verify(app.find("playFromHereRobotItem") === null, "not in the More menu")
        more.close()
        tryVerify(() => !more.visible, 3000)

        app.click("review_prev")
        app.click("review_prev")
        tryCompare(ChessConnector, "ShownPly", 2, 2000, "after 1... e5")
        // The game goes on: no fork of it (the owner, 2026-11-17).
        compare(Object.keys(ChessConnector.PlayFromHere).length, 0, "not while the game goes on")
        verify(!app.find("playFromHereButton").visible, "no button while the game goes on")
        ChessConnector.resignGame()
        tryVerify(() => ChessConnector.GameResult !== "", 3000, "White resigned")
        tryVerify(() => ChessConnector.PlayFromHere.ply === 2, 2000, "over: the position reviewed")
        compare(ChessConnector.ShownPly, 2, "still reviewing")
        TestRun.setEngine("e2e4 e7e5 g1f3 b8c6", 0)
        app.chooseFromMenu("playFromHereButton", "playFromHereRobotItem")
        const dialog = app.item("computerDialog")
        tryVerify(() => dialog.opened, 3000, "the vs computer dialog")
        compare(dialog.title, "Play from here vs computer")
        compare(app.item("fromHereText").text, "From the position after 1… e5: White to move.")
        app.click("startComputerGameButton")
        app.waitClosed(dialog)
        tryCompare(ChessConnector, "GameMode", "computer", 5000)
        compare(ChessConnector.MoveCount, 2, "the moves it begins with")
        compare(ChessConnector.PlayerPlaysWhite, true, "the side to move")
        app.waitBoardIdle()
        compare(app.pieceAt("e5"), "p")
        compare(GameLibrary.count, 53, "no record before a move of its own")

        app.clickMove("g1", "f3")
        tryCompare(ChessConnector, "MoveCount", 4, 5000, "the robot's Nc6")
        app.waitBoardIdle()
        const fork = GameLibrary.games[0]
        compare(fork.kind, "computer")
        compare(fork.forkOfId, sourceId)
        compare(fork.forkPly, 2)
        compare(fork.plyCount, 4)
        compare(GameLibrary.game(sourceId).plyCount, 4, "the game it came from as it was")

        keyClick(Qt.Key_Z, Qt.ControlModifier)   // Nf3 and Nc6 back
        tryCompare(ChessConnector, "MoveCount", 2, 3000)
        app.waitBoardIdle()
        keyClick(Qt.Key_Z, Qt.ControlModifier)   // nothing more: the fork's first moves stay
        app.waitMessage("No more moves")
        compare(ChessConnector.MoveCount, 2)
        app.leaveToHome()
    }

    // Play from here, two players, from a replay of a kept game (Fool's mate after 1. f3 e5): the
    // dialog takes the side to move; the game goes on from there with the two moves; left without a
    // move of its own, it isn't kept.
    function test_07_playFromHereTwoPlayers() {
        const keptId = GameLibrary.games.find(g => g.finished && g.id !== resignedId).id
        const count = GameLibrary.count
        verify(ChessConnector.openGame(keptId))
        app.waitForPage("game", 10000)
        app.click("nextButton")
        app.click("nextButton")
        tryCompare(ChessConnector, "ShownPly", 2, 2000)
        app.chooseFromMenu("playFromHereButton", "playFromHereTwoPlayersItem")
        const dialog = app.item("twoPlayerDialog")
        tryVerify(() => dialog.opened, 3000, "the two players dialog")
        compare(dialog.title, "Play from here, two players")
        app.click("startGameButton")
        app.waitClosed(dialog)
        tryCompare(ChessConnector, "GameMode", "local", 5000)
        compare(ChessConnector.MoveCount, 2)
        compare(ChessConnector.MovableColor, 1, "White to move")
        app.waitBoardIdle()
        app.leaveToHome()
        compare(GameLibrary.count, count, "not kept without a move")

        // The robot's game over there: nothing to play from (the last position is a mate).
        verify(ChessConnector.openGame(keptId))
        app.waitForPage("game", 10000)
        for (let i = 0; i < 4; ++i)
            app.click("nextButton")
        tryCompare(ChessConnector, "ShownPly", 4, 2000)
        compare(Object.keys(ChessConnector.PlayFromHere).length, 0, "after the mate: none")
        verify(!app.find("playFromHereButton").visible, "no button after the mate")
        app.click("prevButton")
        tryCompare(ChessConnector, "ShownPly", 3, 2000)
        compare(ChessConnector.PlayFromHere.ply, 3)
        tryVerify(() => app.find("playFromHereButton").visible, 2000, "the button again")
    }

    // A kept game with a move that can't be played: Continue and Open refuse it and say so, and the
    // record stays as it was (Continue used to go on from the moves before it and save only those).
    function test_08_damagedGame() {
        const id = TestRun.addDamagedGame()
        verify(!ChessConnector.continueGame(id), "not continued")
        app.waitMessage("Couldn't continue the game: one of its moves can't be played.")
        compare(GameLibrary.game(id).plyCount, 4, "all its moves still there")
        verify(!ChessConnector.openGame(id), "not opened")
        app.waitMessage("Couldn't open the game: one of its moves can't be played.")
        compare(GameLibrary.game(id).plyCount, 4)
        verify(GameLibrary.remove(id))
    }

    // The player (White) mates the robot: 1. e4 f6 2. d4 g5 3. Qh5#.
    function mateTheRobot() {
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
    }

    // The rating vs the robot (step 5): a won game is rated (Home's header, the card's change); none of
    // the games before was (two players, unfinished, a fork with a takeback). A takeback after the mate
    // resumes the game and gives its change back; mated again, it isn't rated.
    function test_09_rating() {
        if (app.pageName() !== "home")
            app.leaveToHome()   // test 7 left a replay open
        compare(GameLibrary.ratedGames, 0, "nothing rated yet")
        verify(!app.item("homeRating").visible)
        mateTheRobot()
        tryCompare(GameLibrary, "ratedGames", 1, 3000)
        const change = GameLibrary.games[0].ratingChange
        compare(change, 27, "a win from 1200 against level 3 (1320)")
        compare(GameLibrary.rating, 1227)
        app.leaveToHome()
        compare(app.item("homeRating").text, "Rating 1227?")
        app.laidOut()
        keyClick(Qt.Key_O, Qt.ControlModifier)
        app.waitForPage("library", 5000)
        tryCompare(app.item("gameCard_0"), "waitingText", "You won · +27")
        app.clickIn("library", "backButton")
        app.waitForPage("home", 5000)

        mateTheRobot()
        tryCompare(GameLibrary, "rating", 1227 + 25, 3000)
        app.waitBoardIdle()
        keyClick(Qt.Key_Z, Qt.ControlModifier)   // Qh5# back: the game goes on
        tryCompare(ChessConnector, "GameResult", "", 3000)
        tryCompare(GameLibrary, "rating", 1227, 3000, "its change given back")
        compare(GameLibrary.ratedGames, 1)
        app.waitBoardIdle()
        app.clickMove("d1", "h5")
        tryVerify(() => ChessConnector.GameResult !== "", 3000, "mated again")
        compare(GameLibrary.rating, 1227, "not rated after a takeback")
        app.leaveToHome()
    }

    // LI-14: a kept Lichess game. The Lichess filter, its card's words and its menu (the link copied; the
    // game and its analysis on lichess.org). Open replays it with the Lichess players, as it was played:
    // names, title, ratings; More has lichess.org and Analyse; no Play from here.
    function test_10_keptLichessGame() {
        compare(TestRun.addLichessGame(), "lichess-kept0001")
        app.laidOut()
        keyClick(Qt.Key_O, Qt.ControlModifier)
        app.waitForPage("library", 5000)
        app.click("gamesFilter_3")   // Lichess
        tryVerify(() => app.page()._games.length === 1, 2000, "the Lichess game only")
        const card = app.item("gameCard_0")
        compare(card.title, "GM Magnus (2850)")
        compare(card.details, "Rapid 10+5 · Rated · you play White · 20 Nov 2026 · 3 moves")
        compare(card.waitingText, "You won", "not rated here: no change")
        compare(card.actionText, "Open")
        verify(card.hasMenu)
        app.laidOut()   // the filtered card placed in the grid (a raw click doesn't wait for it)
        tryVerify(() => card.width > 0, 2000, "the card laid out")
        mouseClick(card, card.width / 2, card.height / 2, Qt.RightButton)
        const copy = findChild(card, "gameMenuCopyLink")
        verify(copy !== null, "its Copy link")
        tryVerify(() => copy.menu.opened, 3000, "the card's menu")
        verify(findChild(card, "gameMenuOpen").visible)
        verify(findChild(card, "gameMenuAnalysis").visible)
        mouseClick(copy)
        tryVerify(() => TestRun.clipboardText() === "https://lichess.org/kept0001", 3000, "the link: " + TestRun.clipboardText())
        tryVerify(() => !copy.menu.visible, 3000, "the menu closed")

        app.click("gameCard_0")   // Open
        app.waitForPage("game", 10000)
        compare(ChessConnector.GameMode, "replay")
        compare(ChessConnector.GameTitle, "Lichess · TesterChess vs Magnus · 20 Nov 2026")
        compare(ChessConnector.LichessGameUrl, "https://lichess.org/kept0001")
        const opponent = app.item("opponentCard"), player = app.item("playerCard")
        compare(findChild(opponent, "playerName").text, "Magnus")
        compare(findChild(opponent, "playerTitle").text, "GM")
        compare(findChild(opponent, "playerDetail").text, "2850")
        compare(findChild(player, "playerName").text, "TesterChess", "the player's Lichess name")
        compare(findChild(player, "playerDetail").text, "Tester · 1650")
        verify(ChessConnector.CanAnalyse, "a finished game: Analyse")
        app.click("review_next")   // the replay opens at the start: 1. e4
        tryCompare(ChessConnector, "ShownPly", 1, 2000)
        compare(ChessConnector.PlayFromHere.gameId, undefined, "not Played from here")
        verify(!ChessConnector.continueGame("lichess-kept0001"), "never continued")
        app.leaveToHome()
        compare(ChessConnector.LichessGameUrl, "", "gone with the replay")
    }
}
