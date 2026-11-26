---
id: LI-12
title: "Time out Lichess requests that never answer"
type: bug
status: done
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

## Resolution

### 2026-11-26 — the timeout (branch: bugs/li12LichessTimeout_261126)

- **Cause:** Qt's `QNetworkAccessManager` has no transfer timeout, and only the move POST and the flairs set one: a request on a silently dead connection, or to a server that stalls, was waited for forever, with its busy state on screen.
- **Fix:** `LichessClient::RequestTimeoutMs` (30 s) on `LichessClient`'s and `LichessAuth`'s `_nam` (`setRequestTimeout` for the tests). Qt re-arms it on every chunk received, so the streams live on Lichess's keep-alive lines; the move POST (10 s) and the flairs (10 s) keep their own. `LichessClient::timedOut(reply)` (canceled or `TimeoutError`, not a 401's abort) for a reply we didn't abort: `apiErrorMessage` and `LichessAuth` then say "Lichess didn't answer"; `LichessStream` reports it to `ended` as `TimeoutError`, so `gameStreamEnded` no longer drops a canceled stream (one we drop never reaches it) and a rejoin or an accepted game whose stream stalls before `gameFull` fails (`rejoinFailed` / `networkError`), clearing `rejoiningGameId` / `acceptingChallengeId`.
- **Tests:** `LichessTimeoutTests` (ChessQtTests, FakeLichess, 300 ms timeout): Accept with no answer; Accept whose game stream stays silent; a rejoin whose stream stays silent; Your games with no answer (`ongoingGamesLoading` ends, the error says why); a game stream fed keep-alive lines every 100 ms stays up across four timeouts. ChessQtTests Debug: 356/356 passed; UI tests (`Run-UiTests.ps1 -Build`): every file passed.
