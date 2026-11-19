---
id: LI-04
title: "Rejoin unfinished Lichess games"
type: task
status: done
priority: medium
---

# LI-04 Rejoin unfinished Lichess games

<!-- migrated from TASKS.md:266 (Lichess Improvement Backlog) -->

- [x] LI-04 Rejoin unfinished Lichess games [Medium] (done 2026-11-19) — plan: [`docs/LI-04-RejoinPlan.md`](docs/LI-04-RejoinPlan.md). **Coded (#111–#114): the Online page with Your games and Continue, Days per move, Home's Continue, the leave warning. The owner's hand tests [`docs/LI-04-ManualTests.md`](docs/LI-04-ManualTests.md) (rewritten in #197 for the app after BL-10, BL-17, BL-19, LI-02 and LI-01A) all passed on 2026-11-19; the optional 4b.3 and 8.4 not run. Found: BL-23, LI-10.** (owner's manual check of BL-14 step 5, bugs 2 and 4). Leaving an online game (Home, closing the window, a crash) leaves the game running on Lichess. The opponent sees us disconnect at once, and our clock runs. Today the only way back is pasting the game id into "Join a game" (the `…/game/<id>` link was rejected; fixed on `lichessColorJoinFix_021126`), and nothing warns when leaving. Lichess keeps the game; the Board API can stream it again (`gameFull` with all moves: the step-5 resync). To do:
  - **List the games**: `GET /api/account/playing` ("Get my ongoing games", real-time and correspondence, most urgent first) gives `gameId`, `color`, `opponent`, `isMyTurn`, `secondsLeft`, `speed`, `variant`, `lastMove`. The event stream (`/api/stream/event`) also sends a `gameStart` for every ongoing game when it opens.
  - **Owner's decision (2026-11-02): an Online page**, which replaces the "Play online" dialog. "Play online" on Home opens a page (pushed like Profile / Settings) that gathers the ways to play online:
    - **"Your games"**: the unfinished games from `/api/account/playing`, one row each (opponent, colour, your turn / waiting, time or days left, speed). **Continue** on a row rejoins the game (`streamGame(id)` → `gameFull` → the step-5 resync), and the player never types an id. The list refreshes when the page opens; it's empty with a short note when there are none.
    - **New game**: what the dialog has today: the challenge settings (game type, time control, side), Create challenge, and the pending challenge (link to share, Cancel).
    - **Join by link or id**: kept as a fallback (it works since `lichessColorJoinFix_021126`).
    - Room for more ways to play online later: correspondence (below), challenge a Lichess user by name (Blitz is allowed there, LI-02), incoming challenges (BL-10).
  - **Home's "Continue game"** covers both kinds:
    - a saved game vs the robot → continue it, as today. **The robot has priority** when both exist, because Continue is the only way back to a robot game;
    - otherwise, if Lichess has unfinished games → open the Online page at "Your games";
    - otherwise the card is hidden, as today.
    - The card's description says which one ("vs Robot, level 3" / "Lichess: 2 games, your turn in 1"). Its visibility needs the ongoing-games count: fetch `/api/account/playing` when Home opens (signed in only, not in screenshot mode) and keep the count in `LichessClient` (a property QML reads; `ChessConnector.continueGame()` keeps the robot part).
  - **Warn when leaving** a running online game: "The game continues on Lichess and your clock is running. You can come back from Continue or Play online." with Resign / Leave / Stay.
  - **Correspondence ("async") games**, days per move, can last days. The Board API supports them (`days` in `addTimeControl`) and they're the main reason for a list. Offer "Days per move" in the dialog (1, 2, 3, 5, 7, 10, 14), and notify about "your turn" (overlaps BL-10).
  - Our side is always a logged-in account (the Board API needs the token). An anonymous opponent's game lives as long as their browser session, so rejoining works while they're still there.

## Resolution

### 2026-11-02 — Lichess: the chosen side and the join link (branch: lichessColorJoinFix_021126)

From the owner's manual check of BL-14 step 5 (scenario 1 passed apart from these; scenario 3, Fool's mate, passed).

