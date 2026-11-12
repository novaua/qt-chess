import QtQuick
import QtTest
import ChessPlusPlus
import "qrc:/qml"

// The app in a UI test (BL-05, docs/UiTestSuitePlan.md): makes the app's window (ChessGame.qml, the
// same as ChessGui.exe shows) and plays it as a user does: real mouse events on its items, through
// the same handlers. Each tst_*.qml has one, with `test` set to its TestCase.
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
        Theme.forceReduceMotion = true
        test.tryVerify(() => pageName() !== "", 5000, "the first page")
    }
    function stop() {   // cleanupTestCase
        if (window)
            window.destroy()
        window = null
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
        test.waitForRendering(page())
    }

    // ── Input ────────────────────────────────────────────────────────
    // A click on an item that is visible and enabled, as a user can.
    function click(name) {
        const target = item(name)
        test.tryVerify(() => target.visible && target.enabled, 3000, name + " visible and enabled")
        test.mouseClick(target)
    }

    // A dialog gone, its fade done: until then its modal overlay takes the clicks.
    function waitClosed(dialog) {
        test.tryVerify(() => !dialog.visible, 3000, (dialog.objectName || "the dialog") + " closed")
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
        const p = squarePoint(name)
        test.mouseClick(grid(), p.x, p.y)
    }
    // Press on one square, move over the board in steps (past the drag threshold), release on the other.
    function dragPiece(from, to) {
        const a = squarePoint(from), b = squarePoint(to)
        const g = grid()
        test.mousePress(g, a.x, a.y)
        const steps = 8
        for (let i = 1; i <= steps; ++i)
            test.mouseMove(g, a.x + (b.x - a.x) * i / steps, a.y + (b.y - a.y) * i / steps, 16, Qt.LeftButton)
        test.mouseRelease(g, b.x, b.y)
    }
    // The piece shown on a square ("" for none), found by where its item is drawn: "P", "q", …
    function pieceAt(name) {
        const pieces = item("chessPiecesRptr")
        const g = grid(), board = page().board
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
    }

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
