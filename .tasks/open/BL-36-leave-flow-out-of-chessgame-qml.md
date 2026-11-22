---
id: BL-36
title: "The leave, resign and quit flow out of ChessGame.qml"
type: task
status: open
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
