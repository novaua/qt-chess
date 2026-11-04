# LI-02 Game Clocks and Lichess Speeds — Implementation Plan

| | |
| :---- | :---- |
| **Date** | 2026-11-03 |
| **Status** | Coded: all steps done. Hand tests open: [`LI-02-ManualTests.md`](LI-02-ManualTests.md) |
| **Branch** | `li02Plan_031126` (plan); each step on its own branch from `develop` |

## Context

There is no clock on the board today, in any mode. For Lichess games the stream sends the clocks (`wtime`/`btime`), but `LichessClient::handleGameStreamLine` ignores them, so a player can't see their time and flags without warning. The owner wants the clock to feel like **lichess.org's**, and wants it in **all three modes**: Lichess games, two players on one device, and games against the robot (the robot answers within about 3 s, so a clock makes those games a time challenge for the human).

What the Board API allows was checked on 2026-11-02 (see TASKS.md, LI-02): **Rapid, Classical and Correspondence, plus Blitz only in a challenge to a named user**. Bullet and UltraBullet are never possible online, but they are fine for local and robot games.

The owner chose:
- **Clock + Blitz by name:** a live clock everywhere; the challenge card names the modes and shows the player's rating; an optional Lichess username makes the Blitz presets work as a direct challenge.
- **Clock extras:** red when low, tenths under 10 s, a low-time sound, and a correspondence deadline if Lichess supports it.

LI-02 was listed "after LI-01" (variants). Nothing here depends on LI-01, so it can go first.

The first PR commits this plan as `docs/LI-02-ClockPlan.md` and updates TASKS.md. Each step after that gets its own branch from `origin/develop` and its own PR, as before.

## Lichess behaviour to copy

| What | Rule |
|---|---|
| Display | `MM:SS` with two-digit minutes (`03:00`, `00:45`), `HH:MM:SS` from one hour, tenths under 10 s (`00:09.4`). Tabular figures (`font.features: { "tnum": 1 }`). Checked against Lichess's source (`clockView.ts`). |
| Start | The clock runs only after both sides have made their first move (ply ≥ 2). |
| Increment | Fischer increment: added to the mover's time after each move. |
| Low time | Red (`Theme.critical`) below `min(60 s, max(10 s, start / 8))`; for a start under a minute `max(2 s, start / 5)` (`clockCtrl.ts`). The low-time sound plays once, when **our** clock crosses that line. |
| Flag | The side out of time loses, unless the other side can't mate: then it's a draw (as on Lichess). |
| Correspondence | `2 days 5 hours`, `One day`, then `05:30` (hours and minutes), then `45:10`. No tenths, never red, no sound (`corresClockView.ts`). Runs from the start. Online only. |

## Design

### `GameClock`: one C++ model for all modes (new, `ChessGui/ChessGui/gameclock.h/.cpp`)

- A QObject modelled on `MoveGate` (`movegate.h:40`). It takes an injectable time source, `setClock(std::function<qint64()>)`, so tests can use a fake clock.
- **State:**
  - start time and increment
  - remaining ms for each side, taken at the moment it was last set
  - the running side: white, black or none
- **API:**
  - `start(limitMs, incMs)`
  - `moveMade(bool whiteMoved, int ply)`: adds the increment and switches sides; starts running from ply 2
  - `setSnapshot(whiteMs, blackMs, running)`: Lichess's numbers
  - `pause()`, `resume()`, `stop()`
  - `remainingMs(bool white)`: computed from the snapshot and the elapsed time
- **Display:** `formatClock(ms)` (static) plus a `QTimer` that ticks while the clock runs (100 ms under 10 s, else 250 ms) and emits `changed()`. Each tick updates the texts `WhiteText` / `BlackText` and the flags `WhiteLow` / `BlackLow` and `Running`.
- **Signals:** `flagged(bool white)` (local games only), and `lowTime()` (our side only).
- **Tests:** a new `ChessQtTests/GameClockTest.cpp`, added to the test project the same way as `movegate.cpp`. It covers the formatter, the start rule, the increment, snapshots plus elapsed time, low time crossed once, flag, and pause/resume.

