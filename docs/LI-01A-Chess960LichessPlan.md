# LI-01A stage 2 — Chess960 on Lichess

| | |
| :---- | :---- |
| **Date** | 2026-11-18 |
| **Status** | Step 1 (the C++ side) on `features/chess960LichessGames_181126` |
| **Branch** | `docs/chess960LichessPlan_181126` (plan); each step on its own branch from `develop` |
| **Task** | LI-01A in `TASKS.md`, stage 2 ([`LI-01A-Chess960Plan.md`](LI-01A-Chess960Plan.md) §6) |
| **Rules** | <https://lichess.org/variant/chess960> |

Stage 1 (#175–#180) made Chess960 work in local games. The rules, the castling input (king onto its rook,
premoves too), the engine and the PGN tags all work from the game's `StartPosition`. Stage 2 lets Lichess games
start from a Chess960 position and lets the player challenge and accept Chess960 games.

## 1. What there is (reviewed 2026-11-18)

- **Lichess's Board API plays Chess960.** Lichess limits the Board API by the clock only, with no variant
  rule: `Game.isBoardCompatible` in lila checks the clock (Rapid and slower; Blitz in a direct challenge,
  `friend` / `api` source, or vs the AI), and `PlayApi.WithPovAsBoard` refuses only on that. Its challenge forms
  (`SetupForm.api.user` and `.open`) take `variant` and `fen`. A `fen` is allowed for `chess960` when it is a
  960 start, and only in a casual game.
- **The app refuses anything that isn't standard:**
  - `playableHere(speed, source, variant)` (`LichessClient.cpp`) is false for any variant except standard, so
    `ongoingGames[].canPlayHere` is false for a 960 game (Continue → "Open on lichess.org").
  - `parseChallenge`: `canPlayHere = compat.board && variant == "standard"`; the challenge bar and the Online
    page then offer "Open on lichess.org" instead of Accept.
  - `OnlinePage._challenge()` sends `"standard"` ("Standard only … LI-01"); `challengePlayer` has no variant
    (`RematchInfo.variant` defaults to `standard`).
- **A game always starts from the standard set-up:** `ChessConnector::startOnlineGame` calls
  `restartGame(Chess::StartPosition())` ("standard only on Lichess until LI-01A stage 2").
  `LichessClient::handleGameStreamLine` reads `gameFull` but passes on neither `initialFen` nor `variant`
  (`gameStarted(gameId, white, opponentName, avatar, moves)`).
- **A non-standard game reached some other way isn't caught.** Examples: a link to a Crazyhouse game, or a
  challenge accepted on lichess.org that the stream then opens. The board starts standard and the first move
  that doesn't fit ends it with "Lichess sent a move Chess++ can't play", or worse, the moves fit and the
  positions differ.
- **Rematch**: `parseRematchInfo` keeps `variant` from `gameFull`, so `createRematch` would already send
  `variant=chess960`, but not the start position.
- **Ratings**: `parseRatings` reads bullet, blitz, rapid, classical and correspondence only. `ProfilePage`'s
  `_ratingPools` and `OnlinePage._perf` ("Your Rapid rating: …") are keyed by those.
- **Already right for a 960 game once it starts from its position:**
  - the moves: Lichess writes a 960 castling as the king onto its rook (`f1g1`), the form stage 1 plays and
    posts; a standard game keeps `e1g1`;
  - premoves, `MoveGate`, resync (`resyncMoves`), the clock, `ClaimWin`;
  - `GameTitle` ("· Chess960 #N"), `boardPgn()` (`setStartPosition`: the 960 tags);
  - `BoardPreview` of an ongoing game (it reads `fen`).
- **Test doubles**: `FakeLichessServer::gameFull` (UI tests) always sends `variant: standard`,
  `initialFen: startpos`; `challenge()` has no variant. `ChessQtTests/FakeLichess.h`'s `gameFull(moves)` has
  neither.

## 2. The owner's decisions

1. **Rematch keeps the position** (2026-11-18), as a local Rematch does. The new challenge sends the game's start
   FEN with `variant=chess960`. Lichess allows a FEN only in a casual game, and every challenge the app makes
   is casual already. When the opponent offers the rematch, Lichess picks the position, and the game starts from
   whatever `initialFen` says.
