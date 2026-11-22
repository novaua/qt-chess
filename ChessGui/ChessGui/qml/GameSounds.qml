import QtQuick
import QtMultimedia
import ChessPlusPlus

// Every sound the app plays (BL-35): one instance in the window, `sounds`. The sound effects switch
// (AppSettings.musicEnabled) and the volume apply to them all.
// - play(name): at once. The window asks for "check" (a resumed game that opens in check: GamePage
//   holds it until its board is idle) and "lowTime"; SettingsPage's volume preview.
// - playWhenSeen(name, then): a sound about the position comes once the board shows it (BL-34, BL-35
//   step 1b: the move first, then its sound): at ChessConnector's boardIdle (the slide and a beat); at
//   once if the board is idle or a position is being reviewed. `then` (optional) runs with it, also with
//   sound off: the window's "Check" message. A new game, a takeback or leaving the game drops what waits;
//   a check is told once, and only if the board still shows it when it settles (_playWaiting).
//   Only a move's own sound plays as its slide starts: it is the move.
// - Its own Connections play the moves, the mate and the win, and a new Lichess challenge.
// Names: move, moveOpponent, check, checkmate, win, lowTime, challenge.
Item {
    id: sounds
    visible: false

    // A sound was asked for with sound on (whether or not the effect could play: a test runner may have
    // no audio device).
    signal played(string name)

    readonly property real _volume: AppSettings.effectsVolume()

    SoundEffect { id: sndMove;         source: "qrc:/sounds/move.wav";          volume: sounds._volume }
    SoundEffect { id: sndMoveOpponent; source: "qrc:/sounds/move_opponent.wav"; volume: sounds._volume }
    SoundEffect { id: sndCheck;        source: "qrc:/sounds/check.wav";         volume: sounds._volume }
    SoundEffect { id: sndCheckmate;    source: "qrc:/sounds/checkmate.wav";     volume: sounds._volume }
    SoundEffect { id: sndWin;          source: "qrc:/sounds/win.wav";           volume: sounds._volume }
    SoundEffect { id: sndLowTime;      source: "qrc:/sounds/lowtime.wav";       volume: sounds._volume }
    SoundEffect { id: sndChallenge;    source: "qrc:/sounds/challenge.wav";     volume: sounds._volume }

    readonly property var _effects: ({
        move: sndMove, moveOpponent: sndMoveOpponent, check: sndCheck, checkmate: sndCheckmate,
        win: sndWin, lowTime: sndLowTime, challenge: sndChallenge
    })

    function play(name) {
        if (!AppSettings.musicEnabled)
            return
        const effect = sounds._effects[name]
        if (!effect) {
            console.warn("GameSounds: no sound named", name)
            return
        }
        // One still loading plays once loaded (SoundEffect queues it), except a move's own: it belongs to its
        // slide, and late it would sound like the next move.
        const late = effect.status !== SoundEffect.Ready
        if (effect.status !== SoundEffect.Error && !(late && (name === "move" || name === "moveOpponent")))
            effect.play()
        sounds.played(name)
    }

    // The sounds waiting for the board: { name, then }. Queued with sound off too, for `then`.
    property var _waiting: []
    function playWhenSeen(name, then) {
        sounds._waiting = sounds._waiting.concat([{ name: name, then: then }])
        // A mate comes with its move, which marks the board busy in the same call: ask on the next pass.
        Qt.callLater(sounds._playWaitingIfIdle)
    }
    function _playWaitingIfIdle() {
        if (ChessConnector.isBoardIdle() || ChessConnector.ReviewMode)
            sounds._playWaiting()
    }
    // A check (its sound and `then`) is told once, and only if the board still shows it: a takeback or a
    // reply (a Lichess premove) before the board settled may have ended it; with a mate, the mate says it.
    // While reviewing it plays at once (CheckedKing is then the reviewed position's, not the game's).
    function _playWaiting() {
        const waiting = sounds._waiting
        sounds._waiting = []
        const lastCheck = waiting.map(w => w.name).lastIndexOf("check")
        const checkStands = (ChessConnector.ReviewMode || ChessConnector.CheckedKing >= 0)
                            && !waiting.some(w => w.name === "checkmate")
        waiting.forEach((w, i) => {
            if (w.name === "check" && (i !== lastCheck || !checkStands))
                return
            sounds.play(w.name)
            if (w.then)
                w.then()
        })
    }

    Connections {
        target: ChessConnector
        // The move's own sound comes as the slide starts: it is the move. White's or Black's by the count.
        function onMoveCountChanged() {
            if (!ChessConnector.InGame || ChessConnector.MoveCount === 0)
                return
            sounds.play(ChessConnector.MoveCount % 2 === 1 ? "move" : "moveOpponent")
        }
        function onCheckMateNotify()       { sounds.playWhenSeen("checkmate") }
        function onCheckMateResult(winner) { sounds.playWhenSeen("win") }
        // On the next pass, if the board is still idle: idle can come from inside a change that then goes
        // on (a takeback clears the gate before it undoes the move; a premove plays as the board settles).
        function onBoardIdle()             { Qt.callLater(sounds._playWaitingIfIdle) }
        function onNewGameStarted()        { sounds._waiting = [] }
        function onGameResultChanged() {
            if (ChessConnector.GameResult === "")   // a takeback resumed the game
                sounds._waiting = []
        }
        function onGameInfoChanged() {
            if (!ChessConnector.InGame)
                sounds._waiting = []   // the page is gone: no idle comes for them
        }
    }

    // BL-10: a new challenge to the player (one resent after a reconnect isn't new). The window flashes
    // the taskbar button.
    Connections {
        target: LichessClient
        function onChallengeReceived(challengeId) { sounds.play("challenge") }
    }
}
