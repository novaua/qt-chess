---
id: LI-13
title: "Tell the player when Resign, Abort, Draw or Takeback fails"
type: bug
status: done
priority: low
---

# LI-13 Tell the player when Resign, Abort, Draw or Takeback fails

Found by the async review of 2026-11-21.

**Found:** `LichessClient::postGameAction` (`LichessClient.cpp:1493`), used by `resign`, `offerDraw`, `requestTakeback` and `abortGame`, deletes the reply without looking at it. A Resign that fails (network, Lichess 4xx/5xx) from `LeaveOnlineDialog` leaves the page while the game goes on at Lichess; the player later loses on time without being told.

**Fix:** check the reply as `claimVictory` does: on an error or HTTP ≥ 400, `networkError("Resign failed: …")` (Lichess's message through `apiErrorMessage`). Consider keeping the game page when a Resign-and-leave fails.

**Test:** FakeLichess answers 400 / 500 to `/resign` (and a draw offer): `networkError` comes. UI: leaving with Resign while the fake refuses shows the message.

## Resolution

### 2026-11-26 — fixed (branch: bugs/li13GameActionFails_261126)

- **Cause:** `postGameAction` deleted the reply unread, so a resign, abort, draw or takeback that Lichess refused or never got went unsaid; `resignAndLeave()` then left the page after 5 s while the game went on at Lichess.
- **Fix:** `postGameAction(gameId, action, failed)` reads the reply: on an error, `gameActionFailed(gameId, action, "Resign failed: <reason>")` (Lichess's `{"error"}`, "Lichess sign-in expired" on a 401, "Lichess didn't answer" after LI-12's timeout); nothing is posted in screenshot mode. `ChessConnector` says it (`gameActionFailed(message)` → the window's short message), also after the game was left, but not for a game already over. A refused resign of `resignAndLeave()` stops the leave: the page stays, `resignAndLeaveStopped` resets `LeaveFlow` (Home asks again, a Quit waiting for it doesn't happen). Not `networkError`: that one isn't shown once the game is left.
- **Tests:** ChessQtTests `LichessGameActionTests` (400 / 500 answers with Lichess's reason; taken ones say nothing; no answer: "Lichess didn't answer"): 359/359 passed. `tst_lichess` test_21_resignRefused (`FakeLichess.refuseGameActions`): Leave the game? → Resign refused: the message, the page stays past the 5 s, Home asks again, then the resign goes through. UI tests Debug: 13 files, 0 failed.
