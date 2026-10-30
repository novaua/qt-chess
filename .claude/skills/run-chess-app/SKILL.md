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
  playing? with Bob, Alice and Tester, who has a Lichess account), `home`, `continue` (Home with an autosaved game), `game` (two-player
  game, Tester vs Bob), `review` (a middlegame reviewed at move 5), `result` (Fool's mate: the result
  card), `ended` (the same with the card closed: Rematch / New game in the moves panel, Result in the sheet), `moves` (the middlegame with the move sheet open; narrow window only), `select` (1. e4 e5, the g1 knight selected: tinted square and its move dots; with `-Black` on the turned board), `settings` (Settings page, top),
  `about` (Settings scrolled to About: version, licences, shortcuts), `restore` (Settings with the restore
  dialog: two backups, the newest the same as now), `levels`,
  `profile` (Profile page: Lichess linked with ratings, statistics), `profile-local` (no Lichess),
  `profile-expired` (Lichess sign-in expired: Reconnect), `import` (importing Bob again: the conflict
  dialog), `online` (Lichess dialog, not connected), `computer` (vs-computer setup dialog), `players`
  (two-player setup dialog), `all`. `robot` (not in `all`: needs Stockfish): a saved game vs the robot at
  level 5, continued, where the robot mates with Qh5# (the engine's answer is played on the GUI thread).
  `login`, `menu` and `board` still work as the old names of `profiles`, `home` and `game`.
  The profile scenes get their profiles, Lichess link (a placeholder token, no network) and statistics
  from `seedScreenshotProfiles()` in `main.cpp`.
  `-Moves` replaces a scene's own moves.
- To review many shots at once, tile them into one contact sheet (System.Drawing) and Read that.
- Output: `test-results\screens\<scene>[-black][-dark][-hc][-<Name>].png` (git-ignored). Exit code = number of failed scenes.
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

Not covered yet: a game vs the robot without Stockfish (a stub engine is planned), a
connected Lichess account and the challenge dialog. Sending keys to an interactive window
(SendKeys) doesn't reach the Qt controls reliably and takes over the user's desktop: add a scene instead. Plan for the full UI test suite: `docs/UiTestSuitePlan.md`.

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
`-MaxChangedPercent` (0.05 %). Two runs of all 44 scenes (22 light, 22 dark) match exactly. A baseline without a new
screenshot fails as `MISSING` (Take-Screenshot.ps1 deletes the old PNG first, so a failed scene leaves
none): take the same scenes as the baselines, or use `-Name`. Needs PowerShell 7. A scene that shows
the clock must use fixed times (`restore`: backups at 9:00 and 9:30).

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
