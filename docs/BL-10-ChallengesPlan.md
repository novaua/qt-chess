# BL-10 Lichess Challenges as Notifications — Implementation Plan

| | |
| :---- | :---- |
| **Date** | 2026-11-09 |
| **Status** | Plan, with the owner's choices of 2026-11-09 |
| **Branch** | `bl10Plan_091126` (plan); each step on its own branch from `develop` |
| **Task** | BL-10 in `TASKS.md`. Builds on LI-06 (#125: the game stream's reconnect), which must be merged before step 1 |

## 1. Problem

The app hears from Lichess only during its own flows: while it waits for a challenge it created, and
(since #89) for a rematch offer while the finished game is on screen. Everything else that reaches the
player's Lichess account is lost to the app:

1. **A challenge to the player** from lichess.org (a friend, a stranger, the "Challenge to a game"
   button on the player's profile) never shows up. The challenger waits and gives up.
2. **A rematch offered after we left the game** is not seen (only while the result is on screen).
3. **A game started elsewhere** (accepted in a browser, a correspondence game the opponent started)
   shows up only when Home or the Online page is opened again (LI-04's list is fetched, not live).

Lichess sends all of these on one stream, `/api/stream/event`, but only to a stream that is open at the
time. The app opens it only for a moment.

## 2. Goals (owner's choices, 2026-11-09)

| | |
|---|---|
| **Always listening** | While a profile with a linked Lichess account is active, one event stream is open, and reconnects on its own like the game stream (LI-06). |
| **Incoming challenges, anywhere** | A challenge to the player shows as an InfoBar on any page (Home, Online, a game, Settings): who, rating, time control, rated or casual, the side. **Accept** and **Decline**. All waiting challenges are also listed on the Online page. |
| **Accept during a game: depends on the game** | During a **running real-time Lichess game**: no Accept, only Decline and "Finish your game first: it waits on Lichess". During a **robot or two-player game, or a Lichess correspondence game**: Accept leaves that game (a robot game is autosaved, a correspondence game goes on at Lichess) and starts the new one. A finished game: Accept as on Home. |
| **Attention when in the background** | The taskbar button flashes (`QWindow::alert`) and a short sound plays. No Windows toast (it would need Qt Widgets' tray icon, which BL-12 wants to drop, or WinRT code). |
| **Challenges the app can't play** | Shown like the others, with **Open on lichess.org** and **Decline** instead of Accept (Lichess's `compat.board` is false: Bullet, UltraBullet, some Blitz, Chess960 until LI-01). |
| **Your games stay current** | `gameStart` and `gameFinish` on the same stream refresh LI-04's unfinished games, so a game started elsewhere appears on its own. |

Out of scope: "your turn" notifications for correspondence games (Lichess's event stream has no event
per move; it would need polling), chat (BL-07), sending challenges to a named player (done in LI-02),
seeks and the lobby.

## 3. What Lichess offers

Checked in Lichess's API spec (`lichess-org/api`, `doc/specs`, 2026-11-09):

- **`GET /api/stream/event`** (`api-stream-event.yaml`): ndjson, "an empty line is sent every 7 seconds
  for keep alive purposes". **"Only one global event stream can be active at a time"**: opening one
  closes the previous stream of the same token. When it opens, **all current challenges and games are
  sent**, so after a reconnect the app learns everything it missed (no gap to fill).
  Events: `gameStart`, `gameFinish`, `challenge` ("a player sends you a challenge or you challenge
  someone"), `challengeCanceled` ("a player cancels their challenge to you"), `challengeDeclined`
  ("the opponent declines your challenge").
- **`challenge` event** (`ChallengeEvent.yaml`): `{ type, challenge: ChallengeJson, compat: { bot, board } }`.
  `compat.board`: "compatible with Board API", which is what this app needs to play it.
- **`ChallengeJson`**: `id`, `url`, `status`, `challenger` and `destUser` (id, name, rating, title,
  provisional), `variant`, `rated`, `speed`, `timeControl` (clock limit and increment, or days, or
  unlimited), `color` (the challenger's choice: white, black, random), `finalColor`, `perf`,
  `direction` (`in` / `out`), `initialFen`, `rematchOf`.
- **`gameStart` / `gameFinish`** (`GameEventInfo.yaml`): `gameId`, `color`, `fen`, `lastMove`,
  `isMyTurn`, `secondsLeft`, `opponent`, `speed`, `source`, `compat`, `status`, `winner`.
- **Accept**: `POST /api/challenge/{id}/accept` (already used: `acceptChallenge`), then the game stream
  (`streamGame`), as for a link today.
- **Decline**: `POST /api/challenge/{id}/decline` with `reason`: `generic`, `later`, `tooFast`,
  `tooSlow`, `timeControl`, `rated`, `casual`, `standard`, `variant`, `noBot`, `onlyBot`. Lichess shows
  the challenger the reason.

## 4. Design

### 4.1 One reconnecting stream class (`LichessStream`, LiChessConnector)

LI-06 built the reconnect for the game stream inside `LichessClient` (`openGameStream`, the 20 s
silence watchdog, the 1 → 15 s backoff, 4xx stops it, 5xx and no answer retry). The event stream needs
the same, so it moves into a small class used by both (C++ in OO style, as the owner asked in #92):

```cpp
class LichessStream : public QObject {   // one ndjson GET that stays open
    void open(QNetworkAccessManager&, const QNetworkRequest&);
    void close();                         // no reconnect
    bool lost() const;
signals:
    void line(const QByteArray& json);    // each non-empty line
    void connectionChanged(bool connected);
    void stopped(const QString& reason);  // Lichess said no (4xx): no more attempts
};
```

`LichessClient` owns two: the game stream (as LI-06, now through the class) and the event stream. The
LI-06 tests against `FakeLichess` must keep passing unchanged; the class gets its own tests for what
LI-06 tested on the game stream (closed, silent, keep-alive, 4xx stops, 5xx retries).

### 4.2 The event stream, always open

- **Opens** when `LichessClient` gets a token (`setToken`, `main.cpp` on login and profile switch), not
  in screenshot mode (`setOffline`). **Closes** on logout, a token change and a 401 (then the profile
  shows "Reconnect", as today).
- **Shared by today's users of the event stream**: `waitForGameStart` (our open challenge: it now waits
  for the `gameStart` on the open stream instead of opening its own, and the 3 s probe of the game
  stream stays as the fallback it is) and `watchForRematch` (a flag, no stream of its own). Opening a
  second stream would close the first (§3), so there must never be two.
- **Our own challenges** come back on the stream (`direction: out`): ignored here, the flows that sent
  them already track them (link, rematch, challenge by name).

### 4.3 Incoming challenges in `LichessClient`

```cpp
Q_PROPERTY(QVariantList incomingChallenges READ incomingChallenges NOTIFY incomingChallengesChanged)
Q_INVOKABLE void declineChallenge(const QString& id, const QString& reason = "generic");
static QVariantMap parseChallenge(const QJsonObject& challengeEvent); // tested
```

- One row per challenge to us, newest last: `id`, `challengerName`, `challengerRating` (with `?` when
  provisional), `challengerTitle`, `timeControlText` ("10+5", "3 days per move", "Unlimited"),
  `speed`, `rated`, `variant`, `ourSide` ("white", "black", "random": the other side of the
  challenger's `color`), `canPlayHere` (`compat.board` and the standard variant), `url`, `rematchOf`.
- `challenge` adds or updates a row (the same id after a reconnect: no duplicate), `challengeCanceled`
  and `challengeDeclined` remove it, and so does Accept or Decline from the app. A reconnect sends all
  current challenges again: the list is rebuilt from them.
- A rematch offer (`rematchOf` = our last game) is an incoming challenge like any other; the existing
  rematch bar in a finished game stays, and the rematch row just doesn't show twice there.

### 4.4 Accept and Decline

- **Decline** is challenge setup: QML calls `LichessClient.declineChallenge(id)` directly (CLAUDE.md
  rule 2). The bar's Decline sends `generic`; the Online page's row has a small menu with "Not now"
  (`later`), "Too fast" / "Too slow" / "Not this time control" (`tooFast`, `tooSlow`, `timeControl`),
  "Casual only" / "Rated only" (`casual`, `rated`), "Standard chess only" (`standard`).
- **Accept** may leave a game, so it is one call on `ChessConnector` (rule 1):
  `ChessConnector.acceptChallenge(id)`:
  - no game, or a finished one → `LichessClient::acceptChallenge(id)`; the game starts through
    `gameStarted` as for a link;
  - a robot or two-player game → `endGame()` (the robot game is autosaved) and accept;
  - a Lichess **correspondence** game → leave it (the stream stops; it goes on at Lichess and stays in
    Your games) and accept;
  - a **running real-time Lichess game** → refused (the bar has no Accept then).
  - `ChessConnector.CanAcceptChallenge` (bool) tells QML which case it is.
- The challenger may cancel while the player decides: the row disappears, and an Accept that comes too
  late gets Lichess's error ("Challenge not found") as an InfoBar.

### 4.5 The UI

- **The challenge bar** (`ChessGame.qml`, in the InfoBar column, so it shows over every page): the oldest
  waiting challenge, "TesterTwo (1712) challenges you · 10+5 · Rated · You play Black", **Accept**
  (or **Open on lichess.org**), **Decline**, and "+2 more" when there are more (opens the Online page).
  During a running real-time game: "… · Finish your game first: it waits on Lichess", Decline only.
  A Close button hides it until the next challenge arrives (the challenge stays on the Online page).
- **The Online page**: a "Challenges to you" card above Your games when there are any: one row each,
  with Accept / Open on lichess.org and Decline (with reasons).
- **Home**: the Play online card's description says "1 challenge waiting" when there is one.
- **Attention**: a new challenge (not one resent after a reconnect) plays `sounds/challenge.wav`
  (new, from `scripts/make_sounds.py`, with the sound settings like the others) and, if the window
  isn't active, flashes the taskbar button (`QWindow::alert(0)`: until the window is activated).

### 4.6 Your games, live

`gameStart` and `gameFinish` on the event stream call `refreshOngoingGames()` (once per event, and not
for the game being played here: that one is the game stream's). LI-04's list then follows games
started or ended elsewhere without opening the page again.

## 5. Implementation steps

1. **`LichessStream`** (needs #125 merged): the LI-06 reconnect logic as its own class, the game
   stream moved onto it, the LI-06 tests unchanged, new `LichessStreamTest` against `FakeLichess`.
   No behaviour change.
2. **The event stream, always open**: opened with the token, closed on logout / token change / 401;
   `waitForGameStart` and `watchForRematch` share it; `incomingChallenges` with `parseChallenge`;
   `declineChallenge`; `gameStart` / `gameFinish` refresh the ongoing games. Tests: parse rows (direct,
   open, rematch, unplayable, correspondence, provisional), and against `FakeLichess`: a challenge
   arrives, is canceled, the stream reconnects and resends it (one row), `direction: out` ignored,
   only one event stream ever open during a challenge wait and a rematch watch.
3. **Accept and the bar**: `ChessConnector.acceptChallenge` and `CanAcceptChallenge`; the challenge bar;
   the sound and the taskbar flash. Scenes (a seeded challenge, no network): `challenge` (bar on Home),
   `challenge-ingame` (bar in a running Lichess game: Decline only), `challenge-unplayable`.
4. **The Online page and Home**: the "Challenges to you" card with the decline reasons; Home's Play
   online description. Scene `online-challenges`.
5. **Docs and hand tests**: `docs/BL-10-ManualTests.md`, `CLAUDE.md`, `TASKS.md`, `PROGRESS.md`.

Steps 3 and 4 need step 2; step 2 needs step 1.

## 6. Testing

**Unit and fake-server tests** (steps 1–2): as listed. `FakeLichess` (LI-06) serves `/api/stream/event`
as well as the game stream.

**Screenshots**: the new scenes, light and dark, on an unlocked screen; every other scene unchanged.

**Manual** (owner, two Lichess accounts: the app's and a browser's):

| # | Scenario | Expected |
|---|---|---|
| 1 | Browser challenges the app's account (10+5, casual), app on Home | Bar within a second, sound; Accept → the game page, the game goes on in the browser |
| 2 | Same, app minimised | Taskbar flashes, sound; the bar is there when the window comes back |
| 3 | Decline from the bar; then from the Online page with "Too fast" | The browser shows the challenge declined, with the reason |
| 4 | The browser cancels while the bar shows | The bar goes |
| 5 | A Bullet challenge | Shown with Open on lichess.org and Decline, no Accept |
| 6 | Challenge during a running Rapid game in the app | Bar with Decline only and "Finish your game first"; after the game, Accept works if it still waits |
| 7 | Challenge during a robot game | Accept: the robot game is autosaved (Continue on Home), the Lichess game starts |
| 8 | Two challenges at once | The bar shows the first, "+1 more"; both on the Online page |
| 9 | Wi-Fi off for 30 s, a challenge sent meanwhile, Wi-Fi on | The challenge appears after the reconnect, once |
| 10 | A game started from the browser (e.g. the app's account accepts there) | Shows up in Your games on its own |
| 11 | Rematch offered after leaving the finished game | Arrives as a challenge with "Rematch" |

## 7. Risks and open questions

- **One event stream per token** (§3): anything else that opens one with the same token (a second copy
  of the app, another Board API tool) closes ours, and ours closes it. With both reconnecting they take
  turns every few seconds. The app can't tell this from a network problem; the backoff keeps it to a
  stream every 15 s at worst. A note in the log when the stream is closed right after opening, again
  and again. A second copy of the app with the same profile is the likely case: maybe later a
  single-instance check.
- **Rate limits**: one long-lived stream is what Lichess intends; reconnects back off. No polling is
  added.
- **Profiles**: only the active profile's account listens. Challenges to another profile's account
  wait on Lichess and show when that profile is used (the stream resends them on open).
- **A challenge for a game the Board API refuses after all** (compat said yes, accept fails): Lichess's
  error in an InfoBar, the row stays with Open on lichess.org.
- **Accept during a correspondence game** leaves its stream: the game is safe on Lichess and in Your
  games. If the owner prefers to stay, change it in step 3.
