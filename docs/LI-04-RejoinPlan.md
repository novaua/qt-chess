# LI-04 Rejoin Unfinished Lichess Games — Implementation Plan

| | |
| :---- | :---- |
| **Date** | 2026-11-02 |
| **Status** | Plan |
| **Branch** | `rejoinGamesPlan_021126` (plan); each step on its own branch from `develop` |
| **Task** | LI-04 in `TASKS.md` (owner's manual check of BL-14 step 5, bugs 2 and 4; owner's UI decision of 2026-11-02) |

## 1. Problem

Leaving a Lichess game in the app (Home, Escape, closing the window, a crash) doesn't end it: the game
goes on at lichess.org, the opponent sees us disconnect, and our clock runs. Today:

1. **Nothing warns** when leaving a running online game (`ChessGame.qml` `leaveGame()` →
   `ChessConnector.endGame()` just stops the stream). Only "Switch user" says the game keeps running.
2. **The only way back** is to paste the game's link or id into "Join a game" in the "Play online"
   dialog (`LichessClient.joinFromUrl`, fixed in #109). The player has to know or find the id.
3. **Correspondence games** (days per move, "async") are the main reason to come back to a game later,
   but there is no list of them and no way to create one with days per move (the dialog offers
   "Unlimited" or a few real-time presets).
4. Home's **Continue** only knows the saved game vs the robot.

## 2. Goals (owner's decision, 2026-11-02)

| | |
|---|---|
| **An Online page** | "Play online" opens a page (pushed like Profile and Settings) with the ways to play online. It replaces `LichessGameDialog`. |
| **Your games** | The page lists the player's unfinished Lichess games. **Continue** on a row rejoins the game. The player never types an id. |
| **Continue on Home** | A saved game vs the robot → continue it (as today; **it has priority**, because Continue is its only way back). Otherwise, unfinished Lichess games → open the Online page at "Your games". Otherwise the card stays hidden. |
| **Warn when leaving** | Leaving a running online game asks first: the game goes on and the clock runs; Resign / Leave / Stay. |
| **Correspondence** | "Days per move" in the new-game settings, so async games can be created. |

Out of scope: notifications for "your turn" or incoming challenges (BL-10), a game clock (LI-02), takebacks
(LI-03), premoves (LI-05), Chess960 / Crazyhouse (LI-01).

## 3. What Lichess offers

Checked in Lichess's API spec (`lichess-org/api`, 2026-11-02):

- **`GET /api/account/playing?nb=…`** ("Get my ongoing games", OAuth): real-time **and correspondence**
  games, **most urgent first**, at most 50 (default 9). Per game: `gameId`, `fullId`, `color`, `fen`,
  `lastMove`, `isMyTurn`, `secondsLeft`, `opponent` (`id`, `username`, `rating`; `ai` level for games vs
  Lichess AI), `speed`, `variant`, `perf`, `rated`, `source`, `hasMoved`. And `nbMyTurn` for the whole list.
- **`GET /api/board/game/stream/{id}`**: the Board API stream we already use. Its first line is
  `gameFull` with all moves, which BL-14 step 5 already shows as one resync (no slides, one sound).
  So rejoining = `streamGame(id)`.
- **`GET /api/stream/event`**: "When the stream opens, all current challenges and games are sent"
  (`gameStart`), and `gameFinish` when one ends. It could keep the list live later, with BL-10. But
  "only one global event stream can be active at a time" per token, and the app already uses it while
  waiting for a challenge and for rematch offers. A live list must share that one stream, so this
  plan only fetches the list.
- **Board API restrictions**: Rapid, Classical and Correspondence only (Blitz only in direct challenges
  and games vs AI). **Open question, check in step 1**: can the Board API stream a game that was started
  on lichess.org itself (e.g. a Blitz game from the lobby)? If it can't, such a row gets **"Open on
  lichess.org"** instead of Continue.

## 4. Design

### 4.1 `LichessClient`: the ongoing games

```cpp
Q_PROPERTY(QVariantList ongoingGames READ ongoingGames NOTIFY ongoingGamesChanged)
Q_INVOKABLE void refreshOngoingGames();      // GET /api/account/playing?nb=50; no-op without a token
Q_INVOKABLE void rejoinGame(const QString& gameId); // streamGame(id): gameFull → gameStarted
static QVariantList parseOngoingGames(const QJsonObject& playing); // tested
```

- One row per game, built in C++ (tested), so QML only shows it:
  `gameId`, `color` ("white"/"black"), `opponentName` (`opponent.username`; an anonymous or AI opponent
  has `id: null`, and an AI also has `ai`, its level 1–8: "Lichess AI level N"),
  `opponentRating`, `isMyTurn`, `secondsLeft`, `timeLeftText` ("3 min", "2 days", "" if no clock),
  `speed`, `variant`, `canPlayHere` (standard variant and a speed the Board API plays; false →
  "Open on lichess.org", `https://lichess.org/{gameId}`), `lastMove`.
- In Lichess's order (most urgent first). `nbMyTurn` is not needed: QML counts `isMyTurn`.
- A 401 goes through `checkAuthExpired` like every other call. On a network error, the list is kept
  and the page shows the error.
- Screenshot mode: no network. `main.cpp` seeds the list for the scenes from a fixed JSON
  through `parseOngoingGames` (a `setOngoingGames` for tests and scenes).
- Rejoining is challenge setup, so QML may call `LichessClient` directly (CLAUDE.md rule 2). The game
  itself starts in `ChessConnector` through `gameStarted`, as today.

### 4.2 The Online page (`OnlinePage.qml`, replaces `LichessGameDialog.qml`)

Pushed over Home by `root.openOnline()` (`objectName: "online"`, back button and Escape like Profile).
`SectionCard`s, as on the Profile page:

1. **Not connected**: the same message and "Open profile" button as today's dialog, and nothing else.
2. **Your games**: `ongoingGames` as rows. Each row: the opponent's avatar placeholder and name (and
   rating), "You play White", a status chip ("Your turn" in the accent colour / "Waiting"), the time
   left, the speed. Then **Continue** (or **Open on lichess.org**). A **Refresh** button in the card's
   header; "Loading…" while fetching; "No unfinished games." when empty.
   Maybe later: a small board from `fen`. Not in this task.
