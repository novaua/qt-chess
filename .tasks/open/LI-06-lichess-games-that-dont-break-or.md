---
id: LI-06
title: "Lichess games that don't break or lag"
type: task
status: open
priority: medium
---

# LI-06 Lichess games that don't break or lag

<!-- migrated from TASKS.md:281 (Lichess Improvement Backlog) -->

- [ ] LI-06 Lichess games that don't break or lag [Medium] (branch `lichessReconnect_091126`). **Coded; the owner's check on a real connection is left (`PROGRESS.md`).** (1) The game stream reconnects on its own when it closes or is silent for 20 s (Lichess sends a keep-alive line every 7 s), after 1, 2, 4, 8, then 15 s; the `gameFull` that comes back is a resync, not a new game; a caution bar meanwhile, "Reconnected to Lichess" after. (2) A move POST without an answer (timeout 10 s, or the connection gone) opens the stream again: Lichess has it (the game goes on) or not ("Your move didn't reach Lichess. Play it again.", taken back). (3) A premove is played at once after the opponent's move, not after its slide. Tests: 2 stream-line tests and 7 against `FakeLichess`, a `QTcpServer` stand-in (dropped, silent, keep-alive, a lost move, a timed-out move that was played, game end, leaving). Scene `reconnecting`. (4) In a real-time Lichess game the opponent's reply is no longer held up to 0.4 s for our move's slide and pause (`MoveGate::setHoldOnline`, owner's choice: only with a clock; correspondence, unlimited and robot games keep the hold).

## Resolution

### 2026-11-09 — LI-06: Lichess games that don't break or lag (branch: lichessReconnect_091126)