### ChessCore: mating material (new)

- `Board::CanMate(PieceColor)`, the rule of Lichess's `InsufficientMatingMaterial`: false for a lone king, K+N against a king and nothing but queens, and bishops on one square colour unless the other side has a pawn, a knight or a bishop on the other colour.
- New ChessTests cases. It decides whether a flag is a loss or a draw.

### `ChessConnector`

- Owns one `GameClock`, exposed as a `Q_PROPERTY Clock` (CONSTANT), plus `HasClock`.
- **Local and robot games:**
  - `startLocalGame` and `startComputerGame` take a time control as new trailing arguments (`limitSec`, `incSec`; 0 = no clock).
  - `rematch()` / new game keep the time control.
  - The clock switches when a move is shown on the board: `onHumanMoveDone` for the player, `playRobotMove` for the robot. A move held in the gate still counts for the robot.
  - `flagged` → `flagKing` + `recordResult` + `setResult`, with the text "<side> ran out of time". That is the same text and flag as `onLichessGameEnded`'s `loserText`. If the other side can't mate, the result is a draw.
  - Review (←/→) keeps the clock running, as on Lichess. The robot's answer doesn't wait for the review to end: as with a Lichess opponent it brings the board back to the live position, or else the robot would lose on time while the player reviews.
  - A flag during the piece choice takes the unfinished move back.
  - An engine failure pauses the clock: that isn't a win on time.
  - A stalemate ends a local game as a draw (found in step 2): `Game` only reports a mate, so the stalemated side would have lost on time.
  - A takeback vs the robot (`movePrev`) keeps the time spent, takes off the increments of the moves taken back (`moveTakenBack`, or a move and Undo over and over would add time for ever) and runs the clock of the side to move. Lichess gives the time back (`Rewind.scala`), but there the opponent must agree; here that would be free thinking time. After a flag it does nothing: a flag ends the game for good, as on Lichess (the flagged side has no time left to resume with), so `requestTakeback` refuses it and the Undo button is disabled. `GameClock::setTurn` ignores a flagged clock.
  - Leaving a robot game autosaves the remaining times. `UserManager::GameSaveInfo` gains `clockLimitSec`, `clockIncSec`, `whiteMs`, `blackMs`. Continue restores the clock paused, and it resumes once the page has been open for a second (`pageOpened`). This differs from Lichess, but a local game can be left.
- **Robot thinking time:** the level's `go movetime` is capped by the robot's clock (`UciConnector.cpp:399`): `min(levelMs, remaining / 2, max(100, remaining / 30 + inc / 2))`. The level otherwise stays as it is. The robot can still flag: the gate's 0.4 s counts on its clock, so a very long game without increment can run it out.
- **Online:**
  - `LichessClient` reads `clock.initial` / `clock.increment` (gameFull, already parsed for `RematchInfo` at `LichessClient.cpp:1065`), and `wtime` / `btime` from gameFull's state and from every gameState. It emits `clockUpdated(whiteMs, blackMs, whiteToMove, running)`.
  - The connector calls `setSnapshot`. Our own move switches the clock locally at once, with the increment; the echo then corrects it.
  - No local flag: Lichess ends the game (`gameEnded` "outoftime"), and until then the display stays at `00:00.0`. Check live whether Lichess ends it on its own (the Board API has no "flag" call).
- **Correspondence:** the Board API sends a usable `wtime`/`btime` for days-per-move games (`BotJsonView.scala`: the time left for the move), so `secondsLeft` isn't needed. A game with no clock at all sends the largest int, which is ignored.

### QML

