import QtQuick
import ChessPlusPlus

// The window's messages, their state only (BL-35): one instance in the window, `messages`. The window draws
// them: the two messages over the command bar (CommandBarMessage, BL-13 step 3), the InfoBars for what
// doesn't fit there, and the recent messages (MessagesFlyout). Every short message comes through
// ChessGame.notify() → notify() here.
// - The game's state, by priority: an offer or the opponent left (they wait for an answer), then the lost
//   connection: statusKind, offerText, goneText, canClaim, claimSeconds, goneDismissed, answerable.
// - A short message (message: { text, severity }): the newest replaces the one shown for 4 s, except that an
//   error stays until it is closed (a newer message meanwhile goes only to the recent messages, with the dot).
// - The recent messages (model): the last 20 of the session, newest first, all but "Check"; `unseen` counts
//   the new ones the player didn't cause (an offer, the opponent left, the connection, a decline, an error).
Item {
    id: center
    visible: false

    // In, from the window.
    property bool flyoutOpen: false       // the recent messages are open: nothing new is unseen
    property bool screenshotMode: false   // a fixed time on the recent messages (stable pictures)
    property bool shortShowing: false     // the short message is over the command bar
    property bool statusFits: false       // the game's state fits over the command bar

    // ── The game's state ──
    readonly property string offer: ChessConnector.PendingOffer   // "", "draw", "takeback", "rematch"
    readonly property bool gone: ChessConnector.OpponentGone
    readonly property bool reconnecting: ChessConnector.Reconnecting && ChessConnector.GameResult === ""
    readonly property string statusKind: offer !== "" ? "offer" : gone && !goneDismissed ? "gone" : reconnecting ? "connection" : ""
    function offerTextFor(offer) {
        return offer === "draw"     ? ChessConnector.OpponentName + " offers a draw"
             : offer === "takeback" ? ChessConnector.OpponentName + " asks to take back a move"
             : offer === "rematch"  ? ChessConnector.OpponentName + " offers a rematch" : ""
    }
    readonly property string offerText: offerTextFor(offer)
    // The opponent left: the seconds until Lichess allows the claim (<= 0: now), and whether it takes
    // it now (only on the opponent's turn: on the player's, a move first). ChessConnector counts down.
    readonly property int claimSeconds: ChessConnector.ClaimWinSeconds
    readonly property bool canClaim: ChessConnector.CanClaimVictory
    readonly property string goneText: claimSeconds > 0 ? "Opponent left · claim victory in " + claimSeconds + " s"
                                     : canClaim ? "Opponent left the game" : "Opponent left · your move, then claim victory"
    // Closed with its ×: hidden until the claim becomes possible or the opponent comes back or leaves
    // again (the recent messages keep Claim victory).
    property bool goneDismissed: false
    onCanClaimChanged: if (canClaim) goneDismissed = false
    // Over the command bar, or waiting there behind a short message: not in the InfoBars.
    function statusShows(kind) { return center.statusFits && center.statusKind === kind }
    // The state waits for the player's answer (an offer, Claim victory): a short message never hides it
    // for longer than its 4 s (an error too: it stays in the recent messages), "Check" not at all (the
    // board and the sound tell it), and a message shown when it comes gives way at once.
    readonly property bool answerable: statusKind === "offer" || (statusKind === "gone" && canClaim)
    onAnswerableChanged: if (answerable) closeMessage(message)

    function respondOffer(accept) {
        center._offerAnswer = accept ? "accepted" : "declined"
        if (center.offer === "draw")          ChessConnector.respondDraw(accept)
        else if (center.offer === "takeback") ChessConnector.respondTakeback(accept)
        else if (center.offer === "rematch")  ChessConnector.respondRematch(accept)
    }

    // ── A short message ──
    property var message: null        // { text, severity }
    Timer { id: messageTimer; interval: 4000; onTriggered: center.message = null }
    function notify(text, severity, external) {
        severity = severity ?? "info"
        const behindError = center.message !== null && center.message.severity === "critical" && severity !== "critical"
        if (text !== "Check")   // the board, the sound and the move list ("+") tell it already
            center.addRecent(text, severity, external || severity === "critical" || behindError, "message", "")
        if (behindError || (text === "Check" && center.answerable))
            return
        center.message = { text: text, severity: severity }
        if (severity === "critical" && !center.answerable) messageTimer.stop()
        else                                               messageTimer.restart()
    }
    function closeMessage(m) {
        if (m === center.message) { center.message = null; messageTimer.stop() }
    }
    // The InfoBar's one: a message that doesn't fit the command bar.
    readonly property var barMessage: message !== null && !shortShowing ? message : null

    // ── The recent messages ──
    readonly property ListModel model: ListModel {}
    property int unseen: 0
    property int _recentId: 0
    function addRecent(text, severity, external, kind, offer) {
        const rid = ++center._recentId
        center.model.insert(0, { rid: rid, text: text, severity: severity, kind: kind, offer: offer,
                                 status: kind === "message" ? "" : "open",
                                 time: center.screenshotMode ? "12:41" : Qt.formatTime(new Date(), "HH:mm"),   // stable pictures
                                 source: ChessConnector.InGame ? ChessConnector.GameTitle : "" })
        if (center.model.count > 20)
            center.model.remove(20, center.model.count - 20)
        if (external && !center.flyoutOpen)
            center.unseen++
        return rid
    }
    function clearRecent() {
        center.model.clear()
        center.unseen = 0
    }
    // They are the profile's: another one (or the profile list) starts without them.
    function resetForUser() {
        center.clearRecent()
        center._offerRid = 0
        center._goneRid = 0
        center.closeMessage(center.message)
    }
    function _setRecentStatus(rid, status) {
        for (let i = 0; i < center.model.count; ++i)
            if (center.model.get(i).rid === rid) { center.model.setProperty(i, "status", status); return }
    }
    property int _offerRid: 0
    property string _offerAnswer: ""   // how the open offer ended: "accepted", "declined"; else it was withdrawn
    property int _goneRid: 0
    Connections {
        target: ChessConnector
        // ChessConnector's own values: a binding on them (center.offer) may not be updated yet here.
        function onPendingOfferChanged() {
            const offer = ChessConnector.PendingOffer
            if (center._offerRid) center._setRecentStatus(center._offerRid, center._offerAnswer || "withdrawn")
            center._offerRid = 0
            center._offerAnswer = ""
            if (offer !== "")
                center._offerRid = center.addRecent(center.offerTextFor(offer), "info", true, "offer", offer)
        }
        // Only when the opponent leaves or comes back (not for a new countdown).
        function onOpponentGoneChanged() {
            center.goneDismissed = false
            if (ChessConnector.OpponentGone && !center._goneRid)
                center._goneRid = center.addRecent("Your opponent left the game", "caution", true, "gone", "")
            else if (!ChessConnector.OpponentGone && center._goneRid) {
                center._setRecentStatus(center._goneRid, "")
                center._goneRid = 0
            }
        }
        function onReconnectingChanged() {
            if (ChessConnector.Reconnecting && ChessConnector.GameResult === "")
                center.addRecent("Connection to Lichess lost", "caution", true, "message", "")
        }
    }
}
