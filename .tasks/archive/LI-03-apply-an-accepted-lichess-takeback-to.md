---
id: LI-03
title: "Apply an accepted Lichess takeback to our board"
type: task
status: done
priority: medium
---

# LI-03 Apply an accepted Lichess takeback to our board

<!-- migrated from TASKS.md:259 (Lichess Improvement Backlog) -->

- [x] LI-03 Apply an accepted Lichess takeback to our board [Medium] (from the PR #106 code review). **Done (branch `lichessTakeback_031126`)**: `LichessClient` keeps the stream's list and our sent moves apart and emits `moveListReplaced` for a shorter or different list (and for a move Lichess refused); `ChessConnector` takes back to the common moves and plays the rest; "Move taken back"; 5 stream tests and the `takeback` check scene. **Checked by hand on 2026-11-03** with a real Lichess game: we ask and they accept / refuse, they ask and we accept / refuse — all four work. Lichess allows takebacks between players when the other side agrees (each player's Lichess preference: always, casual games only, or never). The app can already ask for one and answer the opponent's request (`requestTakeback` / `respondTakeback`, the "takeback" offer bar), but when it is accepted Lichess sends a **shorter** `moves` list in `gameState`. `LichessClient::handleGameStreamLine` only emits moves when the list grows and replaces `_lastMovesList`, so our `Game` stays ahead (BL-04 §7 noted this). The next opponent move is then played on the wrong position: `DoMove` throws, or the board silently desyncs. To do:
  - a `takebackApplied(int plies)` signal (or the new full list) when the list shrinks;
  - `ChessConnector` undoes that many plies with the gate cleared first: a move held in the gate (BL-14) may be one of those taken back;
  - fix the move list, captured pieces, last-move squares and check state;
  - a message ("Takeback accepted");
  - resync safety: if the lists disagree in some other way, rebuild the game from Lichess's list;
  - tests: `LichessClientTest` stream lines (a list that shrinks, then grows again), and a scripted check like the step-5 stream hook.

## Resolution

### 2026-11-03 — LI-03: an accepted Lichess takeback on our board (branch: lichessTakeback_031126)

#### What was done
- **`LichessClient`** keeps two lists: the moves the stream has shown (`_lastMovesList`) and ours that were sent but not echoed yet (`_sentMoves`, `trackSentMove()`; before, our move was added to the stream's list at once). A gameState's moves (`applyStreamMoves`):
  - **more moves**: the echoes of ours are dropped, the rest are the opponent's (`opponentMoveReceived`), as before;
  - **the same moves while ours is on its way** (e.g. a draw offer's state came first): nothing; this used to be harmless and must not look like a takeback now;
  - **shorter** (an accepted takeback, by either side) **or different** (Lichess has another move than ours, or an earlier move differs): **`moveListReplaced(allMoves)`**.
  - **A move Lichess refused** (the POST got an HTTP error) also replaces the list (`moveRefused`), so our board takes it back instead of staying a move ahead ("Move failed: …" as before). Without an answer (a timeout) the move stays: Lichess may have played it, and the stream tells.
- **`ChessConnector::onLichessMoveListReplaced`**: clears the gate (a held opponent move may be one of those taken back) and what waited for it, leaves the review, drops a "takeback" offer bar, undoes back to the moves both lists share, rebuilds the move list, then plays the rest of Lichess's list (anew without slides when more than two plies change). A pure takeback (not a refused move) emits **`movesTakenBack(plies)`** → "Move taken back" / "2 moves taken back". A draw offer that waited for the held move is shown after it, not dropped. During a promotion choice the list waits until the piece is chosen, then replaces that move (which isn't sent). After the game's end it isn't applied (the result stands).
- **Tests** (`LichessClientTest`, 5 new): a takeback, then the game grows again (our echo, the opponent's answer); the opponent's move taken back; our sent move not in the state yet; the lists disagree (another move, an earlier move); `gameFull` drops sent moves.
- **Check scene `takeback`** (not in `all`): `main.cpp` feeds a `gameFull` with four moves, waits for the board to show them (`boardIdle`), feeds the list after a takeback of two, and grabs the window once the board is idle again: 1. e4 e5, last move e7–e5, "1… e5", "Tester to move", "2 moves taken back". New baseline.
- Release builds; ChessTests + ChessQtTests **242 pass**.
- **Code review** (`/code-review` of #116), fixed: a timeout no longer takes our move back; a promotion no longer loses the new list; a held draw offer isn't dropped; no "Move taken back" for a refused move; "anew" counts the undone plies too; nothing after the game's end; the scene sets the game id through `LichessClient::setCurrentGameId()` (no id-from-`gameFull` fallback in real code) and starts on `ChessConnector.screenshotSceneSetUp()` from the scene instead of a 500 ms timer. Not done: a `ChessConnector` unit test (ChessQtTests doesn't build the connector yet: it needs QtGui/Quick in the test project) — the `takeback` scene covers the path.
- **Screenshots**: after the review fixes, on an unlocked screen, all 58 baselines match (`all` light and dark, `takeback`, `robot`, `select`/`drag` `-Black`).

#### Next
- The owner's manual round: LI-04 (`docs/LI-04-ManualTests.md`). The real takeback and the draw offer are done: the owner checked all four cases of each on 2026-11-03 (we offer / they offer, accepted / refused).
- Then LI-05 (premove).
