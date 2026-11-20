import QtQuick
import QtTest
import ChessPlusPlus
import ChessGuiTests

// ui-test-engine: none
// (That line is read by scripts\Run-UiTests.ps1: run this file with it. Run alone, TestSetup gives it FakeUci.)
// Without a chess engine (BL-25): the app looks for it at start, not only when Settings opens, so a finished
// game's Analyse is off from the first game page on: the result card has none, the moves panel's button and
// More → Analyse are disabled; also for a kept game opened from the Games page. Settings says so.
TestCase {
    id: test
    name: "NoEngine"
    when: windowShown

    AppDriver { id: app; test: test }

    function initTestCase() {
        app.start()
        app.createFirstProfile("Tester")
    }
    function cleanup() { app.afterTest() }
    function cleanupTestCase() { app.stop() }

    function analyseOffEverywhere() {
        tryVerify(() => app.find("analyseButton") !== null, 5000, "the moves panel made")
        const button = app.item("analyseButton")
        tryVerify(() => button.visible, 3000, "Analyse game in the moves panel")
        verify(!button.enabled, "the moves panel's Analyse game disabled")
        app.click("moreButton")
        const item = app.item("analyseMenuItem")
        tryVerify(() => item.menu.opened, 3000)
        verify(!item.enabled, "More → Analyse disabled")
        item.menu.close()
        tryVerify(() => !item.menu.visible, 3000)
    }

    // Fool's mate, two players, Settings never opened: the engine was looked for at start.
    function test_1_finishedGame() {
        tryVerify(() => AppInfo.engineState === "missing", 10000, "looked for at start (" + AppInfo.engineState + ")")
        verify(!AppInfo.engineFound)
        app.startTwoPlayerGame()
        for (const [from, to] of [["f2", "f3"], ["e7", "e5"], ["g2", "g4"], ["d8", "h4"]])
            app.clickMove(from, to)
        tryVerify(() => app.find("resultCard")?.visible ?? false, 5000, "the result card")
        verify(ChessConnector.CanAnalyse, "a kept game that is over: analysable, with an engine")
        verify(!app.item("analyseResultButton").visible, "no Analyse on the result card")
        analyseOffEverywhere()
        app.leaveToHome()
    }

    // The same game opened from the Games page (a replay).
    function test_2_keptGameOpened() {
        app.click("gamesCard")
        app.waitForPage("library", 5000)
        tryVerify(() => app.find("gameCard_0") !== null, 3000)
        app.clickIn("gameCard_0", "actionButton")
        app.waitForPage("game", 10000)
        compare(ChessConnector.GameMode, "replay")
        analyseOffEverywhere()
        app.leaveToHome()
    }

    // Settings → Computer says the engine isn't there.
    function test_3_settings() {
        app.click("settingsButton")
        app.waitForPage("settings", 5000)
        app.scrollIntoView("analysisDepthBox")
        verify(!AppInfo.engineFound)
        compare(AppInfo.engineState, "missing")
        app.clickIn("settings", "backButton")
        app.waitForPage("home", 5000)
    }

    // The engine there after all (a first start slower than the look at start, e.g. an antivirus scan):
    // the next game page looks again, and Analyse is on, without a restart (review of #204).
    function test_4_foundLater() {
        TestRun.useFakeEngine()
        compare(AppInfo.engineState, "missing", "not looked again on its own")
        app.click("gamesCard")
        app.waitForPage("library", 5000)
        tryVerify(() => app.find("gameCard_0") !== null, 3000)
        app.clickIn("gameCard_0", "actionButton")
        app.waitForPage("game", 10000)
        tryVerify(() => AppInfo.engineFound, 10000, "found by the game page's look (" + AppInfo.engineState + ")")
        tryVerify(() => app.find("analyseButton")?.enabled ?? false, 5000, "Analyse game on")
        app.leaveToHome()
    }
}
