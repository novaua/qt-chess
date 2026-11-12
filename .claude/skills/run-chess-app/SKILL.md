---
name: run-chess-app
description: Build, launch and screenshot the ChessGui app (Qt QML) to see a view or check a UI change. Use when asked to run the app, look at a screen, take screenshots, or confirm a QML/UI change in the real app.
---

# Run and screenshot ChessGui

## Build

```powershell
.\scripts\Take-Screenshot.ps1 -Build            # builds ChessGui Release x64, then screenshots the menu
```

Or build only (MSBuild comes from the VS install; `vswhere` finds it):

```powershell
& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -find 'MSBuild\**\Bin\MSBuild.exe'
& <msbuild> ChessCore.sln /t:ChessGui /p:Configuration=Release /p:Platform=x64 /m /v:minimal /nologo
```

The exe is `x64\Release\ChessGui.exe` (or `x64\Debug\`). Qt DLLs come from `PATH`
(`D:\Tools\Qt\6.12.0\msvc2022_64\bin`); nothing is deployed next to the exe.

## Screenshots (preferred way to look at a view)

`scripts\Take-Screenshot.ps1` runs the app in screenshot mode: fresh temporary profile (the
user's own users/settings/saves are never touched), opens a scene, saves a PNG, exits. The window
is real but transparent, frameless, without a taskbar button and ignores the mouse — nothing shows
on screen. Shots are at 100% scale whatever the monitor scaling. Then **Read the PNG** to see it.

```powershell
.\scripts\Take-Screenshot.ps1 -Scene all                                    # every scene
.\scripts\Take-Screenshot.ps1 -Scene game -Moves "e2e4 e7e5 g1f3"           # position after moves
.\scripts\Take-Screenshot.ps1 -Scene game -Moves "f2f3 e7e5 g2g4 d8h4" -Name foolsmate   # game end
.\scripts\Take-Screenshot.ps1 -Scene game -Black -Dark -Size 1400x900       # flipped, dark, landscape
.\scripts\Take-Screenshot.ps1 -Scene all -Offscreen                          # screen locked: dark only
.\scripts\Take-Screenshot.ps1 -Scene home,game -HighContrast                 # the app's high-contrast colours
```

- Scenes: `welcome` (first run), `create-user` (first run, Create profile dialog), `profiles` (Who's
  playing? with Bob, Alice and Tester, who has a Lichess account), `home`, `continue` (Home with an autosaved robot game and the `online` scene's four Lichess games: "Continue a game", which opens the list), `continue-online` (no robot game, four Lichess games), `continue-list` (BL-17: the Continue list over Home, the robot's game first, then the four Lichess games; also at `1400x900` and `640x560`), `continue-list-online` (the list without a robot game), `continue-list-robot` (the robot's game alone, as when the Lichess games end while the list is open), `game` (two-player
  game, Tester vs Bob), `leave-online` (the game with the leave warning of a running Lichess game: Leave / Resign / Stay), `review` (a middlegame reviewed at move 5), `result` (Fool's mate: the result
  card), `ended` (the same with the card closed: Rematch / New game in the moves panel, Result in the sheet), `moves` (the middlegame with the move sheet open; narrow window only), `select` (1. e4 e5, the g1 knight selected: tinted square and its move dots; with `-Black` on the turned board),
  `drag` (the same knight held over f3: faded on g1, dots, f3 outlined; `-Black` too), `settings` (Settings page, top),
  `about` (Settings scrolled to About: version, licences, shortcuts), `restore` (Settings with the restore
  dialog: two backups, the newest the same as now), `levels`,
  `profile` (Profile page: Lichess linked with ratings, statistics), `profile-local` (no Lichess),
  `profile-expired` (Lichess sign-in expired: Reconnect), `import` (importing Bob again: the conflict
  dialog), `online` (the Online page with four seeded games, at its end since BL-17: your turn / waiting, a Blitz lobby game to open on lichess.org), `online-empty` (signed in, no games), `online-disconnected` (no Lichess account), `computer` (vs-computer setup dialog), `players`
  (two-player setup dialog), `players-many` (seven opponents: with `-Size 640x560` the dialog scrolls),
  `clock` (a two-player game with its clock running), `clock-low` (red, tenths), `clock-flag` (out of time),
  `clock-flag-draw` (out of time against a lone king: a draw), `clock-promotion` (out of time while the piece
  is chosen: the pawn goes back), `clock-continue` (a robot game continued, its clock running), `stalemate`,
  `clock-online` and `clock-correspondence` (a Lichess game fed to the stream parser, no network),
  `online-blitz` (the challenge card with a name: Blitz lit), `online-challenge` (waiting for that player),
  `premove`, `premove-played`, `premove-dropped` (LI-05: a Lichess game fed to the stream parser, no network:
  Bf1-c4 waiting in red and the d2 pawn's premove dots; Black's Nc6 comes and Bc4 is played (our moves aren't
  posted in screenshot mode); e4-e5 waiting, gone after Nc6), `rejoin` (BL-18: a Lichess game joined with ten moves, grabbed the moment its page is up, no `--delay`: the position after 5… a6, never the start position), `reconnecting` (LI-06: the same game with its
  connection lost: the caution bar), `challenge` (BL-10: two Lichess challenges to Tester on Home, fed to
  the event stream's parser: the bar with Accept / Decline / Hide and "+1 more"), `challenge-unplayable`
  (a Bullet challenge: Open on lichess.org instead of Accept), `challenge-ingame` (a real-time Lichess
  game on the board, then a challenge: Decline only, "Finish your game first"), `online-challenges` (the
  Online page's "Challenges to you": three, one of them Bullet), `online-challenges-decline` (the same with
  the first one's Decline… menu open: Lichess's reasons), `captures` (BL-13: Black down to a lone king,
  White lost six: a full captured strip and "+29"), `check` (the page opens in check: "Check" over the command bar), `draw-offer` (a Lichess game, the opponent offers a draw: Accept / Decline over the command bar; at 640x560 it doesn't fit: the InfoBar), `saved` ("Game saved" centred on the board), `home-message` ("Profile imported" over Home's command bar), `messages` (the recent messages open: an open draw offer, an error, the connection, a decline), `opponent-left` (the Lichess opponent left: the countdown, frozen at 12 s), `opponent-left-claim` (Claim victory, on the opponent's turn; at 640x560 it doesn't fit: the InfoBar), `opponent-left-turn` (the player's turn: no Claim victory, "your move, then claim victory"; at 640x560 the text is cut short, not the InfoBar), `opponent-left-error` (a refused claim's error in the command bar's place for its 4 s, the opponent-left message waiting behind it), `saved@640x560` (a short message in a narrow window: over the command bar, the board unmoved), `messages@640x560` (the recent messages kept inside a small window), `continue-starting`, `continue-list-starting`, `challenge-accepting` (BL-16: Lichess, a local server that never answers, hasn't answered yet: Home's Continue card "Connecting to Lichess…", Friend's card "Starting…", the challenge bar's "Accepting…"; the ring is a still clock in the scenes), `starting` ("Starting the game…" as while a game page is built, over the vs-computer dialog that started it: the notice is on the Overlay layer, above popups; the scenes themselves skip it), `more-menu` (BL-11: the game's More menu open; it is made on its first opening),
  `all`.
- **Sized scenes** (BL-13): `scene@WxH[@black]` (quoted: `-Scene 'game@1400x900'`) runs a scene at its own window size, saved as
  `<scene>[-black][-dark]-<WxH>.png`. `all` ends with the list in `$sizedScenes`
  (`Take-Screenshot.ps1`): the player column at `1920x1080` and `1400x900` (`game`, `clock-online`,
  `captures`, `review`, `ended`, `result`, `challenge-ingame`, `drag@black`, `reconnecting`, `check`:
  the bars between the column's cards), and `game` at `1100x900`, `960x1040` and `1000x560` (no room
  for a bar between the cards), which must keep the cards above and below the board. `premove-robot`: 1. e4 vs the robot, Nf3 premoved
  and played after its answer (1. e4 e5 2. Nf3 Nc6). `robot`: a saved game vs the robot at level 5, continued, where the robot mates
  with Qh5# (the engine's answer is played on the GUI thread). These two and `play` run against **FakeUci** (BL-05 step 3,
  `$engineScenes` in `Take-Screenshot.ps1`: the game it follows), so they are the same every time and in `all`; `-Stockfish`
  runs them against the real engine (their pictures then vary).
  `timing` (not in `all`, BL-11; since BL-11 step 2 the board's layers are made in the first frame's polish (`LayoutGate`), so most of the build shows as `first frame`, not `page`): from Home, 1.5 s apart, vs Robot, leave, Continue, leave, two players, leave, vs Robot as Black left while the engine thinks, then Settings and the Online page opened and closed (`[Timing] Settings: N ms to open`); run with `-KeepLog` (keeps `<picture>.png.log` also on success) and read its `[Timing]` lines (and, with `$env:QT_LOGGING_RULES='chess.frames.debug=true'`, "GUI thread busy"). Its picture is not a baseline.
  `play` (not in `all`, BL-11 step 3; FakeUci unless `-Stockfish`): a game vs the robot (level 1) played with mouse events (`SceneInput`: clicks and a drag through the board's handlers), animations on: Nf3 by clicks, the robot's answer, e4 by a drag, a takeback, the review; `[Play]` lines mark the steps (run with `-KeepLog`, the frame trace, or under `qmlprofiler`). Not a baseline.
  `slides` (not in `all`): the BL-14 frame-trace scene — animations on (every other scene forces them
  off), selects the g1 knight and plays Nf3, Nf6, Bc4, Nc6 0.4–1 s apart, so a run with
  `$env:QT_LOGGING_RULES='chess.frames.debug=true'` and `-Delay 5200` gets `[Frame trace]` lines in the
  `.png.log` (frame gaps over 25 ms, the work of the dots/pieces/list). Its picture is an arbitrary
  frame, never a baseline.
  `takeback` (not in `all`, LI-03 check): a Lichess game fed to the stream parser by `main.cpp` (no
  network): four moves, then the list after an accepted takeback of two; the shot shows 1. e4 e5 and
  "2 moves taken back".
  `login`, `menu` and `board` still work as the old names of `profiles`, `home` and `game`.
  The profile scenes get their profiles, Lichess link (a placeholder token, no network) and statistics
  from `seedScreenshotProfiles()` in `main.cpp`.
  `-Moves` replaces a scene's own moves.
- To review many shots at once, tile them into one contact sheet (System.Drawing) and Read that.
- Output: `test-results\screens\<scene>[-black][-dark][-hc][-<Name>].png` (git-ignored). Exit code = number of failed scenes.
- **Sanity gate:** the script first checks that `Qt6Core.dll` is next to the exe or on `PATH` (without it
  Windows shows a "DLL not found" dialog per run) and stops with the line to add Qt's `bin`. If the first
  scene fails, or any run ends with a code that isn't the app's own (missing DLL, crash), the other scenes
  aren't run (`STOP`, counted as failed).
- Screenshot mode turns animations off (`Theme.forceReduceMotion`, not saved): no pulse is caught
  half-way, so the same scene gives the same picture. Page transitions and piece fades are off too.
- `-HighContrast`: our own colours as in a Windows contrast theme (`Theme.highContrast`, system
  palette, solid borders, no photo). The Fluent controls keep their normal look: they only follow
  Windows' own setting, which the script doesn't change.
- `-Size WxH`: default 768x1054 (portrait: the moves are in a sheet at the bottom). At least 960 wide,
  or wider than 1.15 × height, shows the move list beside the board.
- `-Moves` are UCI (`e7e8q` for promotion); an illegal move stops the list there (see the log).
- `-Delay ms` (default 1000) if an animation hasn't finished. A finished game waits for the result card itself.
- On failure the script prints the exit code meaning and the tail of `<file>.png.log`
  (the app's qDebug/QML warnings) — read it for QML errors.

**Locked screen:** a real window renders no frames while Windows is locked (`Get-Process LogonUI`
exists), so animations freeze mid-way (half-closed dialogs, empty squares, page transitions) and a
finished game times out (exit code 2). Then use **`-Offscreen`** (`-platform offscreen`): it renders
fine, but only in dark mode. Offscreen the Fluent style always uses its dark palette and can't be
switched, so `Theme.dark` follows it there. Light shots need an unlocked screen. Dialog backgrounds
don't render offscreen (the page shows through).

Not covered yet: a real connection to Lichess (the scenes feed the stream parser or set the dialog by hand). Sending keys to an interactive window
(SendKeys) doesn't reach the Qt controls reliably and takes over the user's desktop: add a scene instead. Plan for the full UI test suite: `docs/UiTestSuitePlan.md`.

## Does it still work? (UI tests, BL-05)

`scripts\Run-UiTests.ps1` runs `ChessGuiTests.exe` (Qt Quick Test) once per `ChessGuiTests\qml\tst_*.qml`,
offscreen on a fresh profile: the app's window played with real mouse events, its state and items
checked. Works with the screen locked. Results in `test-results\ui\<file>.txt` / `.xml`; the failures
are printed. Exit code = failed files.

```powershell
.\scripts\Run-UiTests.ps1 -Build                                   # build ChessGuiTests, run every file
.\scripts\Run-UiTests.ps1 -Name tst_twoPlayers -Function test_2_newGameAndDrag   # one function
.\scripts\Run-UiTests.ps1 -Build -Screens                          # + all screenshots and the comparison: the pre-PR check
.\scripts\Run-UiTests.ps1 -Name tst_twoPlayers -Visible            # the owner watches: a real window, 500 ms per action (-StepDelay)
```

A failed test function leaves `test-results\ui\<TestCase>-<function>.png`, the window at the failure
(offscreen: dark), listed under the failure: Read it. `-Visible` puts windows on the owner's screen and,
after a failure, waits until they close the window: for the owner, not for an agent's runs.

The robot in the UI tests is FakeUci too: `app.startComputerGame("e2e4 e7e5 …", thinkMs)` gives it the
game to follow and how long it thinks (`tst_robot.qml`).

A new test: a `TestCase` in `ChessGuiTests\qml\tst_<area>.qml` with an `AppDriver` (`app.start()` in
`initTestCase`, `app.stop()` in `cleanupTestCase`; functions `test_1_…`, `test_2_…` run in name order
and go on from each other). Give the items it needs an `objectName`. Keys can't be typed into the
app's window from a test (they go to the test's own): set a `TextField`'s text instead.

## Did the look change? (screenshot baselines)

`scripts\Compare-Screenshots.ps1` compares `test-results\screens\*.png` with
`test-results\baselines\` and writes `test-results\diffs\<name>.png` (baseline greyed, changed pixels
red) for each one that changed. Baselines are local (fonts and scaling differ per PC). Exit code =
number of failures; `NEW` = no baseline yet.

```powershell
# On develop (or before the change): accept the current look
.\scripts\Take-Screenshot.ps1 -Scene all; .\scripts\Take-Screenshot.ps1 -Scene all -Dark
.\scripts\Compare-Screenshots.ps1 -Update
# After the change: what looks different? Read the diffs of the failures.
.\scripts\Take-Screenshot.ps1 -Scene all; .\scripts\Take-Screenshot.ps1 -Scene all -Dark
.\scripts\Compare-Screenshots.ps1            # -Name home*,game* for some only
```

A pixel changed when a channel differs by more than `-Tolerance` (24); a shot fails above
`-MaxChangedPercent` (0.05 %). Two runs of all scenes match, except that `settings`, `about` and `restore` now and then differ by a
line: the Settings page's engine lookup races the shot, so take those again before reading anything into
it. `about` and `about-dark` also fail (~0.08 %) after every rebuild of the exe: the version line shows
the build date; accept them with `-Update -Name 'about*'` once the diff shows only that. A baseline without a new
screenshot fails as `MISSING` (Take-Screenshot.ps1 deletes the old PNG first, so a failed scene leaves
none): take the same scenes as the baselines, or use `-Name`. Needs PowerShell 7. A scene that shows
the time of day must use fixed times (`restore`: backups at 9:00 and 9:30). The game clock stands still in
this mode: a scene sets what it shows with `ChessConnector.setScreenshotClock()`.

## The underlying switches (ChessGui.exe)

`--screenshot <file.png> --scene <name> [--moves "..."] [--black] [--dark] [--high-contrast] [--size WxH] [--delay ms]`,
`--screenshot` also sets `QT_ENABLE_HIGHDPI_SCALING=0` (unless already set). `--data-dir <dir>` (also usable without `--screenshot`) keeps
users, settings (ini instead of registry), saves and logs in `<dir>` — use it to run the app
interactively on a throwaway profile. Exit codes: 0 saved, 1 grab/save failed, 2 timed out,
3 QML failed to load, 4 unknown scene. Implementation: `main.cpp` (options, grab) and
`ScreenshotScene.qml` `setUp()` (loaded by `ChessGame.qml` only in screenshot mode); `ChessConnector::ScreenshotScene` carries the options to QML.

## A scene with other settings

The Settings page's options (`qml/AppSettings.qml`) are QtCore `Settings`; with `--data-dir <dir>` they
are read from `<dir>\ChessPlusPlus_Inc\ChessPlusPlus.ini`. Write that file first, then run the exe
directly (the script makes its own temporary folder):

```powershell
New-Item -ItemType Directory -Force "$dir\ChessPlusPlus_Inc" | Out-Null
"[General]`r`ncoordinatesInside=true`r`nhighlightLastMove=false" | Set-Content "$dir\ChessPlusPlus_Inc\ChessPlusPlus.ini"
& x64\Release\ChessGui.exe --screenshot "$PWD\test-results\screens\game-inside.png" --scene game --moves "e2e4 e7e5" --data-dir $dir | Out-Null
```

`| Out-Null` waits for the app (`$LASTEXITCODE` is its exit code). Not `Start-Process -ArgumentList`: it doesn't
quote, so only the first of `--moves "e2e4 e7e5"` arrives.
The scene's `--dark` (or light) still overrides `theme`.

## Interactive run (when a scene doesn't cover it)

```powershell
Start-Process x64\Release\ChessGui.exe -ArgumentList '--data-dir', "$env:TEMP\chess-profile"
```

To capture a running window: `GetWindowRect` on the process's `MainWindowHandle` +
`System.Drawing.Graphics.CopyFromScreen`, save to the scratchpad, Read it. Clicking/dragging by
coordinates is fragile — prefer adding a scene to `ScreenshotScene.qml`. Stop the process afterwards.
