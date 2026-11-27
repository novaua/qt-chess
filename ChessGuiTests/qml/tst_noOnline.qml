import QtQuick
import QtTest
import ChessPlusPlus
import ChessGuiTests

// ui-test-online: none
// (That line is read by scripts\Run-UiTests.ps1: run this file without the online servers' plugins. Run
// alone, TestSetup loads them as for every file.)
// LI-18: without a server's plugin (online\LichessOnline.dll left out) the app plays locally and hides
// online play; a profile's Lichess account and its kept Lichess games stay, and a kept one still replays.
TestCase {
    id: test
    name: "NoOnline"
    when: windowShown

    AppDriver { id: app; test: test }

    function initTestCase() { app.start() }
    function cleanup() { app.afterTest() }
    function cleanupTestCase() { app.stop() }

    function test_1_welcomeWithoutLichess() {
        app.waitForPage("welcome")
        verify(!Online.available, "no plugin, no server")
        compare(Online.current, null)
        compare(Online.providers.length, 0)
        verify(!app.item("continueWithLichess").visible, "no Lichess sign-in")
        verify(app.item("createLocalProfile").visible)
    }

    function test_2_homeWithoutOnline() {
        app.createFirstProfile("Tester")
        verify(app.item("computerCard").visible)
        verify(app.item("twoPlayersCard").visible)
        verify(!app.item("onlineCard").visible, "no Play online")
    }

    // The profile's Lichess account (a token in users.json) isn't lost: only not used, and not shown.
    function test_3_accountKept() {
        TestRun.linkLichess("testerchess")
        tryVerify(() => UserManager.lichessConnected, 3000, "the profile is linked")
        verify(!app.item("onlineCard").visible, "still no Play online")
        app.chooseFromMenu("accountButton", "profileItem")
        app.waitForPage("profile")
        verify(!app.item("lichessSection").visible, "no Lichess section without its plugin")
        keyClick(Qt.Key_Escape)   // Back
        app.waitForPage("home")
    }

    function test_4_localGame() {
        app.startTwoPlayerGame()
        app.clickMove("e2", "e4")
        app.waitPiece("e4", "P")
        app.leaveToHome()
    }

    // A kept Lichess game is the profile's: listed, and replayed with its players and its link.
    function test_5_keptLichessGameReplays() {
        compare(TestRun.addLichessGame(), "lichess-kept0001")
        app.laidOut()
        keyClick(Qt.Key_O, Qt.ControlModifier)
        app.waitForPage("library", 5000)
        app.click("gamesFilter_3")   // Lichess
        tryVerify(() => app.page()._games.length === 1, 2000, "the Lichess game")
        app.laidOut()
        app.click("gameCard_0")   // Open
        app.waitForPage("game", 10000)
        compare(ChessConnector.GameMode, "replay")
        compare(ChessConnector.OnlineGameUrl, "https://lichess.org/kept0001", "its link, from the record")
        compare(findChild(app.item("opponentCard"), "playerName").text, "Magnus")
        tryVerify(() => app.find("review_next") !== null, 5000, "the moves panel (made after the first frame)")
        app.click("review_next")   // the replay opens at the start: 1. e4
        tryCompare(ChessConnector, "ShownPly", 1, 2000)
        app.waitPiece("e4", "P")
        app.leaveToHome()
    }
}
