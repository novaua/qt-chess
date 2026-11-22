---
id: BL-39
title: "The UI tests and screenshot runs share the app's QML disk cache: a running app read a replaced .qmlc and asserted"
type: bug
status: open
priority: high
---

# BL-39 The UI tests and screenshot runs share the app's QML disk cache

Seen 2026-11-22 18:3x: a Debug `ChessGui.exe` (started 18:00 from Visual Studio, built 17:59 from `develop` with
BL-35 steps 1–4) stopped in Qt's assert `slot < CallData::HeaderSize() / sizeof(QV4::StaticValue) +
frame->jsFrame->argc() + frame->v4Function->compiledFunction->nRegisters` (`qv4vme_moth.cpp:322`) when the
Profile page was opened from Home's account menu.

## What was found (the process, attached read-only: cdb `-pv`, and the V4 frames read from memory)
- JS stack: `HomePage.qml:104` onTriggered → `ChessGame.qml:506` onProfileRequested → `openProfile()` →
  `_openPage()` (StackView push) → a function of `ProfilePage.qml` whose compiled record is all zeros (no name,
  line 0, 0 registers): the interpreter ran garbage.
- `ProfilePage.qml`'s compilation unit had been loaded from the disk cache as a read-only file mapping
  (`MEM_MAPPED`, 48 KB, `qv4cdata`): its functions 0–13 fine, from function 14 on (exactly from the second 4 KB
  page) every record zeros. The cache file on disk
  (`%LOCALAPPDATA%\ChessPlusPlus_Inc\ChessPlusPlus\cache\qmlcache\3ded5439….qmlc`) is valid at those offsets: it
  was rewritten at 18:10:09, while the app ran, and the pages the app hadn't read yet came back empty.
- The rewrite came from a Release `ChessGui.exe --screenshot` run (Take-Screenshot.ps1) of another checkout of
  `ProfilePage.qml` (a `git stash` had changed its timestamp, so its cache entry no longer matched and was
  compiled and saved again).

## Cause
Every build and run with the same organization and application name (`ChessPlusPlus_Inc` / `ChessPlusPlus`) uses
one QML disk cache: the app (Debug and Release), `ChessGuiTests` (its `main.cpp` sets the same names) and the
`--screenshot` runs (which get a temporary data folder, but the cache is `QStandardPaths::CacheLocation`, not the
data folder). A test or screenshot run of other QML sources (another branch, a stash, an edit) rewrites the
cache entries of a running app; a unit the app has mapped then reads zeros on its pages not yet read.

## Fix to make
- `TestSetup` (before the QML engine exists): `QML_DISK_CACHE_PATH` to a folder of its own, e.g. under the test's
  data folder (or `QML_DISABLE_DISK_CACHE=1`).
- `main.cpp` in `--screenshot` mode: the same, in its temporary data folder.
- Check that Qt 6.12 takes `QML_DISK_CACHE_PATH` (the name is in `Qt6Qml.dll`); a test that the app's cache
  folder isn't written by a test run.
- Until then: don't keep the app running from Visual Studio while `Run-UiTests.ps1` or `Take-Screenshot.ps1`
  run, or delete the cache folder after a test run.
