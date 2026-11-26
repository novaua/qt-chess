---
id: BL-36
title: "The leave, resign and quit flow out of ChessGame.qml"
type: task
status: done
priority: low
---

# BL-36 The leave, resign and quit flow out of ChessGame.qml

Kept apart from BL-35 (split `ChessGame.qml`) because it is the riskiest part. The owner decided this on
2026-11-22. Take it after BL-35's steps 1–6.

**What moves into `LeaveFlow.qml`:**
- the functions `leaveGame`, `_doLeaveGame`, `_leaveOnline`, `switchUser`, `_doSwitchUser`, `resign`, and the
  flags `_resigning`, `_quitAfterLeave`, `_quitConfirmed` (the window's lines 228–296);
- the quit after a resignation in `onGameInfoChanged` (1036–1043);
- `onClosing`;
- `resignConfirm`, `switchUserConfirm`, `leaveOnlineDialog` (1370–1394).

Its interface: `leave()`, `switchUser()`, `resign()`, `onWindowClosing(event)`, and a signal `quitRequested()`.
Line numbers are from `develop` at 772be8c.

**Risks:**
- The order of quitting after a resignation: Lichess ends the game first, then the window closes.
  `tst_lichess` and `tst_profiles` cover parts of it.
- ScreenshotScene's `dialogs.leaveOnline` has to be rewired.
- `_doLeaveGame` and `_doSwitchUser` each make two calls: `LichessClient.cancelChallenge` for a waiting
  rematch, then a `ChessConnector` call. Check them against the root rule "one call per user action". A fix
  there is a change in behaviour: say so in the PR.

Plan context: [docs/BL-35-ChessGameSplitPlan.md](../../docs/BL-35-ChessGameSplitPlan.md).

## Resolution

### 2026-11-26 — LeaveFlow.qml; closed (branch: features/bl36LeaveFlow_261126)
- `LeaveFlow.qml` (one instance, `leaveFlow`): `leave()`, `switchUser()`, `resign()`, `windowClosing(event)` (the
  window's `onClosing`), `gameInfoChanged()` (called by the window after its `route()`, so the order "Lichess ends
  the game, the page goes, then the window closes" is kept), `quitRequested()`; `resignConfirm`,
  `switchUserConfirm`, `leaveOnlineDialog` (`onlineDialog` for ScreenshotScene's `dialogs.leaveOnline`) and
  `dialogOpen` for the window's `_dialogOpen`. `ChessGame.qml`: 1035 → 937 lines.
- Root rule 1: `_doLeaveGame` and `_doSwitchUser` made two calls (`LichessClient.cancelChallenge` for a waiting
  rematch, then `ChessConnector`). Now one: `ChessConnector` remembers our challenge made with a game on the board
  (`challengeCreated` / `directChallengeSent` while `InGame`; cleared on canceled, declined, a game started) and
  `endGame()` withdraws it. `LichessChallenges.cancelPendingRematch()` is gone. A change in behaviour only at the
  edges: every `endGame()` withdraws it now (also Home again during a resignation, where none can be waiting).
- Tests: `Run-UiTests.ps1 -Build` 13 files green, with the new `tst_lichess` test 19 (leaving a finished Lichess
  game withdraws its waiting rematch); ChessQtTests 349 passed; the `leave-online` scene identical to `develop`'s
  (pixel compare, offscreen). Closing the window during a running Lichess game (Quit, Resign then quit) has no UI
  test: a test can't close its window.
