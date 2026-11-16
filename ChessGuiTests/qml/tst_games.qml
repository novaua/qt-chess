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
    function test_1_threeGames() {
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
    function test_2_filters() {
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

        app.click("gamesFilter_3")   // Unfinished
        tryVerify(() => shown().length === 2 && shown().every(g => !g.finished), 2000)
        compare(app.item("gameCard_0").actionText, "Continue")
        verify(!app.item("gamesFilterEmpty").visible)

        app.click("gamesFilter_0")   // All
        tryVerify(() => app.find("gameCard_2") !== null, 2000, "all three again")
        verify(app.item("gameCard_0").exportable, "Export PGN on the cards")
    }

    // A kept game's PGN: the tags for a two-player game and a robot game, and the file Export PGN
    // writes (the card's button opens the file dialog, a native one: not in a test).
    function test_3_exportPgn() {
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

    // Delete on Fool's mate's card: the confirm, then the game is gone, the others stay.
    function test_4_delete() {
        const id = foolsMate().id
        const index = GameLibrary.games.findIndex(g => g.id === id)
        app.clickIn("gameCard_" + index, "deleteButton")
        app.clickIn("deleteGameConfirm", "acceptButton")
        app.waitClosed(app.item("deleteGameConfirm"))
        tryCompare(GameLibrary, "count", 2, 2000)
        verify(!GameLibrary.games.some(g => g.id === id))
        compare(GameLibrary.unfinishedCount, 2)
        app.clickIn("library", "backButton")
        app.waitForPage("home", 5000)
    }

    // Settings → Data "Games kept" with 52 finished games: 50 asks (Cancel keeps all of them; Delete
    // removes the 2 oldest, the unfinished ones stay); 200 doesn't ask.
    function test_5_gamesKept() {
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
        app.clickIn("gamesKeptConfirm", "cancelButton")
        app.waitClosed(confirm)
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
    // to 1... e5; More → Play from here vs computer: the dialog says where, the player takes White (to
    // move), the board has the two moves; Nf3 and the robot's Nc6 (FakeUci follows the same game). The
    // fork is a record of its own (forkOfId, forkPly 2); a takeback stops at the moves it began with.
    function test_6_playFromHereVsRobot() {
        app.startTwoPlayerGame()
        for (const [from, to] of [["e2", "e4"], ["e7", "e5"], ["g1", "f3"], ["b8", "c6"]])
            app.clickMove(from, to)
        tryCompare(ChessConnector, "MoveCount", 4, 3000)
        app.waitBoardIdle()
        // Copied: a list property's element read again later is the list's element then.
        const sourceId = GameLibrary.games[0].id
        compare(GameLibrary.games[0].plyCount, 4, "the two-player game kept")

        app.click("review_prev")
        app.click("review_prev")
        tryCompare(ChessConnector, "ShownPly", 2, 2000, "after 1... e5")
        TestRun.setEngine("e2e4 e7e5 g1f3 b8c6", 0)
        app.chooseFromMenu("moreButton", "playFromHereRobotItem")
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
    function test_7_playFromHereTwoPlayers() {
        const keptId = GameLibrary.games.find(g => g.finished).id
        const count = GameLibrary.count
        verify(ChessConnector.openGame(keptId))
        app.waitForPage("game", 10000)
        app.click("nextButton")
        app.click("nextButton")
        tryCompare(ChessConnector, "ShownPly", 2, 2000)
        app.chooseFromMenu("moreButton", "playFromHereTwoPlayersItem")
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
        compare(Object.keys(ChessConnector.playFromHere()).length, 0, "after the mate: none")
        app.click("prevButton")
        tryCompare(ChessConnector, "ShownPly", 3, 2000)
        compare(ChessConnector.playFromHere().ply, 3)
    }
}
