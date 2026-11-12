import QtQuick
import QtTest
import ChessPlusPlus

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
}
