---
id: BL-10
title: "Lichess challenges as notifications while signed in"
type: task
status: open
priority: medium
---

# BL-10 Lichess challenges as notifications while signed in

<!-- migrated from TASKS.md:90 (In Progress / Backlog) -->

- [ ] BL-10 Lichess challenges as notifications while signed in [Hard] — plan: [`docs/BL-10-ChallengesPlan.md`](docs/BL-10-ChallengesPlan.md), with the owner's choices (2026-11-09): Accept during a game depends on the game (a running real-time Lichess game: Decline only; robot, two-player or correspondence: Accept leaves it), taskbar flash and a sound in the background (no toast), unplayable challenges shown with Open on lichess.org, Your games kept live by the same stream. Steps: 1 `LichessStream` (LI-06's reconnect as a class; done, branch `lichessStream_091126`), 2 the always-open event stream and `incomingChallenges` (done, branch `eventStream_091126`), 3 Accept and the bar (done, branch `challengeBar_091126`), 4 the Online page and Home (done, branch `onlineChallenges_091126`), 5 docs and hand tests (done, branch `bl10Docs_091126`). **Coded (#127–#130); the owner's hand tests are open: [`docs/BL-10-ManualTests.md`](docs/BL-10-ManualTests.md).** Original note: today the app hears Lichess only during its own flows (waiting for a game, and since #89 a rematch offer while the finished game is on screen). A challenge sent to the linked account from lichess.org, or a rematch offered after we left the game, never shows up. Lichess sends both on `/api/stream/event` (`challenge`, `challengeCanceled`, `challengeDeclined`, `gameStart`; a rematch is a `challenge` with `rematchOf`), only to a stream open at that moment. To do: keep one event stream open whenever a Lichess account is linked (reconnect with backoff, stop on sign-out / 401, share it with `waitForGameStart` and the rematch watch); keep a list of incoming challenges (challenger, rating, time control, variant, rated, colour) and show them anywhere in the app (badge or InfoBar on Home and the game page, and a list); Accept (needs `compat.board`, else "open on lichess.org") and Decline with a reason; decide what Accept does during a running game; optionally a Windows notification when the window is minimised; parse tests and a screenshot scene for the notification. See `LichessClient::isRematchOffer` / `watchForRematch` (#89).

## Resolution

### 2026-11-09 — BL-10 plan: Lichess challenges as notifications (branch: bl10Plan_091126)

#### What was done
- Plan [`docs/BL-10-ChallengesPlan.md`](docs/BL-10-ChallengesPlan.md), with the owner's choices:
  - **Accept during a game depends on the game**: a running real-time Lichess game shows Decline only ("Finish your game first"); in a robot, two-player or correspondence game, Accept leaves that game;
  - **in the background**: a taskbar flash (`QWindow::alert`) and a sound, no Windows toast;
  - **unplayable challenges** (`compat.board` false) shown with Open on lichess.org and Decline;
  - **Your games** refreshed by `gameStart` / `gameFinish` on the same stream.
- Lichess facts it relies on, checked in `lichess-org/api` `doc/specs`: one event stream per token (a new one closes the old), keep-alive every 7 s, all current challenges and games resent when it opens; `challenge` events carry `compat.board`; the decline reasons.
- Design: LI-06's reconnect becomes a `LichessStream` class used by the game stream and an always-open event stream; `LichessClient.incomingChallenges`; `ChessConnector.acceptChallenge` (it may leave a game: one call, CLAUDE.md rule 1), `LichessClient.declineChallenge` (challenge setup, rule 2).

#### Next
- Step 1: `LichessStream`, no behaviour change (#127, on top of LI-06, #125).

### 2026-11-09 — BL-10 step 1: `LichessStream` (branch: lichessStream_091126)

#### What was done
- **`LichessStream`** (LiChessConnector, new): one Lichess ndjson stream that stays open, LI-06's reconnect taken out of `LichessClient` so the event stream (step 2) can use it too. `open` (not established yet: its end is reported, `ended`), `setReconnect(true)` once established (then the 20 s silence watchdog runs and a loss reconnects after 1, 2, 4, 8, then 15 s, `connectionChanged(false)`), `connected()` from the owner when it's back (`connectionChanged(true)`), `reopen()` (a resync: not a loss), `close()`. Lichess's 4xx stops it (`stopped`), a 401 also when Lichess's own check hasn't marked the reply yet; no answer or a 5xx is retried. It splits and logs the lines; a receiver that closes it on a line (the game's end) stops the reading.
- **`LichessClient`** keeps the game's side: the stream is established by the first `gameFull`, back after the resync's moves (`connected()` before a lost move's message, as in LI-06), closed at the game's end; `gameStreamEnded` (before gameFull: the old messages) and `gameStreamRefused` (4xx mid-game). `_streamReply`, `_gameOver`, `_streamLost`, the timers and the backoff state are gone from it.
- **Tests**: `FakeLichess` moved to `ChessQtTests/FakeLichess.h` (with `waitUntil`); **`LichessStreamTest`**, 10 tests: lines and the end before it's established, closed → back, silent → back, keep-alive keeps it, 404 and 401 stop it, 503 is retried, the backoff doubles (100, 200, 400 ms), `reopen` isn't a loss, `close` while a reconnect waits, a receiver closing on a line. The LI-06 `LichessReconnectTests` unchanged and passing. Release builds; ChessTests + ChessQtTests **325 pass**; the 20 stream tests 10 runs.
- No behaviour change: the online scenes (`takeback`, `clock-online`, `premove*`, `reconnecting`) as before (screen locked: the known 0.06 % text difference in `premove-played`/`-dropped`).
- The branch switch for this step first failed on five locked files (they kept their pre-LI-06 content); stashed (`stash@{0}`, "stale pre-LI-06 copies"), `develop` pulled, the branch made again from it.

#### Next
- Step 2: the always-open event stream and `incomingChallenges`.

### 2026-11-09 — BL-10 step 2: the event stream and incoming challenges (branch: eventStream_091126)

#### What was done
- **The account's event stream, always open** (`LichessClient::startEventStream`, a `LichessStream`): opened by `main.cpp` when a token is set (not in screenshot mode), reconnecting from the start (no "game" to establish it), closed by a token change (another profile, sign-out, an expired link: a 401 reaches `checkAuthExpired`, the link expires, the token goes).
- **One per token, shared**: `waitForGameStart` (our challenge's game) no longer opens its own stream; it waits for the `gameStart` on the open one (the 3 s probe of the game stream stays as the fallback). `watchForRematch` is a flag on it. `stopStream()` (leaving a game) leaves it open: it belongs to the account. `openEventStream`, `stopEventStream` and `_eventStreamReply` are gone.
- **`incomingChallenges`**: challenges *to* us (`direction: in`, or without it when we are the `destUser`; never ours), oldest first, one row each from `parseChallenge` (challenger, rating and provisional, "10+5" / "3 days per move" / "Unlimited", rated, speed, variant, our side, `canPlayHere`: `compat.board` and the standard variant, by speed when compat is missing, the URL, `rematchOf`). `challenge` adds or updates (resent: no second `challengeReceived`), `challengeCanceled` / `challengeDeclined` and the `gameStart` of its game remove it. After a (re)connect Lichess resends all current challenges: those not resent within 3 s were canceled while away and are dropped.
- **`declineChallenge(id, reason)`**: `POST /api/challenge/{id}/decline` with the reason; the row goes at once; a 404 (canceled meanwhile) is fine. `declineRematch` also removes its row.
- **Your games, live**: `gameStart` / `gameFinish` of a game other than the one on the board refresh `ongoingGames`, once per burst (500 ms).
- No UI yet (step 3: the bar, Accept, the sound and the flash).
- **Tests**: `LichessChallengeTest.cpp`, 13 tests: parsing (real time, correspondence, unlimited, sides, what this app can play), stream lines (listed once and canceled, our own not listed, removed by its game's start or a decline, a token change clears), and against `FakeLichess` (which now keeps request bodies): a challenge arrives and is canceled, a reconnect resends one and drops the one gone, waiting for a game shares the stream (one GET, still open after `stopStream`), Decline sends the reason, games elsewhere refresh Your games once, a token change closes the stream. Release builds; ChessTests + ChessQtTests **338 pass**; the 33 Lichess stream tests 10 runs.
- Screenshots: no UI change; `home`, `online`, `continue-online`, `takeback`, `profile` match, and no scene opens the stream (offline).

#### Next
- Step 3: Accept (`ChessConnector.acceptChallenge`, `CanAcceptChallenge`), the challenge bar, the sound and the taskbar flash; scenes.

### 2026-11-09 — BL-10 step 3: Accept and the challenge bar (branch: challengeBar_091126)

#### What was done
- **`ChessConnector.acceptChallenge(id)`** (one call: it may leave a game, CLAUDE.md rule 1) and **`CanAcceptChallenge`**: false only during a running real-time Lichess game (`_onlineRealTime`, from `onLichessClockStarted`). Otherwise it ends the game on the board (`endGame()`: a robot game is autosaved, a Lichess correspondence or unlimited game goes on at Lichess, a finished game is left) and `LichessClient::acceptChallenge(id)` (now also removes the row); the game starts through `gameStarted` as for a link.
- **The challenge bar** (`ChessGame.qml`, in the InfoBar column, so over every page while logged in): the oldest challenge not hidden, "FriendlyRival (1712) challenges you · 10+5 · Casual · You play Black (+1 more)", "offers a rematch" for a rematch (not shown while the finished game's own rematch bar is up). Buttons under the text (`InfoBar.actionsBelow`, new; other bars unchanged): **Accept** (hidden during a running real-time Lichess game: the text says "Finish your game first: it waits on Lichess"), or **Open on lichess.org** for one this app can't play ("Can't be played in this app"), **Decline**, **Hide** (until the next challenge; it stays on Lichess).
- **A running two-player game** asks first ("Leave this game? A two-player game isn't saved."): it would be lost. A robot game (autosaved) and a Lichess game (goes on there) don't.
- **Attention**: a new challenge (`challengeReceived`, not one resent after a reconnect) plays `sounds/challenge.wav` (three rising notes, new in `scripts/make_sounds.py`; the effects switch and volume as the others) and, when the window isn't active, flashes the taskbar button until it is (`Window.alert(0)`, no extra Qt module).
- **Accept fails** (the challenger canceled meanwhile): "Couldn't accept the challenge: …" as an InfoBar on any page.
- **Scenes** (in `all`, fed to the event stream's parser, no network): `challenge` (two challenges on Home), `challenge-unplayable` (a Bullet challenge from an FM), `challenge-ingame` (a real-time Lichess game, Decline only).
- Release builds; ChessTests + ChessQtTests **338 pass**. Screenshots on a locked screen: all board and InfoBar scenes match (the `takeback` bar unchanged by `actionsBelow`); the dialog scenes, `clock-promotion`, `clock-continue` and the premove text fail as known for a locked screen. The three challenge scenes have no baseline yet (on the unlocked-screen list in `TASKS.md`).

#### Not checked yet
- Everything with a real Lichess challenge (step 5's hand tests): the sound, the taskbar flash, Accept from Home, from a robot game and from a two-player game, Decline, Open on lichess.org.

#### Next
- Step 4: the "Challenges to you" card on the Online page (all of them, decline reasons), and Home's Play online description.

### 2026-11-09 — BL-10 step 4: the Online page and Home (branch: onlineChallenges_091126)

#### What was done
- **"Challenges to you"** on the Online page, above Your games, while there are any: one card per challenge, oldest first: the challenger (title, name, rating, `?` when provisional), "10+5 · Rapid · Casual · You play Black" (the variant, "Rematch", "Can't be played in Chess++ yet" when so), **Accept** (`ChessConnector.acceptChallenge`, through the page's busy/error handling like Continue) or **Open on lichess.org**, and **Decline…**: a menu with Lichess's reasons ("Decline", "Not now", "Too fast for me", "Too slow for me", "Not this time control", "Casual games only", "Rated games only", "Standard chess only").
- **The window's challenge bar** isn't shown over the Online page: the page lists them all.
- **Home**: the Play online card says "1 challenge waiting for you" / "N challenges waiting for you" while there are any.
- **Scene `online-challenges`** (in `all`): three challenges (one Bullet: Open on lichess.org). `online`, `online-empty`, `home` unchanged.
- Release builds; ChessTests + ChessQtTests **338 pass** (QML only, apart from the scene). The screen was locked: the new scene and `challenge` look right; baselines and a look at the Decline… menu (a popup) on the unlocked-screen list in `TASKS.md`.

#### Next
- Step 5: `docs/BL-10-ManualTests.md` (the plan's 11 scenarios with the app's texts), docs; then the owner's hand tests.

### 2026-11-09 — BL-10 step 5: hand tests and docs (branch: bl10Docs_091126)

#### What was done
- **`docs/BL-10-ManualTests.md`**: 13 scenarios with the app's own texts, for two Lichess accounts (A in the app, B in a browser; a third for one scenario): a challenge on Home and Accept, in the background (sound, taskbar flash), Decline with and without a reason, the challenger cancels, an unplayable one, during a running Rapid game, during a robot game, during a two-player game (the question), two at once and Hide, the connection drops, a game started elsewhere, a rematch after leaving, another profile.
- The plan's status: coded (#127–#130), hand tests left. `TASKS.md` BL-10 the same.

#### BL-10 is coded (#127–#130); open
- The owner's hand tests (`docs/BL-10-ManualTests.md`).
- The screenshot baselines of the four challenge scenes and a look at the Decline… menu, on an unlocked screen (`TASKS.md`, "Screenshots on an unlocked screen").

#### Next
- The owner's choice: the hand tests (BL-10, LI-06, LI-05, LI-02, LI-04 are all open), or the next item in `TASKS.md`.

### 2026-11-09 — BL-10: code review fixes (branch: bl10Review_091126)

The owner's `/code-review medium` of all BL-10 PRs (#127–#131): 10 findings, all fixed, and one more found while testing them.
- **A refused Accept lost the challenge and the game on the board**: the row was removed and `endGame()` called before Lichess answered. Now the row stays until its game starts (`gameStart` removes it), and `ChessConnector::acceptChallenge` leaves the board alone; when the Lichess game starts, a robot game on the board is autosaved (`onLichessGameStarted`), as when leaving it for Home.
- **Found while testing that**: Lichess answers 404 to Accept when the challenge is gone (canceled meanwhile), and the app took a 404 for "the game has started already" (the rejoin by id) and opened its game stream. For a challenge from the list a 404 now means it's gone: removed, "the challenge is no longer there".
- **Accept failures had no signal of their own**: `_acceptingChallenge` took any `networkError` (e.g. a failed Decline) as the Accept's, and stayed set when `acceptChallenge` returned early. Now `LichessClient::challengeAcceptFailed(id, message)` for a challenge from the list (other accepts keep `networkError`), and `acceptChallenge` returns whether it sent anything; the window and the Online page use both.
- **A refused event stream (a 4xx but 401, e.g. 429) gave up for the session**, silently: tried again after a minute (as Lichess asks after a 429).
- **Blitz without `compat` counted as unplayable**: `parseChallenge` uses `playableHere(speed, "friend", variant)` (a challenge to us is a direct one).
- **A challenge resent unchanged after a reconnect rebuilt the list** (an open Decline… menu would close): `incomingChallengesChanged` only for a changed row.
- **The rematch offer gave two notices** (the challenge sound and the rematch bar), and the bar hid every rematch challenge while one was offered: `LichessClient.rematchOfferId` (new property); that offer makes no `challengeReceived`, and the bar leaves out only it.
- **The accepted challenge's gameStart could beat the POST's answer** and count as a game started elsewhere (a needless refresh of Your games): skipped (`_acceptingId`).
- **`LichessStream` after a 4xx still reopened on a resync**: a refusal closes it (`reopen()` does nothing; a new `open()` tries again).
- **Formatting twice in QML**: `parseChallenge` gives `challengerText` ("FM Speedy (2105?)") and `sideText`; the bar and the Online page use them.
- **Two decline paths**: `declineRematch` goes through `declineChallenge`.
- **Tests**: 5 new (resent unchanged, the rematch offer while watching, Accept refused with 400 and 404, the accepted game's early gameStart, a 429 tried again) and 4 updated (Blitz without compat, the texts, `reopen` after a refusal). Release builds; ChessTests + ChessQtTests **343 pass**; the 38 Lichess stream tests 10 runs. The challenge scenes unchanged (22 bytes of text rendering on the locked screen).

### 2026-11-09 — BL-10 follow-up: the screenshot checks on an unlocked screen (branch: bl10ScreenChecks_091126)

#### What was done
- The checks owed since LI-05 (`TASKS.md`, "Screenshots on an unlocked screen"), on an unlocked screen (no `LogonUI`), after #132:
  - `Take-Screenshot.ps1 -Scene all` and `-Dark`, then `Compare-Screenshots.ps1`: **88 compared, 0 failed**. The dialog scenes, `clock-promotion`, `clock-continue` and the 0.06 % text in `premove-played` / `-dropped` that failed before were all the locked screen; `robot` in a real window passes too.
  - **New baselines, accepted after a look** (contact sheet, light and dark): `reconnecting`, `challenge`, `challenge-ingame`, `challenge-unplayable`, `online-challenges`, and the new `online-challenges-decline`. `premove-robot` played its premove (1. e4 Nf6 2. Nf3) and stays without a baseline: the robot's moves vary.
- **Scene `online-challenges-decline`** (new, in `all`): the Online page's Decline… menu open (Lichess's reasons, light and dark), so the popup is checked with every run instead of once by hand. It was repeatable over two more runs.
- Noticed, not changed: the InfoBars over the game page (reconnecting, a challenge, as the draw offer before them) cover part of the opponent's card (the name is cut). That's the bars' existing design; say if it should change (e.g. BL-13's layout).
- ChessTests + ChessQtTests **343 pass**.

#### Next
- The owner's hand tests on real Lichess: BL-10 (`docs/BL-10-ManualTests.md`), LI-06, LI-05, LI-02, LI-04.
