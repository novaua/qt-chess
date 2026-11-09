# BL-13 Game Page Layout for Wide Screens — Implementation Plan

| | |
| :---- | :---- |
| **Date** | 2026-11-09 |
| **Status** | Step 1 coded (branch `wideGameLayout_091126`); step 2 next. The owner chose the recommended answers to Q1–Q4 (2026-11-09) |
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
3. Later, from the owner: other move-history views (§4.5).

## 7. Risks

- **A switch during a drag or the promotion dialog** (the window resized meanwhile): the board item is
  the same and only its size changes, as when resizing today.
- **Tiny column cards at 200 % scaling**: logical sizes don't change with scaling; the rule of §4.1
  uses logical pixels, so a 4K monitor at 200 % behaves like 1920 × 1080.
- **The InfoBars' width in the column (A)**: a long challenge text wraps to 4–5 lines at 260 px. It
  still fits between the cards at 1400 × 900; at 1280 × 720 (≈ 270 px free) two bars may not, so the
  column's bars scroll or fall back to B when they don't fit (decide in step 2).

## 8. The owner's choices

All four as recommended (the owner, 2026-11-09): Q1 A, Q2 mirrored, Q3 the bigger board, Q4 centred.

| | Question | Recommended |
|---|---|---|
| Q1 | The InfoBars in the column layout | **A**: between the cards in the left column (the board never changes size); B: over the board, the board shrinks while one shows (#134) |
| Q2 | The cards' order in the column | **Mirrored** (clocks at the outer edges, as on lichess.org); or the same order in both cards |
| Q3 | When to switch | **The bigger board wins** by ≥ 24 px (§4.1); or a fixed aspect ratio (e.g. width ≥ 1.4 × height) |
| Q4 | The extra width of very wide windows | **Centred** for now; the moves panel takes it with the move-history work later |
