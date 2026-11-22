# BL-35 — Split ChessGame.qml: plan

`ChessGui/ChessGui/qml/ChessGame.qml`, the window, is about 1480 lines. This plan moves the sounds into one module and
the window's other self-contained parts into their own QML files, in small steps. No behaviour changes, unless
a step says so. Line numbers are from `develop` at 772be8c (after BL-34).

## What stays as it is

- **`notify()`** stays the window's (the `qml/CLAUDE.md` rule "every short message goes through
  `ChessGame.notify()`"). A new part that has something to say emits a `message(text, severity, external)`
  signal, and the window passes it to `notify()`.
- **The visible items** stay in the window: the bars over the command bar, the InfoBars, the challenge bar, the
  recent-messages flyout and the dialogs. They bind to the new objects. Every `objectName` a UI test finds
  stays, and so does the z-order.
- **Routing and the page Components:** `route()`, the pushed pages and the page Components (lines 95–168 and
  567–668). They are the StackView's own logic.
- **The Shortcuts** (1396–1475): moving them would need about 10 signals and gain nothing.
- **No C++.** Every new file is QML, so `ChessGuiTests.vcxproj` is untouched. Each new file goes into
  `qml.qrc`, `ChessGui.vcxproj`'s `<None>` list and `ChessGui.pro`'s `DISTFILES`.

## 1. The sounds: `GameSounds.qml` (first PR)

### Where each sound plays today

| Sound | Played from | Trigger | When it plays |
|---|---|---|---|
| move, move_opponent | `onMoveCountChanged` (1046) | `moveCountChanged` | as the slide starts (by `MoveCount % 2`: White's or Black's move) |
| check | `announceCheck()` (333): from `checkNotify` and GamePage `checkShown` | a move gives check; a resumed game opens in check | a move's check: at once, as the slide starts; a resumed game's: at the first `boardIdle` (GamePage) |
| checkmate, win | `_endSoundWhenIdle` (340–360) | `checkMateNotify`, `checkMateResult` | at `boardIdle`; at once if the board is idle or a position is being reviewed (BL-34) |
| lowtime | GamePage `lowTimeShown` → the window (666) | `GameClock.lowTime` | once the page has been open 1 s (GamePage) |
| challenge | `onChallengeReceived` (1136) | `LichessClient.challengeReceived` | at once; the taskbar flash stays in the window |
| previews: move, opponent | `SettingsPage` (35–36, 144–149) | the volume slider | at once |

### The module

`GameSounds.qml` is a component with one instance in the window:
`GameSounds { id: sounds; objectName: "gameSounds" }`. It has:

- **The seven `SoundEffect`s,** with the mute check (`AppSettings.musicEnabled`) and the volume
  (`effectsVolume()`). SettingsPage loses its own two.
- **`play(name)`:** plays at once, if sound is on and the effect is ready. Names: `move`, `moveOpponent`,
  `check`, `checkmate`, `win`, `lowTime`, `challenge`.
- **`playWhenSeen(name)`:** the rule that a sound about the position comes once it can be seen. It queues the
  sound and plays it at `ChessConnector.boardIdle`, or on the next pass if the board is idle or a position is
  being reviewed. BL-34's queue moves here, including what drops it: a new game, a takeback, leaving the game.
- **Its own `Connections`:** to `ChessConnector` (moves, mate, win, `boardIdle`, and the signals that clear the
  queue) and to `LichessClient.challengeReceived`.
- **`signal played(string name)`:** for a UI test.

The callers:

- **The window** keeps `announceCheck()` (`sounds.play("check")` + `notify("Check")`) and
  `onLowTimeShown: sounds.play("lowTime")`.
- **GamePage** doesn't change: its holds for the check on opening and for low time are the page's own state.
- **SettingsPage** gets `signal soundPreviewRequested(string name)`, and the window answers it with
  `sounds.play(name)`.

**Why a component, and not a singleton or C++** (the owner, 2026-11-22: QML wherever possible; C++ only where
it is clearly better or simpler):

- **A QML singleton** lives as long as the engine, not the window. The UI tests make and close windows in one
  engine (`AppDriver`), so the queue and the Connections would outlive their window. Pages could also call it
  from anywhere, which would scatter the triggers again.
- **C++:** the timing already comes from C++ (`MoveGate`, `boardIdle`), and the mute switch and the volume are
  QML `AppSettings`. A C++ class would need Bindings, a registration in `AppServices` and four project lists.
  Worth it only for unit tests of the queue with `MoveGate`'s fake clock.

**Test:** a UI test that mates and checks that `played("checkmate")` comes after `boardIdle`. To check first:
whether a `SoundEffect` is ready on the test runner, which has no audio device. If it isn't, `played` is
emitted for the request, not the effect.

## 1b. Check after the slide, like mate (its own small PR, right after step 1)

The owner's rule (2026-11-22): the move first, then its sound, the same way everywhere. Step 1 moves the code
and changes nothing. Step 1b is the one change in behaviour:

- A move's check goes through `playWhenSeen("check")`, and the "Check" message waits with it. The window's
  `onCheckNotify` asks GameSounds to tell it when the sound plays, e.g. `playWhenSeen("check", () =>
  notify("Check", "caution"))`. The sound and the message then come together, after the slide.
- A resumed game that opens in check keeps GamePage's hold. It already comes at the first `boardIdle`.
- After this, the only sound that plays as the slide starts is the move's own (move, move_opponent): it is the
  move. Low time and a challenge aren't about the position, so they still play at once.
- **Test:** a UI test that gives check and sees `played("check")` and the "Check" message after `boardIdle`.
- **Scene to compare:** `check`.

## 2. `levelColor` → `Theme`, and `StartNotice.qml`

