import QtQuick
import QtTest
import ChessPlusPlus
import ChessGuiTests

// Scenario 10 (docs/UiTestSuitePlan.md §9): two profiles. A second one added from the picker, a
// switch during a game (it asks first), and each profile's own game to continue.
TestCase {
    id: test
    name: "Profiles"
    when: windowShown

    readonly property string line: "e2e4 e7e5 g1f3 b8c6"

    AppDriver { id: app; test: test }

    function initTestCase() {
        app.start()
        app.createFirstProfile("Tester")
    }
    function cleanup() { app.afterTest() }
    function cleanupTestCase() { app.stop() }

    // Switch user with one profile: the picker with its card and Add profile; Alice added, then on Home.
    function test_1_addASecondProfile() {
        app.switchUserFromHome()
        verify(app.find("profileCard_Tester") !== null, "Tester's card")
        app.addProfile("Alice")
        compare(UserManager.userCount, 2)
        compare(UserManager.activeUserName, "Alice")
        compare(GameLibrary.unfinishedCount, 0, "Alice has no game yet")
    }

    // Alice plays 1. e4 e5 vs the robot, then Switch user from the game's menu: it asks first
    // (the game is kept); then the picker with both profiles.
    function test_2_switchDuringAGame() {
        app.startComputerGame(line, 0)
        app.waitBoardIdle()
        app.dragPiece("e2", "e4")
        tryCompare(ChessConnector, "MoveCount", 2, 5000, "e4 and its answer")
        app.waitBoardIdle()

        app.chooseFromMenu("moreButton", "switchUserItem")
        const confirm = app.item("switchUserConfirm")
        tryVerify(() => confirm.opened, 3000, "Switch user? asked")
        compare(ChessConnector.InGame, true, "nothing left before the answer")
        app.clickIn("switchUserConfirm", "acceptButton")
        app.waitForPage("login")
        app.waitClosed(confirm)
        verify(app.find("profileCard_Tester") !== null)
        verify(app.find("profileCard_Alice") !== null)
    }

    // Tester's Home: no game of Alice's to continue.
    function test_3_theOtherProfile() {
        app.pickProfile("Tester")
        compare(UserManager.activeUserName, "Tester")
        compare(GameLibrary.unfinishedCount, 0, "Alice's game isn't Tester's")
        verify(!app.item("continueCard").visible)
    }

    // Back to Alice: her game is there to continue, as she left it.
    function test_4_backToAlicesGame() {
        app.switchUserFromHome()
        app.pickProfile("Alice")
        compare(UserManager.activeUserName, "Alice")
        tryCompare(GameLibrary, "unfinishedCount", 1, 3000, "her game kept")
        TestRun.setEngine(line, 0)
        app.click("continueCard")
        app.waitForPage("game", 10000)
        tryCompare(ChessConnector, "MoveCount", 2, 5000)
        app.waitBoardIdle()
        compare(app.pieceAt("e4"), "P")
        compare(app.pieceAt("e5"), "p")
    }

    // Alice deletes her own profile from the Profile page (BL-35 step 5: ProfileFiles): it asks first,
    // naming her; then the Profile page goes and the picker is left with Tester.
    function test_5_deleteTheActiveProfile() {
        app.leaveToHome()
        app.chooseFromMenu("accountButton", "profileItem")
        app.waitForPage("profile")
        app.scrollIntoView("deleteProfileButton")
        app.click("deleteProfileButton")
        const confirm = app.item("deleteProfileConfirm")
        tryVerify(() => confirm.opened, 3000, "the delete confirm")
        compare(confirm.title, "Delete Alice's profile?")
        app.clickIn("deleteProfileConfirm", "acceptButton")
        app.waitClosed(confirm)
        tryCompare(UserManager, "userCount", 1, 3000, "Alice's profile gone")
        compare(UserManager.users[0].name, "Tester")
        app.waitForPage("login")
    }
}