3. **New game**: today's dialog settings (game type, time control, side, Create challenge), plus
   **Days per move** (1, 2, 3, 5, 7, 10, 14) as a third time-control choice next to Unlimited and Real
   time. The real-time presets stay as they are here; LI-02 deals with the speeds the Board API refuses.
4. **Join by link or id**: today's field and Join button, as a fallback.

- The pending challenge (link to share, Cancel) stays in `ChallengeDialog`, opened over the page
  as today; `playOnline()` opens the page, then the dialog if a challenge is pending.
- `refreshOngoingGames()` when the page opens and when it's shown again (popped back to).
- A game that starts (created, joined or rejoined) replaces the page by the game page through
  `route()`, as the dialog does now.

### 4.3 Home's Continue

- `ChessConnector.CanContinue` and `continueGame()` stay the robot's (unchanged).
- The card: `visible: ChessConnector.CanContinue || LichessClient.ongoingGames.length > 0`
  (the second only when Lichess is connected).
- Click: robot save → `ChessConnector.continueGame()`; else → `root.openOnline()` (UI navigation, not a game step).
- Description: the robot's as today ("vs Robot, level 3 · move 12"), or "Lichess: 2 games · your turn
  in 1". With both, the robot's description plus "· 2 Lichess games in Play online", so the online
  ones aren't hidden.
- `refreshOngoingGames()` when Home is shown (signed in, not in screenshot mode), and after an online
  game ends (the list changed).

### 4.4 Leaving a running online game

- `leaveGame()` for an online game that isn't over opens **`LeaveOnlineDialog`**: "The game goes on at
  lichess.org and your clock is running. You can come back from Continue or Play online." Three
  buttons: **Leave** (default), **Resign** (`ChessConnector.resignGame()`, then leave), **Stay**.
  `ConfirmDialog` has two buttons, so this is a small dialog of its own (or `ConfirmDialog` gets an
  optional middle button: decide in step 4 with the screenshot).
- **Closing the window** during a running online game: `onClosing` asks the same (Leave quits).
- "Switch user": its text already says the game keeps running; add "You can come back from Continue."
- A finished game or a game vs the robot / two players: no question, as today.

## 5. Implementation steps

1. **`LichessClient` ongoing games** + `LichessClientTest` (parse: real-time and correspondence rows,
   AI opponent, anonymous opponent, Chess960 → `canPlayHere` false, empty list). **Live check with the
   owner's account**: create two games in the browser (a correspondence game started in the app, a
   game started on lichess.org) and confirm `/api/account/playing` lists them and the Board API streams
   them. This settles the open question in §3.
2. **Online page** replacing the dialog, with Your games and Continue (rejoin), New game (days per move),
   Join. `ChessGame.qml`: `openOnline()`, `_page "online"`, Escape, `playOnline()`. Screenshot scenes:
   `online` (now the page, connected, with the seeded games), `online-empty`, `online-disconnected`.
3. **Home Continue** for online games (robot priority); scenes `continue-online` and `continue` (robot
   plus online games).
4. **Leave warning** (Home/Escape and window close); scene `leave-online`.
5. **Docs**: `CLAUDE.md` (OnlinePage in the QML list, `LichessClient.ongoingGames`), `PROGRESS.md`, `TASKS.md`.

Steps 2–4 need step 1. Steps 3 and 4 don't depend on each other.

## 6. Testing

**Unit tests**: `parseOngoingGames` (step 1); the existing `LichessClientTest` and BL-14 tests still pass.

**Screenshots**: the new scenes above (light and dark, narrow and 1400×900). Every other scene unchanged.

**Manual** (owner, real Lichess account and a second player in a browser):

| # | Scenario | Expected |
|---|---|---|
| 1 | Start a game in the app, play 2 moves, Home | Warning (Leave / Resign / Stay); Leave → Home shows Continue "Lichess: 1 game" |
| 2 | Continue on Home | Online page, the game listed with "Your turn" or "Waiting" |
| 3 | Continue on the row | The game page opens in the current position at once (one sound), and the game goes on |
| 4 | Close the window during a game, start the app again | The close asks first; after restart Continue → Online page → the game |
| 5 | A saved robot game and an online game | Continue resumes the robot game; its text mentions the Lichess game; Play online lists it |
| 6 | Create a correspondence game (2 days per move), leave, come back the next day | Listed with "1 day" left; Continue rejoins |
| 7 | A game started on lichess.org (browser) | Listed; Continue rejoins, or "Open on lichess.org" if the Board API can't play it |
| 8 | Opponent resigns while we're away | Gone from the list after Refresh |

## 7. Risks and open questions

- **Games started outside the app** (§3): if the Board API refuses them, only "Open on
  lichess.org". Known after step 1.
- **Real-time games and the clock**: a rejoined Rapid game may have little time left, and there is no
  clock on the board (LI-02). The row shows `timeLeftText`, so the player knows before continuing.
- **Rate limits**: `/api/account/playing` once per page open / Home show is well within Lichess's
  limits. No polling.
- **Many profiles, one Lichess account each**: the list belongs to the active profile's token; switching
  user refreshes it (`activeUserChanged` already sets the token).
- **Chess960 games** in the list can't be played here until LI-01 → `canPlayHere` false.
