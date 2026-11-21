---
id: LI-12
title: "Time out Lichess requests that never answer"
type: bug
status: open
priority: medium
---

# LI-12 Time out Lichess requests that never answer

Found by the async review of 2026-11-21 (file and network code).

**Found:** only the move POST has a transfer timeout (`LichessClient.cpp:1466`, `req.setTransferTimeout(_moveTimeoutMs)`); `makeRequest`, `validateToken` and `LichessAuth` (token exchange, account fetch) set none, and Qt's `QNetworkAccessManager` has none by default. A request sent on a silently dead pooled connection (after sleep, Wi-Fi roaming) waits until Windows TCP gives up (tens of seconds or more); one to a server that stalls waits forever. Nothing freezes, but the busy state stays: "Accepting…" (`acceptChallenge`), the challenge dialog's wait (`createOpenChallengeSec`), the Your games spinner (`refreshOngoingGames`, `ongoingGamesLoading`), the rematch accept, the login spinner. A game stream that never sends its first line (a rejoin, an accept before `gameFull`) has no watchdog either: `LichessStream` starts it only after `setReconnect(true)`.

**How to see it:** FakeLichess accepts the connection and never answers `/api/challenge/<id>/accept`: Accept stays "Accepting…".

**Fix:**
- `_nam.setTransferTimeout(30 s)` in the `LichessClient` and `LichessAuth` constructors. It is an inactivity timeout: the streams (game, event, a direct challenge's, the seek) stay alive on Lichess's keep-alive lines (~7 s); `postMove` keeps its 10 s.
- A timed-out reply ends as `OperationCanceledError`, which `gameStreamEnded` (`LichessClient.cpp:1234`) takes as "stopped by us": tell the two apart (the reply is still the current one), so a stream timed out before `gameFull` fails as one.
- The usual failure signals with "Lichess didn't answer": `networkError`, `challengeAcceptFailed`, `rejoinFailed`; the busy states clear.

**Test:** ChessQtTests against FakeLichess with a silent route (accept, ongoing games, a game stream before gameFull) and a short timeout injected (extend `setStreamTiming`): the failure signal comes, `acceptingChallengeId` / `rejoiningGameId` / `ongoingGamesLoading` clear. The reconnect tests still pass (keep-alive streams don't time out).
