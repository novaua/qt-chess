import QtQuick
import QtTest
import ChessPlusPlus
import ChessGuiTests
import "qrc:/qml"

// The app in a UI test (BL-05, docs/UiTestSuitePlan.md): makes the app's window (ChessGame.qml, the
// same as ChessGui.exe shows) and plays it as a user does: real mouse events on its items, through
// the same handlers. Each tst_*.qml has one, with `test` set to its TestCase, and calls
// start() / stop() in initTestCase / cleanupTestCase and afterTest() in cleanup.
QtObject {
    id: driver

    required property TestCase test
    property var window: null

    // The window at a fixed size, Reduce animations on (as in the screenshot scenes), its first page up.
    // For the whole test file (initTestCase): its test functions go on from where the last one left.
    function start(width, height) {
        const component = Qt.createComponent("qrc:/qml/ChessGame.qml")
        test.verify(component.status === Component.Ready, "ChessGame.qml: " + component.errorString())
        window = component.createObject(test)
        test.verify(window !== null, "the window")
        window.width = width ?? 1400   // wide: the moves beside the board, the player cards in a column
        window.height = height ?? 900
        if (TestRun.watched)
            fitOnScreen()
        Theme.forceReduceMotion = true
        test.tryVerify(() => pageName() !== "", 5000, "the first page")
    }
    // Watched: within the screen the window is on (smaller if it must be), centred, clear of the taskbar.
    function fitOnScreen() {
        const s = window.screen
        const taskbar = 48, frame = 32, margin = 24   // the title bar is above the window's y
        window.width = Math.min(window.width, s.width - 2 * margin)
        window.height = Math.min(window.height, s.height - taskbar - frame - 2 * margin)
        window.x = s.virtualX + Math.round((s.width - window.width) / 2)
        window.y = s.virtualY + frame + Math.max(margin, Math.round((s.height - taskbar - frame - window.height) / 2))
    }

    function stop() {   // cleanupTestCase
        if (window)
            window.destroy()
        window = null
    }

    // After each test function (the TestCase's cleanup): a failed one leaves a picture of the window,
    // <TestRun.resultsDir>/<TestCase>-<function>.png; watched (-Visible), the window then waits
    // with a note on it until it is closed.
    function afterTest() {
        if (!window || !test.qtest_results.failed)
            return
        const what = test.name + "-" + test.qtest_results.functionName
        if (TestRun.resultsDir !== "") {
            const path = TestRun.resultsDir + "/" + what + ".png"
            test.grabImage(window.contentItem.parent ?? window.contentItem).save(path)   // Run-UiTests.ps1 lists it
        }
        if (!TestRun.watched || !window.visible)
            return
        const note = failureNote.createObject(window.contentItem.parent ?? window.contentItem,
                                              { text: "UI test failed: " + test.name + "::" + test.qtest_results.functionName
                                                      + "\nClose this window to go on." })
        while (window && window.visible)
            test.wait(200)
        note?.destroy()
    }
    readonly property Component failureNote: Component {
        Rectangle {
            property alias text: label.text
            anchors.top: parent.top; anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 8
            width: label.implicitWidth + 32; height: label.implicitHeight + 16
            z: 1000000
            radius: 6
            color: "#c42b1c"
            Text { id: label; anchors.centerIn: parent; color: "white"; font.pixelSize: 15; font.bold: true }
        }
    }

    // A pause after an action, when a run is watched (-Visible): TestRun.stepDelay ms.
    function pace() {
        if (TestRun.stepDelay > 0)
            test.wait(TestRun.stepDelay)
    }

    // ── Finding things ───────────────────────────────────────────────
    // Visual children first (a ListView's rows have no QObject parent to find them by), from the
    // window's root item, so the popups' layer too; then the window's QObject children.
    function find(name) {
        const root = window.contentItem.parent ?? window.contentItem
        return test.findChild(root, name) ?? test.findChild(window, name)
    }
    function item(name) {
        const found = find(name)
        test.verify(found !== null, "no item named " + name)
        return found
    }
    function page() { return item("pages").currentItem }
    function pageName() { return window && find("pages")?.currentItem ? page().objectName : "" }
    function waitForPage(name, timeout) {
        test.tryVerify(() => pageName() === name, timeout ?? 5000, "page " + name + " (on " + pageName() + ")")
        // Not waitForRendering(): on a page that is already drawn and doesn't change it waits its whole
        // timeout (5 s) for a frame that never comes.
    }

    // ── Input ────────────────────────────────────────────────────────
    // A click on an item that is visible and enabled, as a user can.
    function click(name) {
        const target = item(name)
        test.tryVerify(() => target.visible && target.enabled, 3000, name + " visible and enabled")
        test.mouseClick(target)
        pace()
    }

    // A dialog gone, its fade done: until then its modal overlay takes the clicks.
    function waitClosed(dialog) {
        test.tryVerify(() => !dialog.visible, 3000, (dialog.objectName || "the dialog") + " closed")
    }

    // A short message over the command bar ("Check", "Game saved"), shown now or within `timeout`.
    function waitMessage(text, timeout) {
        const bar = item("shortMessage")
        test.tryVerify(() => bar.showing && bar.text === text, timeout ?? 3000,
                       "the message \"" + text + "\" (shown: \"" + (bar.showing ? bar.text : "") + "\")")
    }

    // From a game (two players, the robot): the Home button, then Home.
    function leaveToHome() {
        click("homeButton")
        waitForPage("home", 5000)
    }

    // ── The board (a1 = 0 … h8 = 63) ─────────────────────────────────
    function square(name) {   // "e2" → 12
        return (name.charCodeAt(1) - 49) * 8 + (name.charCodeAt(0) - 97)
    }
    function grid() { return item("chessPiecesGrid") }   // the pieces' layer: the squares' coordinates
    // A square's middle in the grid's coordinates; the board turns for Black, and the mouse
    // functions map the point through that rotation.
    function squarePoint(name) {
        const board = page().board
        const sq = square(name)
        return Qt.point(board.squareX(sq) + board.squareSize / 2, board.squareY(sq) + board.squareSize / 2)
    }
    function clickSquare(name) {
        settle()
        const p = squarePoint(name)
        test.mouseClick(grid(), p.x, p.y)
        pace()
    }
    // Press on one square, move over the board in steps (past the drag threshold), release on the other.
    function dragPiece(from, to) {
        settle()
        const a = squarePoint(from), b = squarePoint(to)
        const g = grid()
        test.mousePress(g, a.x, a.y)
        const steps = 8
        const stepMs = Math.max(16, TestRun.stepDelay / steps)   // watched: a drag you can follow
        for (let i = 1; i <= steps; ++i)
            test.mouseMove(g, a.x + (b.x - a.x) * i / steps, a.y + (b.y - a.y) * i / steps, stepMs, Qt.LeftButton)
        test.mouseRelease(g, b.x, b.y)
        pace()
    }
    // The piece shown on a square ("" for none), found by where its item is drawn: White's in capitals
    // ("P", "N", …), Black's in small letters ("p", "q", …).
    // "?" while the board's layers aren't made yet (LayoutGate: the page's first frame), so a wait
    // for "" (an empty square) can't pass before there is a board.
    function pieceAt(name) {
        const pieces = find("chessPiecesRptr"), g = find("chessPiecesGrid")
        if (!pieces || !g)
            return "?"
        const board = page().board
        for (let i = 0; i < pieces.count; ++i) {
            const p = pieces.itemAt(i)
            if (!p || !p.visible || p.taken)
                continue
            const c = p.mapToItem(g, p.width / 2, p.height / 2)
            if (board.squareAt(c.x, c.y) === square(name))
                return p.piece
        }
        return ""
    }
    // Wait until `letter` ("" for none) is shown on the square: in review, where the board is never
    // idle (the gate holds outside moves meanwhile), instead of waitBoardIdle.
    function waitPiece(name, letter, timeout) {
        test.tryVerify(() => pieceAt(name) === letter, timeout ?? 3000,
                       (letter === "" ? "nothing" : letter) + " on " + name + " (shown: \"" + pieceAt(name) + "\")")
    }

    // A move by two clicks: the piece, then its square.
    function clickMove(from, to) {
        waitBoardIdle()
        clickSquare(from)
        test.tryCompare(ChessConnector, "SelectedSquare", square(from), 2000)
        clickSquare(to)
    }

    // The board has nothing left to show: no slide, no held move, the page settled.
    function waitBoardIdle(timeout) {
        test.tryVerify(() => find("chessPiecesGrid") !== null && ChessConnector.isBoardIdle(),
                       timeout ?? 5000, "the board idle")
        settle()
    }
    // Let the queued signals of the last change reach QML: ChessConnector sends some once per move,
    // queued (MovableColor, BL-11). A user's next press always comes after them; a test's could come
    // before, while the pieces still can't be dragged (C++ already says they can).
    function settle() { test.wait(0) }

    // ── Flows ────────────────────────────────────────────────────────
    // First run: Welcome → Create local profile → the dialog → Home.
    function createFirstProfile(name) {
        waitForPage("welcome")
        click("createLocalProfile")
        const dialog = item("createUserDialog")
        test.tryVerify(() => dialog.opened, 3000, "the Create profile dialog open")
        item("nameField").text = name   // keys go to the test's own window, not the app's
        click("createProfileButton")
        waitForPage("home")
        waitClosed(dialog)
    }

    // From Home: Play vs computer → the dialog (the last level and side, no clock) → the game page.
    // The robot is FakeUci: `moves`, a game in UCI, is the line it follows (its own half of it; a
    // move not legal there: its first legal move), `thinkMs` how long it thinks before each answer.
    function startComputerGame(moves, thinkMs) {
        TestRun.setEngine(moves ?? "", thinkMs ?? 0)
        waitForPage("home")
        click("computerCard")
        const dialog = item("computerDialog")
        test.tryVerify(() => dialog.opened, 3000, "the vs computer dialog open")
        click("startComputerGameButton")
        waitForPage("game", 10000)
        waitClosed(dialog)
    }

    // From Home: Two players → the dialog (White, Guest, no clock: the defaults) → the game page.
    function startTwoPlayerGame() {
        waitForPage("home")
        click("twoPlayersCard")
        const dialog = item("twoPlayerDialog")
        test.tryVerify(() => dialog.opened, 3000, "the Two players dialog open")
        click("startGameButton")
        waitForPage("game", 10000)   // "Starting the game…" first, then the page
        waitClosed(dialog)
        waitBoardIdle()
    }
}
