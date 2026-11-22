---
id: BL-35
title: "Split ChessGame.qml: the sounds and the window's other parts into their own files"
type: task
status: in_progress
priority: medium
---

# BL-35 Split ChessGame.qml: the sounds and the window's other parts into their own files

`ChessGui/ChessGui/qml/ChessGame.qml` (the window) has grown to about 1480 lines. The owner wants every sound the app
plays in one module of its own, and the window's other parts (messages, Lichess challenges, profile files, …) in
their own files where that pays.

Plan, for the owner's approval: [docs/BL-35-ChessGameSplitPlan.md](../../docs/BL-35-ChessGameSplitPlan.md). One PR
per step, the sound module first; the UI tests and the screenshots of the scenes named in the step green after
each.

## Resolution

### 2026-11-22 — step 1: GameSounds.qml (branch: features/bl35GameSounds_221126)
- Done: `qml/GameSounds.qml`, one instance in the window (`gameSounds`): the seven `SoundEffect`s, the mute
  switch and the volume, `play(name)`, `playWhenSeen(name)` (BL-34's queue and what drops it), its own
  Connections (moves, mate, win, `boardIdle`, a new challenge), `played(name)`. The window keeps
  `announceCheck()` and the low time hold's answer; SettingsPage lost its two effects and asks through
  `soundPreviewRequested`. `ChessGame.qml` 1476 → 1428 lines. No change in behaviour, except that every sound
  now plays only when its effect is loaded (before: only the moves' and the preview's checked).
- Checked: `Run-UiTests.ps1 -Build`, 13 files green; `tst_twoPlayers` test 1 now checks each move's sound and
  the mate's once, with the board idle. No screenshots: nothing drawn changed.
- Left: step 1b (a move's check after the slide), then steps 2–6.
