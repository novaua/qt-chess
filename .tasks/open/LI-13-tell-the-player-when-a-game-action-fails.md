---
id: LI-13
title: "Tell the player when Resign, Abort, Draw or Takeback fails"
type: bug
status: open
priority: low
---

# LI-13 Tell the player when Resign, Abort, Draw or Takeback fails

Found by the async review of 2026-11-21.

**Found:** `LichessClient::postGameAction` (`LichessClient.cpp:1493`), used by `resign`, `offerDraw`, `requestTakeback` and `abortGame`, deletes the reply without looking at it. A Resign that fails (network, Lichess 4xx/5xx) from `LeaveOnlineDialog` leaves the page while the game goes on at Lichess; the player later loses on time without being told.

**Fix:** check the reply as `claimVictory` does: on an error or HTTP ≥ 400, `networkError("Resign failed: …")` (Lichess's message through `apiErrorMessage`). Consider keeping the game page when a Resign-and-leave fails.

**Test:** FakeLichess answers 400 / 500 to `/resign` (and a draw offer): `networkError` comes. UI: leaving with Resign while the fake refuses shows the message.