- **`PlayerCard.qml`:**
  - A right-aligned clock label as the last child of its row, fed through GamePage's `SideCard` (`GamePage.qml:259`).
  - Properties: `clockText`, `clockRunning` (accent background like Lichess's running clock), `clockLow` (`Theme.critical`).
  - Hidden when `!ChessConnector.HasClock`.
  - Must fit a 200 px narrow card: the name elides first.
- **`TimeControlPicker.qml`** (new, shared): a Lichess quick-pairing style grid of tiles, "10+0" with "Rapid" under it, plus "No clock".
  - Local games: all speeds (Bullet 1+0 2+1, Blitz 3+0 3+2 5+0 5+3, Rapid 10+0 10+5 15+10, Classical 30+0 30+20).
  - The last choice is remembered in `AppSettings`.
- **`ComputerGameDialog` / `TwoPlayerDialog`:** a "Clock" row with the picker.
- **`OnlinePage` challenge card** (`OnlinePage.qml:246-309`):
  - The same picker replaces `tcPreset`. Real time shows Rapid and Classical tiles, plus Blitz tiles that are enabled only when a username is entered.
  - "Your Rapid rating: 1650" for the chosen mode (`UserManager.lichessRatings`).
  - A new optional **"Lichess username"** field. When it's filled, the challenge is a direct one. `challengeUser` (`LichessClient.cpp:838`, today only for rematch) becomes a public `Q_INVOKABLE challengePlayer(username, limitSec, incSec, days, color)`, which reuses the `directChallengeSent` / `waitForGameStart` flow.
  - `ChallengeDialog` gets a "Waiting for <name> to accept" variant without the link, and shows a declined challenge.
- **Low-time sound:** a new `sounds/lowtime.wav`, made by a small Python `wave` script next to `make_icons.py`, added to `qml.qrc`. Played on `Clock.onLowTime` with the existing `_sfxVolume` (`ChessGame.qml:258`).

## Steps (one PR each)

0. **Plan:** `docs/LI-02-ClockPlan.md` (this plan), plus TASKS.md (LI-02 rewritten: local clocks are added, Bullet/UltraBullet are local only).
1. **`GameClock` + `CanMate`:** the classes and their tests. No UI yet.
2. **Clocks in local and robot games:** the picker in both setup dialogs, the clock on the cards, the flag result, autosave/Continue, the robot's capped thinking time, and the low-time sound.
   - Scenes: `clock` (a two-player game with a clock running), `clock-low` (red, tenths), `computer` with the picker.
3. **Lichess clock:** stream parsing (`LichessClientTest`: gameFull/gameState with clocks → `clockUpdated`), connector snapshots, our move switching at once, and the correspondence deadline.
   - Scenes `clock-online`, `clock-correspondence`: like `takeback`, a fed gameFull with its clock and `wtime`/`btime`.
4. **Challenge card:** the picker with mode names and rating, the username → direct challenge with Blitz, and ChallengeDialog's waiting/declined variant.
   - Scenes: `online`, `online-empty` (updated), `online-blitz` (username entered), `online-challenge` (the dialog, waiting).
5. **Docs and hand tests:** `docs/LI-02-ManualTests.md`.
   - Lichess: Rapid open challenge with the clock, Blitz by username, a flag online, correspondence.
   - Local: a flag, a draw by flag against a lone king, robot Bullet, Continue with the clock.
   - Also CLAUDE.md (GameClock, TimeControlPicker), PROGRESS.md and the run-chess-app skill.

## Verification

- **Each step:**
  - Release build, then ChessTests + ChessQtTests pass (GameClockTest and the new LichessClient stream tests among them).
  - `scripts\Take-Screenshot.ps1 -Scene all` and `-Dark`, plus the new scenes, then `Compare-Screenshots.ps1`. Only the intended scenes may change; accept those baselines. Use an unlocked screen, or flag the dialog scenes.
- **Clock scenes:** a fake time source would make them repeatable. In screenshot mode the clock is frozen at fixed times (a `--screenshot` hook in `ScreenshotScene.qml` / `main.cpp`, like `restore`'s fixed backup times).
- **By hand (step 5):** the owner's round with real Lichess games, plus a local game run down to a flag.
