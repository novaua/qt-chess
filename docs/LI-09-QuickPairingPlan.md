# LI-09 Quick Pairing — Implementation Plan

| | |
| :---- | :---- |
| **Date** | 2026-11-20 |
| **Status** | Plan; the owner's decisions of 2026-11-20 in §3 |
| **Branch** | `docs/li09QuickPairingPlan_201126` (plan); each step on its own branch from `develop` |
| **Task** | [`LI-09`](../.tasks/open/LI-09-quick-pairing.md) |

## 1. Problem

A Lichess game starts here only from a challenge: a link for a friend, a named player, or a challenge
received (BL-10). Playing someone new needs a name to type, or a link nobody takes. lichess.org's
**Quick pairing** tiles (10+0, 15+10, …) pair the player with a random opponent of about their rating
in seconds; the app has nothing like it.

## 2. What Lichess allows (checked 2026-11-20)

From the API's own spec (`lichess-org/api`, `doc/specs/tags/board/api-board-seek.yaml`) and Lichess's
eBoard page:

| | |
|---|---|
| **Call** | `POST /api/board/seek`, form fields, scope `board:play` (the app's token has it: `LichessAuth.cpp` `kScope`). |
| **Real time** | `time` (minutes, 0–180) + `increment` (seconds, 0–180). The answer is an ndjson stream of empty keep-alive lines and nothing else. **The seek lives while the connection is open**: closing it cancels the seek. Lichess closes it when the seek is **accepted or expires**. |
| **The game** | Comes as a `gameStart` on the account's **event stream**, which must be open first (it is: `startEventStream`, open while there is a token, BL-10). Its `game.source` is one of `lobby`, `pool`, …; a challenge's is `friend` / `api`. |
| **Correspondence** | `days` (1, 2, 3, 5, 7, 10, 14) instead of a clock. The answer comes at once, `{"id": "…"}`. The seek stays on Lichess **until someone joins it. The API has no call to cancel it** (only lichess.org's lobby can). |
| **Other fields** | `rated` (default false), `variant` (`standard`, `chess960`, …), `ratingRange` (`"1500-1800"`, "better left empty"), `color` ("better left empty": random). |
| **Errors** | 400 with `{"error": …}`; 429 (rate limit: wait a full minute). |
| **Speeds** | The Board API plays **Rapid, Classical and Correspondence** only; Blitz only in a direct challenge (LI-02), Bullet never. Lichess's speed of a clock: `limit + 40 × increment` seconds; Rapid from 480 s (8 min), Classical from 1500 s. |

So of lichess.org's twelve tiles, five can seek from here: **10+0, 10+5, 15+10** (Rapid) and **30+0,
30+20** (Classical). The six Bullet and Blitz tiles (1+0 … 5+3) can't.

## 3. The owner's decisions (2026-11-20)

| # | Question | Decision |
|---|---|---|
| D1 | Bullet and Blitz tiles | **Shown greyed**, as lichess.org's grid, with "Bullet and Blitz from this app: only a direct challenge. Play it on lichess.org" on hover; a click opens lichess.org's lobby. The familiar grid, honest about the limit. |
| D2 | The twelfth tile | **Custom, as on lichess.org**: its "Create a game" dialog, here the **Seek dialog**: Real time (minutes and increment, lichess.org's slider steps) or Correspondence (days per move: 1, 2, 3, 5, 7, 10, 14). The clock's speed is shown as it changes; one below Rapid (`minutes × 60 + 40 × increment` < 480 s) can't be sent, with the reason. Variant and Rated / Casual come from the card. |
| D3 | Rated or casual | **Rated by default**, as lichess.org, with a Rated / Casual switch above the tiles, remembered (`AppSettings.quickRated`). Today every challenge from the app is casual. |
| D4 | Rating range | **None** (Lichess's advice). Not in this ticket. |
| D5 | Leaving the Online page while it seeks | **The seek goes on, and the player is told so and can cancel it at any time**: leaving the page says "Still looking for an opponent. Cancel it from the icon at the top." (`ChessGame.notify`), and **every page's command bar shows a seek icon** while it seeks (a turning ring, the elapsed time beside it in a wide window); a click opens its menu: "Looking for an opponent · 10+0 Rapid · Rated · 0:42" and **Cancel seeking**. The game opens when paired, from any page. Closing the app, signing out or another profile cancels it. |
| D6 | A correspondence seek (D2) | Sent, it says "Your seek is on Lichess. When someone takes it, the game shows in Your games. Only lichess.org can cancel it." with **Open on lichess.org**. The app can't follow or cancel it (the API has no call for that): its game comes through the event stream's `gameStart` and the Your games refresh, as a game started elsewhere does today. |

## 4. Design

### LichessClient (LiChessConnector)

- `Q_INVOKABLE void startSeek(int limitSec, int incrementSec, int days, bool rated, const QString& variant)`
  (seconds: a custom clock can be ¼ minute steps; sent as `time` = `limitSec / 60`, `0.5` for ½; named
  `startSeek` in step 1: QML can't have a method and a property both called `seek`);
  `Q_INVOKABLE void cancelSeek()`.
- `Q_PROPERTY(QVariantMap seek READ … NOTIFY seekChanged)`: `{}` when none, else `{ limitSec, incrementSec,
  speed, rated, variant, since }` (`since`: ms since epoch, for the elapsed time). `seekFailed(QString message)`.
  `static QString speedOf(int limitSec, int incrementSec)` (Lichess's rule above; the picker's speed names).
- **Real time**: as `challengeUser`'s keep-alive request (`_directChallengeReply`): `_seekReply` held open,
  `abort()` to cancel (before `abort()` the pointer is cleared, so its `finished` handler ignores it).
  HTTP ≥ 400: `seekFailed` with `apiErrorMessage` (429: "Lichess asks to wait a minute before seeking
  again"). One seek at a time: `seek()` cancels the last one.
- **Its game**: while a real-time seek is open, the first `gameStart` whose `source` is `lobby` or `pool`
  and that isn't a known game (`isOngoingGame`, `_acceptingId`, `_waitForGameId`) is the seek's:
  `onGameStartDetected(gameId)` → `streamGame` → `gameStarted` → `ChessConnector::onLichessGameStarted`,
  **as a challenge accepted on Lichess's side**: nothing new there (a local game on the board is kept, as
  with BL-10's Accept). The `gameStart` line is logged in full, so the hand test shows what a seek's
  game carries.
- **The fallback** (the `gameStart` missed while the event stream reconnects): Lichess closes the seek's
  connection without our abort. Then `refreshOngoingGames()`, and a real-time game that wasn't in
  `ongoingGames` when the seek began is its game. None after 5 s: `seekFailed("No opponent was found:
  Lichess ended the seek.")`.
- **What ends a seek**: `cancelSeek()`, a game starting (`streamGame`: the seek's, a challenge accepted;
  a rejoin once its game is there), `acceptChallenge` (before its POST), a real-time challenge of ours
  (and no seek while one waits), a new token or none (another profile, sign out, expired), Switch user
  (`ChessConnector::switchUser`), the destructor. **Not** `stopStream()` / `stopAll()`: those end a game or a challenge, and a
  seek outlives leaving a robot game (D5).
- **Correspondence** (D2, D6): the POST answers `{"id"}` at once: `seekSent(QString message)`, no state
  kept.

### ChessConnector

Nothing new for the game: it starts through `gameStarted`. `CanAcceptChallenge` (not during a running
real-time Lichess game) also gates the tiles. `GameStartTiming` begins "quick pairing" when the game
comes (`onLichessGameStarted` already begins "Lichess game" when nothing runs).

### QML

- **`timecontrols.js`**: the presets of `TimeControlPicker` (`[1, 0, "Bullet"] … [30, 20, "Classical"]`),
  shared by the picker and the new grid.
- **`QuickPairingGrid.qml`**: lichess.org's 3 × 4 grid of action tiles (a click seeks; not a choice
  like the picker's): the clock large, the speed under it, and the player's rating in that pool
  (`UserManager.lichessRatings`; Chess960: its one pool; `?` when provisional). The seeking tile: a
  `BusyRing` and the elapsed time ("0:12") in place of the speed, its border the accent; the other
  tiles disabled; a click on it cancels. Greyed Bullet/Blitz tiles (D1), the Custom tile (D2).
  Keyboard: one Tab stop, arrows, Enter seeks (the picker's way).
- **`SeekDialog.qml`** (D2): Real time / Correspondence (a `ChoiceRow`); minutes and increment as two
  sliders with lichess.org's steps (minutes ¼ … 180, increment 0 … 180), the clock and its speed
  ("20+10 · Rapid") above them; Correspondence: the days. **Seek** disabled below Rapid, with "Lichess
  allows this app Rapid, Classical and Correspondence only." A real-time custom seek waits like a tile's
  (the Custom tile shows the ring and the clock); a correspondence one is sent and done (D6).
- **`SeekIndicator.qml`** (D5): in `CommandBar` itself, so every page has it, while `LichessClient.seek`
  isn't empty: an `IconButton` with a `BusyRing` (a still clock with Reduce animations), the elapsed
  time beside it when the bar has room; its menu: what is sought and **Cancel seeking**. Accessible
  name "Looking for an opponent, 0:42".
- **`OnlinePage.qml`**: a **Quick pairing** `SectionCard` (`quickPairingCard`) after "Challenges to
  you": Rated / Casual (D3), its own `VariantPicker` (`AppSettings.quickChess960`: not the challenge
  card's setting, two pickers on one page would fight over one), the grid, and under it while seeking
  "Looking for an opponent…" with **Cancel**. Hidden without a Lichess account, like the challenge card.
  Errors in the page's error bar (`seekFailed`); a failure while the page isn't open: `ChessGame.notify`.
- **`ChessGame.qml`** (step 3, D5): leaving the Online page while it seeks says so once
  ("Still looking for an opponent. Cancel it from the icon at the top.").

## 5. Steps (one PR each)

1. **LichessClient**: `seek` / `cancelSeek` / `seek` state / `seekFailed`, the `gameStart` match, the
   fallback, what ends a seek. `ChessQtTests/FakeLichess.h`: `POST /api/board/seek` holds the stream
   (keep-alive lines), a test hook pairs it (a `gameStart` with `source: lobby`, then closes the seek
   stream) or ends it unpaired. `LichessClientTest`: a seek pairs and its game streams; cancel closes
   the connection (the fake sees it); 400 and 429 reported; a seek ended unpaired; the fallback; another
   game's `gameStart` (a challenge's) isn't taken; accepting a challenge cancels the seek; a new token
   cancels it.
2. **The Online page**: `timecontrols.js`, `QuickPairingGrid`, the Quick pairing card, real-time tiles.
   `fakelichessserver`: the seek route and `pairSeek(opponent)` for QML. `tst_lichess`: a tile seeks
   (the request's body: `time`, `increment`, `rated`, `variant`), the fake pairs it, the game page
   opens; Cancel stops it (the fake sees the close); a refusal shows; Blitz greyed. Scenes
   `quick-pairing`, `quick-pairing-waiting` (also `640x560`).
3. **The wait beyond the page** (D5): `SeekIndicator` in `CommandBar`, the message on leaving the
   page; `tst_lichess`: Back keeps the seek and says so, the icon's Cancel seeking ends it (the fake
   sees the close), a game paired while on Home opens, another profile cancels it. Scenes
   `quick-pairing-indicator` (Home, its menu open; also `640x560`: the icon without the time).
4. **Custom** (D2, D6): `SeekDialog`, real time and correspondence, `seekSent`'s message. Tests: a
   custom clock's request (`time`, `increment`), a clock below Rapid can't be sent, a correspondence
   request (`days`, no `time`) and its message. Scenes `seek-dialog`, `seek-dialog-blitz` (the reason
   shown).
5. **Docs and hand tests**: CLAUDE.md (LichessClient, the Online page), `docs/LI-09-ManualTests.md`
   with two Lichess accounts: account B clicks the same tile on lichess.org (does Lichess pair a
   pool player with our seek, or only from the lobby's list?), B joins our seek from the lobby list;
   Cancel (B no longer sees it); a rated game's rating change; Chess960; a custom clock (20+0); a
   correspondence seek (B joins it later; cancelled on lichess.org); the log's `gameStart` line of a
   seek's game (`source`).

## 6. Risks

- **How fast a seek pairs.** A seek is a lobby entry; lichess.org's tiles use pools. Whether pool
  players are paired with a lobby seek of the same clock is to see in the hand test (step 5). If they
  aren't, a 10+0 seek waits for someone in the lobby list, which can be slow outside busy hours.
- **A seek's game's `source`.** Taken from the spec (`lobby`, `pool`); a different one would miss the
  match, and the fallback (the seek's connection closed) catches it within seconds. The hand test logs it.
- **A seek with a game going on at Lichess.** Lichess may refuse it (400): shown as its message. The
  tiles are off during a running real-time Lichess game in the app anyway.
- **The Board API's limits change** (Lichess's eBoard page says they may): a refused speed is shown as
  Lichess's message, not a silent stop.
