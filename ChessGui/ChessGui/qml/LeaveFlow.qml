import QtQuick
import ChessPlusPlus

// Leaving the game on the board (BL-36): Home (or Esc), Switch user, Resign and closing the window, with their
// confirms. One instance in the window, `leaveFlow`, which forwards the pages' requests and its onClosing.
// - A running Lichess game asks first (LeaveOnlineDialog: Leave or Quit, Resign, Stay); a local game is kept as it
//   is (BL-19), so leaving it never asks. Switch user asks during any running game; Resign as Settings say.
// - Each answer is one ChessConnector call (a rematch challenge still waiting goes with the game: endGame()).
// - Resign in the warning leaves once Lichess has ended the game (ChessConnector.resignAndLeave()); after a
//   Quit the window closes then (quitRequested).
Item {
    id: flow
    visible: false

    // In, from the window.
    property bool screenshotMode: false   // the scenes' window closes without asking

    signal quitRequested()   // close the window: it doesn't ask again

    readonly property bool _inGame:   ChessConnector.InGame
    readonly property string _mode:   ChessConnector.GameMode
    readonly property bool _gameOver: ChessConnector.GameResult !== ""
    // A Lichess game that goes on without us: leaving it asks first (leaveOnlineDialog).
    readonly property bool _onlineRunning: _inGame && _mode === "online" && !_gameOver

    // One of its dialogs is open: the window's shortcuts wait.
    readonly property bool dialogOpen: resignConfirm.visible || switchUserConfirm.visible || leaveOnlineDialog.visible
    readonly property alias onlineDialog: leaveOnlineDialog   // for ScreenshotScene

    // Home button / Escape.
    function leave() {
        if (_resigning) {                 // Home again while the resignation is on its way: now
            _quitAfterLeave = false
            ChessConnector.endGame()
        } else if (_onlineRunning) {
            leaveOnlineDialog.quitting = false
            leaveOnlineDialog.open()
        } else {
            ChessConnector.endGame()
        }
    }

    // A running game asks first (it is kept, or goes on at Lichess); a finished one or a replay doesn't.
    function switchUser() {
        if (_inGame && !_gameOver && _mode !== "replay")
            switchUserConfirm.open()
        else
            ChessConnector.switchUser()
    }

    function resign() {
        if (AppSettings.confirmResign) resignConfirm.open()
        else                           ChessConnector.resignGame()
    }

    // The window's onClosing. During a running Lichess game it asks the same as Home (Leave quits). A local
    // game is kept as it is (ChessConnector's destructor): nothing to ask.
    function windowClosing(event) {
        if (_quitConfirmed || screenshotMode)
            return
        if (!(_onlineRunning || _resigning))
            return
        event.accepted = false
        if (_resigning) {                 // quit when the resignation is through
            _quitAfterLeave = true
            return
        }
        resignConfirm.close()             // the warning replaces a confirm about this game
        switchUserConfirm.close()
        leaveOnlineDialog.quitting = true
        leaveOnlineDialog.open()
    }

    // The window's ChessConnector.gameInfoChanged, after its route(): resignAndLeave() is through once the game
    // is left, and the window closes then if it was Quit.
    function gameInfoChanged() {
        if (ChessConnector.InGame || !_resigning)
            return
        _resigning = false
        if (_quitAfterLeave) {
            _quitAfterLeave = false
            _quit()
        }
    }

    // The leave warning's answers: Leave / Quit, or Resign.
    property bool _resigning:      false
    property bool _quitAfterLeave: false
    property bool _quitConfirmed:  false   // the window closes without asking again
    function _leaveOnline(quit, resign) {
        if (resign) {
            _resigning = true
            _quitAfterLeave = quit
            ChessConnector.resignAndLeave()
        } else if (quit) {
            _quit()
        } else {
            ChessConnector.endGame()
        }
    }
    function _quit() {
        _quitConfirmed = true
        flow.quitRequested()
    }
    // The game ended while the warning was open: it no longer goes on without us.
    on_OnlineRunningChanged: if (!_onlineRunning) leaveOnlineDialog.close()

    ConfirmDialog {
        id: resignConfirm
        title: "Resign this game?"
        text: flow._mode === "online" ? "The game ends as a loss on Lichess." : ""
        acceptText: "Resign"
        onAccepted: ChessConnector.resignGame()
    }

    ConfirmDialog {
        id: switchUserConfirm
        objectName: "switchUserConfirm"
        title: "Switch user?"
        text: flow._mode === "online" ? "You leave the Lichess game; it keeps running on lichess.org. You can come back from Continue or Play online."
                                      : "The game is kept as it is: this profile can continue it later from Continue or Games."
        acceptText: "Switch user"
        onAccepted: ChessConnector.switchUser()
    }

    LeaveOnlineDialog {
        id: leaveOnlineDialog
        objectName: "leaveOnlineDialog"
        onAccepted:        flow._leaveOnline(quitting, false)
        onResignRequested: flow._leaveOnline(quitting, true)
    }
}
