# Progress Log

Use this file to record where work stopped and what to do next. Newest entry at the top.

---

## 2026-11-11 — BL-16: respond at once when a game starts (branch: features/gameStartFeedback_111126)

### What was done
- **The engine stops without the window waiting for it** (`ChessConnector::stopEngineThread`): the robot's thread is told to stop (the process killed through its handle, the worker disconnected, `quit()`), then left to finish on its own among `_stoppingEngines`; `finished` deletes the worker on its thread and the thread here. Only the app's exit waits for them (`waitForStoppedEngines`, 3 s for all, then left behind as before). This was BL-11's "up to 3 s": a stop while the engine was inside its UCI start (the `ChessEnginePlayer` constructor, which the kill can't reach) kept the GUI thread in `wait(3000)`. The log says `[Timing] engine stop: the old engine finished N ms later (not waited for)`.
- **A busy state where an action waits for Lichess** (the local starts don't wait any more: see the numbers): `BusyButton` (a ring in the icon's place, the text says what is going on, no second click) and `BusyRing` (a spinning `BusyIndicator`, or a still clock with Reduce animations and in the scenes). On: a Lichess game's Continue (the Continue list, the Online page: only the card being started, `ChessConnector.StartingGameId`, new), Home's Continue card ("Connecting to Lichess…", the ring in the icon's place), the challenge bar's and the Online page's Accept ("Accepting…", the one being accepted: `_acceptingChallengeId`, `_busyAction`), Join ("Joining…"), Create challenge / Challenge *name* ("Creating the challenge…" / "Sending the challenge…").
- **Scenes** (in `all`): `continue-starting` (Home's Continue card), `continue-list-starting` (Friend's card "Starting…"), `challenge-accepting` (the bar's "Accepting…"). Lichess is a local `QTcpServer` that takes the request and never answers (`useSilentLichess`, `main.cpp`). The `timing` scene also opens and closes Settings and the Online page (`[Timing] Settings: N ms to open`).

### Numbers (Release, this PC, the `timing` scene)
| | before (BL-11's fixes) | after |
|---|---|---|
| Leaving a robot game while its first answer is starting (GUI thread waits) | 755–829 ms | **0** (the old engine finishes 413–461 ms later, alone; once 5.5 s on a cold first run) |
| Leaving while the robot thinks (GUI thread waits) | 37–49 ms | **0** (finished 23–53 ms later) |
| vs Robot, the session's first game, to the first frame | 177–218 ms | 167–223 ms |
| Continue vs Robot | 152–155 ms | 151–181 ms |
| Two players | 121–126 ms | 124–172 ms |
| Opening Settings / the Online page (GUI thread) | — | 36–48 ms / 17–19 ms |

### Open
- **The local starts take 125–225 ms** to their first frame, a little over the ~100 ms the task asked for: there is no busy state for them (a ring that shows for 150 ms only flickers); the pressed button or card stays on screen until the game page is up. Making the page itself faster is the next step if that still feels slow: the board's square-size binding (`ChessBoard.qml:14`, 25 ms) and the 64 piece `Image`s (14 ms) (BL-11).
- The "save" step of a start (12–27 ms: `config.json` and the autosave) could move after the first frame.
- The owner's check in a real window: leave a robot game at once after a move and start another (no freeze); Continue a Lichess game, Accept a challenge and Join by link (the ring and the words, then the game).

---

## 2026-11-11 — BL-11: profiling, the top two fixes (branch: features/profiling_111126)

### Measuring
- **A timing line in the app log, always** (category `chess.timing`, `GameStartTiming` in `frametrace.h`): from the action that starts a game (Start vs Robot / two players, Continue, a Lichess rejoin, an accepted challenge) to the game page's first frame, with its steps, e.g. `[Timing] vs Robot (level 3): 177 ms to the first frame (save 14, stop engine 0, setup 2, start engine 0, page 141, first frame 20)`. `ChessConnector` marks the steps, `GamePage` reports itself (`gameShown`). Also `[Timing] engine stop: the GUI thread waited N ms` when `stopEngineThread()` waits over 20 ms. (BL-16 asked for this line.)
- **Scene `timing`** (not in `all`): from Home, 1.5 s apart, as clicks in a running app: vs Robot, leave, Continue, leave, two players, leave, vs Robot as Black and leave while the engine thinks. `Take-Screenshot.ps1 -KeepLog` keeps a scene's log also when it succeeds; with `QT_LOGGING_RULES="chess.frames.debug=true"` the log has `FrameTrace`'s "GUI thread busy" lines too.
- **QML profile**: `qmlprofiler` on a Release build with QML debugging switched on for the run only (not committed: it lets the app open a debug port), summed per file and line.

### Found and fixed
1. **The flag on a resigned or flagged king** (`ChessFigures.qml`): a `Text` "⚑" in each of the 64 piece items, hidden but created. The glyph isn't in the UI font, and the font fallback search took about 6 ms per item: **361 ms of the first game page's 514 ms** (once per session; later pages had it cached). Now a `Loader`, made only on the flagged king's square: the same look.
2. **Home's photo** (`ChessBackground.jpg`, 3840 × 2160): decoded at full size each time Home appeared (33 MB, too big to stay in the image cache): **64 ms of every return to Home**. Now one image in the window behind the pages (`ChessGame.qml`), shown under Welcome, the profile picker and Home (their backgrounds are transparent): decoded once, at full quality and any scaling; the pictures are the same to the pixel.

### Numbers (Release, this PC, the `timing` scene, two runs each)
| | before | after |
|---|---|---|
| The session's first game start, to its first frame | 507–645 ms (page 466–604) | 177–218 ms (page 141–172) |
| Later game starts | 131–171 ms | 131–176 ms |
| Leaving a game (GUI thread busy) | 86–264 ms | 47–95 ms |
| Engine stop while it thinks (GUI thread waits) | 42–74 ms | 37–49 ms |
| Engine stop while its first answer is starting (GUI thread waits) | — | **755–829 ms** (see below) |

A scene at app start (cold, the window's first frames too): two players 670 → 366 ms, vs Robot 775 → 438 ms, a Lichess rejoin 691 → 240 ms.

### Left (for BL-16 and later)
- **The likely "up to 3 s"**: leaving a robot game right after the player's move, while the robot's first answer is starting, the GUI thread waits **755–829 ms** in `stopEngineThread()` (`_engineThread->wait(3000)`: the engine thread is in its UCI start, not in its event loop). The window is frozen meanwhile, also when a new game starts (it stops the old engine first). Its limit is 3 s; a slower PC or a Debug build comes closer to it. For BL-16: stop the engine off the GUI thread (let the old thread finish on its own, detached, as when it doesn't stop in time), so a click never waits for it. The timing line and `[Timing] engine stop` are in every log now.
- **The code review of #147** (10 findings, all fixed): a start that fails or is dropped (a refused Accept, a failed rejoin, Back, a declined rematch, `endGame()`) cancels its timing, and one older than a minute no longer counts; a rematch is timed; Continue's engine start is its own step; on a game page that was open already the page isn't a step (it was recorded before the setup); "first frame" is the first frame synchronized after the page is up (not one already being drawn), and a window that never draws ends the timing after 5 s; `GamePage` reports once it is in the window; the photo (above); the `timing` scene plays only the player's move before leaving (the scripted reply could race the robot's own).
- **A game page still takes 110–170 ms to create** (warm). Of its 81 ms of QML: the board's square-size binding (`ChessBoard.qml:14`, 25 ms: the board's width and height settle in four steps, each moving every piece) and the 64 piece `Image`s (14 ms). Worth it only with BL-16's busy state.
- **Not done**: the scene graph checks (`QSG_VISUALIZE`): idle boards and per-move repaints; the move, drag, takeback and review profile.

---

## 2026-11-10 — BL-18: continuing a Lichess game showed the start position first (branch: bugs/continueBoardFlash_101126)

### What was done
- **The cause** (as suspected in `TASKS.md`): on the stream's first `gameFull`, `LichessClient` emitted `gameStarted`, so `ChessConnector::startOnlineGame` set up the start position and the game page opened; the moves played before then came after it (`opponentMoveReceived`) and waited in the `MoveGate` until the page had settled, then showed as a resync.
- **The fix**: `gameStarted` carries the game's moves so far (`QStringList moves`, empty for a new game), and they no longer come again as `opponentMoveReceived`. `ChessConnector::playJoinedMoves` puts them on the board in `startOnlineGame`, before `setGameMode` opens the page: the first frame shows the game's position, without slides, move sounds or "Check" for them (the page tells a check in the position it opens on, as for a resumed robot game). The clock is off while they're played (it starts after, from Lichess's times). The same path serves Continue (Home, the list, the Online page), Join by link, an accepted challenge and a rematch (those two start with no moves).
- **Tests**: the stream tests expect the moves with the start (`GameStream_GameFull_MovesComeWithTheStart_Test` replaces the resync-in-one-signal test; the clock tests: `started e2e4 e7e5`, then the clock, then the times). ChessQtTests **222 pass**.
- **Screenshots**: new scene `rejoin` (in `all`): a game joined with ten moves, grabbed as soon as the page is up (no `--delay` for it): the position after 5… a6, not the start position. The other Lichess scenes (fed through the same start) still match.
- **The owner's `/code-review medium` of #146: 9 findings, all fixed** (same branch):
  - A joined game that ended in mate was recorded and told as the previous game's (vs Robot, "Robot wins by checkmate"): the game's mode and opponent are set before its moves are played (`setGameMode()`, which opens the page, comes after them). Its mate's sound and result aren't announced before the page (`checkMateNotify` / `checkMateResult` quiet while `_resuming`).
  - On a game page that was open already, the board never became idle again, so a check (or a waiting result) in the joined position wasn't told: the joined moves make the gate busy, then idle (`_gate->moveShown()`), and `GamePage` takes a new game's position as the one it "opened" on (`onNewGameStarted`).
  - A joined move that can't be played here left the board out of step with Lichess: the game isn't opened then ("Couldn't continue the game: Lichess sent a move Chess++ can't play"; `endGame()` stops the stream).
  - A new game whose opponent moved before our stream opened showed that move without a slide, a sound or the premove: only a game with two moves or more counts as joined; zero or one move comes through the gate as the opponent's, as before.
  - One move loop: `playMoves(moves, announce)` (quiet for joined moves) instead of `playJoinedMoves`.
  - The `rejoin` scene is ready with the first frame drawn after the join (`frameSwapped`), and its delay of 0 is the script's (an explicit `-Delay` still wins), not a special case in `main.cpp`.
  - A stream test waited for one event too many (it sat out its 5 s timeout and sometimes caught the stream's silence warning): fixed. ChessQtTests **222 pass**; the scenes `rejoin`, `takeback`, `premove-played`, `opponent-left-claim`, `draw-offer`, `clock-online` checked one by one.

### Next
- The owner's check with a real Lichess game: Continue an unfinished game with moves (from Home, the list, the Online page): no start position first, no move sounds.

---

## 2026-11-10 — BL-17: Continue shows every unfinished game (branch: features/continueList_101126)

### What was done
- **Home's Continue** counts every unfinished game, the robot's autosaved one and the Lichess ones. With only one, it opens it directly as before (the robot's game, or the one Lichess game if it can be played here; a refused rejoin says so: "Couldn't continue the game: …", and the list is refreshed). With two or more it opens the **Continue list**. The card reads "Continue a game" then, e.g. "vs Robot · Level 3 · 4 Lichess games, your turn in 2". `ChessGame.continueGames()`; `HomePage.continueRequested` replaces `onlineGamesRequested`.
- **`ContinueGamesPage.qml`** (new): a page of its own pushed over Home (decided here: Home stays a launcher; Back and Esc as for the Online page). The robot's game first, in the upper left (its board as left, from the player's side, "Level 3 · Club Player · you play White", "Your turn" / "Robot's turn"), then the Lichess games, most urgent first ("Lichess · Rapid · 9 min left"; Open on lichess.org for one that can't be played here). Cards about 220 px wide, up to four in a row; Refresh for the Lichess games, an error line when they can't be loaded, the rejoin's error over the page.
- **`ChessConnector.ContinueGame`** (new): the autosaved robot game for its card: `{ level, playerPlaysWhite, playerToMove, fen, lastMove }`, read from the autosave once (`Game::Load` into a game of its own), until `canContinueChanged`.
- **`GameCard.qml`** (new): the Online page's game card, shared by both pages; **`lichess.js`**: Lichess's words for a game (`speedName`, `gameDetails`, `gameTitle`), also shared.
- **The Online page**: "Your games" moved, as it was, to the end, after "Join a game".
- **Screenshots**: new scenes `continue-list` (+ `1400x900`, `640x560`), `continue-list-online`, `continue-list-robot`; `continue`, `continue-online` and the `online*` scenes re-baselined after a look (the card's words; "Your games" last). Light and dark: 180 compared, all match after **28 new or changed baselines**.
- **The owner's `/code-review medium` of #144: 10 findings, all fixed** (same branch):
  - A rejoin is one state now, in `LichessClient` (`rejoiningGameId`), with its own failure (`rejoinFailed`, not `networkError`): another request's error is no longer taken for it (nor it for theirs), and it ends when its stream is stopped. `ChessConnector` starts it (`continueOnlineGame(id)`, `StartingOnlineGame`, `gameStartFailed` → "Couldn't continue the game: …" over the command bar, the list refreshed) and drops it on another profile, a sign-out or a new Lichess account, or Back from its page (`cancelOnlineStart()`). Home's Continue card stayed disabled when the stream was stopped elsewhere: no more.
  - Home's Continue is one call: `ChessConnector.continueOnlyGame()`, which also waits for the Lichess list while it loads (the Continue list opens, the games come into it).
  - An unreadable autosave counts as no robot game (`CanContinue` false), read once.
  - `OngoingGameCard` (a Lichess game's card) and `OngoingGamesRefresh`, shared by the Online page and the list; `GameCard.playable` instead of the icon's name; `Chess::BoardPositionToString` for the last move; `ContinueGame.title` dropped.
  - ChessQtTests **222 pass** (3 new: a rejoin's game, a refused rejoin gives `rejoinFailed` only, a stopped rejoin ends quietly).

### Next
- The owner's check in a real window: Continue with only a robot game, only one Lichess game, both; Back / Esc from the list; a Lichess game that ended meanwhile.

---

## 2026-11-10 — BL-13: code review fixes (branch: bugs/bl13Review_101126)

The owner's `/code-review medium` of the BL-13 PRs (#136, #137, #141): 10 findings, all fixed.
- **An offer or Claim victory could be hidden with no way to answer it** (only the recent messages): a short message held the command bar, and an error until it was closed. Now a state that waits for an answer is hidden at most 4 s by a short message (an error too: it stays in the recent messages, with the dot), not at all by "Check" (the board and the sound tell it), and when an offer comes or the claim becomes possible, a message shown gives way at once.
- **Closing "Opponent left" with × didn't last**: each new countdown from Lichess brought it back (`opponentGoneChanged` came for the seconds too). Now only when the opponent leaves or comes back.
- **The recent messages' handlers read bindings that may not have updated yet** (`_offer`, `_reconnecting`, in the same signal): they read `ChessConnector` now.
- **The recent messages and the dot stayed for the next profile** (file paths, game titles): cleared when the profile changes (not on a rename).
- **In a narrow window a short message moved the board** (it didn't fit the command bar: the InfoBar, which the stacked layout makes room for): a message without buttons is cut short to fit now (all of it on hover and in the recent messages); only one with buttons (an offer, Claim victory) still needs the InfoBar there.
- **The recent messages could run off a small window**: kept inside it (the list gets the height left; the panel moves up when needed).
- **The claim rules were worked out in QML** (a countdown timer and "the opponent's turn", copied): `ClaimWin` (`claimwin.h/cpp`, 6 tests) counts down and knows the turn; `ChessConnector.ClaimWinSeconds` counts down, `CanClaimVictory` is new, and `claimVictory()` posts only when it's true.
- **`PlayerCard` built both layouts**: Loaders build only the one in use (one avatar, clock and captured strip per card, not three).
- One tooltip path in `CommandBarMessage` (the click area's never ran); `NewMessagesDot` and `RecentMessagesMenuItem`, shared by the game page and Home.
- **Screenshots** (screen on): all 164 existing scenes match, light and dark (the cards' Loaders, the cut text and the `ClaimWin` claim change no picture); new sized scenes `opponent-left-turn@640x560` (the text cut short over the command bar), `saved@640x560`, `messages@640x560`: 6 new baselines after a look.
- ChessQtTests 219 pass (6 new).

### Next
- The owner's hand check of #141 still stands (a draw offer, the lost connection, a challenge, the opponent left in a real Lichess game; Ctrl+M and the dot), now also: an error while an offer is open, and × on "Opponent left" during the countdown.

---

## 2026-11-10 — BL-13 step 3: messages over the command bar, recent messages (branch: features/commandBarMessages_101126)

### What was done (plan §9, the owner's Q5–Q10)
- **Over the command bar** (`CommandBarMessage`, now declarative: `active`, buttons as children, `closable`, `fits`): besides the short messages, the **offers** ("Friend offers a draw" with Accept / Decline; takeback, rematch), the **lost connection** ("Reconnecting to Lichess…", the full text as its tooltip) and the **opponent left** ("Opponent left · claim victory in 12 s", Claim victory). One at a time: the game's state (offer > opponent left > connection) before a short message; what doesn't fit (e.g. an offer at 640 wide) or waits behind goes to the InfoBars as before (one offer bar now for the three kinds). An **error stays until closed** (×); a newer message meanwhile goes to the InfoBar for its 4 s. `ChessGame.notify(text, severity, external)` replaces `messageBar.show()` (21 callers).
- **Recent messages** (`MessagesFlyout`): the last 20 of the session, newest first, all but "Check": full text, the time, the game it came from when not the one on screen; an open offer keeps Accept / Decline there, then shows "accepted" / "declined" / "withdrawn"; the opponent left keeps Claim victory. "Recent messages" in the game's **More** menu and Home's **account menu**, **Ctrl+M** everywhere (in Settings' shortcut list); an accent **dot** on More / the account button for new ones the player didn't cause (an offer, the opponent left, the connection, a decline, an error). A panel in the window, not a `Popup` (see below); Esc or a click outside closes it.
- **Screenshots**: new scenes `draw-offer` (default, 1400×900, 640×560: the InfoBar), `saved`, `home-message`, `messages` (the list open); `reconnecting` re-baselined (the pill). **20 baselines** new or changed, light and dark, after a look.
- **The dialog and timer scenes**: the first full run, at night, drew no popups and stalled QML timers (the screen was locked; `develop`'s build failed the same way). Run again with the screen on: all 150 scenes match, after 4 more baselines: `about` (light, dark: the Ctrl+M row in the shortcut list) and `draw-offer` at 640×560 (light, dark: the same picture, its text smoothed in grey instead of ClearType; the night's baseline was the odd one).
- The list was a `Popup` first; in that state it neither drew nor let the window's frames go on, so it is a plain panel now (it works either way, and needs no popup window).
- QML, the scenes in `main.cpp` and the script: no C++ tests run.

### Fixed after the owner's test with a real Lichess game (the opponent left)
- **Claim victory was offered when Lichess refuses it.** Lichess takes the claim only on the opponent's turn (lila `Pov.mightClaimWin = forceResignable && !isMyTurn`; else "This is not the time to claim victory"), but sends `claimWinInSeconds: 0` either way. On the player's turn the message now says "Opponent left · your move, then claim victory" and has no button (also in the InfoBar and the recent messages); during the countdown, no button either, so the message is narrower and fits.
- **The board changed size**: the countdown's message with its (disabled) button didn't fit the command bar, so it went to the InfoBar; and the refused claim's error went to the InfoBar because the state held the command bar. Now a short message takes the state's place for its time (an error until closed) and the state waits there, not in the InfoBar; a newer message behind an error goes only to the recent messages (with the dot). Only what doesn't fit at all (a narrow window) still uses the InfoBar.
- **The opponent-left message can be closed** (×, over the command bar and in the InfoBar): hidden until the claim becomes possible or the opponent comes back or leaves again; the recent messages keep Claim victory.
- Scenes `opponent-left` (+ 1400×900), `opponent-left-claim` (+ 1400×900, 640×560), `opponent-left-turn`, `opponent-left-error`: 14 new baselines, light and dark; all 150 others match.

### Next
- The owner's hand check: a draw offer, the lost connection and a challenge in a real Lichess game; Ctrl+M and the dot.

---

## 2026-11-10 — BL-13: short messages over the command bar (branch: infoBarsColumn_091126, PR #137)

### What was done
- The owner, checking #137: "Check" still changed the board's size in a tall window (half the screen), where the cards are above and below the board. The owner's idea (plan step 3, PR #140): short messages over the command bar. Done here for the message bar only; the rest of step 3 (offers and the reconnecting status in the command bar, recent messages in the More menu) stays as planned.
- **`CommandBarMessage.qml`** (new): "Check", "Game saved", "PGN copied", … as a pill over the command bar's free middle: no room taken, nothing on the page moves. Centred on the board on the game page (right of the window's centre with the player column), kept inside the free middle; where it would cross the title's text, the title is hidden up to it (no cut word). A message that doesn't fit (e.g. "Couldn't save <long path>" at 768 wide), or a page without a command bar (Welcome, the profile picker), shows as the InfoBar under the command bar as before (`messageBarFallback`). The decision follows the window's size while it shows.
- **`CommandBar.messageArea`**: the item filling the free middle (the title, Home's filler); set on the game, Home, Online, Profile and Settings pages. `GamePage.boardCenterX`.
- Checked with a temporary long message at 768, 960 and 1400 wide: the bar, over the command bar with the title hidden, over the command bar with the title kept.
- **Screenshots**: `check` re-baselined (default, 1400×900) and new `check@960x1040`, light and dark: "Check" in the command bar, the board as in `game`. All other scenes match (the profile scenes' 0.04 % is their "since <today's date>").
- QML only: no C++ tests run.

### Next
- The owner's choices for the rest of step 3 (Q8 "Recent messages" in the More menu, Q9 all but "Check", the last 20, Q10 this session only) go into the plan (#140).

---

## 2026-11-09 — BL-13 step 2: the InfoBars between the column's cards (branch: infoBarsColumn_091126)

### What was done
- The owner saw "Check" shrink the full-height board for 4 s (#134's room for the bars). Now, with the player column, the window's InfoBars ("Check", saved, offers, a challenge, reconnecting, opponent gone, damaged profiles) go between the column's cards: `GamePage.barsArea` (the free space, page coordinates), `ChessGame.qml` places the bars there (a `Flickable`: they scroll when more than fit). The board keeps its size. Without the column the bars stay under the command bar and the page starts below them, as before.
- The layout is chosen without the bars (`area.stackedBoard` from the full height; `topInset` only in the stacked layout), so a bar never switches it.
- The column needs room for one bar between the cards (`area._barsRoom`, a challenge with buttons ≈ 140 px): at 1000 × 560 the bar was cut and its buttons scrolled out of sight, so such a low window keeps the cards above and below the board.
- `InfoBar`: the buttons wrap (`Flow`, as wide as its buttons beside the text); the window's bars put them under the text while in the column (`actionsBelow: root._barsInColumn`). The moves panel's review bar is unchanged.
- **Screenshots**: new scene `check` (the page opens in check), new sized `reconnecting`, `check` at 1400 × 900 and `game` at 1000 × 560. Light and dark: all others match; `challenge-ingame-1400x900` changed as intended (the bar in the column); **10 baselines** new or updated after a look. No QML warnings in the log.
- QML, the scene list in `main.cpp` and the script: no C++ tests run.

### Next
- The owner's hand check: a draw offer, "Check" and a challenge during a game in a maximised window; resize while a bar shows.

---

## 2026-11-09 — BL-13 step 1: the player column in wide windows (branch: wideGameLayout_091126)

### What was done
- The owner chose the plan's recommended answers to Q1–Q4 (recorded in §8).
- **`GamePage.qml`**: `area.side` computes the board for both layouts and puts the cards in a column left of the board when that board is ≥ 24 px bigger (and two stacked cards fit beside the squares). The `Column` became explicit `x`/`y` bindings, so the same card items move on a switch; the stacked layout is pixel-identical to before (the cards' x rounded, as the anchors did).
- **`PlayerCard.stacked`** / `clockFirst`: in the column the clock gets its own row (`title` size), mirrored (top card: clock, name, pieces; bottom: pieces, name, clock). `Avatar`, `NameRow`, `Clock` are inline components shared by both layouts; the avatar's pulse runs only in the visible one.
- **`CapturedStrip.wrapWidth`**: the groups go on to a next line where they don't fit (a group never split; "+N" on its own line if needed). 0 = one line, as before.
- `Theme.playerColumnWidth` 260; ThemeTokens §11; CLAUDE.md.
- **Screenshots**: `Take-Screenshot.ps1` takes `scene@WxH[@black]`; `all` ends with 11 sized scenes (`$sizedScenes`), and the new scene `captures` (Black down to a lone king: White's full strip wraps, "+29"). Light and dark: the 100 old ones match; **24 new baselines** accepted after a look (contact sheet). No QML warnings in the log.
- QML and the scene list only (`main.cpp`): no C++ tests run.

### Next
- The owner's hand check in a real window: drag the window's edge across the switch, maximise, snap to half, Windows at 150 %.
- Step 2: the InfoBars between the column's cards (Q1 = A).

---

## 2026-11-09 — BL-13 plan: the game page on wide screens (branch: bl13Plan_091126)

### What was done
- **`docs/BL-13-WideLayoutPlan.md`**: the player cards move to a 260 px column left of the board when that gives a board at least 24 px bigger than today's layout (computed from both layouts, not an aspect ratio): +16 % at 1920 × 1080 and 1400 × 900, +27 % at 1280 × 720; snapped halves, landscape windows bound by width and narrow windows keep today's layout. The column's cards: name, a larger clock on its own row, the captured pieces wrapping; mirrored as on lichess.org. The moves panel unchanged. 22 new sized screenshot scenes (light and dark) in `all`, incl. two sizes that must not switch.
- Builds on #134 (the InfoBars make room on the game page): in the column layout that would shrink the board while a bar shows, so the plan proposes the bars between the column's cards.

### Next
- The owner's choices in the plan's §8 (Q1 the InfoBars, Q2 the cards' order, Q3 the switch rule, Q4 the extra width), then step 1.

---

## 2026-11-09 — The InfoBars no longer cover the opponent's card (branch: infoBarsRoom_091126)

### What was done
- The owner's choice on the note below: the game page makes room for the window's InfoBars (an offer, a challenge, the reconnecting bar, the opponent gone, a message). `GamePage.infoBarsHeight` (bound to the bar column's height in `ChessGame.qml`) moves the board and its cards down by the bars' height while any shows, 8 px under them. In a tall window the board uses its spare height first; in a wide window (height-bound) it shrinks while the bar shows. No animation: the bar itself appears at once. Home and the other pages are unchanged (their content is centred or scrolls).
- Screenshots, light and dark: 100 compared, only `reconnecting` and `challenge-ingame` changed (the card now under the bar): looked at and accepted as new baselines. Also checked at `-Size 1400x900`. QML only: no C++ tests run.

### Next
- BL-13 plan (wide-screen layout); it decides where the bars go in the new layout.

---

## 2026-11-09 — BL-10 follow-up: the screenshot checks on an unlocked screen (branch: bl10ScreenChecks_091126)

### What was done
- The checks owed since LI-05 (`TASKS.md`, "Screenshots on an unlocked screen"), on an unlocked screen (no `LogonUI`), after #132:
  - `Take-Screenshot.ps1 -Scene all` and `-Dark`, then `Compare-Screenshots.ps1`: **88 compared, 0 failed**. The dialog scenes, `clock-promotion`, `clock-continue` and the 0.06 % text in `premove-played` / `-dropped` that failed before were all the locked screen; `robot` in a real window passes too.
  - **New baselines, accepted after a look** (contact sheet, light and dark): `reconnecting`, `challenge`, `challenge-ingame`, `challenge-unplayable`, `online-challenges`, and the new `online-challenges-decline`. `premove-robot` played its premove (1. e4 Nf6 2. Nf3) and stays without a baseline: the robot's moves vary.
- **Scene `online-challenges-decline`** (new, in `all`): the Online page's Decline… menu open (Lichess's reasons, light and dark), so the popup is checked with every run instead of once by hand. It was repeatable over two more runs.
- Noticed, not changed: the InfoBars over the game page (reconnecting, a challenge, as the draw offer before them) cover part of the opponent's card (the name is cut). That's the bars' existing design; say if it should change (e.g. BL-13's layout).
- ChessTests + ChessQtTests **343 pass**.

### Next
- The owner's hand tests on real Lichess: BL-10 (`docs/BL-10-ManualTests.md`), LI-06, LI-05, LI-02, LI-04.

---

## 2026-11-09 — BL-10: code review fixes (branch: bl10Review_091126)

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

---

## 2026-11-09 — BL-10 step 5: hand tests and docs (branch: bl10Docs_091126)

### What was done
- **`docs/BL-10-ManualTests.md`**: 13 scenarios with the app's own texts, for two Lichess accounts (A in the app, B in a browser; a third for one scenario): a challenge on Home and Accept, in the background (sound, taskbar flash), Decline with and without a reason, the challenger cancels, an unplayable one, during a running Rapid game, during a robot game, during a two-player game (the question), two at once and Hide, the connection drops, a game started elsewhere, a rematch after leaving, another profile.
- The plan's status: coded (#127–#130), hand tests left. `TASKS.md` BL-10 the same.

### BL-10 is coded (#127–#130); open
- The owner's hand tests (`docs/BL-10-ManualTests.md`).
- The screenshot baselines of the four challenge scenes and a look at the Decline… menu, on an unlocked screen (`TASKS.md`, "Screenshots on an unlocked screen").

### Next
- The owner's choice: the hand tests (BL-10, LI-06, LI-05, LI-02, LI-04 are all open), or the next item in `TASKS.md`.

---

## 2026-11-09 — BL-10 step 4: the Online page and Home (branch: onlineChallenges_091126)

### What was done
- **"Challenges to you"** on the Online page, above Your games, while there are any: one card per challenge, oldest first: the challenger (title, name, rating, `?` when provisional), "10+5 · Rapid · Casual · You play Black" (the variant, "Rematch", "Can't be played in Chess++ yet" when so), **Accept** (`ChessConnector.acceptChallenge`, through the page's busy/error handling like Continue) or **Open on lichess.org**, and **Decline…**: a menu with Lichess's reasons ("Decline", "Not now", "Too fast for me", "Too slow for me", "Not this time control", "Casual games only", "Rated games only", "Standard chess only").
- **The window's challenge bar** isn't shown over the Online page: the page lists them all.
- **Home**: the Play online card says "1 challenge waiting for you" / "N challenges waiting for you" while there are any.
- **Scene `online-challenges`** (in `all`): three challenges (one Bullet: Open on lichess.org). `online`, `online-empty`, `home` unchanged.
- Release builds; ChessTests + ChessQtTests **338 pass** (QML only, apart from the scene). The screen was locked: the new scene and `challenge` look right; baselines and a look at the Decline… menu (a popup) on the unlocked-screen list in `TASKS.md`.

### Next
- Step 5: `docs/BL-10-ManualTests.md` (the plan's 11 scenarios with the app's texts), docs; then the owner's hand tests.

---

## 2026-11-09 — BL-10 step 3: Accept and the challenge bar (branch: challengeBar_091126)

### What was done
- **`ChessConnector.acceptChallenge(id)`** (one call: it may leave a game, CLAUDE.md rule 1) and **`CanAcceptChallenge`**: false only during a running real-time Lichess game (`_onlineRealTime`, from `onLichessClockStarted`). Otherwise it ends the game on the board (`endGame()`: a robot game is autosaved, a Lichess correspondence or unlimited game goes on at Lichess, a finished game is left) and `LichessClient::acceptChallenge(id)` (now also removes the row); the game starts through `gameStarted` as for a link.
- **The challenge bar** (`ChessGame.qml`, in the InfoBar column, so over every page while logged in): the oldest challenge not hidden, "FriendlyRival (1712) challenges you · 10+5 · Casual · You play Black (+1 more)", "offers a rematch" for a rematch (not shown while the finished game's own rematch bar is up). Buttons under the text (`InfoBar.actionsBelow`, new; other bars unchanged): **Accept** (hidden during a running real-time Lichess game: the text says "Finish your game first: it waits on Lichess"), or **Open on lichess.org** for one this app can't play ("Can't be played in this app"), **Decline**, **Hide** (until the next challenge; it stays on Lichess).
- **A running two-player game** asks first ("Leave this game? A two-player game isn't saved."): it would be lost. A robot game (autosaved) and a Lichess game (goes on there) don't.
- **Attention**: a new challenge (`challengeReceived`, not one resent after a reconnect) plays `sounds/challenge.wav` (three rising notes, new in `scripts/make_sounds.py`; the effects switch and volume as the others) and, when the window isn't active, flashes the taskbar button until it is (`Window.alert(0)`, no extra Qt module).
- **Accept fails** (the challenger canceled meanwhile): "Couldn't accept the challenge: …" as an InfoBar on any page.
- **Scenes** (in `all`, fed to the event stream's parser, no network): `challenge` (two challenges on Home), `challenge-unplayable` (a Bullet challenge from an FM), `challenge-ingame` (a real-time Lichess game, Decline only).
- Release builds; ChessTests + ChessQtTests **338 pass**. Screenshots on a locked screen: all board and InfoBar scenes match (the `takeback` bar unchanged by `actionsBelow`); the dialog scenes, `clock-promotion`, `clock-continue` and the premove text fail as known for a locked screen. The three challenge scenes have no baseline yet (on the unlocked-screen list in `TASKS.md`).

### Not checked yet
- Everything with a real Lichess challenge (step 5's hand tests): the sound, the taskbar flash, Accept from Home, from a robot game and from a two-player game, Decline, Open on lichess.org.

### Next
- Step 4: the "Challenges to you" card on the Online page (all of them, decline reasons), and Home's Play online description.

---

## 2026-11-09 — BL-10 step 2: the event stream and incoming challenges (branch: eventStream_091126)

### What was done
- **The account's event stream, always open** (`LichessClient::startEventStream`, a `LichessStream`): opened by `main.cpp` when a token is set (not in screenshot mode), reconnecting from the start (no "game" to establish it), closed by a token change (another profile, sign-out, an expired link: a 401 reaches `checkAuthExpired`, the link expires, the token goes).
- **One per token, shared**: `waitForGameStart` (our challenge's game) no longer opens its own stream; it waits for the `gameStart` on the open one (the 3 s probe of the game stream stays as the fallback). `watchForRematch` is a flag on it. `stopStream()` (leaving a game) leaves it open: it belongs to the account. `openEventStream`, `stopEventStream` and `_eventStreamReply` are gone.
- **`incomingChallenges`**: challenges *to* us (`direction: in`, or without it when we are the `destUser`; never ours), oldest first, one row each from `parseChallenge` (challenger, rating and provisional, "10+5" / "3 days per move" / "Unlimited", rated, speed, variant, our side, `canPlayHere`: `compat.board` and the standard variant, by speed when compat is missing, the URL, `rematchOf`). `challenge` adds or updates (resent: no second `challengeReceived`), `challengeCanceled` / `challengeDeclined` and the `gameStart` of its game remove it. After a (re)connect Lichess resends all current challenges: those not resent within 3 s were canceled while away and are dropped.
- **`declineChallenge(id, reason)`**: `POST /api/challenge/{id}/decline` with the reason; the row goes at once; a 404 (canceled meanwhile) is fine. `declineRematch` also removes its row.
- **Your games, live**: `gameStart` / `gameFinish` of a game other than the one on the board refresh `ongoingGames`, once per burst (500 ms).
- No UI yet (step 3: the bar, Accept, the sound and the flash).
- **Tests**: `LichessChallengeTest.cpp`, 13 tests: parsing (real time, correspondence, unlimited, sides, what this app can play), stream lines (listed once and canceled, our own not listed, removed by its game's start or a decline, a token change clears), and against `FakeLichess` (which now keeps request bodies): a challenge arrives and is canceled, a reconnect resends one and drops the one gone, waiting for a game shares the stream (one GET, still open after `stopStream`), Decline sends the reason, games elsewhere refresh Your games once, a token change closes the stream. Release builds; ChessTests + ChessQtTests **338 pass**; the 33 Lichess stream tests 10 runs.
- Screenshots: no UI change; `home`, `online`, `continue-online`, `takeback`, `profile` match, and no scene opens the stream (offline).

### Next
- Step 3: Accept (`ChessConnector.acceptChallenge`, `CanAcceptChallenge`), the challenge bar, the sound and the taskbar flash; scenes.

---

## 2026-11-09 — BL-10 step 1: `LichessStream` (branch: lichessStream_091126)

### What was done
- **`LichessStream`** (LiChessConnector, new): one Lichess ndjson stream that stays open, LI-06's reconnect taken out of `LichessClient` so the event stream (step 2) can use it too. `open` (not established yet: its end is reported, `ended`), `setReconnect(true)` once established (then the 20 s silence watchdog runs and a loss reconnects after 1, 2, 4, 8, then 15 s, `connectionChanged(false)`), `connected()` from the owner when it's back (`connectionChanged(true)`), `reopen()` (a resync: not a loss), `close()`. Lichess's 4xx stops it (`stopped`), a 401 also when Lichess's own check hasn't marked the reply yet; no answer or a 5xx is retried. It splits and logs the lines; a receiver that closes it on a line (the game's end) stops the reading.
- **`LichessClient`** keeps the game's side: the stream is established by the first `gameFull`, back after the resync's moves (`connected()` before a lost move's message, as in LI-06), closed at the game's end; `gameStreamEnded` (before gameFull: the old messages) and `gameStreamRefused` (4xx mid-game). `_streamReply`, `_gameOver`, `_streamLost`, the timers and the backoff state are gone from it.
- **Tests**: `FakeLichess` moved to `ChessQtTests/FakeLichess.h` (with `waitUntil`); **`LichessStreamTest`**, 10 tests: lines and the end before it's established, closed → back, silent → back, keep-alive keeps it, 404 and 401 stop it, 503 is retried, the backoff doubles (100, 200, 400 ms), `reopen` isn't a loss, `close` while a reconnect waits, a receiver closing on a line. The LI-06 `LichessReconnectTests` unchanged and passing. Release builds; ChessTests + ChessQtTests **325 pass**; the 20 stream tests 10 runs.
- No behaviour change: the online scenes (`takeback`, `clock-online`, `premove*`, `reconnecting`) as before (screen locked: the known 0.06 % text difference in `premove-played`/`-dropped`).
- The branch switch for this step first failed on five locked files (they kept their pre-LI-06 content); stashed (`stash@{0}`, "stale pre-LI-06 copies"), `develop` pulled, the branch made again from it.

### Next
- Step 2: the always-open event stream and `incomingChallenges`.

---

## 2026-11-09 — BL-10 plan: Lichess challenges as notifications (branch: bl10Plan_091126)

### What was done
- Plan [`docs/BL-10-ChallengesPlan.md`](docs/BL-10-ChallengesPlan.md), with the owner's choices:
  - **Accept during a game depends on the game**: a running real-time Lichess game shows Decline only ("Finish your game first"); in a robot, two-player or correspondence game, Accept leaves that game;
  - **in the background**: a taskbar flash (`QWindow::alert`) and a sound, no Windows toast;
  - **unplayable challenges** (`compat.board` false) shown with Open on lichess.org and Decline;
  - **Your games** refreshed by `gameStart` / `gameFinish` on the same stream.
- Lichess facts it relies on, checked in `lichess-org/api` `doc/specs`: one event stream per token (a new one closes the old), keep-alive every 7 s, all current challenges and games resent when it opens; `challenge` events carry `compat.board`; the decline reasons.
- Design: LI-06's reconnect becomes a `LichessStream` class used by the game stream and an always-open event stream; `LichessClient.incomingChallenges`; `ChessConnector.acceptChallenge` (it may leave a game: one call, CLAUDE.md rule 1), `LichessClient.declineChallenge` (challenge setup, rule 2).

### Next
- Step 1: `LichessStream`, no behaviour change (#127, on top of LI-06, #125).

---

## 2026-11-09 — LI-06: Lichess games that don't break or lag (branch: lichessReconnect_091126)

### What was done
- **Reconnect** (`LichessClient`): the game stream lost during a game (it closes or errors, or brings nothing for 20 s: Lichess sends an empty keep-alive line every 7 s, seen in the owner's log of 2026-10-29) is opened again after 1, 2, 4, 8, then every 15 s, until it's back or the game is left (`stopStream`). Not after the game's end, and not after a 401 ("Lichess sign-in expired"). `gameConnectionChanged(false/true)`.
- **The gameFull that comes back is a resync** (`resyncMoves`): no new `gameStarted`, the clock isn't restarted. Lichess's list against ours (its moves plus ours not echoed yet): ours plus new ones → the opponent's moves as usual; without our sent move → it never arrived: taken back (as a refused move: the clock goes back) with "Your move didn't reach Lichess. Play it again."; any other difference → replaced (LI-03).
- **A move without an answer**: the POST now gives up after 10 s (`setTransferTimeout`; there was no limit, so a stuck POST waited for ever). No HTTP status at all (timeout, connection gone) → `resyncGame()` reopens the stream, whose gameFull tells. No "Move failed" message for that any more: it was wrong when the move had arrived. Lichess's own refusal (HTTP 4xx) stays as before.
- **`ChessConnector.Reconnecting`**: a caution bar "Connection to Lichess lost. Reconnecting… Your clock keeps running on Lichess."; "Reconnected to Lichess" when it's back. The player may move meanwhile: the move is posted, and the resync says whether it got there.
- **Premove at once** (from #124's open question): after the opponent's move, not after its slide. A zero timer: the opponent's move gets its own pieces sync first, so its piece starts sliding before ours.
- **Tests**:
  - 2 stream-line tests: a gameFull again is a resync (no start, only the new move); a resync without our move takes it back.
  - 7 against **`FakeLichess`**, a `QTcpServer` on 127.0.0.1 standing in for lichess.org (`setBaseUrl`, `setStreamTiming` for short waits), real sockets, no Lichess: the stream closes mid-game and the opponent moved meanwhile; the stream goes silent; keep-alive lines keep it; a move whose POST is cut off and Lichess hasn't got; a move whose POST times out but Lichess played; the game ends (no reconnect); leaving while reconnecting stops it.
  - Release builds; ChessTests + ChessQtTests **308 pass**, three full runs; the 9 new tests 25 runs. One early failure in a 5-run loop, not seen again: the keep-alive test allowed only 300 ms of silence, now 1 s.
- **Scene `reconnecting`** (in `all`): the bar over a Lichess game.
- **No 0.4 s hold in real-time Lichess games** (the owner's choice, only for games with a clock): `MoveGate::setHoldOnline(false)` from `onLichessClockStarted` for a real-time clock. The opponent's moves then wait only for the page (open a second, not turning, not reviewing), not for our move's slide and pause, so our Lichess clock doesn't run while the board holds their reply. Their piece may start sliding while ours still does, as on lichess.org. Correspondence, unlimited and robot games keep the hold; every new game starts with it (`beginGame`). 3 `MoveGateTest` tests.
- **Screenshots**: the screen was locked (`LogonUI`): the dialog scenes and `clock-promotion` / `clock-continue` failed as known for a locked screen, and text renders slightly differently (premove-played/-dropped: 0.06 %, only the move-list text). All board scenes match. `reconnecting` has no baseline yet: take it on an unlocked screen (`Compare-Screenshots.ps1 -Update -Name reconnecting,reconnecting-dark`).

### Code review (the owner's `/code-review medium` of #125, same branch)
9 findings: 8 fixed, 1 in part.
- **Fixed:**
  - **A move still on its way was taken back as lost**: a reconnect within the POST's 10 s could find Lichess without our move yet; the board took it back, then the POST arrived and its echo was played as the opponent's move. Now the moves whose POST has no answer yet (`_postsInFlight`) aren't lost: the answer or the echo decides. Test `ReconnectBeforeOurPostIsAnswered_WaitsForIt`.
  - **A 401 during a reconnect left "Reconnecting…" up for good**, and **a 4xx (a game Lichess no longer streams) was retried for ever, silently**. Lichess's own "no" (4xx) now stops the reconnects (`gameStreamStopped`, the bar goes, networkError says why); no answer or a 5xx (Lichess restarting) is still retried. Tests `ReconnectRefusedByLichess_StopsTrying`, `ReconnectServerError_TriesAgain`.
  - **"Reconnected" came before the moves** it had brought: now the moves, then "connected", then a lost move's message (so that one stays on screen).
  - **A move's POST with a non-error status below 400 counted as refused** (a 3xx): only Lichess's 4xx/5xx refuse it; anything else is resynced.
  - **Without the hold, a takeback's slide no longer held the opponent's next move**: only the player's own move is skipped now (`MoveGate::playerMoveShown`); pieces sliding back still hold it. Test `OnlineWithoutHold_StillWaitsForATakebackSlide`.
  - **The premove's order relied on a zero timer** running after the posted pieces sync: now a queued call to `playPremove`, posted after the sync to the same object (FIFO).
  - **Back from a review, the premove jumped in with the slide to the live position**: that path waits for the slide again (no clock hurry there: the opponent's move that brings our turn is the immediate path).
- **In part:** "two flags for one state (`LichessClient::_streamLost`, `ChessConnector::_reconnecting`)". `gameConnected()`, which only a test read, is gone. The connector keeps its own: it's the QML property, cleared with the game (`endGame`, a new game), while the client's is the connection's state.
- ChessTests + ChessQtTests **315 pass** (three full runs); the 14 reconnect and gate tests 10 runs. The board scenes as before (screen still locked: 0.06 % text in `premove-played`/`-dropped`); `robot` and `premove-robot` pass `-Offscreen`.

### Not checked yet (the owner, with a real Lichess game)
- Pull the network cable / turn Wi-Fi off for 30 s in a game: the bar after at most 20 s, back on its own once the network is, with the opponent's moves made meanwhile.
- Sleep the laptop during a correspondence game, wake it: the game catches up.
- A move while offline: "Your move didn't reach Lichess" after the reconnect, or the game simply goes on if it got there.
- Premoves and instant replies feel as fast as on lichess.org (a fast opponent's reply now slides in while our piece still slides).

### Next
- The owner's checks above; the `reconnecting` baseline on an unlocked screen.

---

## 2026-11-09 — LI-05: premove (branch: premove_091126)

### What was done
- **The owner's choices**: premove vs the robot and on Lichess, not in a two-player game (both sides sit at the board); an illegal premove is dropped silently, as on Lichess.
- **`Game::GetPremoveTargets(square)`** (ChessCore): where a piece may go once the other side has made one move, by its shape only. A line stops at our own piece (it may be taken: a recapture) and passes one of theirs (it may move away), not two. A pawn steps unless our own piece is in front, and has both captures. The king can castle while it and that rook haven't moved and none of our pieces stands between. Never onto our own king. 8 tests (`PremoveTests.cpp`).
- **`ChessConnector`**:
  - `canPremove()`: a robot or Lichess game, the opponent's turn, not reviewing, not finished. `MovableColor` is then the player's colour, so the pieces can be picked up as usual.
  - `figureSelected` / `movePiece` set the premove instead of moving: our piece shows its premove targets (`PremoveDots`), a dot or a drop on one sets `PremoveFrom`/`PremoveTo`. A new premove replaces the old one. A click on another square, a drop elsewhere, a right click or Esc (`cancelPremove()`) cancels it. Picking another piece keeps the pending one until a new target is chosen.
  - When the robot's or the opponent's move has been played (`playRobotMove`, `playOnlineMoves`), the premove waits for that move's slide (`slideMs`, 0 with Reduce animations), not for the gate's pause, then is played like the player's move if it is legal then (`playPremove`). A pawn to the last rank becomes a queen (Lichess's "auto-queen when premoving"). A selected piece without a premove gets its legal-move dots when the turn comes.
  - Cancelled by: the game's end, a new / continued / loaded game, leaving, a takeback vs the robot, a Lichess move list replaced. While reviewing it stays but isn't shown.
- **The board**: the two squares in red (`Theme.premove`), the premove dots in dark red (`Theme.premoveDot`); a right-click `TapHandler`; Esc in the window's Escape shortcut (move sheet first, then the premove, then leave).
- **`LichessClient::postMove`** does nothing in screenshot mode (`setOffline`), so a scene can play our move.
- **Scenes** (`main.cpp` feeds a Lichess game, no network): `premove` (Bf1-c4 waiting, the d2 pawn's premove dots), `premove-played` (Black's Nc6, then Bc4), `premove-dropped` (e4-e5 waiting, gone after Nc6), all in `all` with baselines. `premove-robot` (not in `all`, Stockfish): 1. e4, Nf3 premoved and played after the robot's answer; checked once, no baseline.
- Release builds; ChessTests + ChessQtTests **298 pass**. Screenshots: 82 compared, 0 failed; 6 new premove baselines (light and dark).

### Code review (the owner's `/code-review medium` of #124, same branch)
9 findings: 7 fixed, 1 in part, 1 left for the owner.
- **Fixed:**
  - **A premove outlived a move made by hand**: in the moment it waits for the opponent's slide the player may already move. That move now clears it (`makeMove`), or it would have been played a move later in another position.
  - **A click on our own piece that is a dot set a premove onto it** (e.g. Nb1 selected, then a click on the d2 pawn gave Nb1-d2). A click on our own piece now always selects it; a recapture premove is made by dragging.
  - **The robot's move that can't be played** left the premove on the board: cancelled.
  - **The review dropped the premove** although the property only hides it: it is now kept, and `reviewLast()` plays it (if it is our turn) once the board is back at the live position.
  - **Esc left the game while premove dots showed**: it clears them first, like the right click.
  - **The promotion rule** moved to ChessCore: `Game::IsPromotion(from, to)` (1 test).
  - **The player's colour** in one helper, `isPlayerPiece()`. `GetPremoveTargets` and the castling checks are `const`.
- **In part:** "`GetPremoveTargets` repeats the piece geometry of `MoveGeneration`". It's `const` now, but stays separate: its lines follow other rules (through one of theirs, onto our own pieces), which `GenerateBasicMoves` can't express without a flag through all its paths.
- **Left for the owner:** "the premove waits `slideMs` and that costs clock time". The open question above.
- Release builds; ChessTests + ChessQtTests **299 pass**; screenshots 88 compared, 0 failed; `premove-robot` checked again.

### Not checked yet (the owner, in a real window)
- vs the robot: premove by click and by drag, change it, cancel it by right click, Esc and a click on an empty square; a premove that becomes illegal (e.g. the robot takes the piece) disappears; a recapture premove (onto our own piece the robot takes); a premove promotion becomes a queen; castling as a premove.
- On Lichess: the same against a person, and that the premoved move reaches Lichess (the opponent sees it at once).
- The premove waits for the opponent's slide (about a quarter of a second): on a fast clock that time counts. Lichess plays it at once; tell me if it should not wait.

### Next
- The owner's checks above; then the LI-02 and LI-04 hand tests, or the next task.

---

## 2026-11-04 — LI-02 step 5: docs and hand tests (branch: li02Docs_041126)

### What was done
- **`docs/LI-02-ManualTests.md`**: 15 scenarios, 7 on this computer and 8 on the real Lichess.
- **Scene `clock-flag-draw`**: the draw on a flag against a lone king, by a real 32-ply game, so nobody has to play one by hand. The last path of step 2 that had never run.
- **Docs**: `CLAUDE.md` (`GameClock`, `TimeControlPicker`, the tests, the challenge), the `run-chess-app` skill (the new scenes, the settings flake, the clock in screenshot mode).

### LI-02 is coded (#118–#122); open
- The owner's hand tests. Two questions only they can answer: does Lichess end a flagged game by itself (the Board API has no flag call), and how far the shown time drifts.
- A move lost on the way (a POST that times out and never reached Lichess) leaves the board and the clock waiting for an echo: a board resync, its own task if the owner wants it.

### Next
- The owner's choice: the hand tests, then LI-01, BL or the next item in `TASKS.md`.

---

## 2026-11-04 — LI-02 step 4: the challenge card (branch: challengeCard_041126)

### What was done
- **The picker on the card**: `TimeControlPicker` gets `online` (no Bullet, no No clock) and `blitzAllowed`. Blitz tiles are dim until a username is entered, with a line saying why. Real time is now the card's default, not Unlimited, so the tiles show first.
- **The rating**: "Your Rapid rating: 1688" for the chosen mode, also Correspondence; nothing for Unlimited or a pool without a rating.
- **By name**: an optional "Lichess username" field. `LichessClient::challengePlayer()` sends a direct challenge through the rematch's `challengeUser()` and its wait for the game. The Create button then says "Challenge <name>", and stays off for a Blitz tile with no name.
- **The rematch's code assumed every direct challenge was a rematch**: the opponent's name came from the last game, so a new challenge would have named the wrong player, and the dialog, the declined message and the errors all said "rematch". `_directOpponentName` holds who it was sent to, and `_pendingRematch` is now true only in a game, as for a link.
- **Usernames**: `isUsername()` (Lichess's `[a-zA-Z0-9_-]{2,30}`) before the name goes into the URL. The test caught `$` letting a trailing newline through: `anchoredPattern`.
- **Scenes**: `online-blitz`, `online-challenge`; `online` and `online-empty` show the new card.
- **Tests**: 3 in `LichessClientTest`. Release builds; ChessTests + ChessQtTests **290 pass**.
- **Review of #122**:
  - A challenge by name with days per move or unlimited no longer needs the app open: Lichess keeps those two weeks (only a clock's `timeControl.l` is swept after 20 s), so we don't hold a request. `directChallengeLeft`: a message, and the game comes up under Your games. A correspondence rematch goes the same way.
  - `open` and `ai` aren't usernames: they're other calls next to `/api/challenge/:user`, and `open` would have created an open challenge.
  - A late answer to a cancel no longer closes a newer challenge. A challenge by name no longer deletes a saved link. The card can't stay busy after a decline.
  - The arrow keys skip dim tiles. The screenshot hooks left the page: the scene finds its items by `objectName`.

### Not checked yet
- Nothing has run against the real Lichess (step 5): a challenge by name, Blitz, declined, no such player, days per move by name. `challengePlayer()` past the name check has no test: it needs the network.
- The challenges are casual, as links and rematches always were here: `rated=false`. The card shows a rating, but these games don't change it.

### Next
- Step 5: docs and hand tests.

---

## 2026-11-04 — LI-02 step 3: the Lichess clock (branch: lichessClock_041126)

### What was done
- **The stream**: `LichessClient` now reads the clock: `clockStarted(limitMs, incrementMs, correspondence)` after `gameStarted`, `clockUpdated(whiteMs, blackMs, whiteToMove, running)` after the moves of every gameFull and gameState. A real-time clock runs from the second move, and the state that ends the game stops it with the final times.
- **Our move** switches the clock at once (`moveOnBoard()`, was `localMoveOnBoard()`), and Lichess's next state brings the exact times. A state without our sent move yet is older than our board, so it's skipped.
- **A refused move**: the list is replaced as before, and the clock goes back to the side to move without its increment, because no state follows. Any other list comes with Lichess's exact times.
- **Low time**: our own side only. Rejoining with little time left, the sound waits until the page has opened (`GamePage`); checked in a throwaway run.
- **Correspondence**: the plan's question is answered from Lichess's source (`BotJsonView.scala`): `wtime` and `btime` are the time left for the move, so `secondsLeft` isn't needed. A game with no clock at all sends the largest int, which is ignored. `GameClock::startCorrespondence()` runs from the start, is never low and makes no sound, as `corresClockView.ts`.
- **Correspondence format differs from the plan**: Lichess shows `2 days 5 hours`, `One day`, `05:30` (hours and minutes), `45:10`, without tenths. `formatCorrespondence()` follows it; `format()` no longer knows days.
- **Scenes**: `clock-online` (the opponent low, our clock lit), `clock-correspondence`.
- **Tests**: 6 in `GameClockTest`, 8 in `LichessClientTest`. Release builds; ChessTests + ChessQtTests **287 pass**. All other scenes unchanged.
- **Review of #121**: the clock's own estimate now runs only after a refused move. On every list change it could take increments off times that were already exact, and play a false low-time sound that then stayed silent at the real crossing. The rejoin sound waits for the page instead of being dropped for the rest of the game. A correspondence clock no longer shows `72:00:00` for a moment.

### Not checked yet
- Nothing has run against the real Lichess: step 5's hand tests. Open there: does Lichess end a flagged game by itself (the Board API has no flag call; until then the clock stays at `00:00.0`), and how far the shown time drifts from Lichess's.
- Our move switching the clock has no test: `ChessConnector` has none, and a scene can't make a move without the network.
- `players` failed once in a full run (a shift of about a pixel), then passed 6 of 6.
- Left from the review, on purpose:
  - A move POST lost without an answer (a timeout) leaves our board and clock waiting for an echo that never comes. The board is out of sync too, so that needs its own fix (a resync), not a clock one.
  - The stream closing mid-game: there's no reconnect yet, and the clock goes on counting. Lichess's time does go on, so that's true as far as anyone can know.
  - After a refused move the round trip is charged to the opponent until the next state: under a second normally, and refusals are rare.
  - The clock switches when Lichess's state arrives, up to 0.4 s before the opponent's move has slid in. Holding it back would show us more time than we have.

### Next
- Step 4: the challenge card.

---

## 2026-11-04 — LI-02 step 2: clocks in local and robot games (branch: localClocks_031126)

### What was done
- **The picker**: `TimeControlPicker.qml` (new) in both setup dialogs: Lichess's quick-pairing tiles, 1+0 to 30+20, and No clock (the default). The last choice is kept in `AppSettings`.
- **The clock on the cards**: `PlayerCard` shows it at the right, lit while it runs, red when low, the same width at any time. `ChessConnector.Clock` is the `GameClock`; `startComputerGame` and `startLocalGame` take `limitSec` and `incSec`, and a rematch keeps them.
- **The moves**: `localMoveOnBoard()` switches the clock after the player's and the robot's move. A takeback vs the robot keeps the time spent, takes off the increments of the moves taken back and runs the side to move's clock. Review doesn't stop it.
- **A flag**: `onClockFlagged()`: "<side> ran out of time" with the flagged king, or "Draw: <side> ran out of time" when the other side can't mate. It ends the game for good: no takeback (the Undo button is disabled), as on Lichess. The robot's answer on its way is dropped.
- **Stalemate (new for local games)**: `Game` only reports a mate, so a stalemated side would have lost on time. `localMoveOnBoard()` now ends the game as "Draw by stalemate" (`Game::IsStalemate()`), with or without a clock. Before, a stalemate just left the game sitting there.
- **Continue**: the autosave keeps the time control and both times (`GameSaveInfo`). The clock is paused until the page has been open for a second (`pageOpened`).
- **The robot**: `GameClock::moveBudgetMs()` caps levels 3 to 5 at a thirtieth of its time plus half the increment, at least 0.1 s, never more than half of what's left (`findMove`, `FindMove`). Levels 1 and 2 search by depth and are quick anyway.
- **Low-time sound**: `sounds/lowtime.wav`, drawn by `scripts/make_sounds.py`.
- **Checked against Lichess's source** (`clockView.ts`, `clockCtrl.ts`), as the plan asked: hours are padded (`01:30:00`, was `1:30:00`), and below a minute's start the low line is a fifth of it, at least 2 s. Both fixed. The rest matched. Not copied: hundredths on a stopped clock under a second.
- **Scenes**: `clock`, `clock-low`, `clock-flag`, `clock-promotion`, `clock-continue`, `stalemate`, `players-many`; `computer` and `players` show the picker. Time stands still in screenshot mode; `setScreenshotClock()` sets what a scene shows.
- **Tests**: `GameClockTest` (`MoveBudget`, the line, `timeUp`), `FindMove_StaysWithinTheClock_Test`, the autosave's clock fields. Release builds; ChessTests + ChessQtTests **273 pass**. All scenes light and dark: only `computer` and `players` changed, baselines accepted.
- **Real time, once**: a throwaway build without the freeze showed the clock ticking, tenths, a flag by the real timer and Continue resuming. Not committed.
- **Review of #120**:
  - A flag no longer outlives its game: `stop()` forgets it, so a next game without a clock keeps its Undo.
  - Review could flag the robot for free, since the gate held its answer while its clock ran. Now the answer brings the board back to the live position, as a Lichess opponent's move does, and entering a review plays a held answer (`showHeldMoves`). Checked with the real engine in a throwaway scene.
  - A flag during the piece choice: the pawn goes back and the picker closes (`clock-promotion`). The move used to complete after the result.
  - Move and Undo over and over added increments for ever. Lichess gives the time back on a takeback (`Rewind.scala`), but there the opponent must agree; here that would be free thinking time, so only the increments go (`moveTakenBack`).
  - Continue then Review in the first second froze the clock: it now starts from `pageOpened()`, not from the board settling.
  - An engine failure pauses the clock instead of giving the player a win on time.
  - `Game::IsStalemate()` replaces the connector's own check (the rule belongs in `Chess::`); `endGameWithLoser()` and `dropRobotAnswer()` replace the copies.
  - The two-player dialog scrolls when many opponents would push Start out of a small window (`players-many`). The clock pill grows for a longer time.

### Not checked yet
- Nobody has heard `lowtime.wav`. A whole robot game with a clock hasn't been played (step 5's hand tests).
- The robot can still flag: the gate's 0.4 s between the player's move and its answer counts on its clock, which matters only in a very long game without increment, and levels 1 and 2 aren't capped.
- `settings`, `about` and `restore` sometimes differ by a line: the Settings page's engine lookup races the shot (not from this work).

### Next
- Step 3: the Lichess clock.

---

## 2026-11-03 — LI-02 step 1: GameClock and CanMate (branch: gameClock_031126)

### What was done
- **`GameClock`** (`gameclock.h/.cpp`, new, not used by the app yet): the two clocks of a game, built like `MoveGate` with a time source tests can replace. `moveMade()` for local games, `setSnapshot()` for Lichess's times or a saved game, `setTurn()` after a takeback, `pause()`/`resume()`. It runs only after both sides have moved; the increment goes to a mover whose clock ran. `format()`: `03:00`, `1:30:00`, `00:09.4`, `3 days`. `flagged()` only for local games. `lowTime()` once below Lichess's line (an eighth of the start, 10 s to 60 s), again only above 1.5 times the line and after 20 s. The timer wakes only when the shown time changes.
- **`Board::CanMate(side)`** (ChessCore): the rule of Lichess's `InsufficientMatingMaterial` (checked against scalachess): false for a lone king, K+N against a king and nothing but queens, and bishops on one square colour unless the other side has a pawn, a knight or a bishop on the other colour. It decides whether a flag loses or draws.
- **Review of #119**: a Lichess clock at zero no longer wakes its timer every millisecond; a move or takeback after the time ran out no longer cancels the flag (the timer can be late); the low-time sound only while that side's clock runs, so never during a paused Continue; K+N against queens only is a draw.
- **Tests**: `GameClockTest` (15), `MatingMaterialTests` (8). Release builds; ChessTests + ChessQtTests **265 pass**. No UI change, so no screenshots.

### Next
- Step 2: clocks in local and robot games.

---

## 2026-11-03 — LI-02 plan: game clocks and Lichess speeds (branch: li02Plan_031126)

### What was done
- Plan [`docs/LI-02-ClockPlan.md`](docs/LI-02-ClockPlan.md), with the owner's choices: a Lichess-like clock in every mode (Lichess, two players, vs Robot); red when low, tenths under 10 s, a low-time sound, the correspondence deadline if the Board API gives it; Blitz online through a challenge by Lichess username. `TASKS.md`'s LI-02 rewritten (Bullet/UltraBullet: local only; no dependency on LI-01).

### Next
- Step 1: `GameClock` (fake-clock tests) and ChessCore's `CanMate`.

---

## 2026-11-03 — LI-03: an accepted Lichess takeback on our board (branch: lichessTakeback_031126)

### What was done
- **`LichessClient`** keeps two lists: the moves the stream has shown (`_lastMovesList`) and ours that were sent but not echoed yet (`_sentMoves`, `trackSentMove()`; before, our move was added to the stream's list at once). A gameState's moves (`applyStreamMoves`):
  - **more moves**: the echoes of ours are dropped, the rest are the opponent's (`opponentMoveReceived`), as before;
  - **the same moves while ours is on its way** (e.g. a draw offer's state came first): nothing; this used to be harmless and must not look like a takeback now;
  - **shorter** (an accepted takeback, by either side) **or different** (Lichess has another move than ours, or an earlier move differs): **`moveListReplaced(allMoves)`**.
  - **A move Lichess refused** (the POST got an HTTP error) also replaces the list (`moveRefused`), so our board takes it back instead of staying a move ahead ("Move failed: …" as before). Without an answer (a timeout) the move stays: Lichess may have played it, and the stream tells.
- **`ChessConnector::onLichessMoveListReplaced`**: clears the gate (a held opponent move may be one of those taken back) and what waited for it, leaves the review, drops a "takeback" offer bar, undoes back to the moves both lists share, rebuilds the move list, then plays the rest of Lichess's list (anew without slides when more than two plies change). A pure takeback (not a refused move) emits **`movesTakenBack(plies)`** → "Move taken back" / "2 moves taken back". A draw offer that waited for the held move is shown after it, not dropped. During a promotion choice the list waits until the piece is chosen, then replaces that move (which isn't sent). After the game's end it isn't applied (the result stands).
- **Tests** (`LichessClientTest`, 5 new): a takeback, then the game grows again (our echo, the opponent's answer); the opponent's move taken back; our sent move not in the state yet; the lists disagree (another move, an earlier move); `gameFull` drops sent moves.
- **Check scene `takeback`** (not in `all`): `main.cpp` feeds a `gameFull` with four moves, waits for the board to show them (`boardIdle`), feeds the list after a takeback of two, and grabs the window once the board is idle again: 1. e4 e5, last move e7–e5, "1… e5", "Tester to move", "2 moves taken back". New baseline.
- Release builds; ChessTests + ChessQtTests **242 pass**.
- **Code review** (`/code-review` of #116), fixed: a timeout no longer takes our move back; a promotion no longer loses the new list; a held draw offer isn't dropped; no "Move taken back" for a refused move; "anew" counts the undone plies too; nothing after the game's end; the scene sets the game id through `LichessClient::setCurrentGameId()` (no id-from-`gameFull` fallback in real code) and starts on `ChessConnector.screenshotSceneSetUp()` from the scene instead of a 500 ms timer. Not done: a `ChessConnector` unit test (ChessQtTests doesn't build the connector yet: it needs QtGui/Quick in the test project) — the `takeback` scene covers the path.
- **Screenshots**: after the review fixes, on an unlocked screen, all 58 baselines match (`all` light and dark, `takeback`, `robot`, `select`/`drag` `-Black`).

### Next
- The owner's manual round: LI-04 (`docs/LI-04-ManualTests.md`). The real takeback and the draw offer are done: the owner checked all four cases of each on 2026-11-03 (we offer / they offer, accepted / refused).
- Then LI-05 (premove).

---

## 2026-11-03 — LI-04 step 5: docs and the manual tests (branch: li04Docs_031126)

### What was done
- **`docs/LI-04-ManualTests.md`** (new): the plan's eight manual scenarios (§6) as step-by-step checks with the app's own texts and a Result line each, plus **4b** (Resign from the warning, from Home and on window close, and the 5 s fallback offline). Corrected against the code: a correspondence game shows hours up to two days ("47 h", "23 h"), not "1 day".
- `TASKS.md` LI-04: coded (#111–#114), manual tests left. Plan status the same. `CLAUDE.md`: `LichessClient.ongoingGames`.

### Next
- The owner's manual tests (`docs/LI-04-ManualTests.md`); then LI-03 (apply an accepted Lichess takeback).

---

## 2026-11-03 — LI-04 step 4: the leave warning (branch: leaveWarning_031126)

### What was done
- **`LeaveOnlineDialog.qml`** (new, like `ImportConflictDialog`: three buttons): "Leave the game? The game goes on at lichess.org and your clock is running. You can come back to it from Continue or Play online." **Leave** (default, Enter), **Resign**, **Stay** (Esc). A small dialog of its own rather than a middle button on `ConfirmDialog` (plan §4.4: decided with the screenshot).
- **Home and Esc** during a running Lichess game (`_onlineRunning`: in game, online, no result) open it (`ChessGame.qml` `leaveGame()`; the old body is `_doLeaveGame()`). A finished game, a robot or two-player game: no question, as before.
- **Closing the window** during a running Lichess game asks the same (`onClosing`, title "Quit during the game?", **Quit** instead of Leave). Not in screenshot mode.
- **Resign** is `ChessConnector.resignAndLeave()` (new, after the code review): it resigns and calls `endGame()` **once Lichess has ended the game**, so the loss counts in the statistics (`onLichessGameEnded` records it; stopping the stream at once would lose it). After 5 s without an answer it leaves anyway (then only Lichess has the loss). Quit closes the window when the game has been left. Home or closing again while it waits doesn't ask twice (Home leaves at once; closing quits when it's through).
- Leave is `ChessConnector.endGame()` as before.
- **Switch user** during an online game adds "You can come back from Continue or Play online."
- **Screenshot scene `leave-online`** (in `all`): the `game` scene (1. e4 e5) with the dialog open (an online game needs Lichess, so it's over a local game).
- Release builds; ChessTests + ChessQtTests 237 pass. Screenshots: 55 compared, 0 failed; `leave-online*` new (accepted). No QML warnings in the scene logs.
- Not checked live: the close and Resign paths need a running Lichess game (owner's manual tests 1 and 4, plan §6).

### Code review (the owner's `/code-review medium`, same branch)
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

### Next
- LI-04 step 5: docs (`TASKS.md` LI-04 done, plan status).

---

## 2026-11-03 — LI-04 step 3: Home's Continue for online games (branch: homeContinueOnline_031126)

### What was done
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

### Next
- LI-04 step 4: the leave warning (Leave / Resign / Stay, also on window close); step 5: docs.

---

## 2026-11-02 — LI-04 step 2: the Online page (branch: onlinePage_021126)

### What was done
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

### Code review (PR #112 comments, same branch)
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

### Board previews (owner's request, same branch)
- The owner's check: **Continue rejoins the game** ✓. The owner pointed at Lichess's own "Current games" page, which shows each game as a small board ("Ваш ход" = your turn).
- "Your games" is now a **grid of cards** like that (two columns from 440 px, else one). Each card has:
  - a **`BoardPreview`** (new `BoardPreview.qml`: a still board from the row's `fen`, the game board's squares, pieces and last-move tint, turned for Black). A click on the board continues too;
  - the opponent (rating) and "Your turn" / "Waiting";
  - speed · time left (the board shows our side; the accessible name still says it);
  - Continue or Open on lichess.org.
- Card and board sizes come from the page's content width. Binding them to the GridLayout's own width laid everything out in one column with the text over the boards, because that width isn't known yet when the cards are sized.
- The `online` scene's seeded games got positions and last moves; `online` / `online-dark` baselines re-accepted (53 compared, 0 failed). ChessTests 111, ChessQtTests 126 pass.
- The Debug build's link of `LiChessConnector.dll` failed (LNK1168): the owner's Debug `ChessGui.exe` was running. This round has no C++ change in that library; the Release build is complete.

### Second code review (the owner's `/code-review medium`, same branch)
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

### Where to continue
- **Owner's check** (Release build of this branch):
  - Play online: your unfinished games are listed as boards; Continue (or a click on a board) opens it in the current position;
  - Back and Escape return to Home;
  - "Days per move" creates a correspondence challenge (Lichess shows "2 days" etc.);
  - not signed in: the page says so and Open profile works.
- **Step 3**: Home's Continue (the robot first, otherwise the Online page). **Step 4**: the warning when leaving a running online game.

## 2026-11-02 — LI-04 step 1: the unfinished Lichess games in `LichessClient` (branch: ongoingGames_021126)

### What was done
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

### Code review (PR #111 comments, same branch)
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

### Where to continue
- **Step 2** (after this is merged): the Online page.
- When convenient: rejoin a Rapid game from the lichess.org lobby, to confirm the lobby case.

## 2026-11-02 — LI-04 plan: rejoin unfinished Lichess games (branch: rejoinGamesPlan_021126)

### What was done
- The owner checked #109 live: a challenge created as Black is played as Black, and rejoining by link or id works.
- **Plan written**: [`docs/LI-04-RejoinPlan.md`](docs/LI-04-RejoinPlan.md), following the owner's decision.
  - An **Online page** replaces the "Play online" dialog, with these sections: Your games (Continue rejoins, no id), New game (with days per move), and Join by link.
  - Home's **Continue** resumes the robot game first, otherwise opens the Online page.
  - A **warning** when leaving a running online game (Leave / Resign / Stay, also when the window is closed).
- Lichess facts it relies on, from the API spec:
  - `GET /api/account/playing` lists real-time and correspondence games (most urgent first) with `isMyTurn`, `secondsLeft` and the opponent (player / anonymous / AI);
  - the event stream sends all current games when it opens, but only one event stream may be open per token;
  - rejoining is `streamGame(id)` → `gameFull`, which BL-14 step 5 already shows as one resync.

### Where to continue
- **Step 1** (new branch from `develop`): `LichessClient` ongoing games + tests, then a **live check with the owner's account**: does the Board API stream a game started on lichess.org itself? The answer decides between Continue and "Open on lichess.org" for such games.

## 2026-11-02 — Lichess: the chosen side and the join link (branch: lichessColorJoinFix_021126)

From the owner's manual check of BL-14 step 5 (scenario 1 passed apart from these; scenario 3, Fool's mate, passed).

### What was done
- **Bug 1, "Black" selected but the game started as White**: Lichess's `POST /api/challenge/open` has **no `color` parameter**. The app log shows every challenge answered with `"color":"random"` and a random `finalColor`, and the app always played `finalColor`. The side is chosen when a player **accepts** an open challenge: `POST /api/challenge/{id}/accept?color=white|black` ("only valid if this is an open challenge", Lichess API spec). Now:
  - `createOpenChallenge` no longer sends `color` and takes our seat with `accept?color=<chosen>` (`waitForGameStart(id, true, color)`); "Random" accepts without a colour (Lichess's `finalColor`);
  - after a restart, a pending challenge is resumed with `resumeOpenChallenge(id, friendUrl)`: we play the side the friend's saved link (`…?color=white`) doesn't;
  - joining by link takes the side in the link (`joinFromUrl`: `…?color=black`, `…/<id>/black`).
- **Bug 4, the `https://lichess.org/game/<id>` link was rejected** ("Invalid URL or game ID"), although it's the form Lichess gives for open challenges (and the one we show to the friend). `gameIdFromUrl` accepts `/game/`; `colorFromUrl` reads the side. The dialog makes one call, `LichessClient.joinFromUrl(text)`.
- Tests: `LichessClientTest` +2 (`GameIdFromUrl_ChallengeLink_ReturnsId`, `ColorFromUrl_PlayerLinks_Test`). Debug and Release build; ChessTests 111, ChessQtTests 121 pass in both; `online` and `home` screenshots unchanged.
- Not verified against Lichess itself (needs the owner's account and a second player): see the checks below.
- **Backlog** (`TASKS.md`): bug 2 (no way back to a game after leaving it, no warning; correspondence games) and the "list of unfinished games" part of bug 4 → **LI-04**; bug 3 (premove) → **LI-05**. **LI-02 corrected**: the Board API allows only Rapid, Classical and Correspondence (Blitz only in direct challenges), so Bullet and UltraBullet are impossible for this app; the dialog's 1+0, 3+2 and 5+3 presets for open challenges need checking.

### Where to continue
- **Owner's check**: create a challenge as Black, open the friend's link (it ends `?color=white`) in the browser; the app must play Black (board turned). Then as White, and Random a few times. Join by the `https://lichess.org/game/<id>` link and by a `…?color=black` link.
- LI-04 needs the owner's decision on where the unfinished games are listed.

## 2026-11-02 — BL-14 step 5: the Lichess opponent through the gate (branch: lichessThroughGate_021126)

### What was done
- **The opponent's moves wait for the board.** `LichessClient::opponentMoveReceived` now goes to `ChessConnector::onLichessOpponentMoves()`, which pushes each move into the gate as `Source::Online` (no `robotMinMs`: only the slide and pause). `playOnlineMoves()` plays a released move (or a batch), updates the history and counters, and marks the board busy (`moveShown(true)`). Lichess's own turn rule (`canPlayerMove()`) keeps the player from moving while the move waits.
- **Resync** (`gameFull` on (re)joining a game with moves): more than two moves at once come out as one `releaseBatch`. The pieces are shown anew without slides (`BoardPieces::reset`, now public, via `_piecesAnew`), and there is one move sound; only a check in the final position is announced.
- **Game end while the opponent's last move waits** (e.g. a resync before the page has settled): the end is applied at once (game over, statistics, the flag), and the result card waits for the board as for any result (step 4). See the code review below; the first version held the end back.
- **The opponent's mating move was lost** (found while doing this): `LichessClient` returned on a `gameState` with a final status before emitting its new moves. Lichess sends the mate in that same state, so our board never showed it. The stream handler now emits the new moves first, then `gameEnded`. The per-line handling is `handleGameStreamLine()` (public, for tests).
- **Reviewing when the opponent moves**: the gate shows nothing while reviewing, but the player's clock runs on Lichess, so the board goes back to the live position (`reviewLast()`) and then shows the move.
- Plain `applyMoves()` (Alt+S dialog, screenshot scenes) stays direct.
- **Tests:** `LichessClientTest` +3 (mate move before the end, resync in one signal, only new moves), `BoardPiecesTest` +1 (`reset` without slides). Debug and Release build; ChessTests 111, ChessQtTests 117 pass in both.
- **Verified end to end** with a temporary hook (not committed) that fed stream lines into `LichessClient` in screenshot mode: (1) a resync of `f2f3 e7e5 g2g4` plus the opponent's `d8h4` mate and `status: mate` arriving before the page settled — the end waited, all four moves appeared at once after the settle, mate, result card; (2) the mate arriving after the resync was shown — the three moves at once, then Qh4# alone, then Lichess's end. Both screenshots: Qh4#, the flagged king, "Friend wins by checkmate".
- Screenshots: all 49 match. The 6 profile shots differed only in the "since <date>" text (today's date); their local baselines were re-accepted.

### Code review (PR #106 comments, same branch)
8 findings; fixed:
- **A resync on a settled page let the first move through alone** (each move was pushed separately, and an idle gate released the first at once). `MoveGate::push(QStringList, Source)` queues them all before the gate looks; test `ResyncPushedTogetherIntoAnIdleGateIsOneBatch`.
- **A held game end kept the game "running"** (rematch offers dropped; Resign / Offer draw still posted to an ended game), and **a held end stayed hidden while reviewing**. The held end is gone: Lichess's end is applied at once, the card already waits for the board. This also removes the two loose strings and the second winner-to-score mapping (cleanup finding).
- **Re-entering the review while the opponent's move waits** hid it until the review ended. Entering a review of an online game (`reviewFirst/Prev/To`) now plays the held moves first (`MoveGate::releaseAll()`, test `ReleaseAllLetsTheHeldMovesThroughAtOnce`); the board shows an earlier position anyway.
- **`_piecesAnew` stayed set if a resync's first move failed**: now set only when more than one move was played.
- **A draw offer / takeback request showed before the move it came with**: `afterHeldMoves()` runs it once the held moves are shown.
- **`playOnlineMoves` copied `applyMoves`' loop**: both use `playMoves()` now. Side effect: `applyMoves()` (screenshot scenes, Alt+S) also announces only a check in the final position. The `review` and `moves` baselines showed a stale "Check" bar from …Qa5+ earlier in the scripted game (the final position is not check); they were re-accepted without it.
- **Found while verifying (a step-3 bug): `GamePage` reported a stale "settled"** to the gate. `onReviewStateChanged` / `onTurningChanged` called `_reportSettled()` before the `_boardSettled` binding had been updated, so entering a review reported "settled" and leaving it reported "not settled" — a move held then waited forever (the robot's answer after Back to live too). Now `on_BoardSettledChanged` reports it, on the next pass (`Qt.callLater`) because the gate may play a held move at once, which re-enters the binding (QML warned of a binding loop).
- Not fixed here: **an accepted Lichess takeback** (shorter move list) is not undone on our board — the existing gap from BL-04 §7; now **LI-03** in the Lichess backlog (`TASKS.md`, PR #107).
- Verified with the temporary stream hook (not committed), five scenarios: mate held during the settle (result at once, card after the move), mate after the resync, a resync on a settled page (0 → 4 moves in one step), a draw offer with a held move (offer after the moves), review while a move is held, then the opponent's move + resignation during the review (back to live, move, card). No QML warnings. ChessTests 111, ChessQtTests 119 pass in Debug and Release; 49 screenshots match, `robot` included.

### Where to continue
- **Manual checks by the owner with a real Lichess game** (plan §5 rows 8 and 9): the opponent answers instantly (our slide, then theirs); reconnect with several moves (the position at once, one sound); the opponent mates us (the move slides, then the card); review while the opponent moves.
- Steps 3–5 manual checks are still open (see the step-3 entry).
- Step 6 (render-thread slides) only if the step-1 numbers still show stalls.

## 2026-10-31 — BL-15 fixed: the engine thread stops without crossing threads (branch: engineKillFix_311026)

### What was done
- The `robot` scene — it ends a game while the engine is still thinking — crashed at shutdown in **2 of 3** offscreen runs: `QThread: Destroyed while thread '' is still running`. The steps: `stopEngineThread()` called `killEngine()` → `QProcess::kill()`/`state()` **from the GUI thread** (QProcess is thread-affine), which could fail to unblock the engine thread's UCI read; after `wait(3000)` it deleted the `QThread` anyway, and Qt aborts on a running thread.
- **`UciConnector::Kill()`** now terminates the engine through its own Win32 process handle: `OpenProcess(PROCESS_TERMINATE)` after a successful start, `TerminateProcess` on Kill, `CloseHandle` on restart/destruction. The handle is owned by a `std::unique_ptr` with a `CloseHandle` deleter and sits behind a mutex; QProcess is never touched from another thread, and a failed `OpenProcess`/`TerminateProcess` is logged. A stale pid can't kill an unrelated process (the handle pins the process object).
- **`EngineWorker`:** `killEngine()` sets an `_abandoned` atomic and calls the player's thread-safe `KillEngine()` through an atomic player pointer stored when the player is created (no shared_ptr read from the GUI thread). `findMove()` gives up before creating the engine and again before searching; `ChessEnginePlayer::FindMove` checks the same flag after it restarts a dead engine, so a restart cannot slip through a stop.
- **`stopEngineThread()`:** kill → `quit()` → wait 3 s. If it doesn't stop, the thread is **left behind** (`setParent(nullptr)`, pointers dropped) — never `QThread::terminate()`: TerminateThread can kill the thread inside the heap or the log lock and leave it held. The thinking state is cleared on both paths.
- **`UciConnector::Init()`**'s `uci`/`uciok` handshake now has a 10 s deadline (it previously looped forever on an engine that starts but never answers — a thread that could not be stopped cleanly).
- New test `EngineWorkerTest.Kill_StopsAThinkingWorker_FromAnotherThread_Test`: the worker on its own `QThread`, a 2.5 s search, `killEngine()` from the test thread, the thread must exit within 2.5 s.
- `ChessEnginePlayer::KillEngine()` stays as the thread-safe wrapper the engine tests use.
- Verified: `robot` scene **5/5** offscreen runs pass (before: 1/3); Debug and Release build; ChessTests 111, ChessQtTests **111** (the new test) pass in both.

### Code review (medium, same branch)
14 findings; fixed:
- **`QThread::terminate()` removed** — TerminateThread can leave the CRT heap or `AppLog`'s mutex locked (the GUI thread then hangs); the leave-behind path is now the only fallback, and both stop paths share one cleanup tail.
- The terminate path's worker/process leak is gone with it.
- **Stop during Init/restart**: the cancel check inside `FindMove` closes the window where a restart started after `killEngine()`.
- **A hung handshake** no longer lasts forever (10 s deadline).
- **Silent handle failures** are logged (`OpenProcess` pid + error, `TerminateProcess` error).
- **Raw `HANDLE` as `void*`** replaced by a `std::unique_ptr` with a `CloseHandle` deleter.
- **No automated test** for the cross-thread kill — added.
- **Late sweep addendum:** `IsRunning()` now reports a killed engine immediately (`_killed` flag), so `ChessEnginePlayer::FindMove`'s restart recovery cannot send the next request to the process that was just terminated; test `IsRunning_IsFalseRightAfterKill_Test`.
Not changed (with reasons): the GUI can still wait up to 3 s when the kill misses (the engine process normally dies in ms; the alternative, `terminate()`, is worse); on non-Windows `Kill()` is a no-op (the app is Windows-only); the public `Connector()` accessor was removed in favour of the atomic player pointer.

### Where to continue
- BL-15 is merged (#104). BL-14 step 4 (`resultOnIdle_311026`) is rebased onto it and its `robot` scene re-run (3/3); next is step 5 (the Lichess opponent through the gate).

## 2026-10-31 — BL-14 step 4: the result card on the gate's idle (branch: resultOnIdle_311026)

### What was done
- **The result card no longer waits a fixed 1600 ms from the result.** On a result, `GamePage` now waits for the board to settle — the move gate's idle: the mating move's slide and pause are over, or, for a resignation / draw / timeout (no move), the board was already still — and then shows the card after a **600 ms beat**, so the final position is seen first.
  - `ChessConnector.isBoardIdle()` (Q_INVOKABLE, `MoveGate::isIdle()`) lets the page check the state when the result arrives: a result that comes with a move is checked on the next pass (`Qt.callLater`), after the move has marked the board busy; otherwise the next `boardIdle` shows it.
  - Undo after mate/resign still clears the pending result and hides the card.
- `ScreenshotScene`: a finished game's ready delay is 2000 ms (the card now comes after the settle + beat).
- Verified with the Debug build **offscreen** (the screen was locked): `result`, `ended` and `robot` logs show result → page settle (`openTimer` at 1 s) → `boardIdle` → card 600 ms later → screenshot; the `result` and `robot` frames were inspected (card, mate, flagged king correct). ChessTests 111, ChessQtTests 110 pass in Debug and Release.
- **Real-window comparison done after the unlock:** `Take-Screenshot.ps1 -Scene all`, `-Scene all -Dark` and `-Scene robot` on this branch, then `Compare-Screenshots.ps1`: **49 compared, 0 failed** (`result`/`result-dark` show the card over the settled board, `robot` too). The `about` pair failed at 0.09 % first — only the build timestamp of the fresh build (`Version 0.1.0 Build Oct 31, 2026, 4:13:12`) — and both were re-accepted with `-Update -Name 'about*'` (local baselines). The `robot` scene passed in a real window this run.
- **Rebased onto BL-15 (#104)** after its merge: Debug and Release build; ChessTests 111 + ChessQtTests 113 (224) pass in both; the `robot` scene — it ends the game while the engine thinks, the BL-15 shutdown path — passed **3/3** real-window runs, and `robot`, `result`, `result-dark` match the baselines.

### Code review (medium, same branch)
3 findings, all fixed:
- **A result that arrives while reviewing stayed pending**: the gate is deliberately not-ready during review, so neither the one-shot `isBoardIdle()` probe nor a `boardIdle` edge could show the card (the old 1600 ms timer ignored review). Now a pending result shows when it arrives during review (`_resultIdleCheck` treats review as "no live animation"), and `onReviewStateChanged` shows it if the user enters review after the result. Verified with a temporary scene (resign while reviewing): the card shows over the reviewed position.
- **MoveGate lost its idle edge when retiming** (`setTiming` after an idle edge, e.g. Reduce animations switched off within a pause): `updateBusy()` moved the window into the future while `_wasIdle` stayed true, so the next idle was suppressed. `tick()` now clears `_wasIdle` whenever the board is busy again; new test `RetimingAfterIdleEmitsTheNextIdle`.
- **ScreenshotScene guessed the card timing** (2000 ms): it now polls `_resultShown` instead, so it grabs only once the card is actually up.

### Where to continue
- **Step 5**: the Lichess opponent's moves through the gate (`push(..., Source::Online)`, `releaseBatch` for a resync, `moveShown` after applying them).
- Manual checks for step 3 are still open (see the step-3 entry below).

## 2026-10-31 — Debug crash fixed: the log handler wrote from two threads (branch: logRaceFix_311026)

### What was done
- The owner's **Debug** build crashed during a normal robot game: Qt's `ASSERT: "bytes <= bufferSize"` in `QRingBuffer::free()` (`qringbuffer.cpp:76`, Qt 6.12.0). The screenshot, dump analysis and a standalone repro are in [`docs/bugs/2026-10-31-qringbuffer-assert-crash.md`](docs/bugs/2026-10-31-qringbuffer-assert-crash.md).
- **Dump:** the faulting thread was the engine worker (`0x476c0`), inside `EngineWorker::findMove` → the Qt read path of its `QProcess`; no `stopEngineThread`/`killEngine` frames. **Log:** in 1,985 lines there was exactly one duplicated consecutive line — `ask: request 37 posted` twice — at the crash moment, i.e. the GUI and engine threads wrote the log at the same time. Missing request numbers (4, 8, 11) show the owner pressing Undo while robot answers were held.
- **Root cause:** `customLogMessageHandler()` writes the shared `g_logFile` with a `QTextStream`, and Qt calls message handlers from any thread without serializing them. Two streams writing the same `QFile` corrupt the heap; the corruption surfaced as the ring-buffer assert in the engine thread. In Release there is no assert, so it would be silent.
- **Reproduced:** a standalone Qt console program with the same handler and two logging threads dies with `STATUS_HEAP_CORRUPTION` after 4 lines, duplicate line included; with a `QMutex` + flush around the write it writes 40,001 intact lines and exits cleanly.
- **Fix:** the log writer is now `AppLog` (`ChessGui/applog.h/.cpp`): `open` / `close` / `write`, with a `QMutex` + `QMutexLocker` around each and a flush per line so a crash keeps the last lines. `main.cpp`'s message handler calls it.
- **Regression test** (`ChessQtTests/AppLogTest.cpp`): two threads write 2,000 lines each; it requires the exact line count, the documented format, and every line exactly once. It was written first against the unsynchronized writer: the test host died with no results (`vstest` exit 3) — the same crash as the app, now caught by the suite; with the mutex it passes in 0.7 s.
- Debug and Release build; ChessTests 111, ChessQtTests 110 pass; `home` and `robot` scenes unchanged.

### Where to continue
- **The owner re-tests the Debug build**: normal robot play, pressing Undo often; the duplicated log lines and the assert should be gone. The stress program in the report is the regression test.
- **BL-15 (new, TASKS):** `stopEngineThread()` still calls `QProcess::kill()`/`state()` from the GUI thread while the engine thread uses the same `QProcess`, reads `_enginePlayer` unsynchronized, and deletes the thread even if `wait(3000)` timed out — a separate cross-thread hazard, documented in the crash report.
- BL-14 step 4 is next (PR #102 is merged).
## 2026-10-30 — BL-14 step 3: the robot through the gate (branch: robotThroughGate_301026)

### What was done
- **The robot's answer is held, not played on arrival.** `onEngineMoveFound` pushes it into the gate as `Source::Robot`; `EngineThinking` stays true until the gate releases it, so the "Robot is thinking…" bar, the wait cursor and `MovableColor` stay until the move is shown. `playRobotMove()` is the old play tail moved out of the handler (play the move, history, counters); it consumes `_resignAfterEngineMove` too, so a resignation pressed meanwhile happens after the answer (the trap noted in step 2).
- **The board settles** (`GamePage.qml`): `ChessConnector.setBoardSettled(bool)` after the page has been open for 1 s and the board isn't turning (`ChessBoard.turning`); a Flip or a new game as Black holds the answer until the turn ends. `setMoveTiming(Theme.reduceMotion ? 0 : Theme.slow)` on opening and whenever Reduce animations changes.
- **The player's own move** calls `moveShown()` in `onHumanMoveDone()`: the robot's answer waits for its slide + pause and at least `robotMinMs`. The player's move itself stays instant.
- **Clearing:** `beginGame()` (every new, continued and online game), `stopEngineThread()` (also when no engine is running, so every path that stops the robot), `endGame()`, `loadGame()`. A **takeback** drops the held or still-coming answer, bumps `_engineRequest` and clears the thinking state (otherwise `movePrev()`'s `_engineThinking` guard would make Undo a silent no-op); the undo slide is marked with `moveShown()`, so a re-asked answer waits for it. A **live review** drops the waiting answer; `reviewLast()` asks the engine again.
- **Verified** with temporary scripted scenes (not committed):
  - Continue as Black: Qh5# was held from its arrival until the page settled, then played (469 ms later);
  - level 1, the player plays e4, with `robotMinMs` temporarily raised to 1500: the answer arrived after 751 ms and was held until exactly 1499 ms after the player's move;
  - a takeback while that answer was held: it was never played, the thinking state cleared, and no new engine request was made.
  - Continue in check (Qxf7+ and the robot to move): the answer was held to the settle and played; the first `boardIdle` came after it (count changed), so no check was announced — the check sound/bar now follow the gate (fixing the 1 s race below).
- Debug and Release build; ChessTests 111, ChessQtTests 109 pass; all 49 screenshot scenes match, the `robot` scene (Continue, Qh5#) included.

### Code review (medium, same branch)
5 findings, all fixed:
- **Undo while the robot's first answer was pending**: the thinking block dropped the answer, then `count == 0` returned before re-asking, so the robot never moved — and `canPlayerMove()` let the human move its pieces. Now a count-0 Undo re-asks when the robot is to move, and, as a safety net, a computer game only lets the player move its own colour.
- **The deferred resignation (`_resignAfterEngineMove`) survived a takeback**: the next answer would resign on its own. A takeback now cancels it.
- **Review jumps**: dropping the answer on review and re-asking wasted the running search and could show the answer during the jump. New design: a live review reports the board as not settled (`_boardSettled` excludes `ReviewMode`), so the answer simply stays held — no drop, no re-ask; `reviewLast()` marks the jump with `moveShown()` so the answer waits for it.
- **The resumed-check announcement raced the held answer**: `checkShownTimer` and the settle timer fired in the same tick, announcing a check that the answer parried in the same pass. The check is now announced on the gate's first `boardIdle` (new `ChessConnector.boardIdle`), which is silent while an answer waits. This is part of step 4's §3.3, pulled in here because step 3 introduced the race.
- **Stacked searches after review round-trips**: gone with the no-drop review design.
Not changed: a takeback during a search still lets that search finish before the re-ask — the engine cannot be cancelled mid-search without more work in `EngineWorker`; the answer is simply dropped (request number) and a new search queued.

### Where to continue
- **Step 4 (new branch from `develop`)**: the result card on the gate's `idle` (replace the 1600 ms `resultTimer`; use `isIdle()` for a result that comes without a move — resignation, draw, timeout). The check sound/bar already follow `boardIdle` (done here); mate and the result card are left.
- **Step 5**: the Lichess opponent's moves through the gate (`push(..., Source::Online)`, `releaseBatch` for a resync, `moveShown` after applying them).
- **Manual check by the owner** (Release, animations on): level-1 quick moves (never both pieces sliding at once); a new game as Black (board turns first, then the robot's move); Flip board while it thinks; the robot gives check / mates; Continue in check; takeback / New game while its answer waits; **Undo before the robot's first move** (as Black; the robot must still move); review ←/→ while it thinks, then Back to live; Reduce animations on.
- The owner's §3.4 decision is still open (currently A: think during the animation, show the answer after).

## 2026-10-30 — BL-14 step 2: `MoveGate` and its tests (branch: moveGate_301026)

### What was done
- **`MoveGate`** (`ChessGui/movegate.h/.cpp`; not wired into `ChessConnector` yet — that is step 3): holds the moves that come from outside and releases them one at a time only when the board is idle.
  - Idle = `setStageReady(true)` (the page has settled and the board isn't turning) and `now >= busyUntil`. `moveShown(byRobot)` sets `busyUntil = now + slideMs + pauseMs`; a robot move also waits `robotMinMs` after the player's move, an Online move does not.
  - More than 2 waiting at once (a Lichess resync) are released together as one `releaseBatch`, so the receiver can show the position at once; otherwise one at a time.
  - `clear()` drops a waiting move and the pacing of the old board; `idle()` fires once each time the gate becomes idle — when the page settles and after each shown move's slide (step 4 uses it for the check sound and the result card). `isIdle()` answers the same question on demand.
  - Timing comes from QML later: `slideMs = Theme.slow`, `0` with Reduce animations; `pauseMs` stays, and `setTiming()` shortens a window already under way. A single-shot `QTimer` drives it; the clock is injectable and `tick()` is public so the tests drive time by hand.
- **12 tests** (`ChessQtTests/MoveGateTest.cpp`, fake clock): a robot answer waits for slide + pause + `robotMinMs`; an Online move ignores `robotMinMs`; stage not ready holds; `clear()` drops, and resets the pacing; two Online moves come out one after the other; more than two at once as one batch; `slideMs = 0` still keeps the pause; shortening the timing under way; a release without `moveShown()` still settles; `idle()` once, after the last released move, and `isIdle()`.
- `ChessQtTests/TestSupport.h`: the `ToString(QString)` specialization the test DLL needs, shared by its five test files instead of copied into each.
- Debug and Release build; ChessTests 111, ChessQtTests 109 pass; the `home` screenshot still matches (no UI change).

### Code review (medium, same branch)
11 findings, 7 fixed:
- `idle()` is edge-triggered, so a result without a move (resignation, draw, timeout) would never get one; added `isIdle()` for step 4 to ask.
- A resync batch was N ordinary releases, which a receiver couldn't tell from N moves (a sound and a slide each); now one `releaseBatch`.
- A released move that the receiver applies as a position reset (no `moveShown()`) left no busy window, so `idle()` never came; the gate now marks the board busy before it releases.
- `clear()` kept the old board's pacing into the new game; it resets it now.
- `setTiming()` claimed to shorten a running slide without doing so; it recomputes the window now.
- The `schedule()` copy of `tick()`'s pacing rules is gone.
- The `ToString` copy-paste is one shared header.
Not changed: `setClock()`/`tick()` stay public as the documented test seam; non-move animations (the takeback/undo slide, the first paint, review jumps) still need the connector to call `moveShown()` — listed for step 3; a Robot answer's release also sets a busy window optimistically, which the connector's `moveShown()` simply confirms.

### Where to continue
- **Step 3 (new branch from `develop`)**: the robot through the gate — `onEngineMoveFound` does the stale check then `push`; the release plays the move (`playRobotMove()`), `EngineThinking` stays true until it is shown, `clear()` on new game / takeback / leaving / review; `GamePage` reports stage ready (at least 1 s after opening and not turning, with `Theme.slow` as the slide time).
  - **Move the `_resignAfterEngineMove` consume block with the play tail into `playRobotMove()`**: it only works today because `onEngineMoveFound` clears `_engineThinking` before it; left behind, a resignation while the answer waits would be re-deferred and never happen.
  - **A takeback while the robot's answer waits in the gate**: `movePrev()` returns at once when `_engineThinking` is set, so Undo would silently do nothing and the held answer would still slide in. Before that guard, step 3 must `_gate->clear()`, bump `_engineRequest`, clear `_engineThinking` and `emit engineThinkingChanged()`, then let the takeback run — the dropped answer never reaches `onEngineMoveFound()`, so nothing else would reset the thinking state.
  - **Call `moveShown()` after a takeback** (and any other non-move board animation, e.g. the new game's first paint), so the engine's answer waits for the undo slide.
  - Fixes the robot moving together with the player and while the board turns.
- Then step 4 (check, mate and result on `idle`; `isIdle()` covers a result without a move), step 5 (the Lichess opponent's moves, and `releaseBatch` for the resync).
- The owner's §3.4 decision is still open (recommended A: think during the animation, show the answer after).

## 2026-10-30 — BL-14 step 1: the freezes measured and fixed (branch: boardFreezes_301026)

### What was done
- **Frame trace** (`frametrace.h/.cpp`, off unless `QT_LOGGING_RULES="chess.frames.debug=true"`, Release too): the app log gets gaps between two frames the board drew over 25 ms ("slow frame"; over 500 ms "no frame for" — the window resting, or a long stall), a 16 ms GUI-thread heartbeat's late ticks ("GUI thread busy", however long — this also catches the page-open stalls), and how long the board's own work took — the move dots, the pieces sync, the signals after a move — with log timestamps to line them up. `FrameTrace::watch(window)` in `main.cpp`.
- **`slides` screenshot scene** (not in `all`): animations on (every other scene forces them off), selects the g1 knight and plays Nf3, Nf6, Bc4, Nc6 0.4–1 s apart. It makes the numbers below reproducible: `ChessGui.exe --screenshot out.png --scene slides --delay 5200` with `QT_LOGGING_RULES="chess.frames.debug=true"`, then read `out.png.log`.
- **`NavigationLayer.qml` without rebuilds:** the 64 dots stay for the page's life; a selection only shows and hides them. Before, the `Repeater` was bound to `PossibleMoves`, so every click and every move (dots cleared) destroyed and created all 64 delegates, and each new delegate started its 450 ms pop-in **even when its dot was hidden**. Now the pop-in starts from `onVisibleChanged`, only for a dot that appears, and hiding a dot stops it.
- **`MoveListModel`** (`movelistmodel.h/.cpp`, `QAbstractListModel`, 12 tests in `ChessQtTests/MoveListModelTest.cpp`): the move list is `ChessConnector.MoveHistory` as a model; a move adds a row (White) or fills Black's half of the last one, a takeback removes or clears that row, never a reset; only the new plies are copied. Before, the whole `QVariantList` was rebuilt on every move and `MoveList.qml` read `modelData`. `ChessConnector.ShownMove` (e.g. "12. Nf3", "12… exd5") replaces the QML computation from the list; Copy/Export PGN use `PlyCount` instead of `MoveHistory.length`.

### Measurements (Release, the `slides` scene above, animations on; 3 runs of the old code, 2 of the new)
| | before | after |
|---|---|---|
| Selecting a piece (`PossibleMovesChanged` work) | 3.5–4.3 ms | 0.5–0.8 ms |
| The move right after a selection (dots cleared) | 3.5–4.2 ms | 0.4–1.1 ms |
| Later moves, dots already hidden | 0.2–0.4 ms | 0.4–0.8 ms |
| `select` scene in screenshot mode (no animations) | 2.9 ms | 0.3 ms |

What the owner sees as "the slide stops for a few ms" is the thread that runs the animation doing this rebuild in the same frame the slide starts; it is ~4 ms, a quarter of a 60 Hz frame, plus the hidden pop-ins that keep animating. Frame gaps over 25 ms during the slides were **not** reproducible: one run of the old code showed 21 frames at 32–34 ms stretched over ~3 s, but later runs of the same build showed none, so that was not the delegate rebuilds alone. The trace now reports frame gaps and GUI-thread stalls, so if the owner still sees stops, the log says which side it is. (Checked with the code review: the original frame trace silently dropped gaps of 500 ms and more, and had no GUI-thread measure; both are fixed.)

- Debug and Release build; ChessTests 111, ChessQtTests 97 pass; all 49 screenshot scenes match the baselines (light, dark, `select`/`drag -Black`), `robot` scene OK, no QML warnings.

### Code review (medium, same branch)
12 findings, 8 fixed: the frame trace's 500 ms blind spot and its missing GUI-thread measure (now the heartbeat), the pop-in that kept running when its dot hid, test assertions that counted signals but not their rows/roles (now they check both, and a new test covers a shorter game with other moves), the redundant full copy of the move history on every move, the robot's move trace that double-counted its inner scopes and claimed a rejected move was played, and the test inventory in `CLAUDE.md`. The rest, not changed: the model keeps its own copy of the moves (a model owns its data; only `ChessConnector` calls `show()`), SAN piece letters are inherent to SAN, `ShownMove` and the cells' accessible names are built separately, the test scaffolding repeats `BoardPiecesTest`'s, and `GamePage`'s `shownMove` alias is a plain pass-through.

### Where to continue
- **BL-14 step 2 (`MoveGate`) and step 3 (the robot through the gate)** — the next branch from `develop`; the plan is `docs/BL-14-MovePacingPlan.md`. The owner's decision on §3.4 is still open (recommended: think during the animation, show after).
- The frame trace and the `slides` scene stay for step 6: if the owner still sees stops after the gate is in, they will say whether GUI-thread work or the render side is left.
- BL-04's manual drag check is still open.

## 2026-10-30 — BL-14 plan: one thing at a time on the board (branch: movePacingPlan_301026)

### What was done
- The owner tried BL-04 (#96, #98 merged; slides, castling, en passant fine) and found: the level-1 robot's answer slides together with the player's move; as Black the robot moves while the board turns; sounds follow the game, not the board; slides sometimes stop for a few ms.
- **Plan:** `docs/BL-14-MovePacingPlan.md`, task BL-14 in TASKS. A `MoveGate` (ChessGui) holds moves from outside — the robot's answer, the Lichess opponent's move — and plays one only when the board is idle: page open ≥ 1 s and not turning (PR #95's rule, now general), last slide done, a short pause (and ≥ ~400 ms after the player's move for the robot). A held move isn't played on `Game` until shown, so last move, check, move list and sounds stay in step by themselves; `EngineThinking` lasts until the move is shown. #95's `checkShownTimer` and the 1600 ms `resultTimer` become "the gate is idle". Freezes: measure first (frame trace in Debug); suspect `NavigationLayer`'s `Repeater` over the `PossibleMoves` string list (64 delegates rebuilt on every click and after every move).
- Checked while planning: Lichess echoes of our own moves are skipped (`LichessClient::postMove` appends to `_lastMovesList`); a resync on joining comes as one batch.

### Where to continue
- **Owner's decision** (plan §3.4): the robot thinks during the animation and its move is shown after (recommended, planned), or it is asked only once the board is idle.
- Step 1 (measure the freezes, `NavigationLayer` without rebuilds) and step 2 (`MoveGate` + tests) can start in either order, each on a new branch from `develop`.
- BL-04: the manual check of dragging is still open.

## 2026-10-30 — BL-04 steps 6–7: dragging pieces (branch: dragPieces_301026)

### What was done
- **Drag and drop** (`ChessFigures.qml`): each piece has a `DragHandler` (`target: null`), enabled for the side to move when the player may move (`ChessConnector.MovableColor`). Starting a drag selects the piece (dots); the piece stays faded on its square and `ChessBoard` draws it under the pointer, a little lifted, above the dots. The square under it is outlined (`ChessBoxes`, `Theme.dropSquare` = accent). A drop calls `ChessConnector.movePiece(from, to)`: played → the piece only settles from the drop point into the square (the drop point becomes its slide offset), refused (illegal square, off the board) → slides back, dots cleared. Dropped on its own square → stays selected. A press without a drag is still a click (`TapHandler`).
- **Esc** while dragging puts the piece back (`ChessBoard`'s Shortcut, `dragCancelled`); the window's Esc (leave the game) waits while a piece is held (`GamePage.pieceDragging`).
- Cursors: open hand over a movable piece, closed hand while dragging. The pointer is mapped from scene coordinates into the board's, so it is right on the turned board.
- **Screenshot scene `drag`** (in `all`): 1. e4 e5, the g1 knight held over f3; `-Black` too. `main.cpp` knows the name.
- Tested with a temporary hook (not committed) that sent mouse events to the window: Nf3 by drag; g1→g4 refused (back, no dots); Nf6 as Black on the turned board; press-release = click (selected, dots); a7xb8 by drop (pawn on b8, promotion dialog); Esc mid-drag (back, no move, still on the game page). Synthetic events need timestamps, else each move looks like a new press.
- Release and Debug build; all 49 shots (46 scenes, `select`/`drag -Black`) match, `drag` baselines new; no QML warnings.

### Where to continue
- **Manual check:** plan §5 rows 2, 3, 7, 11–14 — dragging with the mouse vs the robot (White and Black, after Flip board), refused drops, Esc, promotion by drop, no dragging while the robot thinks / in review / after mate / opponent's turn online, touch if available, Reduce animations on. Then tick BL-04.
- Plan §7 risk still open: the last-move highlight comes one event-loop pass before the piece (not noticed so far).

## 2026-10-30 — BL-04 step 5: slides and fades (branch: pieceSlides_301026)

### What was done
- **Pieces slide** (`ChessFigures.qml`): when a piece's `square` changes it slides there (`Theme.slow`, `OutCubic`), whoever moved it — the player, the robot, a Lichess opponent, a takeback, one review step. The slide is an offset in squares (`slideFile`/`slideRank`, animated to 0), not a `Behavior on x/y`, so resizing the window or turning the board doesn't slide anything, and a slide that starts while another is under way starts from where the piece is drawn.
- **Captures fade out** (`taken`, `Theme.normal`), under the piece that slides onto the square; **new pieces fade in** (a model reset — new game, review jump, Continue — or a piece put back by a takeback). This replaces the old 450 ms fade-in per piece.
- **Promotion:** the pawn slides and turns into the new piece when it arrives (`shownPiece`); the player's choice after the pawn's move turns it in place. A queen taken back turns into a pawn once on the 7th rank.
- Reduce animations (and screenshot mode): pieces jump and appear at once.
- Release and Debug build; all 46 scenes (light, dark, `select -Black`) match the baselines, no QML warnings. A temporary scene with animations on (not committed) showed e4xd5 mid-slide: the pawn over d5's fading pawn at 60 ms, settled by 1 s.

### Where to continue
- Manual check by the owner (animations on): moves, castling and en passant slide and fade as planned.
- Step 6 (plan §3.4): dragging — `DragHandler` (`target: null`) with a pixel `dragOffset` next to the slide offset, `movePiece`, slide back on a refused drop, Esc, cursors, the drop-square outline.

## 2026-10-30 — Continue fixes: autosave only vs the robot, check sound after the page shows (branch: continueFixes_301026)

### What was done
- **Only a game vs the robot is autosaved.** Closing the app (`~ChessConnector`) autosaved any unfinished game, a Lichess game too; `endGame()` also autosaved two-player games. Home then offered them as a local game, but neither can really be resumed. Now one rule, `canAutoSave()` (vs the robot, not online, not a replay, not over, at least one move), decides in both places. `canContinue()` ignores an older autosave of another kind (`autoSaveInfo().isSinglePlayer`), so it isn't offered; `continueGame()` lost its two-player branch. Two-player games can no longer be continued (owner's decision: neither they nor Lichess games can be resumed properly today).
- **Check sound on Continue.** `continueGame()` replays the saved moves (`ResumeFromLoad`), and every check in them emitted `checkNotify`: the sound (once per check of the game) and the "Check" bar came before the game page appeared. The replay is quiet now (`_resuming`), and `GamePage` signals `checkShown()` when it has settled — at least 1 s after it opened and after the board has turned to the player (`ChessBoard.turning`; Black) — if the position is in check and no move was made meanwhile (that move announces its own check); `ChessGame.qml` plays the sound and shows the bar (`announceCheck()`, shared with `onCheckNotify`).
- Screenshot scene `continue` now autosaves a game vs the robot (Level 3, as White: no engine needed); its Home card says "vs Robot · Level 3".
- Debug and Release build; ChessTests 111, ChessQtTests 85 pass; all scenes match (`continue` baselines renewed).

### Where to continue
- Manual check: Continue a robot game that stands in check (sound and bar once, after the board shows); close the app during a Lichess game and a two-player game (no Continue offered on Home next time).

## 2026-10-30 — BL-04 step 4: the QML board on `Pieces` (branch: boardQml_301026)

### What was done
- **One board geometry** (`ChessBoard.qml`): `squaresSize`, `squareSize`, `squareX(square)`, `squareY(square)` (a1 at the bottom left, in the board's own coordinates) and `squareAt(x, y)`. `ChessBoxes`, `ChessFigures` and `NavigationLayer` place their items with them; the three X-mirrored `Grid`s and the per-item Y-mirror/180° counter-rotations are gone. The board still turns for Black with `rotation`; a piece turns back with `rotation: chessBoard.angle` (same animation as before).
- **`ChessFigures.qml`**: a `Repeater` over `ChessConnector.Pieces`, one item per piece at its square (`taken` ones hidden until step 5 fades them). Before, every move removed and re-inserted `ListModel` rows, rebuilding square delegates (64 on page creation and on every review step).
- **Clicks**: one `TapHandler` on the pieces layer → `figureSelected(squareAt(...))`, instead of 64 `MouseArea`s. The selected piece's tint is in `ChessBoxes` (`SelectedSquare`), above the other highlights.
- **Removed**: `EmptyBoardModel.qml`, `ChessConnector::boardChanged`, `refreshBoard()`, the QML-only `selectedIndex`. `emitBoardState()` (review) now only syncs `Pieces` and clears the dots.
- **Screenshot scene `select`** (in `all`): 1. e4 e5 with the g1 knight selected — tint and dots; `-Black` shows the turned board.
- Debug and Release build; ChessTests 111, ChessQtTests 85 pass; all 44 scenes (light, dark) and `robot` match the baselines, no QML warnings. The old `robot` baseline was still from Qt 6.10 (`all` doesn't take it): renewed, only its text antialiasing differed.

### Where to continue
- **Manual check (needs a person, screenshot mode ignores the mouse):** click-select-click moves vs the robot as White and as Black, in a two-player game, after Flip board; clicking an opponent's piece or an empty square; promotion; review ← →; takeback.
- Step 5 (plan §2, §3.4): slides (`Behavior on x/y`, `Theme.slow`), captured pieces fading out (`taken`), resets without slides, all off with Reduce animations.

## 2026-10-30 — BL-04 step 3: the pieces model (branch: boardPieces_301026, from pieceDrag_301026 / #92)

### What was done
- **`BoardPieces`** (`ChessGui/boardpieces.h/.cpp`, a `QAbstractListModel`): one row per piece, roles `pieceId`, `square`, `piece` ("N"/"n"), `taken`. `sync(board)` compares with the last shown position (`BoardDiff`): a moved or promoted piece keeps its row and id (`dataChanged`), a captured one is marked `taken` (its row stays until the next reset, so the fade isn't cut when the robot answers at once), a piece put back by a takeback is a new row. More than 4 changes (new game, review jump) → model reset with new ids.
- **`ChessConnector`:**
  - `Pieces` (CONSTANT). The board listener and `emitBoardState()` (review) post one queued `sync` per change (`schedulePiecesSync()`), so castling's four squares, a two-move takeback or a Lichess batch become one comparison. It syncs the **shown** position: `shownGame()` (a saved game's replay, a reviewed replay, or the game), now also used by `checkedKing()` and `materialBalance()`.
  - `canPlayerMove()`: the checks `figureSelected` had (replay, review, robot thinking, promotion choice, game over, the Lichess opponent's turn), now shared. `MovableColor` (0 none, 1 White, 2 Black) on it, notified by the signals its inputs change with.
  - `SelectedSquare`: the clicked piece of the side to move, or -1 (today's tint is QML-only). `clearPossibleMoves()` resets it with the dots, in the 4 places that cleared the dots.
  - `movePiece(from, to)`: a drop. Plays it if legal (`GetAllowedMoves(from)`), else returns false; clears the dots either way.
  - `boardChanged` / `refreshBoard()` stay until step 4 moves the QML board to `Pieces`.
- **Tests:** `ChessQtTests/BoardPiecesTest.cpp`, 9 tests with `QAbstractItemModelTester` (warnings collected, expected none): every piece on the first sync, nothing on the same position, id kept on a move, capture marks taken, takeback of a capture, castling (2 rows), promotion keeps the pawn's id, a big change resets, role names. ChessQtTests now uses Qt Test (`testlib`).
- Debug and Release build; ChessTests 111, ChessQtTests 85 pass; all 43 screenshot scenes match the baselines (no QML change yet).

### Where to continue
- Step 4 (plan §3.4): one board geometry, `ChessFigures` on `Pieces`, a board `TapHandler`, the selection tint in `ChessBoxes`.
- Locally, after changing a project's `QtModules` (here `testlib`), Rebuild that project once: Qt's build cache keeps the old module list.

## 2026-10-30 — BL-04 plan, steps 1–2: takeback fix and BoardDiff (branch: pieceDrag_301026)

### What was done
- **Plan:** `docs/BL-04-PieceMovePlan.md` (pieces with an identity that slide, drag and drop, one board geometry), linked from BL-04 in TASKS.
- **Step 1, `Game::UndoMove` fixed.** It only moved the piece back and put a captured piece on the target square. After a takeback the castled rook stayed on f1/d1 (d8/f8), a pawn taken en passant came back on the target square instead of beside it, and a promoted piece went back to the 7th rank as itself. Takeback vs the robot (`ChessConnector::movePrev`) and stepping back in a loaded game (`HistoryPlayer::MoveBack`) use it; review (←/→) replays and wasn't affected. All changed squares are reported to the board listeners. En passant is recognised by the move before being a double step over the target square (`Game` records en passant as a capture on the target square). `ChessTests/UndoTests.cpp`: 14 tests (each castling, en passant both colours, promotion with and without capture, the player's promotion choice, two moves back), comparing FENs and the reported squares; 11 failed before the fix.
- **Step 2, `Chess::BoardDiff(before, after)`** (`ChessCore/BoardDiff.h/.cpp`; a class after review, first a free function `MatchPieces`): which piece went where between two positions: `Moved()` (`PieceTravel`: from, to, piece before/after), `Removed()`, `Added()`. Equal pieces pair nearest first; a pawn leaving the 7th (2nd) rank and a piece arriving beside it on the last rank is a promotion, reversed a takeback of one, on the same square the player's choice. `ChessTests/BoardDiffTests.cpp`: 11 tests, each case both ways.
- Debug and Release build; ChessTests 111, ChessQtTests 76 pass. No UI change yet.
- **Robot trace in Debug builds** (`ENGINE_TRACE` in `engineworker.h`, nothing in Release): the app log gets a line when the robot is asked (request number), when the worker starts and answers, when the answer is played or dropped as stale, and when the engine thread stops. For a flaky hang seen once in manual testing: playing Black vs a weak level, the board stayed on "Robot is thinking" although `uci_debug_log.txt` shows Stockfish answered in 40 ms; not reproduced since (8 runs of the `robot` scene, and the user's second try). If it happens again in a Debug build, the log shows where the answer stopped.

### Where to continue
- BL-04 step 3: the `BoardPieces` model and the `ChessConnector` wiring (plan §3.2–3.3), on a new branch from `develop` once this PR is in.
- Locally: after switching from #91's branch, ChessGui needs a Rebuild (its Qt build cache still has Widgets dropped).

## 2026-10-30 — Moved to Qt 6.12.0 (branch: qt612Upgrade_301026)

### What was done
- **Every Qt project now builds against `6.12.0_msvc2022_64`**: ChessGui, LiChessConnector, UciConnector, ChessQtTests. ChessCore was already plain C++.
- **ChessTests no longer uses Qt.** It was still a Qt VS Tools project (`QtVS_v304`, `core;network`, the QtMsBuild imports, include paths to UciConnector and ChessGui), but its tests use only ChessCore. Now it's a plain `Win32Proj`: its DLL depends only on the test framework and the C++ runtime, and its 86 tests pass with no Qt on `PATH`. CI (`release.yml`, `tests.yml`) installs 6.12.0 with only `qtmultimedia`: `qt5compat` and `qtshadertools` aren't used since BL-02 P1.
- **Modules:** the app needs Qt Multimedia (the `SoundEffect`s in `ChessGame.qml` and `SettingsPage.qml`). It's an add-on that the 6.12.0 install didn't have at first; with it added, nothing else is missing. Also not installed, and not used: Qt5Compat, Charts, Quick3D, ShaderTools, Quick Timeline.
- **Qt 6.11 change: `DialogButtonBox` sets `highlighted` on its buttons** and highlights only its new `defaultButton`. A `highlighted: true` on a footer button was overwritten to `false`, so every dialog's primary button lost its accent colour. The 8 dialogs with a primary button now set `defaultButton: primaryButton` on the box (see `AppDialog.qml`).
- **Checked:** Debug and Release build clean; 86 + 71 tests pass; all 42 screenshot scenes render. Compared with the 6.10 baselines, the rest of the differences are 6.12's semibold Fluent dialog titles, text anti-aliasing, and the Qt version and build date on About.

### Where to continue
- **CI installs aqtinstall from a pinned `master` commit** (`aqtsource:` in `release.yml` and `tests.yml`): 3.3.0, its latest release, can't find Qt 6.11+ for Windows (one repository folder per compiler since 6.11; fix miurahr/aqtinstall#1000 is merged but unreleased). Drop the `aqtsource` line once an aqtinstall release after 3.3.0 is out.
- Accept the new look as the baselines: `scripts\Compare-Screenshots.ps1 -Update` (local).
- Locally, put `D:\Tools\Qt\6.12.0\msvc2022_64\bin` on `PATH` in place of 6.10.2's (the app takes Qt's DLLs from there). You can also make 6.12.0 the default in Qt VS Tools.

## 2026-10-29 — BL-09 engine move on the GUI thread done (branch: engineThreadMove_291026)

### What was done
- **The engine thread no longer touches the app's `Game`.** Before, `ChessEnginePlayer::DoMove` ran on the engine thread and called `_game->DoMove()` while the GUI thread read the same game (captured pieces, material, last move, review). Now:
  - `ChessEnginePlayer::FindMove(history)` gets a copy of the move list and returns the engine's move in UCI. For FEN it replays the moves on its own `Game`. `DebugDump(history)` works the same way.
  - `EngineWorker::findMove(request, history)` runs on the engine thread and answers `moveFound(request, move)` or `moveError(request, …)`.
  - `ChessConnector::askEngineMove()` (one place for the robot's turn: after the player's move, a new game with the robot as White, a takeback, Continue) sends a copy of the moves. `onEngineMoveFound()` plays the move on the GUI thread. A move that can't be played goes the error path, with the FEN in the log.
  - A request number (`_engineRequest`) drops an answer for an old game or position: `stopEngineThread()` bumps it. `_gameOver` is a plain `bool` again: only the GUI thread uses it.
- **Tests:** `ChessQtTests/ChessEnginePlayerTest.cpp` (6 tests, real Stockfish like `UciConnectorTest`): mate in one found in StartPos, PureFen and FenWindow (12 plies, so a FEN plus moves); the dump; the worker returns the request number; an unresolved promotion is an error with the moves in the dump. All 157 tests pass.
- **Screenshot scene `robot`** (not in `all`: needs Stockfish): a level 5 game where the player is Black. The robot's first answer is dropped (`endGame()` while it thinks), then Continue: the robot mates with Qh5#, and the shot waits for the engine. All 42 baseline scenes still match.

### Where to continue
Next backlog candidates: BL-05 (stub engine, Quick tests: the `robot` scene would then run without Stockfish), BL-06 (engine choice), BL-04 (dragging pieces). The manual checks for BL-02 (P1–P6) are still open in TASKS.

## 2026-10-29 — BL-02 P6 Polish done (branch: polish_291026); BL-02 is coded, manual checks left

### What was done
- **Keyboard:** Ctrl+N (Home: new game vs the computer; a finished game: the result card's New game, so a running game is never dropped) and Ctrl+O (Home: open the saved game, or say there is none); both on the cards' tooltips and in About's shortcut list. Home cards take Enter as well as Space. The avatar picker (Create profile, Profile) and the opponent picker (Two players) are one Tab stop each, like radio buttons: arrow keys pick, and the new `FocusRing.qml` outlines the chosen one. A profile card's More button is hidden with opacity instead of `visible`, so Tab still reaches it.
- **Accessibility:** every icon-only button already had a tooltip and `Accessible.name` (checked); the pickers now have list roles and names. No OK/Yes/No buttons are left.
- **High contrast:** `Theme.highContrast` follows a Windows contrast theme (`styleHints.accessibility.contrastPreference`, Qt 6.10). The Fluent controls already switch to the system colours; our roles now do too (`SystemPalette`): window, windowText, disabled text, highlight as accent; no selected fill, no scrim, no Home photo; hovered cards get an accent border. The board keeps its colours. `--high-contrast` / `Take-Screenshot.ps1 -HighContrast` forces our part (the Fluent controls can't be forced).
- **Motion:** page transitions in the `StackView` (fade in and rise 24 px over `Theme.normal`, fade out over `fast`) replace the Basic style's 400 ms slide, and switch off with Reduce animations. `Theme.reduceMotion` is also on when Windows' animation effects are off (`AppInfo.windowsAnimationsOff`, `SPI_GETCLIENTAREAANIMATION`); Settings then shows the switch on and disabled.
- **App icon and installer:** a new icon (the board's white knight on a rounded tile in the board's green) in `chess.ico` (16–256 px), `pics/app-icon.png` (window, Welcome, About; replaces `chess_icon.svg`), and installer images `installer/art/wizard-*.bmp` for 100–250 % DPI with `WizardStyle=modern`. `scripts/make_icons.py` (Pillow) redraws them all. `Build-Installer.ps1 -SkipBuild` builds the installer with them.
- **Screenshot baselines (BL-05 step 4):** `scripts/Compare-Screenshots.ps1` compares `test-results\screens` with local baselines (`-Update` accepts), with a tolerance, and writes a diff image per failure. Screenshot mode turns animations off (`Theme.forceReduceMotion`, not saved), so two runs of all 42 scenes (light and dark) match; a swapped picture fails with a diff.
- Release and Debug build OK; all 151 tests pass; all 42 scenes OK, no QML warnings. The `run-chess-app` skill, `ThemeTokens.md` (§1, §6, §11), the plan's §7 and `UiTestSuitePlan.md` are updated.

### Code review (medium, same branch)
9 findings: 8 are fixed (one of them partly), 1 isn't changed.
- **Pages:** Reduce animations makes the page transitions instant, not disabled. Before, a disabled `popEnter` didn't undo the `pushExit` fade, so Home came back blank after Settings if the switch changed in between. `popEnter` also resets `y` after a push that was cut short.
- **High contrast:** the shown move in the move list gets an accent border. It had no highlight at all, since high contrast has no selected fill.
- **Arrow keys:**
  - In the avatar and opponent pickers, an arrow at an edge leaves the selection alone. Before, Up in the first row jumped to the first avatar.
  - An arrow that changes nothing no longer saves the profile. Each step that does change it still saves, as a click does.
- **Profile picker:** on a touchscreen, a tap on the hidden More button now selects the profile instead of opening the menu.
- **Ctrl+O** just calls `loadGame()`, which says itself when there is no saved game, so there's one message, not two.
- **Screenshots:**
  - `Take-Screenshot.ps1` deletes the old picture before each scene, so a scene that fails can't pass with last run's PNG.
  - `Compare-Screenshots.ps1` fails a baseline that has no new screenshot (`MISSING`), and writes a diff image only for failures.
  - The `restore` scene's backups get fixed times (9:00 and 9:30), so all 42 scenes now match exactly.
- Not changed: the opponent picker has no Up/Down. It is a `Flow` with a varying number per row, and Left/Right reach every opponent. The two step functions differ (a grid and a list), so they stay separate.

### Not checked live
- Keyboard in a real window: Tab order on Home, the profile picker, Create profile and Two players; the focus ring; Ctrl+N / Ctrl+O; the page transitions. Screenshots can't show focus (the window never takes it).
- A real Windows contrast theme, 200 % scaling, Windows animation effects off, and the installer's wizard pages. Listed in TASKS under P6.

### Where to continue
BL-02 is coded. What's left are the manual checks in TASKS (P1–P6). Next backlog candidates: BL-09 (engine thread vs GUI thread, a real bug), BL-05 (stub engine, Quick tests), BL-06 (engine choice), BL-04 (dragging pieces).

---

## 2026-10-29 — BL-02 P5 Settings and About done (branch: settingsAbout_291026); next: P6

### What was done
- **Settings page** (`SettingsPage.qml`, pushed over Home or the game like Profile; Back, Esc, Ctrl+,) replaces `SettingsDialog`. One `SectionCard` per group, one `SettingRow` per setting:
  - Appearance: theme; **board coordinates** around the board or on the squares (the squares then fill the board item: `Theme.boardSquares`); **Reduce animations** (`Theme.reduceMotion`: no board turn, piece fade, legal-move dot pop, check glow or turn pulse, sheet slide).
  - Sound: an on/off switch and a volume of 1–5 (muting is the switch only), with a preview sound.
  - Gameplay: **confirm before resigning** (`root.resign()`), **show legal moves**, **highlight the last move**.
  - Computer: the engine's name and path from a short `uci` run (`AppInfo.probeEngine()`, e.g. "Stockfish 19"), or "not found" with what to do; Show levels. BL-06 adds "Change…" there.
  - Data: Back up now, Restore… (`RestoreBackupDialog`: pick a backup; the one equal to `users.json` is disabled; off during a game), Export… / Import…, Open data folder.
  - Advanced: send the game to the engine as a position (FEN) or the moves (`ChessConnector.UseFen`, `config.json`), Open log folder.
  - About: icon, version and build (`Built Oct 29, 2026 · Qt 6.10.2`), GitHub and issue links, licences (Chess++ GPLv3, Qt LGPLv3 + source, Stockfish GPLv3 + source), keyboard shortcuts.
- **`AppSettings.qml`** singleton: the QtCore `Settings` that was `appSettings` in `ChessGame.qml`, with the same keys, so nothing is lost. On load, an unset `theme` comes from the old `darkMode`, and the old "volume 0 = muted" becomes sound off at `lastSoundVolume`. `Theme.scheme` binds to it directly (the `Binding` is gone). Note: `Settings` stores *every* property, so derived values are functions (`effectsVolume()`).
- **`AppInfo`** (C++ service singleton in `ChessPlusPlus`): version from `APP_VERSION_*` (now also defined for C++ in `Directory.Build.props`), build date, Qt version, data and log folders, engine probe. `UciConnector::EngineCandidates()` is the engine lookup that `Init()` and the probe share.
- **`UserManager`**: `backups` (number, date, profile names, `current`), `backupNow()`, `restoreBackup(n)`. A restore first backs up the current profiles, so it can be undone, and keeps the active profile logged in when the backup has it. Saved games aren't in backups (the Data card says so). 3 new tests in `ChessQtTests`; all 149 pass (ChessTests + ChessQtTests).
- **Board colours fixed** (not P5, found while testing the coordinates on the squares): the squares were mirrored, with a1 and h8 light and the queens on the wrong colour. Now a1 is dark (`ChessBoxes.qml`).
- **Scenes:** `about` (Settings scrolled to About) and `restore` (two seeded backups, the dialog open) are new; `settings` shows the page. All 21 scenes OK in light and dark, no QML warnings. The `run-chess-app` skill explains how to run a scene with other settings (an ini in `--data-dir`). After shots: `docs/design/after-p5/`. Debug and Release build OK.

### Follow-ups on the same branch
- **Move list colours** (user report): since P4 every piece was drawn in the text colour, so White's and Black's moves looked alike. Pieces have their own colour again, with a soft glow of the other colour (black glows white, white glows black: `MultiEffect`, `Theme.pieceGlowOnBlack/OnWhite`) so both show on either panel.
- **Code review** (medium): 10 findings, 6 fixed.
  - The engine probe tries every candidate, like `UciConnector::Init()`.
  - `backupsChanged` fires on every save, so "the same as now" stays right; it compares the profiles, not who is logged in.
  - A restore keeps, for profiles that exist now, the saved-game info (the files aren't restored) and the Lichess sign-in (a removed one doesn't come back).
  - The piece fade and dot pop start once per piece, so switching Reduce animations doesn't replay them.
  - The `APP_VERSION_*` C++ defines are for ChessGui only.
  - Not changed: the coordinates are drawn over the pieces on purpose (as on Lichess); a glow per visible move is cheap (`ListView` only creates the visible rows); `backups` reads 4 small files, only while Settings is open.
  - 2 more tests; all 151 pass.

### Not checked live
- Clicking through the page in a real window: each switch in a game (legal-move dots need a selected piece, resign without confirmation, Reduce animations while flipping), the volume preview sound, Back up now / Restore… / the folder buttons / the About links. Listed in TASKS under P5.
- The engine line when Stockfish is missing (here it is on the PATH).

### Where to continue
P6 (Polish), on a new branch from `origin/develop` once this PR is merged: keyboard and accessibility pass, high contrast, motion tokens (`Theme.reduceMotion` exists now; StackView page transitions don't follow it yet), app icon and installer visuals, screenshot baselines (with BL-05). The plan's Ctrl+N and Ctrl+O shortcuts aren't there yet.

---

## 2026-10-29 — BL-02 P3 Users and Lichess done (branch: usersLichess_291026); next: P5

### What was done
- **Pages:** `WelcomePage` on first run: Continue with Lichess (OAuth, then a profile named after the account with a random avatar, or the profile already linked to it), Create local profile, Import a profile. `ProfilePickerPage` replaces `UserLoginScreen`. Its cards are centred and show the avatar, the Lichess username and when the profile last played, and each card has a menu with Export and Delete. With 2 or more profiles the app now starts here; with one, straight to Home. `ProfilePage` (a full page pushed over Home or the game; Back or Esc pops it) replaces the profile dialog: name and avatar apply right away; a Lichess card (ratings for bullet to correspondence, "?" = provisional; Open on lichess.org; Disconnect with confirmation; Advanced: API token); statistics; Export / Delete (Delete is off during a game). `CreateUserDialog` only creates now.
- **Lichess (BL-08, BL-03):** the sign-in moved from `ChessConnector::loginWithLichess` into `UserManager` (`linkLichess`, `linkLichessToken`, `createUserWithLichess`, `cancelLichessLink`), so the token never passes through QML. `LichessAuth::cancel()` closes the local listener and aborts the token exchange: **Cancel** while "Waiting for the browser…". `LichessClient::refreshAccount()` reads the ratings from `/api/account` (on login, on a new token, 3 s after an online game). Any call made with our token that gets a 401 emits `authExpired`; the profile then shows "Lichess sign-in expired · Reconnect", Home's Play online card says so, and the client drops the token.
- **Statistics per opponent**, with draws: the robot by level, each profile, Guest, Lichess. `ChessConnector::recordResult()` runs on mate, resignation and a Lichess game end, once per game. A two-player game between profiles counts for both (checked: the `result` scene gives Tester a loss vs Bob and Bob a win vs Tester). `GameSaveInfo.opponentUserId` keeps the opponent of a continued game. Version 1 totals are migrated: robot games become "level not recorded", the rest "Two players, before results were kept". Plan §9 Q2 is decided: a Guest game counts for the profile.
- **Data safety:** `users.json` and `config.json` are written with `QSaveFile`. On start a readable `users.json` goes to `backups/users-1.json`, and 3 are kept (an unchanged file isn't copied again). If `users.json` doesn't parse, nothing is loaded and an InfoBar offers **Restore** (the newest good backup). The damaged file is kept as `users.damaged-<time>.json`, also when the user starts over instead.
- **Export / import:** `<name>.chessprofile` (JSON, format version 1) holds the profile, statistics, ratings and both saves (base64), and never the token or a pending challenge. Importing adds the profile; on the same id or name, `ImportConflictDialog` offers Import as new ("Bob (2)") or Replace. Replacing the profile that is playing is refused. An imported Lichess link asks to reconnect.
- **Tests:** `ChessQtTests` has 16 new tests: `UserManagerTest.cpp` (14: restart, start page, per-opponent stats, v1 migration, delete, backup rotation, unchanged file, damaged file + restore, damaged file kept, export without token, import on another PC, conflict as new / replace, replace refused, bad files, file name) and `parseRatings` ×2. `UserManager.cpp`, `AppConfig.cpp` and `AvatarProvider.cpp` are compiled into the test DLL. All 143 tests pass (ChessTests + ChessQtTests).
- **Scenes:** `welcome`, `profiles` (`login` is its old name), `profile`, `profile-local`, `profile-expired`, `import`; `create-user` is now the first-run dialog. Their profiles come from `seedScreenshotProfiles()` in `main.cpp` (placeholder token, no network in screenshot mode). All 19 scenes OK, no QML warnings. After shots: `docs/design/after-p3/` (light and dark).

### Code review fixes (same branch)
A medium `/code-review` found 10 issues; 8 were real and are fixed, 2 can't happen (Delete is off during a game; the autosave runs only when leaving a game, not per move).
- A damaged `users.json` is **copied** to `users.damaged-<time>[-n].json`. If that fails, nothing overwrites it. Restore is only offered for a backup that has a `users` array.
- A login cancels a pending Lichess sign-in, so a late browser sign-in can't switch profiles. A profile made by Continue with Lichess gets "Name (2)" if the name is taken, like an import.
- The autosave keeps the robot's level (`computerLevel`): Continue plays at that level and the statistics use it, even if another profile changed `lastLevel`. It also keeps `resultRecorded`, so a mate that was undone and then played again after Continue doesn't count twice.
- Import reads the file once. A 401 is reported once per reply.
- `ChessQtTests`: 2 more tests (backup without users, autosave level and counted flag). All 145 pass, and all 19 scenes are OK.

### Not checked live
- Continue of a game vs the robot at a level other than `lastLevel` (needs Stockfish; the saved fields are unit-tested).
- Anything that needs lichess.org: OAuth from Welcome and Profile, Cancel, the API token, ratings, a real 401 (revoke the token on lichess.org). Listed in TASKS under P3.
- Export / import through the real file dialogs, delete, and Restore from a damaged file in a real window (the logic is unit-tested).

### Where to continue
P5 (Settings and About pages), on a new branch from `origin/develop` once this PR is merged. Settings → Data can reuse `exportProfile` / `inspectImport` / `importProfile` and `restoreFromBackup`; "Back up now" needs a new `UserManager` call.

## 2026-10-29 — Check glow on the king (branch: checkHighlight_291026)

- The king in check glows red-yellow and pulses gently, so a missed "Check" InfoBar doesn't leave the player wondering why most moves are refused. `ChessConnector::CheckedKing` gives the square for the position on the board (live, reviewed, or a saved game's replay); `Game::IsInCheck()` (side to move) is new in ChessCore with a test. The glow is a `QtQuick.Shapes` radial gradient in `ChessBoxes.qml` (`Theme.checkGlowCenter` / `checkGlowEdge` replace the unused `checkSquare`).
- Checked: `game` with `e2e4 f7f6 d1h5` (light; dark and flipped with Bb5+), a mate (the red flagged king on top), and a reviewed check that is blocked live. No QML warnings. ChessTests 86/86.
- Independent of P3 (#84): branched from `develop`.

---

## 2026-10-29 — PGN date kept in saves (branch: pgnGameDate_291026)

- Hand check by the user: Copy PGN and Export PGN work, but a continued game exported `[Date "????.??.??"]`.
- `GameSaveInfo.startDate` (`startDate` in users.json) is written with the autosave and the saved game, and read back by Continue and Open saved game. Saves made before this still give `????.??.??`. Checked: the `continue` scene's autosave holds `"startDate": "2026-10-29"`. `ChessTests` 85/85.

---

## 2026-10-29 — P4 code review fixes (branch: p4ReviewFixes_291026); next: P3

A `/code-review` of #81 found 10 issues. I checked all of them: 9 were real, and 1 (the overlapping review properties) was mostly style. This branch fixes 9, including the style one; #2 became backlog item BL-09.
- **Rematch after closing the result card:** the moves panel now shows Rematch and New game, and the sheet handle shows a Result button that reopens the card. The new `ended` scene shows this.
- **Review moves** on the result card goes to the start position (`reviewFirst()`); in a narrow window it also opens the sheet.
- **Move list:** after a single move, only that move is formatted. `Game::IsCheckMate()` supplies the mate, and `UndoMove` now clears it. The full replay runs only after undo, load or several moves at once. In `makeMove`, an exception now only covers `DoMove`.
- **Captured pieces:** they come from `HistoryMove::CapturedPiece()` in ChessCore. A `Game` records en passant as a normal capture, so the old GUI special case never ran.
- **PGN:** the date is the day the game started. A saved or continued game gets `????.??.??` and no date in the file name. The suggested export name is URL-encoded, so `#` and `%` in names work. Export rejects URLs that aren't local files.
- Esc no longer needs two presses after the window grows wide with the sheet open. `capturedChanged` fires once per update. `PlyCount` replaced `ReviewIndex`, which QML no longer used.
- `ChessTests`: 85/85, 5 new (`CapturedPiece` ×3, `IsCheckMate` set by mate and cleared by undo). All 15 scenes OK in a real window.
- **Not fixed here, now BL-09:** the engine thread changes `_game` while the GUI thread reads it.

---

## 2026-10-29 — BL-02 P4 Game chrome done (branch: gameChrome_291026); next: P3

### What was done
- **Player cards** (`PlayerCard.qml`) above and below the board: avatar, name (`ChessConnector.PlayerName` / `OpponentName`), the robot's level and Elo, the turn (accent bar and avatar pulse), and a **captured strip** (`CapturedStrip.qml`). The strip groups pieces by type (P N B R Q), overlaps pieces of a type, and shows "+N" (`MaterialBalance`, counted from the board, so promotions count). Captures and material follow the reviewed position. When the board is flipped, the cards swap.
- **Moves:** a side panel (`MovesPanel` + `MoveList`) at width ≥ 960 or aspect > 1.15; otherwise a **bottom sheet** whose handle shows "Moves 12. Nf3", the status, and Back to live while reviewing. Esc closes the sheet first. The move list has two columns and a move-number column, piece images in the text colour, and tabular figures. **Click a move** to review it (`reviewTo(ply)`; `ShownPly` is highlighted). While reviewing, a slim InfoBar "Viewing 5. Nf3 · Back to live" sits at the bottom of the panel. The blinking button and "Reviewing" are gone.
- **Result:** one `ResultCard` over the dimmed board in every layout, with Rematch, New game (the setup dialog of the same mode; online goes Home → Lichess dialog) and Review moves. `GameResultDialog`, `GameResultCard`, `MoveHistoryPanel` and `CapturedPanel` were deleted. Their statistics moved to a "Statistics" line in the profile dialog until P3's Profile page.
- **PGN:** Copy PGN and Export PGN… are in the panel and under More. Export uses a `FileDialog` that suggests "White vs Black yyyy-MM-dd.pgn" and writes with `QSaveFile`; an InfoBar confirms. The tags are Event, Site (the lichess.org URL online), Date, Round, White, Black and Result.
- **ChessCore:** `Game::FormatHistorySan()` replays the game (in `Game`, so `San` stays free of `Game.h`) and marks '#' from the real mate event, no longer from the result string. `FormatPgn()` escapes tag values and wraps at 80 columns. `PieceValue()`/`MaterialBalance()` are in `Board.h`. `ChessTests` has 80 tests, 13 new (history SAN, PGN, material). En passant now counts in the captured strip.
- **Scenes:** `review`, `result` and `moves` are new; `game` and `continue` work as before. `Take-Screenshot.ps1 -Offscreen` works when the screen is locked, dark only (see Notes). Debug and Release build OK; no QML warnings in the scene logs.

### Notes
- **Locked screen:** while Windows is locked, a real window renders no frames. Animations freeze (half-closed dialogs, empty squares) and the mate scene times out, on `develop` too. Use `-Offscreen` there: Fluent is always dark offscreen, so `Theme.dark` follows it. Dialog backgrounds don't render offscreen. After unlocking, all scenes (including the mate) worked in a real window. `docs/design/after-p4/` has the light shots (real window) and the dark ones (offscreen).
- Board coordinates are still outside the squares (the inside/outside setting is P5).

### Where to continue
P3 (Users and Lichess), on a new branch from `origin/develop` once this PR is merged.

---

## 2026-10-29 — BL-02 P2 Shell and navigation done (branch: shellNavigation_291026); next: P3 or P4

### What was done
- **Pages:** `ChessGame.qml` (986 → 491 lines) is the window: a `StackView` holding `UserLoginScreen`, `HomePage` or `GamePage`. `route()` picks the page from `UserManager.isLoggedIn` and `ChessConnector.InGame`. All dialogs live in the window and are opened with `open()`; pages emit signals for them. The `_show*` flags and `screen_1…4` states are gone.
- **Home:** an account button (avatar and name; menu with Profile and Switch user), Settings, a **Continue game** card (`ChessConnector.ContinueDescription`, e.g. "vs Bob"), and cards for Play vs computer, Two players, Play online (dimmed until Lichess is linked; then it opens Profile) and Open saved game (always enabled; it explains when there is nothing to open). The Exit button is gone.
- **Setup dialogs:** `ComputerGameDialog` (White/Black/Random and level; its info button swaps to the levels help and back, so dialogs never stack) and `TwoPlayerDialog` (side, and the opponent picked by profile **id** or Guest). Both use the new shared `ChoiceRow`.
- **Game command bar:** Home, title (`ChessConnector.GameTitle`: "vs Robot · Level 3", "Alice vs Bob", "Lichess · vs X", "Saved game · …"), Prev/Next (replay), Undo/Takeback, **Flip board** (new), Save, Offer draw, Resign (with `ConfirmDialog`), More (Computer levels, Settings, Profile, Switch user). Undo, Draw and Resign are no longer duplicated in the move panel.
- **InfoBar** replaces `Notificator` and the three banners: one message bar (Check, Game saved, errors, rematch declined) plus bars bound to `ChessConnector.PendingOffer` (draw or takeback) and `OpponentGone`/`ClaimWinSeconds`. Castling is no longer announced.
- **Game flows in C++:** `startComputerGame(level, side)`, `startLocalGame(white, opponentUserId)`, `switchUser()` (autosave, end the game, log out) and `refreshBoard()` (a new board view fills itself in). Lichess game start and end are handled in `ChessConnector`, which sets the result text, flags the king and sets the opponent's avatar. Game-end text is now `ResultText`, with names: "Bob wins by checkmate", "Alice resigned". `GameSaveInfo.opponentName` is stored for Continue and the title. The avatar is set per mode (the `newGameStarted → randomize` connection in `main.cpp` is gone), so a rematch keeps the opponent's avatar.
- **Typed singletons:** `qmlRegisterSingletonInstance("ChessPlusPlus", …)` for `ChessConnector`, `UserManager`, `LichessClient` and `AvatarProvider`, parented to the app so they outlive the engine. No `setContextProperty` is left.
- **Shortcuts** (off while a dialog is open): Esc Home, F flip, Ctrl+S, Ctrl+Z, ←/→/Home/End review, F1 levels, Ctrl+, settings, Alt+S move input. Minimum window size 640×560; the window title shows the game.
- **Scenes:** `home`, `continue`, `game` (Tester vs Bob), `computer`; `menu` and `board` still work as aliases. After shots: `docs/design/after-p2/`. Debug and Release builds OK; `ChessTests` 67/67; no QML warnings in the scene logs.

### Not checked live
- Keyboard shortcuts and Enter/Esc in a real window. Driving the window with SendKeys didn't reach the Qt controls, so the skill now says to add a scene instead.
- Lichess flows (challenge, game start and end, draw/takeback InfoBars, opponent gone) need a connected account. Add them to the Lichess live test in TASKS.

### Where to continue
P3 (Users and Lichess) and P4 (Game chrome) can run in parallel; each needs a new branch from `origin/develop` once this PR is merged. P3 replaces `UserLoginScreen` with the Welcome page and profile picker, and moves `CreateUserDialog`'s Lichess part to a Profile page. P4 replaces `CapturedPanel`, `GameResultDialog`/`GameResultCard` and `MoveHistoryPanel`.

---

## 2026-10-29 — BL-02 P1 Foundation done (branch: fluentFoundation_281026); next: P2

### What was done
- **Style:** `QQuickStyle::setStyle("FluentWinUI3")` in `main.cpp`. The palette block, inline `Button`/`ModeButton` and every `background:`/`contentItem:` override are gone. `Button.qml`, `ActionButton.qml`, `CustomSlider.qml`, `NetworkPlayers.qml` and `menu_icon.svg` were deleted.
- **`qml/Theme.qml`** singleton (`qml/qmldir`): colour roles, board, type ramp, spacing and motion as in `docs/design/ThemeTokens.md`; §8 there lists the differences. No `Window.window.color` and no hex colour outside `Theme.qml`.
- **System / Light / Dark:** `appSettings.theme` (Settings → "App theme"). It sets `Application.styleHints.colorScheme`; until it is set, the old `darkMode` decides. "System" follows Windows; checked live.
- **Icons:** `pics/icons/*.svg` are in `qml.qrc` under `qrc:/icons/` (`Theme.icon("undo")`). New `IconButton.qml` (icon-only `ToolButton` with tooltip and `Accessible.name`) is used in the command bar and the move panel.
- **`AppDialog.qml`:** a modal Fluent `Dialog` on the overlay with the `Theme.scrim`. Enter runs `accept()` and Esc runs `reject()`; footers are `DialogButtonBox`es. Settings, Profile/Create profile, Levels, Lichess, Challenge, Select player and Apply moves use it. Shown with `visible: _showX; onClosed: _showX = false`.
- **Board:** the `Glow` is replaced by an accent tint on the selected piece's square, cleared after a move. Legal-move dots use `Theme.legalMoveDot`, and coordinates use `textSecondary`. The photo shows only behind the start menu and login, faint.
- **Screenshots:** `-platform offscreen` can't force light/dark, and Fluent falls back to its dark palette there. Screenshot mode now uses a real window that is transparent, frameless, ignores input and has no taskbar button, at 100% scale. `Take-Screenshot.ps1` dropped `-Windowed`. New scenes: `online`, `players`. After shots: `docs/design/after-p1/`.
- Keyboard checked live: Enter creates a profile, F1 opens levels, Esc closes the dialog. `windeployqt` deploys the FluentWinUI3 plugin and its assets. Debug + Release build OK; `ChessTests` 67/67.

### Where to continue: P2 Shell and navigation (new branch from `origin/develop`)
Spec: plan §6 and §8 (P2 row). `StackView` pages, command bar, account menu, Home, InfoBar replaces `Notificator` and the banners, `_show*` flags removed, services as typed singletons, game flows into `ChessConnector`.

### Notes
- Not seen in a scene yet: the connected Lichess dialog, the Challenge dialog, the draw/takeback banners and the two resign confirmations. Check them during the Lichess live test (TASKS).
- `windeployqt --qmldir ChessGui\ChessGui` locally still lists Qt5Compat, but only because stale Qt VS Tools caches (`ChessGui\ChessGui\x64\*\qt\qmake\*\import.qml`) import it. CI starts clean.
- The board's own animations (elastic flip, InCirc piece rotation) are unchanged; motion tokens come in P6.

---

## 2026-10-28 — App modernisation planned (BL-02/BL-03); next: Phase 1

### What was done
- **#76 Screenshot mode:** `ChessGui.exe --screenshot <png> --scene <name>` (with `--moves`, `--black`, `--dark`, `--size`, `--delay`) on a throwaway profile; `--data-dir <dir>` for interactive runs. `scripts\Take-Screenshot.ps1 -Scene all [-Dark] [-Windowed] [-Build]` writes to `test-results\screens\`. The `run-chess-app` skill explains it.
- **#77 Modernisation plan:** `docs/AppModernizationPlan.md` holds the screen-by-screen review, target flows, roadmap P1–P6 and the mockup link (§10).
  - Design kit: 38 icons in `ChessGui/ChessGui/pics/icons/` (not in `qml.qrc` yet), `docs/design/ThemeTokens.md`, and "before" screenshots in `docs/design/before/`.
  - Decisions: Windows 11 Fluent (`FluentWinUI3` style, system theme and accent); local profiles with a linked Lichess account; profile export/import plus backups.
  - QML ↔ C++ rule (CLAUDE.md "Key Conventions"): game flows go through `ChessConnector`, one call per action; `UserManager` and `LichessClient` stay separate services and become typed singletons in P2.
- `TASKS.md`: BL-02/BL-03 is at the top of *In Progress*, with one sub-item per phase.

### Where to continue: P1 Foundation (new branch from `origin/develop`, e.g. `fluentFoundation_<ddmmyy>`)
Plan first (`/plan`), then implement. Spec: plan §6 and §8, `docs/design/ThemeTokens.md`.
1. `main.cpp`: `QQuickStyle::setStyle("FluentWinUI3")`. Remove the per-file `background:`/`contentItem:` overrides that fight the style: inline `Button` and `ModeButton` in `ChessGame.qml`, `Button.qml`, `ActionButton.qml`, `CustomSlider.qml`, the primary-button copies in the dialogs. Prefer plain Fluent controls (`highlighted: true` for the primary action).
2. `qml/Theme.qml` singleton (`qmldir` + `qml.qrc`). It replaces the 16 `Window.window.color` dark-mode blocks. System/Light/Dark sets `Application.styleHints.colorScheme`; migrate `appSettings.darkMode`.
3. Icons: add `pics/icons/*.svg` to `qml.qrc` and use them via `icon.source` + `icon.color`. This replaces the `ColorOverlay` menu icon and glyph labels ("⚑", "↩", "|<<").
4. `AppDialog` base: modal, with a scrim and Enter/Esc. Move the existing dialogs onto it (Settings, Profile, Levels, Lichess, Challenge, Select player, Apply moves).
5. Remove `Qt5Compat.GraphicalEffects` (`ColorOverlay` in `ChessGame.qml`; `Glow` in `ChessFigures.qml`, which becomes an accent square tint) and the unused `NetworkPlayers.qml`. That closes BL-01.
6. Keep every screenshot scene working (`setUpScreenshotScene()` in `ChessGame.qml`). Before/after shots, light and dark, go in the PR.

Out of scope for P1: the page split and navigation (P2), and piece moving/drag (BL-04).

### Notes
- After P1, offscreen screenshots should match windowed ones, because Qt5Compat effects are what rendered blank offscreen.
- Login cards are left-aligned (`UserLoginScreen.qml` `Flow`); this is fixed in P3, not P1.
- Manual checks still open (no code): VC++ runtime install, Stockfish install log, Lichess rematch live test. They're listed under *In Progress* in `TASKS.md`.

---

## 2026-10-28 — Lichess Rematch (branch: feLichessRematch_281026)

### What was done
- Merged earlier today: computer levels rework + levels help (#68), promotion dialog rotation fix (#69).
- Analysed why Rematch doesn't work vs Lichess: the button is hidden for online games and the Board API has no rematch endpoint. Plan written to `docs/LichessRematchPlan.md`: Rematch creates a new challenge (direct to a registered opponent, open link for anonymous).
- Implemented steps 1–4 (see `TASKS.md`): `LichessClient::createRematch()` / `parseRematchInfo()`, `ChessConnector::CanRematch`, Rematch button for online games, `ChallengeDialog` direct mode. Direct challenges use `keepAliveStream=true` (otherwise they expire after 20 s).
- Debug + Release build OK; `LichessStaticTests` 21/21 (4 new); app starts without QML errors.
- Testing showed an opponent's resignation was received (`status:"resign"`) but nothing was shown: result and Rematch lived only in the landscape side panels. Added step 4b (result card, notification, red flag on the losing king, frozen board, `opponentGone` + Claim victory).
- Note for testers: resigning on lichess.org needs the flag **and** the ✓ confirm within ~3 s; otherwise nothing reaches the server.

- Code review fixes (step 4d) and PR #70 opened. Live check OK for resign from the app and from an anonymous Lichess opponent.

- Merged: #70 (rematch + game end) and #71 (test host crash fix). `ChessQtTests` engine tests now run reliably (10/10).

### Where to continue
1. Play a game against a second Lichess account and run the manual scenarios in plan §5 (accept, decline, cancel, anonymous link, vs Robot unchanged, restart, leave while pending).
2. Watch the log for `LichessClient: challenge stream line:`. It confirms the keep-alive challenge JSON and the final `{"done":...}` shape this code expects.
3. If a live scenario fails, fix it on a new branch from `develop`.

### Notes
- If the opponent clicks Rematch in their browser, the Board API client probably never sees it; the app must send the challenge.

---

## 2026-10-27 — Release 0.1.0 preparation (branch: feRelease010_271026)

### What was done
- CI/CD re-checked. It is MSBuild-based, not CMake: `tests.yml` (Debug build + `ChessTests` on every push/PR) and `release.yml` (Release build, windeployqt, signing, Inno Setup installer). Last runs on `develop` and the PR installer build (2026-06-07) were green.
- `release.yml`: release step now uses `docs/release-notes/<tag>.md` as the body (generated notes appended) and fails if no installer is found.
- `installer/ChessSetup.iss`: `AppPublisherURL` corrected to `novaua/qt-chess`.
- Added `docs/release-notes/v0.1.0.md`; README links to the first release.

### Where to continue
1. Merge this branch to `develop` once CI is green.
2. Tag `v0.1.0` on `develop` and push the tag; the Build Installer workflow publishes the GitHub Release with `ChessSetup-0.1.0.exe`.
3. Check the release page, then install the EXE on a clean machine and run it.
4. The release-notes link to the EXE is `.../releases/download/v0.1.0/ChessSetup-0.1.0.exe`; confirm it resolves.

### Notes
- The old CMake pipeline (feMakeInstaller_0430a) was replaced by the MSBuild workflow; there is no `CMakeLists.txt`.
- The app does not display its version anywhere; the version only lives in the tag and installer name.
