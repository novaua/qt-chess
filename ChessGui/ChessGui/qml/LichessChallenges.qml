import QtQuick
import ChessPlusPlus

// The Lichess challenges to and from the player, their state only (BL-35): one instance in the window,
// `lichessChallenges` (`window.challenges` for ScreenshotScene). The window draws them (the challenge bar, ChallengeDialog) and answers the signals.
// - From the player (challenge setup stays in QML, see CLAUDE.md): the challenge waiting for the opponent
//   (pendingId, pendingUrl, opponentName for one to a named player, rematch for one made with the finished
//   game still on the board); dialogRequested / dialogClose for ChallengeDialog.
// - To the player (BL-10): the challenges shown (challengesShown, the oldest first: `challenge`), Accept and
//   Hide; the finished game's rematch offer has its own bar. A new one's sound is GameSounds', the taskbar
//   flash the window's.
// The game itself starts and ends in ChessConnector.
Item {
    id: center
    visible: false

    // In, from the window.
    property string page: ""            // the page on top: the Online page shows its own errors and list
    property string mode: ""            // ChessConnector.GameMode
    property bool dialogVisible: false  // ChallengeDialog is open

    signal dialogRequested()
    signal dialogClose(bool atOnce)     // atOnce: no fade (a game starts, BL-16)
    signal message(string text, string severity, bool external)

    // ── A challenge from the player, waiting for the opponent ──
    property string pendingId:    ""
    property string pendingUrl:   ""
    property string opponentName: ""      // set for a challenge to a named player, which isn't saved
    property bool   rematch:      false   // the pending challenge is a rematch of the game on screen

    function clearPending() {
        if (center.opponentName === "")   // else a saved link isn't this challenge's
            UserManager.clearPendingChallenge()
        center.pendingId    = ""
        center.pendingUrl   = ""
        center.opponentName = ""
        center.rematch      = false
        center.dialogClose(false)
    }
    // The Online page opens: a challenge still waiting for the friend (saved) opens over it.
    function resumePending() {
        if (!UserManager.lichessConnected || !UserManager.hasPendingChallenge)
            return
        center.pendingId  = UserManager.pendingChallengeId
        center.pendingUrl = UserManager.pendingChallengeUrl
        center.dialogRequested()
        LichessClient.resumeOpenChallenge(center.pendingId, center.pendingUrl)
    }
    // The game is left or the profile changes: a rematch offered from it goes with it.
    function cancelPendingRematch() {
        if (center.rematch && center.pendingId !== "")
            LichessClient.cancelChallenge(center.pendingId)
    }

    // ── BL-10: challenges to the player ──
    // The bar shows the oldest one not hidden. The finished game's rematch offer has its own bar.
    property var  _hidden: ({})   // id → true: "Hide" until the next challenge arrives
    // Lichess's answer to an Accept is awaited (the bar's or the Online page's: one state, BL-16)
    readonly property bool accepting: ChessConnector.AcceptingChallengeId !== ""
    readonly property var challengesShown: LichessClient.incomingChallenges.filter(c =>
        !_hidden[c.id] && c.id !== LichessClient.rematchOfferId)
    readonly property var challenge: challengesShown.length > 0 ? challengesShown[0] : null

    function text(c) {
        let text = c.challengerText + (c.rematchOf !== "" ? " offers a rematch" : " challenges you")
                 + " · " + c.timeControlText + (c.variant !== "standard" ? " · " + c.variantName : "")
                 + " · " + (c.rated ? "Rated" : "Casual") + " · " + c.sideText
        if (!c.canPlayHere)
            text += " · Can't be played in this app"
        else if (!ChessConnector.CanAcceptChallenge)
            text += ". Finish your game first: it waits on Lichess."
        if (center.challengesShown.length > 1)
            text += " (+" + (center.challengesShown.length - 1) + " more)"
        return text
    }

    // A local game on the board is kept for Continue (BL-19), a Lichess game goes on at Lichess.
    function accept(id) {
        ChessConnector.acceptChallenge(id)
    }

    function hide(id) {
        const hidden = Object.assign({}, center._hidden)
        hidden[id] = true
        center._hidden = hidden
    }

    Connections {
        target: LichessClient

        // A challenge created while the finished game is still on screen is a rematch
        // (the Online page is only reachable from Home).
        function onChallengeCreated(gameId, joinUrl) {
            center.pendingId  = gameId
            center.pendingUrl = joinUrl
            center.rematch    = ChessConnector.InGame
            UserManager.savePendingChallenge(gameId, joinUrl)
            center.dialogRequested()
        }

        // Not saved with savePendingChallenge: the challenge lives only while the app keeps
        // its request open, so it can't be resumed after a restart.
        function onDirectChallengeSent(challengeId, opponentName) {
            center.pendingId    = challengeId
            center.pendingUrl   = ""
            center.opponentName = opponentName
            center.rematch      = ChessConnector.InGame
            center.dialogRequested()
        }

        function onDirectChallengeLeft(opponentName) {
            center.message((ChessConnector.InGame ? "Rematch" : "Challenge") + " sent to " + opponentName
                           + ". Once accepted, the game is under Your games on the Online page.", "success", false)
        }

        function onChallengeDeclined(opponentName) {
            const what = center.rematch ? "rematch" : "challenge"
            center.clearPending()
            center.message(opponentName + " declined the " + what, "caution", true)
        }

        // Back to the Online page, which is under the dialog (a rematch: the game page).
        function onChallengeCanceled() { center.clearPending() }

        function onGameStarted(gameId, playingAsWhite, opponentName, opponentAvatarUrl) {
            center.dialogClose(true)   // BL-16: not a fade under "Starting the game…"
            center.clearPending()
        }

        // e.g. the challenger canceled meanwhile. The Online page shows its own error.
        function onChallengeAcceptFailed(challengeId, message) {
            if (center.page !== "online")
                center.message("Couldn't accept the challenge: " + message, "critical", false)
        }

        function onNetworkError(message) {
            if (center.dialogVisible && center.rematch) {
                center.clearPending()
                center.message("Rematch failed: " + message, "critical", false)
            } else if (center.dialogVisible) {
                center.clearPending()   // the Online page under it shows the error
            } else if (center.mode === "online") {
                center.message(message, "critical", false)
            }
        }
    }
}
