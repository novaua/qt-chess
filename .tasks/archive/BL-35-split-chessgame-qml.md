---
id: BL-35
title: "Split ChessGame.qml: the sounds and the window's other parts into their own files"
type: task
status: done
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

### 2026-11-22 — step 1b: a move's check after the slide (branch: features/bl35CheckAfterSlide_221126)
- Done: a move's check goes through `playWhenSeen("check", …)`, and its "Check" message comes with it: both at
  `boardIdle`, after the slide, as the mate's (the owner's rule: the move first, then its sound). The queue keeps
  entries with sound off too, so "Check" still shows. A resumed game that opens in check keeps GamePage's hold.
  Only a move's own sound plays as its slide starts.
- Checked: `Run-UiTests.ps1 -Build`, 13 files green. `tst_board` test 2 checks the order (each move's sound, then
  the check's and "Check") and that they come with the board idle, at least 100 ms after the move's sound (the
  gate's beat); step 1's mate test in `tst_twoPlayers` got the same time check. Both fail on the old timing (at
  once) and on a queue that only waits for the next pass. The check signal comes inside the move, before the
  board is marked busy, so the board's idle state alone couldn't tell. Scene `check` (`-Offscreen`): "Check"
  shown; 1.86 % from the stale baseline (piece edges, an old command bar icon).
- Left: steps 2–6.

### 2026-11-22 — step 2: levelColor → Theme, StartNotice.qml (branch: features/bl35StartNotice_221126)
- Done: `levelColor` is `Theme.levelColor(level, count)`, the count passed in so that `Theme` doesn't read the
  game; `LevelsHelpDialog` no longer reaches into the window by its id. "Starting the game…" is
  `qml/StartNotice.qml` (`shown`, bound to the window's `_startingGame`, which stays for `route()` and the
  `starting` scene). `ChessGame.qml` 1429 → 1363 lines. No change in behaviour.
- Checked: `Run-UiTests.ps1 -Build`, 13 files green. Scenes `starting`, `timing`, `levels` (`-Offscreen`): the
  same as `develop`'s, taken just before as their baselines (the old ones were stale: the setup dialog's Variant
  row, the levels dialog's place).
- Left: steps 3–6.

### 2026-11-22 — step 3: MessageCenter.qml (branch: features/bl35MessageCenter_221126)
- Done: `qml/MessageCenter.qml`, one instance in the window (`id: messageCenter`; `window.messages` for
  ScreenshotScene, whose five `_addRecent` calls are now `messages.addRecent`): the game's state, the short message
  with its 4 s Timer, the recent messages' `ListModel`, and the Connections for offers, the opponent leaving and
  the lost connection. The window keeps `notify()` as a one-line forward, `openRecentMessages()`, the profile check
  (`_userChanged()` → `resetForUser()`) and `property alias unseenMessages`. Apart from the plan: it is an
  invisible `Item` (like GameSounds: it holds a Timer, a ListModel and Connections), and takes two more inputs,
  `shortShowing` and `statusFits` (`barMessage` and `statusShows()` need them; the two bars stay in the window).
  `ChessGame.qml` 1363 → 1268 lines. No change in behaviour.
- Checked: `Run-UiTests.ps1 -Build`, 13 files green. 20 message scenes (`-Offscreen`: `home-message`, `messages`
  ×3, `draw-offer` ×3, `opponent-left*` ×7, `reconnecting` ×2, `check` ×3): the same as `develop`'s, taken just
  before as their baselines.
- Found on the way, filed: BL-37 (`tst_chess960` test 6 failed once on `develop`), BL-38 (a `ChessFigures.qml`
  TypeError in the test logs, on `develop` too).
- Left: steps 4–6.

### 2026-11-22 — step 4: LichessChallenges.qml (branch: features/bl35LichessChallenges_221126)
- Done: `qml/LichessChallenges.qml`, one instance in the window (`id: lichessChallenges`; `window.challenges` for
  ScreenshotScene's `online-challenge` and `challenge-accepting`): the pending challenge, the challenges shown, Hide,
  Accept, and the `LichessClient` Connections for them, `onNetworkError` whole (it decides between the challenge
  dialog and the online game's error). Signals `dialogRequested`, `dialogClose(atOnce)`, `message`. The window keeps
  the challenge bar, `ChallengeDialog` (now `objectName: "challengeDialog"`), the taskbar flash and the seek's
  dialog and messages. Beyond the plan: `cancelPendingRematch()` for leaving the game and switching the profile,
  and an invisible `Item` as the other two. `ChessGame.qml` 1268 → 1174 lines. No change in
  behaviour.
- Checked: `Run-UiTests.ps1 -Build`, 13 files green. New `tst_lichess` test 16: a challenge to a named player,
  its dialog, declined (the message, nothing pending), another canceled; it raises `LichessClient`'s signals, as
  FakeLichess doesn't take the player's own challenges. It fails when the decline handler doesn't clear the
  challenge. 8 challenge scenes (`-Offscreen`): the same as `develop`'s, taken just before as baselines
  (`online-challenge` 0.006 %: its spinner's angle).
- Left: steps 5–6.

### 2026-11-22 — step 5: ProfileFiles.qml, DataCheckNotices.qml (branch: features/bl35ProfileFiles_221126)
- Done: `qml/ProfileFiles.qml` (`profileFiles`): the export and import `FileDialog`s, `ImportConflictDialog` and the
  delete confirm (now `objectName`s `importConflictDialog`, `deleteProfileConfirm`), with `exportProfile()`,
  `openImport()`, `inspectImport()`, `deleteProfile()`, `dialogOpen`, signals `message` and
  `closeProfileRequested`. `qml/DataCheckNotices.qml` (`dataCheckNotices`): BL-20's notices, `tellAll()` and their
  Connections. The window keeps `inspectImport()` as a one-line forward (the `import` scene) and the Lichess
  disconnect confirm. `ChessGame.qml` 1174 → 1058 lines. No change in behaviour.
- Checked: `Run-UiTests.ps1 -Build`, 13 files green (on the rerun: see BL-37). New `tst_profiles` test 5: the
  active profile deleted from the Profile page (Home's `profileItem`, `deleteProfileButton`): the confirm names
  it, the profile goes, the picker is left. `tst_restart` still sees the notices. Scenes `import`, `profile`,
  `profiles` (`-Offscreen`): the same as `develop`'s, taken just before as baselines.
- Left: step 6.
### 2026-11-22 — step 6: "Games kept" → SettingsPage; closed (branch: features/bl35GamesKept_221126)
- Done: `setGamesKept()` and `gamesKeptConfirm` are SettingsPage's (a page's own confirm stays in it); its signal
  `gamesKeptChangeRequested` is gone. A page with a dialog of its own has `dialogOpen`, and the window's
  `_dialogOpen` reads `stack.currentItem.dialogOpen`. The window keeps the `keepCount` Binding.
  `ChessGame.qml` 1058 → 1035 lines.
- The plan's check: GamesPage's `deleteGameConfirm` wasn't in `_dialogOpen`, so with it open the window's
  shortcuts (Esc, F1, Ctrl+M, Ctrl+,) stayed enabled. Whether a key really got past the confirm isn't seen: it is
  modal, Qt Quick Controls may hold back shortcuts outside a modal popup, and a test can't type into the app's
  window. GamesPage got `dialogOpen` too (one line, the same mechanism), so it counts like every other dialog;
  no ticket of its own.
- Checked: `Run-UiTests.ps1 -Build`, 13 files green. `tst_games` test 4 (Delete) and test 5 (Games kept) check
  `_dialogOpen` while the confirm is open and after; test 4 fails without GamesPage's `dialogOpen`. No
  screenshots: nothing drawn changed.
- BL-35 done: `ChessGame.qml` about 1480 → 1035 lines over steps 1–6 (`GameSounds`, `StartNotice`,
  `MessageCenter`, `LichessChallenges`, `ProfileFiles`, `DataCheckNotices`, `levelColor` in `Theme`). The leave
  flow is BL-36.