#### What was done
- **Reconnect** (`LichessClient`): the game stream lost during a game (it closes or errors, or brings nothing for 20 s: Lichess sends an empty keep-alive line every 7 s, seen in the owner's log of 2026-10-29) is opened again after 1, 2, 4, 8, then every 15 s, until it's back or the game is left (`stopStream`). Not after the game's end, and not after a 401 ("Lichess sign-in expired"). `gameConnectionChanged(false/true)`.
- **The gameFull that comes back is a resync** (`resyncMoves`): no new `gameStarted`, the clock isn't restarted. Lichess's list against ours (its moves plus ours not echoed yet): ours plus new ones → the opponent's moves as usual; without our sent move → it never arrived: taken back (as a refused move: the clock goes back) with "Your move didn't reach Lichess. Play it again."; any other difference → replaced (LI-03).
- **A move without an answer**: the POST now gives up after 10 s (`setTransferTimeout`; there was no limit, so a stuck POST waited for ever). No HTTP status at all (timeout, connection gone) → `resyncGame()` reopens the stream, whose gameFull tells. No "Move failed" message for that any more: it was wrong when the move had arrived. Lichess's own refusal (HTTP 4xx) stays as before.
- **`ChessConnector.Reconnecting`**: a caution bar "Connection to Lichess lost. Reconnecting… Your clock keeps running on Lichess."; "Reconnected to Lichess" when it's back. The player may move meanwhile: the move is posted, and the resync says whether it got there.
- **Premove at once** (from #124's open question): after the opponent's move, not after its slide. A zero timer: the opponent's move gets its own pieces sync first, so its piece starts sliding before ours.
- **Tests**:
  - 2 stream-line tests: a gameFull again is a resync (no start, only the new move); a resync without our move takes it back.
  - 7 against **`FakeLichess`**, a `QTcpServer` on 127.0.0.1 standing in for lichess.org (`setBaseUrl`, `setStreamTiming` for short waits), real sockets, no Lichess: the stream closes mid-game and the opponent moved meanwhile; the stream goes silent; keep-alive lines keep it; a move whose POST is cut off and Lichess hasn't got; a move whose POST times out but Lichess played; the game ends (no reconnect); leaving while reconnecting stops it.
  - Release builds; ChessTests + ChessQtTests **308 pass**, three full runs; the 9 new tests 25 runs. One early failure in a 5-run loop, not seen again: the keep-alive test allowed only 300 ms of silence, now 1 s.
- **Scene `reconnecting`** (in `all`): the bar over a Lichess game.
- **No 0.4 s hold in real-time Lichess games** (the owner's choice, only for games with a clock): `MoveGate::setHoldOnline(false)` from `onLichessClockStarted` for a real-time clock. The opponent's moves then wait only for the page (open a second, not turning, not reviewing), not for our move's slide and pause, so our Lichess clock doesn't run while the board holds their reply. Their piece may start sliding while ours still does, as on lichess.org. Correspondence, unlimited and robot games keep the hold; every new game starts with it (`beginGame`). 3 `MoveGateTest` tests.
- **Screenshots**: the screen was locked (`LogonUI`): the dialog scenes and `clock-promotion` / `clock-continue` failed as known for a locked screen, and text renders slightly differently (premove-played/-dropped: 0.06 %, only the move-list text). All board scenes match. `reconnecting` has no baseline yet: take it on an unlocked screen (`Compare-Screenshots.ps1 -Update -Name reconnecting,reconnecting-dark`).

#### Code review (the owner's `/code-review medium` of #125, same branch)
9 findings: 8 fixed, 1 in part.
- **Fixed:**
  - **A move still on its way was taken back as lost**: a reconnect within the POST's 10 s could find Lichess without our move yet; the board took it back, then the POST arrived and its echo was played as the opponent's move. Now the moves whose POST has no answer yet (`_postsInFlight`) aren't lost: the answer or the echo decides. Test `ReconnectBeforeOurPostIsAnswered_WaitsForIt`.
  - **A 401 during a reconnect left "Reconnecting…" up for good**, and **a 4xx (a game Lichess no longer streams) was retried for ever, silently**. Lichess's own "no" (4xx) now stops the reconnects (`gameStreamStopped`, the bar goes, networkError says why); no answer or a 5xx (Lichess restarting) is still retried. Tests `ReconnectRefusedByLichess_StopsTrying`, `ReconnectServerError_TriesAgain`.
  - **"Reconnected" came before the moves** it had brought: now the moves, then "connected", then a lost move's message (so that one stays on screen).
  - **A move's POST with a non-error status below 400 counted as refused** (a 3xx): only Lichess's 4xx/5xx refuse it; anything else is resynced.
  - **Without the hold, a takeback's slide no longer held the opponent's next move**: only the player's own move is skipped now (`MoveGate::playerMoveShown`); pieces sliding back still hold it. Test `OnlineWithoutHold_StillWaitsForATakebackSlide`.
  - **The premove's order relied on a zero timer** running after the posted pieces sync: now a queued call to `playPremove`, posted after the sync to the same object (FIFO).
  - **Back from a review, the premove jumped in with the slide to the live position**: that path waits for the slide again (no clock hurry there: the opponent's move that brings our turn is the immediate path).
- **In part:** "two flags for one state (`LichessClient::_streamLost`, `ChessConnector::_reconnecting`)". `gameConnected()`, which only a test read, is gone. The connector keeps its own: it's the QML property, cleared with the game (`endGame`, a new game), while the client's is the connection's state.
- ChessTests + ChessQtTests **315 pass** (three full runs); the 14 reconnect and gate tests 10 runs. The board scenes as before (screen still locked: 0.06 % text in `premove-played`/`-dropped`); `robot` and `premove-robot` pass `-Offscreen`.

#### Not checked yet (the owner, with a real Lichess game)
- Pull the network cable / turn Wi-Fi off for 30 s in a game: the bar after at most 20 s, back on its own once the network is, with the opponent's moves made meanwhile.
- Sleep the laptop during a correspondence game, wake it: the game catches up.
- A move while offline: "Your move didn't reach Lichess" after the reconnect, or the game simply goes on if it got there.
- Premoves and instant replies feel as fast as on lichess.org (a fast opponent's reply now slides in while our piece still slides).

#### Next
- The owner's checks above; the `reconnecting` baseline on an unlocked screen.
