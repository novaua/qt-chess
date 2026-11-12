import QtQuick
import QtTest
import ChessPlusPlus
import ChessGuiTests
import "qrc:/qml"   // AppSettings

// Scenario 9 (docs/UiTestSuitePlan.md §9): what the app keeps when it is closed and started again.
// This file runs twice on one data folder (Run-UiTests.ps1 reads the line below): run 1 turns
// "Highlight last move" off and leaves a game vs the robot on the board; run 2 is the next start.
// ui-test-runs: 2
TestCase {
    id: test
    name: "Restart"
    when: windowShown

    readonly property string line: "e2e4 e7e5 g1f3 b8c6"

    AppDriver { id: app; test: test }

    function initTestCase() { app.start() }
    function cleanup() { app.afterTest() }
    function cleanupTestCase() { app.stop() }

    function onlyInRun(run) {
        if (TestRun.run !== run)
            skip("run " + run + " only (this is run " + TestRun.run + ")")
    }

    // ── Run 1 ──
    function test_1_run1_turnOffLastMove() {
        onlyInRun(1)
        app.createFirstProfile("Tester")
        app.click("settingsButton")
        app.waitForPage("settings")
        const toggle = app.item("highlightLastMoveSwitch")
        compare(toggle.checked, true, "on by default")
        app.click("highlightLastMoveSwitch")
        tryCompare(toggle, "checked", false, 2000)
        compare(AppSettings.highlightLastMove, false)
        app.click("backButton")
        app.waitForPage("home")
    }

    // 1. e4 e5 vs the robot; the window then closes with the game on (the app autosaves it).
    function test_2_run1_leaveAGameOn() {
        onlyInRun(1)
        app.startComputerGame(line, 0)
        app.waitBoardIdle()
        app.dragPiece("e2", "e4")
        tryCompare(ChessConnector, "MoveCount", 2, 5000, "e4 and its answer")
        app.waitBoardIdle()
        compare(app.pieceAt("e5"), "p")
    }

    // ── Run 2: the app started again ──
    // The profile is there (straight to Home), the setting still off.
    function test_3_run2_profileAndSetting() {
        onlyInRun(2)
        app.waitForPage("home")
        compare(UserManager.activeUserName, "Tester")
        compare(AppSettings.highlightLastMove, false, "the setting kept")
        app.click("settingsButton")
        app.waitForPage("settings")
        compare(app.item("highlightLastMoveSwitch").checked, false, "and shown off")
        app.click("backButton")
        app.waitForPage("home")
    }

    // Continue: the game as it was; the last move isn't marked (the setting).
    function test_4_run2_continueTheGame() {
        onlyInRun(2)
        compare(ChessConnector.CanContinue, true, "the game kept")
        TestRun.setEngine(line, 0)
        app.click("continueCard")
        app.waitForPage("game", 10000)
        tryCompare(ChessConnector, "MoveCount", 2, 5000)
        app.waitBoardIdle()
        compare(app.pieceAt("e4"), "P")
        compare(app.pieceAt("e5"), "p")
        compare(ChessConnector.MovableColor, 1, "the player's move")
        compare(ChessConnector.LastMoveTo, app.square("e5"), "the last move is known…")
        verify(!app.lastMoveMarked(), "…but not marked on the board")
    }
}