#### What was done
- **Bug 1, "Black" selected but the game started as White**: Lichess's `POST /api/challenge/open` has **no `color` parameter**. The app log shows every challenge answered with `"color":"random"` and a random `finalColor`, and the app always played `finalColor`. The side is chosen when a player **accepts** an open challenge: `POST /api/challenge/{id}/accept?color=white|black` ("only valid if this is an open challenge", Lichess API spec). Now:
  - `createOpenChallenge` no longer sends `color` and takes our seat with `accept?color=<chosen>` (`waitForGameStart(id, true, color)`); "Random" accepts without a colour (Lichess's `finalColor`);
  - after a restart, a pending challenge is resumed with `resumeOpenChallenge(id, friendUrl)`: we play the side the friend's saved link (`…?color=white`) doesn't;
  - joining by link takes the side in the link (`joinFromUrl`: `…?color=black`, `…/<id>/black`).
- **Bug 4, the `https://lichess.org/game/<id>` link was rejected** ("Invalid URL or game ID"), although it's the form Lichess gives for open challenges (and the one we show to the friend). `gameIdFromUrl` accepts `/game/`; `colorFromUrl` reads the side. The dialog makes one call, `LichessClient.joinFromUrl(text)`.
- Tests: `LichessClientTest` +2 (`GameIdFromUrl_ChallengeLink_ReturnsId`, `ColorFromUrl_PlayerLinks_Test`). Debug and Release build; ChessTests 111, ChessQtTests 121 pass in both; `online` and `home` screenshots unchanged.
- Not verified against Lichess itself (needs the owner's account and a second player): see the checks below.
- **Backlog** (`TASKS.md`): bug 2 (no way back to a game after leaving it, no warning; correspondence games) and the "list of unfinished games" part of bug 4 → **LI-04**; bug 3 (premove) → **LI-05**. **LI-02 corrected**: the Board API allows only Rapid, Classical and Correspondence (Blitz only in direct challenges), so Bullet and UltraBullet are impossible for this app; the dialog's 1+0, 3+2 and 5+3 presets for open challenges need checking.

#### Where to continue
- **Owner's check**: create a challenge as Black, open the friend's link (it ends `?color=white`) in the browser; the app must play Black (board turned). Then as White, and Random a few times. Join by the `https://lichess.org/game/<id>` link and by a `…?color=black` link.
- LI-04 needs the owner's decision on where the unfinished games are listed.

### 2026-11-02 — LI-04 plan: rejoin unfinished Lichess games (branch: rejoinGamesPlan_021126)

#### What was done
- The owner checked #109 live: a challenge created as Black is played as Black, and rejoining by link or id works.
- **Plan written**: [`docs/LI-04-RejoinPlan.md`](docs/LI-04-RejoinPlan.md), following the owner's decision.
  - An **Online page** replaces the "Play online" dialog, with these sections: Your games (Continue rejoins, no id), New game (with days per move), and Join by link.
  - Home's **Continue** resumes the robot game first, otherwise opens the Online page.
  - A **warning** when leaving a running online game (Leave / Resign / Stay, also when the window is closed).
- Lichess facts it relies on, from the API spec:
  - `GET /api/account/playing` lists real-time and correspondence games (most urgent first) with `isMyTurn`, `secondsLeft` and the opponent (player / anonymous / AI);
  - the event stream sends all current games when it opens, but only one event stream may be open per token;
  - rejoining is `streamGame(id)` → `gameFull`, which BL-14 step 5 already shows as one resync.

#### Where to continue
- **Step 1** (new branch from `develop`): `LichessClient` ongoing games + tests, then a **live check with the owner's account**: does the Board API stream a game started on lichess.org itself? The answer decides between Continue and "Open on lichess.org" for such games.

### 2026-11-02 — LI-04 step 1: the unfinished Lichess games in `LichessClient` (branch: ongoingGames_021126)

#### What was done
- `LichessClient.ongoingGames` (with `ongoingGamesLoading` / `ongoingGamesError`, one `ongoingGamesChanged`):
  - `refreshOngoingGames()` fetches `GET /api/account/playing?nb=50`. A newer refresh replaces one in flight. On an error the list is kept and the error is set; a 401 goes through `checkAuthExpired` as for every call.
  - A token change (another profile, or a disconnect) clears the list.
- `parseOngoingGames()` builds one row per game, in Lichess's order (most urgent first): `gameId`, `color`, `opponentName` (the username, "Anonymous", or "Lichess AI level N"), `opponentRating`, `opponentIsAi`, `isMyTurn`, `secondsLeft` (-1: no clock), `timeLeftText` ("9 min", "2 days"), `speed`, `source`, `variant`, `canPlayHere`, `lastMove`, `fen`, `rated`.
- `canPlayHere` follows the Board API's restrictions:
  - **yes** for Rapid, Classical and Correspondence, and for Blitz from a direct challenge (`source` "friend") or vs Lichess AI;
  - **no** for Bullet, UltraBullet and lobby Blitz, and for any variant but standard (LI-01).
- `rejoinGame(id)` = `streamGame(id)` → `gameFull` → `gameStarted` (the step-5 resync).
- `main.cpp` fetches the list together with the account when the token is set (not in screenshot mode), and again 3 s after a Lichess game ends. The log lists each game ("LichessClient: ongoing games: N" and one line per game), which is the live check below.
- Tests: `LichessClientTest` +4 (player / anonymous / AI / Chess960 rows, the speeds the Board API plays, an empty list, `timeLeftText`). Debug and Release build; ChessTests 111, ChessQtTests 125 pass in both. The `home`, `online` and `profile` screenshots are unchanged (no UI yet).

- **Live check (owner, 2026-11-02)**:
  - **Listing works.** The log listed 3, then 4 unfinished games: all correspondence "Unlimited", `source` "friend", against the owner's second account. "my turn" / "their turn" was right, and the time left was empty (no clock).
  - **Joining works.** All four were joined by id and moves were posted, as White in two and as Black in two (`accept` answers 404 for a started game, then the stream opens; `rejoinGame` goes straight to the stream).
  - **Not tested:** a game started on lichess.org itself (lobby). `canPlayHere` stays as the Board API's rules say: lobby Rapid and Classical yes, lobby Blitz no.

#### Code review (PR #111 comments, same branch)
10 findings: 9 fixed, 1 declined.
- **Fixed:**
  - the list is refreshed when an online game is left;
  - a 401 reads "Lichess sign-in expired";
  - a new refresh clears the old error;
  - `setOngoingGames` cancels a refresh in flight;
  - one cancel helper, and a token change always resets the list;
  - Blitz from an API challenge or bulk pairing (`source` "api") is playable;
  - `rejoinGame` ignores an empty id;
  - `timeLeftText` shows "47 h" rather than "1 day" and "119 s" rather than "1 min";
  - a test checks the row count first.

  New test: a token change clears the list. ChessQtTests 126 pass in Debug and Release.
- **Declined:** a list model and separate change signals. At most 50 rows refreshed now and then, like `UserManager.users`; to revisit in step 2 if the page shows a cost.

#### Where to continue
- **Step 2** (after this is merged): the Online page.
- When convenient: rejoin a Rapid game from the lichess.org lobby, to confirm the lobby case.

### 2026-11-02 — LI-04 step 2: the Online page (branch: onlinePage_021126)

#### What was done
- **`OnlinePage.qml` replaces `LichessGameDialog.qml`** (removed). "Play online" on Home pushes it (`root.openOnline()`; Back and Escape pop it, like Profile). Its sections:
  - **Not signed in** (or the sign-in expired): only a card that says so, with "Open profile". Before, "Play online" jumped straight to the profile.
  - **Your games**: `LichessClient.ongoingGames`, refreshed whenever the page is shown (`StackView.onActivated`) and by a Refresh button.
    - Each row: the side we play, the opponent (rating), speed · side · time left, "Your turn" (accent) or "Waiting".
    - **Continue** (`LichessClient.rejoinGame`, highlighted on our turn), or **Open on lichess.org** for a game the Board API can't play (`canPlayHere` false: Bullet, lobby Blitz, Chess960).
    - The subtitle says "Loading…", the error, "No unfinished games…", or "4 games · your turn in 2".
  - **Challenge a friend**: today's settings plus **Days per move** (1–14, Lichess's values) as a third time control (`createOpenChallenge(…, days)`). **Chess960 is no longer offered**: it is broken until LI-01, as `TASKS.md` suggested.
  - **Join a game**: the link / id field (Enter joins).
- Errors of a challenge, join or rejoin show in an InfoBar at the top of the page. A challenge waiting for the friend is still `ChallengeDialog`, opened over the page (also when "Play online" resumes a saved one).
- Going **Back while a join / rejoin / challenge is starting** stops it (`StackView.onRemoved` → `stopStream()`), as the dialog's Cancel did. This doesn't happen when a game did start: `route()` replaces the page then (`ChessConnector.InGame` is already true).
- **Screenshot mode never calls Lichess**: `LichessClient::setOffline(true)` makes `refreshOngoingGames()` a no-op (the scenes' token is a placeholder). New scenes, all in `all`:
  - `online`: four seeded games (`ScreenshotScene` calls `LichessClient.setOngoingGames(shot.ongoingGames)`);
  - `online-empty`;
  - `online-disconnected`.
- Docs: `CLAUDE.md` (OnlinePage, ChallengeDialog) and the run-chess-app skill's scene list.
- Debug and Release build; ChessTests 111, ChessQtTests 126 pass. Screenshots: 53 compared, 0 failed after accepting the new `online*` baselines (the old `online` showed the dialog) and `about*` (build timestamp).

#### Code review (PR #112 comments, same branch)
9 findings: 8 fixed, 1 declined.
- **Fixed:**
  - **Back didn't stop a Join**: the accept POST wasn't kept, so `stopStream()` couldn't drop it, and its answer started the game over Home. It is now `_acceptReply`, aborted by `stopStream()`. Checked against lichess.org with a temporary hook: join then stop after 5 ms → no game stream request; without the stop → the stream opens.
  - **The buttons stayed disabled** when the game stream ended without a game: an expired token (the auth handler aborts it, "canceled", which was ignored), or Lichess closing the stream before `gameFull`. The stream now reports both as `networkError` ("Lichess sign-in expired" / "Lichess closed the game before it started"). The page also resets when it is shown again, or when the account's connected state changes.
  - **The list stayed empty** when the sign-in completed while the page was open: it now refreshes when `lichessConnected` changes (not on every `lichessChanged`, which a ratings update fires too).
  - **The error bar** was inside the scrolling content (out of sight after scrolling to Join): it now floats under the header.
  - **Join parsed the link twice**: one `joinFromUrl` call, as the old dialog did.
  - **`openOnline/closeOnline` were a third copy**: `_openPage(name)` / `_closePage(name)` with `_pushedPages`, and Escape closes any pushed page.
  - **`setOffline` covered only the games refresh**: `refreshAccount` too, and `main.cpp`'s own screenshot-mode checks for both are gone.
- **Declined:** "Continue should go through `ChessConnector` (CLAUDE.md rule 1)". A rejoin is Lichess challenge setup like "Join a game" (`LichessClient.joinFromUrl`, called from QML before this PR too). The game itself starts in `ChessConnector` through `gameStarted`, and rule 2 says not to forward `LichessClient`'s API through `ChessConnector`. This was decided in the plan (§4.1); the owner can overrule it.
- ChessTests 111, ChessQtTests 126 (Debug and Release); 53 screenshots unchanged; no QML warnings in the scene logs.

#### Board previews (owner's request, same branch)
- The owner's check: **Continue rejoins the game** ✓. The owner pointed at Lichess's own "Current games" page, which shows each game as a small board ("Ваш ход" = your turn).
- "Your games" is now a **grid of cards** like that (two columns from 440 px, else one). Each card has:
  - a **`BoardPreview`** (new `BoardPreview.qml`: a still board from the row's `fen`, the game board's squares, pieces and last-move tint, turned for Black). A click on the board continues too;
  - the opponent (rating) and "Your turn" / "Waiting";
  - speed · time left (the board shows our side; the accessible name still says it);
  - Continue or Open on lichess.org.
- Card and board sizes come from the page's content width. Binding them to the GridLayout's own width laid everything out in one column with the text over the boards, because that width isn't known yet when the cards are sized.
- The `online` scene's seeded games got positions and last moves; `online` / `online-dark` baselines re-accepted (53 compared, 0 failed). ChessTests 111, ChessQtTests 126 pass.
- The Debug build's link of `LiChessConnector.dll` failed (LNK1168): the owner's Debug `ChessGui.exe` was running. This round has no C++ change in that library; the Release build is complete.

#### Second code review (the owner's `/code-review medium`, same branch)
9 findings: 8 fixed, 1 left for the owner.
- **Fixed:**
  - **A failed accept went on to the stream** and hid Lichess's reason: the stream now opens only after a success or a 404 (a started game); otherwise Lichess's own error, or "Lichess sign-in expired";
  - a failed stream reports its error once (`errorReported`);
  - **Real time offered 1+0, 3+2 and 5+3**, which an open challenge can't be for a Board API player (Bullet, Blitz): now 10+0, 10+5, 15+10, 30+0, 30+20 (Rapid, Classical);
  - **the seven "Days per move" buttons overflowed a narrow window**: labels 1 … 14, then "days per move";
  - **a sign-in change re-enabled the buttons without cancelling** what was starting: `_dropStarting()` stops it, as Back does;
  - **up to three refreshes for one action**: the page doesn't refresh while one runs, and the sign-in change leaves it to `main.cpp` (new token);
  - `SectionCard.contentWidth` instead of hard-coding its margin;
  - `setOngoingGames` isn't callable from QML any more: `main.cpp` seeds the screenshot scene's games after the token.
- **Left for the owner:** start / stop of a rejoin through `ChessConnector` (CLAUDE.md rule 1), as in the first review.
- ChessTests 111, ChessQtTests 126 (Debug and Release). Screenshots: all 6 `online*` scenes match. The 12 dialog scenes failed only because **the Windows screen was locked** (`LogonUI` running): popups don't draw in the hidden window then. The same dialog renders correctly offscreen, and no dialog uses `SectionCard`, so their baselines were not touched.

#### Where to continue
- **Owner's check** (Release build of this branch):
  - Play online: your unfinished games are listed as boards; Continue (or a click on a board) opens it in the current position;
  - Back and Escape return to Home;
  - "Days per move" creates a correspondence challenge (Lichess shows "2 days" etc.);
  - not signed in: the page says so and Open profile works.
- **Step 3**: Home's Continue (the robot first, otherwise the Online page). **Step 4**: the warning when leaving a running online game.

### 2026-11-03 — LI-04 step 3: Home's Continue for online games (branch: homeContinueOnline_031126)

#### What was done
- **Home's Continue card** (`HomePage.qml`) now also covers the unfinished Lichess games (`LichessClient.ongoingGames`, only while Lichess is connected):
  - **A saved robot game has priority**: "Continue game" resumes it as before (`ChessConnector.continueGame()`). If there are Lichess games too, its description adds them: "vs Robot · Level 3 · 4 Lichess games in Play online".
  - **No robot game, Lichess games**: "Continue online game(s)", "4 Lichess games · your turn in 2" (or "· your turn", "· waiting for your opponent(s)"). A click opens the Online page (new signal `onlineGamesRequested` → `root.openOnline()`; unlike "Play online" it doesn't reopen a pending challenge).
  - **Neither**: the card stays hidden.
- Home **refreshes the list when it is shown** (back from a game, Settings, the profile, the Online page), unless a refresh is already running. It isn't shown during a game, so the list is fetched at most once per return to Home.
- `ChessConnector` is unchanged: navigating to a page isn't a game step (CLAUDE.md rule 1 is about game flows).
- **Screenshot scenes** (`main.cpp` seeds the `online` scene's four games and a linked Tester for them):
  - `continue`: the robot game and the Lichess games (new baseline: the description and the Lichess rating in the header);
  - `continue-online` (new, in `all`): only the Lichess games.
- Release builds; ChessTests 111, ChessQtTests 126 pass. Screenshots: 53 compared, only `continue*` changed (accepted), `continue-online*` new. Debug didn't link: the owner's running Debug app held `LiChessConnector.dll` (no C++ change there).

#### Next
- LI-04 step 4: the leave warning (Leave / Resign / Stay, also on window close); step 5: docs.

### 2026-11-03 — LI-04 step 4: the leave warning (branch: leaveWarning_031126)

#### What was done
- **`LeaveOnlineDialog.qml`** (new, like `ImportConflictDialog`: three buttons): "Leave the game? The game goes on at lichess.org and your clock is running. You can come back to it from Continue or Play online." **Leave** (default, Enter), **Resign**, **Stay** (Esc). A small dialog of its own rather than a middle button on `ConfirmDialog` (plan §4.4: decided with the screenshot).
- **Home and Esc** during a running Lichess game (`_onlineRunning`: in game, online, no result) open it (`ChessGame.qml` `leaveGame()`; the old body is `_doLeaveGame()`). A finished game, a robot or two-player game: no question, as before.
- **Closing the window** during a running Lichess game asks the same (`onClosing`, title "Quit during the game?", **Quit** instead of Leave). Not in screenshot mode.
- **Resign** is `ChessConnector.resignAndLeave()` (new, after the code review): it resigns and calls `endGame()` **once Lichess has ended the game**, so the loss counts in the statistics (`onLichessGameEnded` records it; stopping the stream at once would lose it). After 5 s without an answer it leaves anyway (then only Lichess has the loss). Quit closes the window when the game has been left. Home or closing again while it waits doesn't ask twice (Home leaves at once; closing quits when it's through).
- Leave is `ChessConnector.endGame()` as before.
- **Switch user** during an online game adds "You can come back from Continue or Play online."
- **Screenshot scene `leave-online`** (in `all`): the `game` scene (1. e4 e5) with the dialog open (an online game needs Lichess, so it's over a local game).
- Release builds; ChessTests + ChessQtTests 237 pass. Screenshots: 55 compared, 0 failed; `leave-online*` new (accepted). No QML warnings in the scene logs.
- Not checked live: the close and Resign paths need a running Lichess game (owner's manual tests 1 and 4, plan §6).

#### Code review (the owner's `/code-review medium`, same branch)
10 findings: 8 fixed, 2 declined.
- **Fixed:**
  - **Resign strung game steps together in QML** (rule 1): a timer, waiting for `GameResult`, then `endGame()`. Now one call, `ChessConnector::resignAndLeave()`, with its own timer (`_leaveAfterResign`). This also fixes:
    - **a stale wait ended the next game**: Switch user (or anything else that calls `endGame()`) during the wait left the QML timer running; `endGame()` now stops it;
    - **leaving inside the `gameResultChanged` signal**: the leave is queued (`start(0)`) until after `onLichessGameEnded`;
    - **the quit/home logic in two places**: `_leaveOnline(quit, resign)`.
  - **Home during a resign-then-quit quit the app**: Home now leaves for Home and drops the quit.
  - **The warning stayed open after the game ended** ("your clock is running"): it closes when `_onlineRunning` turns false.
  - **Closing the window stacked the warning on the resign / switch-user confirm**: those close first.
  - **"Come back from Continue" was wrong with a robot autosave** (Continue opens that game): "Continue or Play online", as in the warning.
- **Declined:**
  - "After the 5 s fallback the loss isn't recorded": without Lichess's answer there is no result to record. Waiting longer keeps the player on the board; Lichess still has the loss.
  - "Switch user should offer Resign too": the plan (§4.4) only changes its text; the owner can ask for it.
- Release builds; ChessTests + ChessQtTests 237 pass; 57 screenshots unchanged; no QML warnings.

#### Next
- LI-04 step 5: docs (`TASKS.md` LI-04 done, plan status).

### 2026-11-03 — LI-04 step 5: docs and the manual tests (branch: li04Docs_031126)

#### What was done
- **`docs/LI-04-ManualTests.md`** (new): the plan's eight manual scenarios (§6) as step-by-step checks with the app's own texts and a Result line each, plus **4b** (Resign from the warning, from Home and on window close, and the 5 s fallback offline). Corrected against the code: a correspondence game shows hours up to two days ("47 h", "23 h"), not "1 day".
- `TASKS.md` LI-04: coded (#111–#114), manual tests left. Plan status the same. `CLAUDE.md`: `LichessClient.ongoingGames`.

#### Next
- The owner's manual tests (`docs/LI-04-ManualTests.md`); then LI-03 (apply an accepted Lichess takeback).

### 2026-11-19 — LI-04: the hand tests passed, LI-04 done (branch: docs/li04HandTestResults_191126)

#### Done
- [`docs/LI-04-ManualTests.md`](docs/LI-04-ManualTests.md) (rewritten in #197): the owner ran every scenario on lichess.org with two accounts, and all passed. That covers leave with Home, Your games, Continue (directly and from the card), closing the window, resign from the warning, a kept robot game beside a Lichess game, a correspondence game over a day ("47 h", "23 h" left), games started on lichess.org, and the opponent resigning while away. The optional steps 4b.3 (resign offline) and 8.4 (the clock running out while away) weren't run. TASKS.md: LI-04 done.
- Found on the way, as new tasks:
  - BL-23 (bug): the Online page's "Challenge a friend" card: the Days per move buttons 10 and 14 go out of the card, and so does the "Lichess username (optional)" field;
  - LI-10: "Copy link" on a card's **Open on lichess.org**, by right click;
  - BL-24 (bug): Home's Continue sometimes opened the list for a single Lichess game after a restart (not reproduced in 5+ tries). The cause, from the code: `continueOnlyGame()` opens the list while the Lichess games reload, and they reload several times in the first seconds after a start (the event stream's `gameStart` for each game going on, Home's activation).
