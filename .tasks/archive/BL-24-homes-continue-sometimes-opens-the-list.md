---
id: BL-24
title: "Home's Continue sometimes opens the list for a single Lichess game"
type: bug
status: done
priority: medium
---

# BL-24 Home's Continue sometimes opens the list for a single Lichess game

<!-- migrated from TASKS.md:152 (In Progress / Backlog) -->

- [x] BL-24 **Bug**: Home's Continue sometimes opens the list for a single Lichess game [Easy], **fixed (branch `bugs/continueSingleGame_191126`): fixes (1)–(3) below; LichessChallengeTest +1, `tst_lichess` test 9 continues during a refresh** (the owner, LI-04 hand test 3, 2026-11-19: seen a few times after restarting the app, not reproduced in 5+ tries).
  - **Expected:** with one unfinished game, Continue rejoins it directly.
  - **Seen:** sometimes the "Continue a game" page opened, with the one game on it.
  - **Cause, from the code (`chessconnector.cpp`, `continueOnlyGame()`):** while `LichessClient` reloads the list of Lichess games (`ongoingGamesLoading`), Continue doesn't decide and opens the list, even though the list it already has is right.
  - After a start the list reloads several times in the first seconds; the app log of 2026-11-19 20:00–20:03 shows it after each restart:
    - Home's first load (the first request after a start took 4.4 s);
    - the event stream opens and Lichess sends a `gameStart` for every game going on. The app takes that as a game started elsewhere and reloads 0.5 s later (`_ongoingRefresh`);
    - Home's `StackView.onActivated` reloads again.
  - Each reload is 80–150 ms, so a click on Continue soon after the card shows can fall into one.
  - **Fix:**
    - (1) `continueOnlyGame()` decides on the list it has once one has loaded, and opens the list only before the first load;
    - (2) a `gameStart` from the event stream for a game already in the list doesn't reload it;
    - (3) a log line when Continue opens the list, with why (local / online counts, loading).
  - **Tests:** `tst_lichess` (or a C++ test with `FakeLichess`): Continue while a reload is in flight still rejoins the only game; the event stream's opening `gameStart` lines don't trigger a reload.

## Resolution

### 2026-11-19 — BL-24: Continue opens the list for a single Lichess game (branch: bugs/continueSingleGame_191126)

#### Done
- `ChessConnector::continueOnlyGame()` gave up while the Lichess games reloaded (`ongoingGamesLoading`) and opened the Continue list, even with one game. It now decides on the last list. It opens the list only before the first list has come for the account: `LichessClient::ongoingGamesLoaded()`, reset when the token changes.
- The event stream's `gameStart` of a game already in Your games no longer reloads the list (`isOngoingGame`). Lichess sends one for every game going on when the stream opens, which made a needless reload 0.5 s after each start.
- A log line when Continue opens the list, with the counts and why.
- Tests:
  - `LichessChallengeTest::GameStartOfAListedGame_NoRefresh`: a listed game's `gameStart` doesn't reload, an unknown one does; the list stays "loaded" during a reload;
  - `tst_lichess` test 9: Continue during a reload joins the only game.

#### Checked
- ChessQtTests (Lichess) 115 passed; UI tests (Debug) 12 files, 0 failed.
