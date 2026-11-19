---
id: BL-13
title: "Game page layout for wide screens"
type: task
status: open
priority: medium
---

# BL-13 Game page layout for wide screens

<!-- migrated from TASKS.md:96 (In Progress / Backlog) -->

- [ ] BL-13 Game page layout for wide screens [Medium] — plan: [`docs/BL-13-WideLayoutPlan.md`](docs/BL-13-WideLayoutPlan.md) (2026-11-09; the owner chose the recommended Q1–Q4). Steps: 1 the column layout with sized screenshot scenes (done, branch `wideGameLayout_091126`), 2 the InfoBars in the column (done, branch `infoBarsColumn_091126`), 3 messages over the command bar and the recent messages (done, branch `features/commandBarMessages_101126`), the code review fixes (done, branch `bugs/bl13Review_101126`), later more move-history views. Original note: on a monitor much wider than it is high, the player cards (avatar, name, captured pieces, +N) should move to a column on the left of the board (opponent at the top, player at the bottom), so the board can take all of the available height. Today, in a wide window (`GamePage.qml`: `area.wide`, width >= 960 or landscape), only the moves panel goes beside the board (on the right); the two cards stay above and below it, so the board loses their height (`area.boardSize`). Keep the current layout when the window is taller than wide (e.g. snapped to half the screen) and in narrow windows (move sheet). The plan should cover: when to switch (aspect ratio, minimum height), the left column's width and what it shows (captured pieces vertical or wrapped, the active-side marker), the moves panel on the right, and screenshot scenes at e.g. `-Size 1920x1080` and `1400x900` with baselines. The owner will add more features to this task, e.g. other ways to show the move history.

## Resolution

### 2026-11-09 — BL-13 plan: the game page on wide screens (branch: bl13Plan_091126)

#### What was done
- **`docs/BL-13-WideLayoutPlan.md`**: the player cards move to a 260 px column left of the board when that gives a board at least 24 px bigger than today's layout (computed from both layouts, not an aspect ratio): +16 % at 1920 × 1080 and 1400 × 900, +27 % at 1280 × 720; snapped halves, landscape windows bound by width and narrow windows keep today's layout. The column's cards: name, a larger clock on its own row, the captured pieces wrapping; mirrored as on lichess.org. The moves panel unchanged. 22 new sized screenshot scenes (light and dark) in `all`, incl. two sizes that must not switch.
- Builds on #134 (the InfoBars make room on the game page): in the column layout that would shrink the board while a bar shows, so the plan proposes the bars between the column's cards.

