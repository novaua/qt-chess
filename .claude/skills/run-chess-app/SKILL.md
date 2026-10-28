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
(`D:\Tools\Qt\6.10.2\msvc2022_64\bin`); nothing is deployed next to the exe.

## Screenshots (preferred way to look at a view)

`scripts\Take-Screenshot.ps1` runs the app in screenshot mode: fresh temporary profile (the
user's own users/settings/saves are never touched), opens a scene, saves a PNG, exits. The window
is real but transparent, frameless, without a taskbar button and ignores the mouse — nothing shows
on screen. Shots are at 100% scale whatever the monitor scaling. Then **Read the PNG** to see it.

```powershell
.\scripts\Take-Screenshot.ps1 -Scene all                                    # every scene
.\scripts\Take-Screenshot.ps1 -Scene board -Moves "e2e4 e7e5 g1f3"          # position after moves
.\scripts\Take-Screenshot.ps1 -Scene board -Moves "f2f3 e7e5 g2g4 d8h4" -Name foolsmate  # game end
.\scripts\Take-Screenshot.ps1 -Scene board -Black -Dark -Size 1400x900      # flipped, dark, landscape
```

- Scenes: `create-user`, `login`, `menu`, `board` (two-player game), `settings`, `levels`, `profile`,
  `online` (Lichess dialog, not connected), `players` (second-player picker), `all`.
- To review many shots at once, tile them into one contact sheet (System.Drawing) and Read that.
- Output: `test-results\screens\<scene>[-black][-dark][-<Name>].png` (git-ignored). Exit code = number of failed scenes.
- `-Size WxH`: default 768x1054 (portrait). Wider than 1.15 × height shows the move-history panel
  and the side result panel instead of the result card.
- `-Moves` are UCI (`e7e8q` for promotion); an illegal move stops the list there (see the log).
- `-Delay ms` (default 1000) if an animation hasn't finished. A finished game waits for the result card itself.
- On failure the script prints the exit code meaning and the tail of `<file>.png.log`
  (the app's qDebug/QML warnings) — read it for QML errors.

**Don't use `-platform offscreen`** for looks: it can't force light/dark, and the FluentWinUI3
style then uses its dark palette (white text on our light surfaces, invisible buttons). Fine for
checking that QML loads without errors, not for colours.

Not covered yet: single-player vs the robot (needs Stockfish; a stub engine is planned), a
connected Lichess account and the challenge dialog. Plan for the full UI test suite: `docs/UiTestSuitePlan.md`.

## The underlying switches (ChessGui.exe)

`--screenshot <file.png> --scene <name> [--moves "..."] [--black] [--dark] [--size WxH] [--delay ms]`,
`--screenshot` also sets `QT_ENABLE_HIGHDPI_SCALING=0` (unless already set). `--data-dir <dir>` (also usable without `--screenshot`) keeps
users, settings (ini instead of registry), saves and logs in `<dir>` — use it to run the app
interactively on a throwaway profile. Exit codes: 0 saved, 1 grab/save failed, 2 timed out,
3 QML failed to load, 4 unknown scene. Implementation: `main.cpp` (options, grab) and
`ChessGame.qml` `setUpScreenshotScene()`; `ChessConnector::ScreenshotScene` carries the options to QML.

## Interactive run (when a scene doesn't cover it)

```powershell
Start-Process x64\Release\ChessGui.exe -ArgumentList '--data-dir', "$env:TEMP\chess-profile"
```

To capture a running window: `GetWindowRect` on the process's `MainWindowHandle` +
`System.Drawing.Graphics.CopyFromScreen`, save to the scratchpad, Read it. Clicking/dragging by
coordinates is fragile — prefer adding a scene to `setUpScreenshotScene()`. Stop the process afterwards.
