---
id: LI-17
title: "A withdrawn challenge is resumed by the Online page"
type: bug
status: done
priority: high
---

# LI-17 A withdrawn challenge is resumed by the Online page

Found by GitHub's UI tests: since #257 every Tests run on `develop` failed `tst_lichess`'s `test_20_newOpponent` ("the seek's request open" false), in the Debug build only.

**Cause:** `LichessClient::cancelChallenge` emitted `challengeCanceled` only when Lichess answered the cancel POST. Until then the saved pending challenge (`UserManager.savePendingChallenge`, cleared by `LichessChallenges.clearPending()` on that signal) stayed. Leaving a finished game withdraws its rematch (`ChessConnector::endGame` → `withdrawRematch`); when the Online page opened before the answer came, `resumePending()` resumed the withdrawn challenge: its dialog, and `LichessClient` waiting for its game (`_waitForGameId`, polled every 3 s), so `challengeWaiting()` refused every real-time seek ("A challenge of yours is waiting"). test_19 waited only until FakeLichess had *received* the POST; the slower Debug runner opened the Online page in between.

**How to see it:** in the app, leave a finished Lichess game with a rematch challenge waiting and open Play online within one round trip to Lichess.

## Resolution

### 2026-11-26 — fixed (branch: bugs/withdrawnChallengeResumed_261126)

- **Fix:** `cancelChallenge` emits `challengeCanceled` at once, after `stopStream()`, not on Lichess's answer (the reply is only deleted): the challenge is ours to drop, so `endGame()` clears the saved one before anything else can resume it, and the dialog's Cancel closes it without the round trip.
- **Tests:** ChessQtTests `CancelChallenge_CanceledAtOnce` (FakeLichess that never answers the POST: the signal at once and once, a seek starts right after); `tst_lichess` test_19 checks `!UserManager.hasPendingChallenge` right after leaving. ChessQtTests Debug 352/352; UI tests Debug (`Run-UiTests.ps1 -Build -Configuration Debug`): 13 files, 0 failed.
