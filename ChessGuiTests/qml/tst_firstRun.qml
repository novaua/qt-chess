import QtQuick
import QtTest
import ChessPlusPlus
import ChessGuiTests

// Scenario 1 (docs/UiTestSuitePlan.md §9): the app's first run, on an empty profile folder.
TestCase {
    id: test
    name: "FirstRun"
    when: windowShown

    AppDriver { id: app; test: test }

    function initTestCase() { app.start() }
    function cleanup() { app.afterTest() }
    function cleanupTestCase() { app.stop() }

    function test_1_welcome() {
        app.waitForPage("welcome")
        compare(UserManager.hasUsers, false)
        verify(app.item("createLocalProfile").visible)
    }

    function test_2_createProfile() {
        app.createFirstProfile("Tester")
        compare(UserManager.isLoggedIn, true)
        compare(UserManager.userCount, 1)
        compare(UserManager.activeUserName, "Tester")
        verify(app.item("twoPlayersCard").visible, "Home's cards")
    }

    // BL-39: the tests' compiled QML goes to a cache folder of their own (ChessGuiTests-<hash of its path>),
    // not the one the app and every other build would share: a test run never rewrites a running app's.
    function test_3_ownQmlCache() {
        const path = TestRun.qmlCachePath
        verify(/[\\/]qml[\\/]ChessGuiTests-[0-9a-f]{12}$/.test(path), "the tests' own folder: " + path)
        const files = TestRun.filesIn(path)
        verify(files.some(f => f.endsWith(".qmlc")), "the window's QML compiled there: " + files)
    }
}