#### Next
- The owner's choices in the plan's §8 (Q1 the InfoBars, Q2 the cards' order, Q3 the switch rule, Q4 the extra width), then step 1.

### 2026-11-09 — BL-13 step 1: the player column in wide windows (branch: wideGameLayout_091126)

#### What was done
- The owner chose the plan's recommended answers to Q1–Q4 (recorded in §8).
- **`GamePage.qml`**: `area.side` computes the board for both layouts and puts the cards in a column left of the board when that board is ≥ 24 px bigger (and two stacked cards fit beside the squares). The `Column` became explicit `x`/`y` bindings, so the same card items move on a switch; the stacked layout is pixel-identical to before (the cards' x rounded, as the anchors did).
- **`PlayerCard.stacked`** / `clockFirst`: in the column the clock gets its own row (`title` size), mirrored (top card: clock, name, pieces; bottom: pieces, name, clock). `Avatar`, `NameRow`, `Clock` are inline components shared by both layouts; the avatar's pulse runs only in the visible one.
- **`CapturedStrip.wrapWidth`**: the groups go on to a next line where they don't fit (a group never split; "+N" on its own line if needed). 0 = one line, as before.
- `Theme.playerColumnWidth` 260; ThemeTokens §11; CLAUDE.md.
- **Screenshots**: `Take-Screenshot.ps1` takes `scene@WxH[@black]`; `all` ends with 11 sized scenes (`$sizedScenes`), and the new scene `captures` (Black down to a lone king: White's full strip wraps, "+29"). Light and dark: the 100 old ones match; **24 new baselines** accepted after a look (contact sheet). No QML warnings in the log.
- QML and the scene list only (`main.cpp`): no C++ tests run.

#### Next
- The owner's hand check in a real window: drag the window's edge across the switch, maximise, snap to half, Windows at 150 %.
- Step 2: the InfoBars between the column's cards (Q1 = A).

### 2026-11-09 — BL-13 step 2: the InfoBars between the column's cards (branch: infoBarsColumn_091126)

#### What was done
- The owner saw "Check" shrink the full-height board for 4 s (#134's room for the bars). Now, with the player column, the window's InfoBars ("Check", saved, offers, a challenge, reconnecting, opponent gone, damaged profiles) go between the column's cards: `GamePage.barsArea` (the free space, page coordinates), `ChessGame.qml` places the bars there (a `Flickable`: they scroll when more than fit). The board keeps its size. Without the column the bars stay under the command bar and the page starts below them, as before.
- The layout is chosen without the bars (`area.stackedBoard` from the full height; `topInset` only in the stacked layout), so a bar never switches it.
- The column needs room for one bar between the cards (`area._barsRoom`, a challenge with buttons ≈ 140 px): at 1000 × 560 the bar was cut and its buttons scrolled out of sight, so such a low window keeps the cards above and below the board.
- `InfoBar`: the buttons wrap (`Flow`, as wide as its buttons beside the text); the window's bars put them under the text while in the column (`actionsBelow: root._barsInColumn`). The moves panel's review bar is unchanged.
- **Screenshots**: new scene `check` (the page opens in check), new sized `reconnecting`, `check` at 1400 × 900 and `game` at 1000 × 560. Light and dark: all others match; `challenge-ingame-1400x900` changed as intended (the bar in the column); **10 baselines** new or updated after a look. No QML warnings in the log.
- QML, the scene list in `main.cpp` and the script: no C++ tests run.

#### Next
- The owner's hand check: a draw offer, "Check" and a challenge during a game in a maximised window; resize while a bar shows.

### 2026-11-10 — BL-13: short messages over the command bar (branch: infoBarsColumn_091126, PR #137)

#### What was done
- The owner, checking #137: "Check" still changed the board's size in a tall window (half the screen), where the cards are above and below the board. The owner's idea (plan step 3, PR #140): short messages over the command bar. Done here for the message bar only; the rest of step 3 (offers and the reconnecting status in the command bar, recent messages in the More menu) stays as planned.
- **`CommandBarMessage.qml`** (new): "Check", "Game saved", "PGN copied", … as a pill over the command bar's free middle: no room taken, nothing on the page moves. Centred on the board on the game page (right of the window's centre with the player column), kept inside the free middle; where it would cross the title's text, the title is hidden up to it (no cut word). A message that doesn't fit (e.g. "Couldn't save <long path>" at 768 wide), or a page without a command bar (Welcome, the profile picker), shows as the InfoBar under the command bar as before (`messageBarFallback`). The decision follows the window's size while it shows.
- **`CommandBar.messageArea`**: the item filling the free middle (the title, Home's filler); set on the game, Home, Online, Profile and Settings pages. `GamePage.boardCenterX`.
- Checked with a temporary long message at 768, 960 and 1400 wide: the bar, over the command bar with the title hidden, over the command bar with the title kept.
- **Screenshots**: `check` re-baselined (default, 1400×900) and new `check@960x1040`, light and dark: "Check" in the command bar, the board as in `game`. All other scenes match (the profile scenes' 0.04 % is their "since <today's date>").
- QML only: no C++ tests run.

#### Next
- The owner's choices for the rest of step 3 (Q8 "Recent messages" in the More menu, Q9 all but "Check", the last 20, Q10 this session only) go into the plan (#140).

### 2026-11-10 — BL-13 step 3: messages over the command bar, recent messages (branch: features/commandBarMessages_101126)

#### What was done (plan §9, the owner's Q5–Q10)
- **Over the command bar** (`CommandBarMessage`, now declarative: `active`, buttons as children, `closable`, `fits`): besides the short messages, the **offers** ("Friend offers a draw" with Accept / Decline; takeback, rematch), the **lost connection** ("Reconnecting to Lichess…", the full text as its tooltip) and the **opponent left** ("Opponent left · claim victory in 12 s", Claim victory). One at a time: the game's state (offer > opponent left > connection) before a short message; what doesn't fit (e.g. an offer at 640 wide) or waits behind goes to the InfoBars as before (one offer bar now for the three kinds). An **error stays until closed** (×); a newer message meanwhile goes to the InfoBar for its 4 s. `ChessGame.notify(text, severity, external)` replaces `messageBar.show()` (21 callers).
- **Recent messages** (`MessagesFlyout`): the last 20 of the session, newest first, all but "Check": full text, the time, the game it came from when not the one on screen; an open offer keeps Accept / Decline there, then shows "accepted" / "declined" / "withdrawn"; the opponent left keeps Claim victory. "Recent messages" in the game's **More** menu and Home's **account menu**, **Ctrl+M** everywhere (in Settings' shortcut list); an accent **dot** on More / the account button for new ones the player didn't cause (an offer, the opponent left, the connection, a decline, an error). A panel in the window, not a `Popup` (see below); Esc or a click outside closes it.
- **Screenshots**: new scenes `draw-offer` (default, 1400×900, 640×560: the InfoBar), `saved`, `home-message`, `messages` (the list open); `reconnecting` re-baselined (the pill). **20 baselines** new or changed, light and dark, after a look.
- **The dialog and timer scenes**: the first full run, at night, drew no popups and stalled QML timers (the screen was locked; `develop`'s build failed the same way). Run again with the screen on: all 150 scenes match, after 4 more baselines: `about` (light, dark: the Ctrl+M row in the shortcut list) and `draw-offer` at 640×560 (light, dark: the same picture, its text smoothed in grey instead of ClearType; the night's baseline was the odd one).
- The list was a `Popup` first; in that state it neither drew nor let the window's frames go on, so it is a plain panel now (it works either way, and needs no popup window).
- QML, the scenes in `main.cpp` and the script: no C++ tests run.

#### Fixed after the owner's test with a real Lichess game (the opponent left)
- **Claim victory was offered when Lichess refuses it.** Lichess takes the claim only on the opponent's turn (lila `Pov.mightClaimWin = forceResignable && !isMyTurn`; else "This is not the time to claim victory"), but sends `claimWinInSeconds: 0` either way. On the player's turn the message now says "Opponent left · your move, then claim victory" and has no button (also in the InfoBar and the recent messages); during the countdown, no button either, so the message is narrower and fits.
- **The board changed size**: the countdown's message with its (disabled) button didn't fit the command bar, so it went to the InfoBar; and the refused claim's error went to the InfoBar because the state held the command bar. Now a short message takes the state's place for its time (an error until closed) and the state waits there, not in the InfoBar; a newer message behind an error goes only to the recent messages (with the dot). Only what doesn't fit at all (a narrow window) still uses the InfoBar.
- **The opponent-left message can be closed** (×, over the command bar and in the InfoBar): hidden until the claim becomes possible or the opponent comes back or leaves again; the recent messages keep Claim victory.
- Scenes `opponent-left` (+ 1400×900), `opponent-left-claim` (+ 1400×900, 640×560), `opponent-left-turn`, `opponent-left-error`: 14 new baselines, light and dark; all 150 others match.

#### Next
- The owner's hand check: a draw offer, the lost connection and a challenge in a real Lichess game; Ctrl+M and the dot.

### 2026-11-10 — BL-13: code review fixes (branch: bugs/bl13Review_101126)

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

#### Next
- The owner's hand check of #141 still stands (a draw offer, the lost connection, a challenge, the opponent left in a real Lichess game; Ctrl+M and the dot), now also: an error while an offer is open, and × on "Opponent left" during the countdown.
