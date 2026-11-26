# LiChessConnector/

Lichess's Board API: a DLL (`LICHESSCONNECTOR_EXPORT`), Qt Core + Network. `LichessClient` is a QML singleton
(`AppServices`); QML calls it directly for non-game work (account, challenges, seeks), the game itself goes
through `ChessConnector` (rule 1 and 2 of the root's Key Conventions). Its tests: `ChessQtTests/` (`Lichess*Test.cpp`,
against `FakeLichess`).

## Files
- `LichessClient` — everything below; most of its contract is in the header's comments.
- `LichessStream` — one ndjson GET that stays open (the game stream, the event stream) with a silence watchdog.
- `LichessAuth` — the sign-in: OAuth with PKCE in the browser (a local `QTcpServer` takes the callback), or a
  personal API token (`useToken`). The token goes to `UserManager` (ChessGui), which stores it.

## LichessClient by area
- **Variants (LI-01A stage 2):** `playableVariant` (standard, chess960); a challenge's or ongoing game's
  `canPlayHere` is false for another variant, and for what the Board API can't play (Bullet, UltraBullet, and Blitz
  except from a friend, the API or the AI); `variantName` (Lichess's
  names, QML too); `gameStarted` ends with the game's `variant` and start FEN (`""` for `startpos`):
  `ChessConnector::onLichessGameStarted` starts from it or refuses the game with `gameStartFailed`;
  `challengePlayer(…, variant)`; a Chess960 rematch sends the game's `fen` (the same position); the `chess960`
  rating pool.
- **Players (LI-07, LI-11):** `gamePlayer(white)`: the game's `LichessPlayer` from `gameFull` (id, name, title,
  rating, provisional; the AI is "Stockfish level N"; gameFull has no flair), set before `gameStarted`;
  `toJson()` writes it back in that shape, with the flair (a kept Lichess game's players, LI-14);
  `currentGameFull()`: the started game's `gameFull` as it came (rated, speed, clock, `createdAt`). Then one
  `POST /api/users` for both players' flairs (`fetchPlayerFlairs`, never on a resync); `gamePlayersChanged` when
  they come. `flairUrl()`: `<asset base>/assets/flair/img/<flair>.webp` on lichess1.org; `setAssetBaseUrl` for
  the fakes.
- **The game stream (LI-06, BL-10 step 1):** lost mid-game (closed, or 20 s without even a keep-alive line), it
  reconnects on its own (`LichessStream`: the 1 → 15 s backoff, a 4xx stops it); its `gameFull` is then a resync,
  as is a move POST without an answer: `gameConnectionChanged`, `ChessConnector.Reconnecting`.
  `setBaseUrl` / `setStreamTiming` for the fake-server tests.
- **Game actions (LI-13):** `resign`, `abortGame`, `offerDraw`, `requestTakeback` (also the answers to an offer):
  one that Lichess refuses or doesn't answer gives `gameActionFailed(gameId, action, "Resign failed: <reason>")`;
  one it takes says nothing (the game stream shows it). `ChessConnector` says it, also after the game was left.
- **Joining (BL-17, BL-18):** `gameStarted` carries the game's moves so far, which
  `ChessConnector::startOnlineGame(…, joinedMoves)` puts on the board before the game page opens. `rejoinGame`
  has its own `rejoiningGameId` / `rejoinFailed`, not `networkError`.
- **Ongoing games (LI-04, BL-24):** `ongoingGames`, the unfinished Lichess games (`refreshOngoingGames`,
  `GET /api/account/playing`); `ongoingGamesLoaded`: while a refresh is in flight the last list stays.
- **The account's event stream (BL-10):** open while there is a token (`startEventStream`, from `AppServices`);
  Lichess allows one per token, so the challenge wait and the rematch watch share it. It gives
  `incomingChallenges` / `challengeReceived` / `declineChallenge` (Lichess's reasons) / `challengeAcceptFailed`,
  `rematchOfferId` (the one the rematch bar shows), and refreshes `ongoingGames` on games started or ended
  elsewhere.
- **Quick pairing (LI-09):** `startSeek(limitSec, incrementSec, days, rated, variant)` (`POST /api/board/seek`;
  real time: the request held open while it waits, `cancelSeek()` aborts it; `days`: a correspondence seek left
  on Lichess, `seekSent`); `seek` (`{ limitSec, incrementSec, speed, rated, variant, since }`), `seeking`,
  `seekWaitedSec` (counted once here for every view), `seekFailed`, `speedOf`. Its game is the first `gameStart`
  that `seekMatches` (source lobby or pool, its speed, variant and rating, no move of ours, not a known game),
  else Your games polled quietly once Lichess ends the request. A seek ends with a game starting,
  `acceptChallenge`, a real-time challenge of ours, a new token, Switch user (`ChessConnector::switchUser`).
  Hand tests: `docs/LI-09-ManualTests.md`. LI-16: the seek's game is remembered (`_seekGameId`; its `gameFull`
  sets `RematchInfo::fromSeek`, with `rated`): `canSeekAgain()`, `seekAgain()` (the same clock, rating and
  variant); a rejoin of it after a restart isn't known as one. Hand tests: `docs/LI-16-ManualTests.md`.
- **Screenshot mode:** no refreshes of its own, our moves aren't posted, the token is a placeholder.

## Invariants
- The token never reaches QML: `UserManager::lichessToken()` is C++ only, and profile exports never carry it.
- One event stream per token.
- Every request ends after 30 s without a byte (`RequestTimeoutMs`, an inactivity timeout on `LichessClient`'s
  and `LichessAuth`'s `QNetworkAccessManager`; `setRequestTimeout` for the tests); the streams live on
  Lichess's keep-alive lines. A reply that timed out (`timedOut`: canceled or `TimeoutError`, and we didn't abort it)
  fails as "Lichess didn't answer" (`noAnswerMessage`), and the busy state it held clears; a game stream timed
  out before its `gameFull` fails as one (`rejoinFailed`, else `networkError`).
- One seek at a time; none while a real-time challenge of ours waits.
- Chess++ plays only standard and chess960 (`playableVariant`): another variant is listed with `canPlayHere`
  false, and a game of one that starts anyway is refused by `ChessConnector::onLichessGameStarted`.
- Tests never play against lichess.org: `FakeLichess` on 127.0.0.1 (`setBaseUrl`). The one exception,
  `LichessNetworkTests`, only checks a fake token there.
