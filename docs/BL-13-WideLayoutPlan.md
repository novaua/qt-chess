# BL-13 Game Page Layout for Wide Screens — Implementation Plan

| | |
| :---- | :---- |
| **Date** | 2026-11-09 |
| **Status** | Steps 1 (#136), 2 (#137) and 3 (branch `features/commandBarMessages_101126`) coded; later: more move-history views. The owner chose the recommended answers to Q1–Q4 (2026-11-09) |
| **Branch** | `bl13Plan_091126` (plan); each step on its own branch from `develop` |
| **Task** | BL-13 in `TASKS.md`. Builds on #134 (the InfoBars make room on the game page) |

## 1. Problem

On a monitor much wider than it is high, the game page wastes the height the board needs. In a wide
window (`GamePage.qml`: `area.wide`, width ≥ 960 or landscape) only the moves panel goes beside the
board, on the right. The two player cards stay above and below the board, so the board loses
2 × (60 + 8) = 136 px of height (`area.boardSize`), while there is empty width on both sides.

A maximised window on a 1920 × 1080 monitor (client area about 1920 × 1009) shows a 793 px board
with about 400 px of empty width on each side.

## 2. Goals

| | |
|---|---|
| **The board takes the full height** | When the window is wide enough, the player cards move to a column on the left of the board: the opponent at the top, the player at the bottom (swapped when the board is flipped, as today). The board then takes all the available height. |
| **Same information** | The column's cards show what today's cards show: avatar (the robot's level tip and click), name, detail, the pieces taken and "+N", whose turn it is, the clock. |
| **The moves stay on the right** | The moves panel keeps its place and width, now as tall as the board. |
| **Nothing changes elsewhere** | A window taller than wide (e.g. snapped to half the screen), a landscape window where the board is bound by the width, and narrow windows (the move sheet) keep today's layout. |
| **Checked by screenshots** | New scenes at `1920x1080` and `1400x900` (and two sizes that must not switch), with baselines, in `all`. |

Out of scope, later in BL-13 (the owner will add them): other ways to show the move history, and
using the extra width of very wide windows (§4.5).

## 3. Today (`GamePage.qml`)

```
area = page − header (48) − margins (16 on each side)
wide:   panelW = sidePanelWidth (280) + 16
boardSize = max(200, min(area.width − panelW, area.height − 2 × (playerCardHeight 60 + gap 8)))

   ┌──────── card ────────┐
   │        board         │   ┌ moves ┐
   │                      │   │       │
   └──────── card ────────┘   └───────┘
```

The cards are as wide as the squares (`squaresSize`: 95 % of the board item, all of it with the
coordinates inside). The moves panel starts at the squares' top and ends at the squares' bottom.

## 4. Design

### 4.1 When to switch: the layout that gives the bigger board

Compute both board sizes and use the column layout when it gives a clearly bigger board:

```
stacked = min(area.width − panelW,                 area.height − 2 × (cardH + gap))   // today
side    = min(area.width − panelW − columnW − s4,   area.height)
useSide = wide && side ≥ stacked + 24 && side ≥ 2 × columnCardH + s4                   // both cards fit beside it
```

This is the goal itself ("the board takes the height") rather than an aspect-ratio guess, so it
handles any window and any scaling. The switch can't loop: the layout doesn't change the window size,
so there is no feedback and no need for hysteresis. The 24 px threshold avoids switching for a few
pixels. No animation when it switches (it happens while resizing).

With `columnW` = 260 (§4.2):

| Window (client) | Today (stacked) | Column (side) | Chosen |
|---|---|---|---|
| 1920 × 1080 (`-Size`) | 864 | 1000 | **side**, +16 % |
| 1920 × 1009 (maximised on a 1080p monitor) | 793 | 929 | **side**, +17 % |
| 1707 × 912 (maximised on 1440p at 150 %) | 696 | 832 | **side**, +20 % |
| 1400 × 900 | 684 | 796 | **side**, +16 % |
| 1280 × 720 | 504 | 640 | **side**, +27 % |
| 1280 × 1000 | 784 | 676 | stacked |
| 1100 × 900 | 684 | 496 | stacked |
| 960 × 1040 (half of a 1080p screen) | 632 | 356 | stacked |
| 768 × 1054 (the default window) | — | — | not wide: the move sheet |

### 4.2 The left column

```
   ┌ opponent ┐ ┌──────────────────┐ ┌ moves ┐
   │  09:56   │ │                  │ │ 1. e4 │
   │ 🦄 Friend │ │                  │ │       │
   │ ♟♟♞ +2   │ │      board       │ │       │
   └──────────┘ │  (full height)   │ │       │
                │                  │ │       │
   ┌ player ──┐ │                  │ │       │
   │ ♝♙  …    │ │                  │ │       │
   │ 🧙 Tester │ │                  │ │       │
   │  09:50   │ │                  │ │       │
   └──────────┘ └──────────────────┘ └───────┘
```

- **Width**: `Theme.playerColumnWidth` = 260 (new token, ThemeTokens §5). 236 px inside the card. Most captured sets fit in
  one row at 18 px; a full set (8 pawns overlapping, 2 + 2 + 2 + 1 pieces, "+39": about 240 px) and
  captured promoted pieces wrap to a second row.
- **Place**: the opponent's card is level with the squares' top, the player's with the squares'
  bottom (as the moves panel). The three parts are centred in the area as one group:
  column, 16 px, board, 16 px, moves panel.
- **The cards** (`PlayerCard`, with a new `stacked` property instead of a second component, so the
  two layouts can't drift apart):
  - the avatar (40 px) with the name and the detail beside it, as today;
  - the clock on its own row, larger (`Theme.title` size), full width, lit while it runs and red when
    low as today; no clock row in a game without a clock;
  - the captured pieces in a wrapping `Flow` (`CapturedStrip` gets a `wrap` width), "+N" after them;
  - **mirrored, as on lichess.org**: the clocks sit at the outer edges (the top card's clock at its
    top, the bottom card's at its bottom), so the clock of the side at the top of the board is at the
    top. The order inside each card: clock, name, captured (top card) / captured, name, clock (bottom).
  - whose turn: the accent bar on the left edge and the avatar's pulse, as today, and the lit clock.
  - height from the content (about 150 px with a clock, 100 without); the top card grows down, the
    bottom one up.
- **The space between the cards**: empty in step 1; the InfoBars in step 2 if the owner chooses so
  (§4.4).

### 4.3 The moves panel

Unchanged: `Theme.sidePanelWidth` (280), from the squares' top to their bottom, the status and the
review controls at its bottom, Rematch / New game there once the result card is closed. The result
card stays over the board.

### 4.4 The InfoBars

#134 made the game page move the board down by the bars' height while one shows. In the column layout
the board is bound by the height, so a draw offer or the reconnecting bar shrinks the board while it
shows, and grows it back afterwards. Two choices (§8, Q1):

- **A (recommended)**: in the column layout, the window's bars go into the left column, between the
  two cards, 260 px wide with the buttons under the text (`actionsBelow`), so the board never changes
  size during a game. `GamePage` exposes the free rectangle (`barsArea`, in window coordinates and
  only in the column layout); `ChessGame.qml` places its bar column there while the game page is on
  top, else under the command bar as now. The free space is about 420 px at 1400 × 900 (two bars fit).
- **B**: keep #134's behaviour (the bars over the board's top, the board shrinks while one shows).

### 4.5 Very wide windows

At 1920 × 1080 the group is 260 + 16 + 1000 + 16 + 280 = 1572 px wide, which leaves about 300 px
(150 on each side). It stays centred for now. Later in BL-13 the moves panel may take that width (another view of the move
history); the layout keeps `panelW` as one property so that change stays local.

### 4.6 Code (`GamePage.qml`, `PlayerCard.qml`, `CapturedStrip.qml`, `Theme.qml`)

- `area.side` (the rule of §4.1) next to `area.wide`; `area.boardSize` takes the chosen formula.
- The `boardColumn` `Column` becomes explicit `x`/`y` bindings for the two `SideCard`s and the board
  item, for both layouts. The same two card items move: nothing is recreated on a switch (the
  avatar's pulse and the clock keep running, the board keeps a drag or a promotion choice).
- `PlayerCard.stacked`, `CapturedStrip.wrap`; `Theme.playerColumnWidth`.
- No C++ change.

## 5. Screenshots

`Take-Screenshot.ps1` gets a list of sized scenes, run by `all` after the others, saved as
`<scene>-<size>.png` (the `-Name` suffix), with baselines:

| Scene | Size | What it checks |
|---|---|---|
| `game` | 1920x1080, 1400x900 | the column layout, a game without a clock |
| `clock-online` | 1920x1080 | the clocks in the column, the side to move |
| `captures` (new) | 1400x900 and the default size | a middlegame with 10+ pieces taken and +N: the wrap in the column, today's strip |
| `review` | 1400x900 | the review in the moves panel |
| `ended` | 1400x900 | Rematch / New game in the moves panel |
| `result` | 1400x900 | the result card over the full-height board |
| `challenge-ingame` | 1400x900 | the bar: in the column (A) or over the board (B) |
| `drag` with `-Black` | 1400x900 | the flipped board: the player's card at the top |
| `game` | 1100x900, 960x1040 | must stay stacked |

Light and dark: 22 new shots. The existing 100 must not change.

## 6. Steps

1. **The column layout** (`wideGameLayout_ddmmyy`): §4.1–4.3 and §4.6, the sized scenes of §5 with
   baselines, ThemeTokens §5 and CLAUDE.md (`GamePage` description). Hand check in a real window:
   drag the window's edge across the switch, maximise, snap to half, Windows at 150 %.
2. **The InfoBars in the column** (only with Q1 = A): `barsArea`, the bar column moved while the game
   page is on top; `challenge-ingame` at 1400x900 shows it. Hand check: a draw offer and the
   reconnecting bar in a real Lichess game, maximised.
3. **Messages in the command bar** (§9, the owner's idea, 2026-11-10): the short messages ("Check",
   "Game saved", offers, the reconnecting status) as a pill over the command bar's middle, so they take
   no room and the board never moves for them, in every layout; and "Recent messages" in the More menu,
   for one missed (§9.9). The message bar's part ("Check", "Game saved", …) is done early in #137
   (`CommandBarMessage.qml`); the rest on `features/commandBarMessages_<DDMMYY>`.
4. Later, from the owner: other move-history views (§4.5).

## 7. Risks

- **A switch during a drag or the promotion dialog** (the window resized meanwhile): the board item is
  the same and only its size changes, as when resizing today.
- **Tiny column cards at 200 % scaling**: logical sizes don't change with scaling; the rule of §4.1
  uses logical pixels, so a 4K monitor at 200 % behaves like 1920 × 1080.
- **The InfoBars' width in the column (A)**: a long challenge text wraps to 4–5 lines at 260 px.
  Decided in step 2: the column is used only when there is room for one such bar between the cards
  (about 140 px, `area._barsRoom`; e.g. 1000 × 560 keeps the cards above and below the board), and
  more bars than fit scroll there. The layout is chosen without the bars, so a bar never switches it.
  In the column the bars' buttons go under the text and wrap (`InfoBar.actionsBelow`, a `Flow`).

## 8. The owner's choices

All four as recommended (the owner, 2026-11-09): Q1 A, Q2 mirrored, Q3 the bigger board, Q4 centred.

| | Question | Recommended |
|---|---|---|
| Q1 | The InfoBars in the column layout | **A**: between the cards in the left column (the board never changes size); B: over the board, the board shrinks while one shows (#134) |
| Q2 | The cards' order in the column | **Mirrored** (clocks at the outer edges, as on lichess.org); or the same order in both cards |
| Q3 | When to switch | **The bigger board wins** by ≥ 24 px (§4.1); or a fixed aspect ratio (e.g. width ≥ 1.4 × height) |
| Q4 | The extra width of very wide windows | **Centred** for now; the moves panel takes it with the move-history work later |

## 9. Step 3: messages in the command bar

### 9.1 Problem

Since #134 every InfoBar takes room on the game page: the board moves down (and shrinks when the window's
height bounds it) while one shows. Step 2 put the bars between the column's cards, but only in the
column layout. With the cards above and below the board (a window snapped to half the screen, a narrow
window) "Check" still changes the board's size for 4 s, at every check (found by the owner, 2026-11-10).

### 9.2 Idea (the owner's)

The command bar is on every page, always 48 px high, and its middle is mostly empty (the title or a
filler). A message is drawn **over** the command bar's middle, as a separate layer of the window: it
takes no room, nothing moves, and it looks embedded in the bar.

```
┌────────────────────────────────────────────────────────────────────────────┐
│ ⌂ │ Tester vs Bob              ( ⚠ Check )                  ↶  ⇅  💾  ⚑  ⋯ │
└────────────────────────────────────────────────────────────────────────────┘
┌────────────────────────────────────────────────────────────────────────────┐
│ ⌂ │ Lichess · vs F… ( ⓘ Friend offers a draw  [Accept] [Decline] )  ↶ ⇅ ½ ⚑ ⋯ │
└────────────────────────────────────────────────────────────────────────────┘
```

### 9.3 Where each message goes

| Message | Today | Step 3 |
|---|---|---|
| "Check" | the message bar, 4 s | **pill**, 4 s |
| Confirmations: "Game saved", "Saved <path>", "PGN copied to the clipboard", "Move taken back", "N moves taken back", "Reconnected to Lichess", "No more moves", "Profile imported / exported", "Profiles restored", "Rematch / Challenge sent to …", "… declined the …" | the message bar, 4 s | **pill**, 4 s |
| The reconnecting status (LI-06) | a caution bar while it lasts | **pill** while it lasts: "Reconnecting to Lichess…", with the full text ("Your clock keeps running on Lichess.") as its tooltip and accessible description |
| Draw / takeback / rematch offer | a bar with Accept / Decline | **pill** with Accept / Decline: "Friend offers a draw", "Friend asks to take back a move", "Friend offers a rematch" |
| The opponent left (Lichess) | a caution bar with the countdown and Claim victory | **pill**: "Opponent left · claim in 12 s" / "Opponent left", with Claim victory |
| Errors ("Couldn't save <path>", "Couldn't accept the challenge: <reason>", a profile error) | the message bar | **pill** when the text fits; else the bar (an error must be read in full, never cut) |
| A challenge (BL-10; 2–3 lines, up to 4 buttons) | the challenge bar | **unchanged**: the bar (between the column's cards with the column, else under the command bar, taking room) |
| Damaged profiles (Restore) | a critical bar | **unchanged** |

The rule for everything: **a pill when it fits the command bar's free middle (§9.4), else the bar as
today.** A pill never cuts its text and never covers a button.

### 9.4 Geometry

- **The free middle**: each page's command bar has one item that fills the free width (the game's title,
  the Settings / Profile / Online title, Home's filler). `CommandBar.messageArea` names it; the pill may
  use that item's whole width (the title gives way, below). Free width on the game page (5 action
  buttons): about **360 px at 640** wide, **490 at 768**, **1120 at 1400**, **1640 at 1920**.
- **Centred on the board**: on the game page the pill is centred on the board's middle (`GamePage`
  reports its x; with the player column that is right of the window's centre), clamped into the free
  middle; on the other pages, on the free middle's centre.
- **Size**: 36 px high (centred in the bar's 48), padding 12, the severity colours and icon of
  `InfoBar`; buttons in the pill are compact (32 px high). Typical widths: "Check" ≈ 110, "PGN copied to
  the clipboard" ≈ 250, an offer with two buttons ≈ 360–400.
- **Doesn't fit** (`pill.implicitWidth > free middle`): the message goes to the bars instead (between the
  column's cards, or under the command bar taking room, as today). E.g. a draw offer below about 700
  wide.
- **The title gives way**: the pill sits on a backdrop in the command bar's colour, as wide as the pill
  plus a 24 px fade on each side, so the title under it is hidden while the message shows and comes back
  after. The pages need no change for it.
- **One pill at a time**, by priority: an offer or the opponent left (they wait for an answer) > the
  reconnecting status > an error > "Check" > a confirmation. A newer message of the same or lower rank
  replaces the current one (a new "Check" restarts its 4 s). A lower-ranked message while a higher one
  shows goes to the bars (rare: e.g. "Check" while a draw offer waits).
- **Pages without a command bar** (Welcome, the profile picker): the bars under the window's top, as now
  (those pages are centred; nothing is covered).
- Fade in and out (`Theme.fast`); none with Reduce animations.

### 9.5 Keyboard, screen readers, high contrast

- The pill is `Accessible.AlertMessage` (announced as the bar is today); the reconnecting tooltip text is
  its accessible description.
- Its buttons are in the Tab order after the command bar's buttons. Esc doesn't dismiss an offer (it has
  to be answered with Accept or Decline; the offer bars have no close button either).
- High contrast: the system colours and the solid border, as `InfoBar`.

### 9.6 Code

- `CommandBarMessage.qml` (new): the pill (text, severity, icon, buttons as children, `timeout`), its
  backdrop, and the fit check (`fits`: its implicit width against the free middle).
- `CommandBar.qml`: `messageArea` (the item that fills the free width); each page sets it (`GamePage`,
  `HomePage`, `OnlinePage`, `ProfilePage`, `SettingsPage`).
- `GamePage.qml`: `boardCenterX` (page coordinates).
- The history (§9.9): a `ListModel` in `ChessGame.qml` that `notify()` and the offers append to;
  `MessagesFlyout.qml` (the list, a `Popup` under the button whose menu opened it); "Recent messages" in
  the game's More menu and Home's account menu; the dot on those buttons.
- `ChessGame.qml`: one pill layer over the pages (z above the bars). The `messageBar.show()` callers go
  through one `notify(text, severity)` that picks the pill or the bar; the offers, the reconnecting
  status and the opponent left get a pill version, and their bar version is used when the pill doesn't
  fit or is taken. The bars' column (step 2) and the room on the game page (#134) stay for what goes to
  the bars.
- No C++ change.

### 9.7 Screenshots

| Scene | Size | What it checks |
|---|---|---|
| `check` | default, 960x1040, 1400x900 | "Check" in the command bar; the board as in `game` (no room taken) |
| `reconnecting` | default, 1400x900 | the reconnecting pill (re-baselined: it was a bar) |
| `draw-offer` (new: a Lichess game fed a draw offer, no network) | default, 1400x900, 640x560 | the offer in the pill; at 640 it doesn't fit: the bar under the command bar |
| `challenge-ingame` | default, 1400x900 | unchanged: the challenge stays a bar |
| `saved` (new: the game saved) | 1400x900 | a confirmation centred on the board, right of the window's centre |
| `home-message` (new: "Profile imported" on Home) | default | the pill on another page |
| `messages` (new: the history open: a confirmation, a draw offer still open, a declined challenge, a lost and restored connection, an error) | default, 1400x900 | the More button's dot, the list, live and finished actions |

Light and dark. `reconnecting` and `check` are re-baselined after a look; the rest stay unchanged.

### 9.8 The owner's choices

Q5 yes, Q6 as recommended, Q7 as is (the owner, 2026-11-10).

| | Question | Recommended |
|---|---|---|
| Q5 | Offers (draw, takeback, rematch) in the pill too, or only messages without buttons? | **In the pill** when they fit (≈ 400 px: every window ≥ 700 wide); else the bar |
| Q6 | The title while a pill shows | **Hidden under the pill's backdrop**, and it comes back after; or the pill only in the space the title leaves (it would rarely fit at 640–768) |
| Q7 | How long a message stays | **4 s** as today ("Check" too); an error stays until closed (a × on the pill) |

### 9.9 Recent messages (the owner, 2026-11-10: "if I missed a notification")

A message lasts 4 s, and the window may have been in the background or the player busy with the board.
The recent messages are kept and can be looked at again.

- **Where** (Q8, the owner): **"Recent messages"** in the game page's **More** menu (⋯), and in Home's
  account menu (Home has no More menu); **Ctrl+M** opens it on every page (Settings, Profile and Online
  have neither menu). No new button: the command bar keeps its free width (§9.4).
- **The dot**: an accent dot on the More button (Home: the account button) when a message the player
  didn't cause came since the list was last opened: an offer, the opponent left, a lost or restored connection, "… declined the challenge",
  an error. Messages about the player's own actions ("Game saved", "PGN copied", "Move taken back") are
  kept but don't light it.
- **The list**: a flyout under the More button (`Popup`, where the menu was), newest first, the **last 20**: the
  severity icon, the text in full (wrapped: also the long errors), the time ("12:41"), and the page or
  game it came from when that isn't the current one ("Lichess · vs Friend").
- **Actions**: an offer still open keeps its Accept / Decline in the list (the same calls as the pill);
  once answered or gone it shows what happened ("Draw offer · declined", "· expired"), without buttons.
  The opponent left keeps Claim victory while it can be claimed. A challenge isn't listed (the Online page
  lists them, BL-10).
- **Lifetime**: this session only (not saved; the app's log has them all). A new game doesn't clear it.
  "Clear" at the list's bottom empties it.
- **Keyboard**: the menu item, or **Ctrl+M** from anywhere (F1's list of shortcuts says so); arrow keys
  move through the list; Esc closes it.
- **Screen readers**: the More button's name says the new count ("More, 2 new messages"); each row reads as "Friend offers a draw, 12:41,
  open".
- **"Check"**: not kept (Q9): it comes with every check, and the board, the sound and the move list
  ("+") already tell it.

The owner's choices (2026-11-10): **Q8** "Recent messages" in the More menu (not a bell button);
**Q9** all but "Check", the last 20, the dot only for those the player didn't cause; **Q10** this
session only.

### 9.10 Done early in #137 (2026-11-10)

Checking #137, the owner found "Check" still changed the board's size with the cards above and below the
board. The message bar's part of this step was done there: `CommandBarMessage.qml` shows "Check",
"Game saved", "PGN copied", … over the command bar (`CommandBar.messageArea`, centred on the board,
the title hidden up to it where they'd cross), and falls back to the InfoBar when a message doesn't fit
or the page has no command bar; the decision follows the window's size. Left for this step: the offers,
the reconnecting status and the opponent left in the command bar, the priority between them, an error
staying until closed, and the recent messages (§9.9).