2. From stage 1 and §6 of the stage 1 plan: the challenge form offers Chess960, incoming 960 challenges can be
   accepted, the `chess960` rating pool is shown, and a 960 game can be rejoined (LI-04).

## 3. Design

### 3.1 Which games the app plays

One rule, `LichessClient::playableVariant(variant)`: `standard` or `chess960`. It is used in `playableHere` (the
ongoing games, and challenges without `compat`) and in `parseChallenge` (`compat.board && playableVariant`).
`fromPosition` and the other variants stay "Open on lichess.org" (a general FEN start is not in scope; LI-01B is
Crazyhouse).

### 3.2 The start of a game

- `gameStarted` gains `QString startFen` (last, so the QML handlers that take four arguments keep working):
  `""` for `initialFen: "startpos"` (or none), else Lichess's `initialFen`. Also `variant` (`"standard"`,
  `"chess960"`, …).
- `ChessConnector::onLichessGameStarted` turns them into a `Chess::StartPosition`:
  - `standard` with no start FEN → `StartPosition()`;
  - `chess960` → `StartPosition::FromFen(startFen)` (also #518: a 960 game at the standard set-up still castles
    as 960, as on Lichess);
  - anything else, or a 960 FEN `FromFen` doesn't take → `gameStartFailed("This Lichess game is <Variant>:
    Chess++ plays standard chess and Chess960. Open it on lichess.org.")` and `endGame()`, as the "can't play a
    move" case does now. A start position that doesn't hold can't be played move by move.
- `startOnlineGame(gameId, white, start, joinedMoves)` restarts from `start`. The joined moves (BL-18) and
  everything after are played from there. A resync `gameFull` (LI-06) keeps the game's start: only its moves
  are compared.