- **`levelColor(level)`** (74–79) moves to `Theme`. `LevelsHelpDialog.qml:59` calls `root.levelColor`, and
  that works only because QML finds the window's id by dynamic scoping. Nothing else reaches into the window
  this way. This has to move before any dialog does.
- **`StartNotice.qml`:** the "Starting the game…" overlay (502–565), with a `shown` property bound to
  `_startingGame`. The `starting` scene sets `window._startingGame`, which stays in the window.
- **Scenes to compare:** `starting`, `timing`.

## 3. `MessageCenter.qml`: the messages' state

A `QtObject` that holds the state of the short message, the game's status and the recent messages (686–735,
790–877), with their `ListModel`, the 4 s `Timer` and the `Connections` for offers, the opponent leaving and the
lost connection.

- **In:** `flyoutOpen`, `screenshotMode`.
- **Out:**
  - state: `message`, `barMessage`, `statusKind`, `offerText`, `goneText`, `canClaim`, `claimSeconds`,
    `goneDismissed`, `answerable`, `unseen`, `model`;
  - functions: `notify()`, `closeMessage()`, `addRecent()`, `respondOffer()`, `resetForUser()`,
    `statusShows()`.
- **The window keeps:** `notify()` as a one-line forward, `openRecentMessages()`, and
  `property alias unseenMessages` (`tst_board` sets `app.window.unseenMessages`).
- **What changes:** ScreenshotScene's five `window._addRecent(…)` (lines 300–304) become
  `window.messages.addRecent(…)`.
- **Scenes to compare:** `home-message`, `messages`, `messages@640x560`, `draw-offer`, `opponent-left*`,
  `reconnecting`, `check`.

## 4. `LichessChallenges.qml`: the challenge to and from the player

A `QtObject` with the pending challenge, the challenges shown and the `LichessClient` Connections (16–20, 41–72,
362–370, 1090–1171, part of 300–308).

- **In:** `page`, `mode`, `dialogVisible`.
- **Out:**
  - state: `pendingId`, `pendingUrl`, `opponentName`, `rematch`, `challenge`, `challengesShown`, `accepting`;
  - functions: `text(c)`, `accept(id)`, `hide(id)`, `clearPending()`, `resumePending()`;
  - signals: `dialogRequested()`, `dialogClose(atOnce)`, `message(…)`.
- **What stays:** the challenge bar stays in the window with its objectNames. The sound and the taskbar flash
  are not part of this: the sound is GameSounds', the flash stays in the window.
- **What changes:** ScreenshotScene's `window._pendingChallengeId`, `_pendingOpponentName` and
  `acceptChallenge` become `window.challenges.*`. GamePage's `rematchPending` binds to `challenges.rematch`.
- **Scenes to compare:** `challenge*`, `online-challenge`, `challenge-accepting`.

## 5. `ProfileFiles.qml` and `DataCheckNotices.qml`

- **`ProfileFiles.qml`:** an `Item` that holds the export and import `FileDialog`s, `ImportConflictDialog` and
  the delete confirm, with their functions (174–226, 1244–1286).
  - Out: `exportProfile(id)`, `openImport()`, `inspectImport(file)`, `deleteProfile(id)`, `dialogOpen`;
    signals `message(…)`, `closeProfileRequested()`.
  - The `import` scene calls `window.inspectImport`: the window keeps a one-line forward.
- **`DataCheckNotices.qml`:** BL-20's notices that games were set aside and that the rating or the statistics
  were restored (1302–1331, 375–379). It has `tellAll()` and signal `message(…)`.
- **Tests:** `tst_profiles`, `tst_restart` (the texts don't change). Scene: `import`.

## 6. "Games kept" → `SettingsPage`

- `setGamesKept` and its confirm (1288–1299, 1335–1345) move into SettingsPage, which asks for them. The rule
  is that a page's own confirm stays in it, like GamesPage's Delete.
- The page gets `dialogOpen`, and the window's `_dialogOpen` also reads `stack.currentItem.dialogOpen`.
- `tst_games` still finds `gamesKeptConfirm` by objectName.
- **To check here:** GamesPage's `deleteGameConfirm` isn't in `_dialogOpen`, so Esc with it open may also pop
  the page. If it does, that bug gets its own ticket.

## Not in BL-35: the leave flow (BL-36)

The leave, resign, switch-user and quit flow (`LeaveFlow.qml`) is its own task, **BL-36**, done after BL-35
(the owner, 2026-11-22). It is the plan's highest risk, and the only part that may change behaviour (root rule
1), so it is kept apart.

## Each step

- `scripts\Run-UiTests.ps1 -Build`, every test file green.
- The screenshots of the scenes named above, compared with the baselines (`-Screens`; the local baselines are
  stale, see `PROGRESS.md`).
- `qml/CLAUDE.md`: the new files under "The window". The rule "`ChessGame.qml` owns the dialogs and the
  InfoBars" becomes "`ChessGame.qml` and its parts (…)".

## Size

| Step | Lines leaving the window (net, about) |
|---|---|
| 1 Sounds | 45 |
| 2 StartNotice | 60 |
| 3 MessageCenter | 125 |
| 4 LichessChallenges | 105 |
| 5 ProfileFiles + DataCheckNotices | 115 |
| 6 Games kept | 20 |

Steps 1–6 take `ChessGame.qml` from about 1480 to about 1000 lines; BL-36 takes it to about 925.

## Decided (the owner, 2026-11-22)

1. **QML wherever possible.** C++ only where it is clearly better or simpler. The sounds are a QML component.
2. **The move first, then its sound, the same way everywhere.** A move's check waits for the slide too (step 1b).
3. **The leave flow is its own task** (BL-36), after BL-35.