- Rejoin (LI-04: Continue, Home's Continue, the Online page) goes through the same `gameFull`, so nothing more is
  needed beyond `canPlayHere`.

### 3.3 Challenges the player makes

- `challengePlayer(username, limitSec, incSec, days, color, variant = "standard")`; `createOpenChallenge`
  already takes `variant`.
- `RematchInfo` gains `startFen` (a 960 game's `initialFen`). `challengeUser` and `createOpenChallengeSec` add
  `fen=<startFen>` when it is set. Only a rematch sets it; a new challenge leaves the position to Lichess.
- `OnlinePage`: a **Variant** row under the time control. It reuses `VariantPicker` with a Lichess line: "A
  random one of the 960 start positions. Castle by moving the king onto its rook." without the robot's "Not
  rated". The choice is remembered on its own: `AppSettings.onlineChess960`, apart from the local dialogs'
  `chess960`. The form's rating line names the Chess960 rating when it is chosen ("Your Chess960 rating:
  1650?"), since Lichess has one 960 pool for every speed. With no 960 rating yet, the line is hidden, as now.

### 3.4 Words

- `LichessClient.variantName(key)` (a static `Q_INVOKABLE`, done in step 1 for the refusal's message) gives
  Lichess's names (`chess960` → "Chess960", `kingOfTheHill` → "King of the Hill", `threeCheck` → "Three-check",
  `racingKings` → "Racing Kings", `fromPosition` → "From position", else the key with its first letter
  capitalised). Step 2 uses it in `lichess.js` `gameDetails` (the ongoing cards) and
  `OnlinePage._challengeDetails` (the challenge rows and the bar), which show the raw key now.
- `ProfilePage._ratingPools` + `{ key: "chess960", name: "Chess960" }`; `parseRatings` reads `chess960`.

### 3.5 Test doubles

- `FakeLichessServer`: a game has `variant` and `initialFen` (`gameFull` sends them); `challenge(…, variant =
  "standard", startFen = "")` and `addOngoingGame(…, variant, startFen)`. A challenge accepted with a 960 start
  makes a 960 game. `bodyOf()` already shows a challenge's `variant` / `fen`.
- `FakeLichess.h` (ChessQtTests): `gameFull(moves, variant = "standard", initialFen = "startpos")`.

## 4. Steps

Each step is one branch and one PR from the latest `origin/develop`.

| Step | Branch | What |
|---|---|---|
| 1 | `features/chess960LichessGames_…` | §3.1, §3.2, §3.3 in C++: `playableVariant`, `gameStarted`'s variant and start FEN, `startOnlineGame(…, start)`, the refusal of other variants, `challengePlayer`'s variant, the rematch's `fen`, `parseRatings`' `chess960`; the ChessQtTests (§5). No QML change except the handlers' new arguments |
| 2 | `features/chess960LichessUi_…` | §3.3's Variant row on the Online page, §3.4, §3.5's `FakeLichessServer`; scenes; `tst_lichess960` |
| 3 | `docs/chess960LichessManualTests_…` | `LI-01A-LichessManualTests.md`, CLAUDE.md, the run-chess-app skill's scenes, `TASKS.md`, `PROGRESS.md` |

## 5. Tests

**ChessQtTests** (step 1):
- `LichessClientTest`:
  - `parseOngoingGames`: a `chess960` rapid game `canPlayHere`, a `crazyhouse` one not, a `fromPosition` one not;
  - `parseChallenge`: a 960 challenge with `compat.board` can be played here, a Crazyhouse one can't;
  - `parseRatings` reads `chess960`;
  - `parseRematchInfo` keeps `variant` and a 960 game's `initialFen`, and none for `startpos`.
- `LichessStreamTest`: a `gameFull` with `variant: chess960` and an `initialFen` → `gameStarted`'s variant and
  start FEN; `startpos` → `""`.
- `LichessChallengeTest`: `challengePlayer(…, "chess960")` posts `variant=chess960` and no `fen`; a rematch of a
  960 game posts `variant=chess960` and `fen=<its start>` (named opponent and open challenge); a standard
  rematch posts no `fen`.

**UI** (step 2, `tst_lichess960.qml`, FakeLichess at Chess960 #3, king f1 / rook g1, as `tst_chess960`):
- a 960 challenge on the Online page shows "Chess960" and **Accept**. Accepted: the board is at #3, the title
  ends "· Chess960 #3". Castling O-O by dropping the king on g1 posts `…/move/f1g1`, and the opponent's castling
  from the stream (`f8g8`) lands king g8 / rook f8;
- a premoved castling during the opponent's turn is posted as the king onto its rook;
- Continue of an ongoing 960 game (`addOngoingGame` with moves past a castling): the board at that position;
- a Crazyhouse challenge: "Open on lichess.org", no Accept; an ongoing Crazyhouse game's card isn't playable;
- the Online page's Variant row: Chess960 → the challenge's body has `variant=chess960` (open and named),
  remembered after the page reopens; the local dialogs' choice is untouched;
- Rematch after a 960 game posts `fen=` with #3's start.
- Every existing `tst_lichess` test passes unchanged (standard games).

**Screenshot scenes** (step 2): `online` with the Variant row (its baseline changes), `online-960` (Chess960
chosen, the 960 rating line), a 960 challenge among `online-challenges`' rows (named "Chess960").

**By hand** (step 3, real lichess.org, two accounts as in LI-04 / BL-10): a 960 challenge by link and to a named
player, both castlings on both sides, a premoved castling, leave and Continue (also after a restart), Rematch
(same position, colours swapped) and the opponent's rematch offer, a 960 challenge from lichess.org accepted in
the app, a correspondence 960 game, the Profile's Chess960 rating, Export PGN of the game on the board pasted
into lichess.org/paste, a link to a Crazyhouse game refused with its message.

## 6. Risks

- **Lichess's start FEN**: `initialFen`'s castling field may be `KQkq` or Shredder letters (`HFhf`).
  `StartPosition::FromFen` reads only the pieces and the side to move, so either works. A 960 game whose FEN
  isn't a 960 start (it shouldn't happen) is refused (§3.2), not played wrong.
- **The castling's UCI form on the stream**: the king onto its rook in 960 (lila's `Uci` of a 960 game), `e1g1`
  in standard. The UI test plays it against FakeLichess; the hand test confirms it on lichess.org.
- **`gameStarted`'s new arguments**: QML handlers declared with fewer arguments keep working. The C++ connects
  (`ChessConnector`, the tests) change in step 1.
- **The rematch's FEN makes it casual**: every app challenge is casual already (`rated=false`, the open challenge
  has none), so nothing changes for the player.
- **The opponent's rematch offer** (`acceptRematch`): Lichess decides its position; the app plays whatever
  `initialFen` says.
