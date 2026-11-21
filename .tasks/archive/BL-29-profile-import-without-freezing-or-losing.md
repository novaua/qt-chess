---
id: BL-29
title: "Profile import and export without freezing the window; a Replace import that can't lose the old profile"
type: bug
status: done
priority: medium
---

# BL-29 Profile import and export without freezing the window; a Replace import that can't lose the old profile

Found by the async review of 2026-11-21.

**Found:**
- **Data loss (the bug):** a Replace import removes the existing profile's folder first (`UserManager.cpp:1085`, `removeRecursively()`), then writes the imported files ignoring each write's result (`:1095`, `writeFileAtomically`). A full disk or a file locked by antivirus part-way through leaves a profile with games missing, the old ones gone, and the import reported as done.
- **Freeze:** `inspectImport`, `importProfile` and `exportProfile` (`UserManager.cpp` ~975–1119) run on the GUI thread. The whole `.chessprofile` is read and decoded twice (inspect, then import), `GameLibrary::checkImport` parses and HMACs every game, and every file is a `QSaveFile` commit (flushed to disk, then renamed). With "Games kept" at 500 (its maximum) plus the unfinished games: 500+ flushed writes, estimated 0.5–2 s on an SSD, seconds on an HDD ("Not responding"). Not measured yet.
- Export silently skips a game file it can't read.

**How to see it:** `TestRun.addFinishedGames(500)`, export, import with Replace, timed (`[Timing]`). The loss: a target file read-only or locked during the import.

**Fix:**
1. Replace: write into a staging folder (`users/<id>.importing/`), check every write, then swap the folders by rename (old aside, new in, old deleted). On a failure: the staging folder removed, the old profile untouched, an error returned.
2. The read, check and write on a worker (a worker QObject on a QThread: no new Qt module; or `QtConcurrent::run(...).then(this, …)`: the `concurrent` module and `Qt6Concurrent.dll` in the installer). The worker gets paths and bytes only; the GUI thread then updates `_users`, saves and emits. Still one call per user action (with a finished signal); the dialogs show a busy state.
3. Export reports a file it couldn't read.

**Test:** `UserManagerTest`: a profile with many games still round-trips; a write failure during Replace leaves the old profile intact and returns an error; export reports an unreadable file. UI: the profile import/export flows (`tst_profiles`) wait for the finished signal and pass.

## Resolution

### 2026-11-21 (branch: bugs/safeProfileImport_211126)
- **The data loss, fixed:** `importProfile` no longer touches the profile before every file is written. The files go into `users/<id>.importing`, each write checked; then the folders are swapped by rename (the old one to `<id>.replaced`, removed once `users.json` is saved). A failed write, or an old folder that can't be moved (a file in it open), returns the error and leaves the old profile and `users.json` as they were. A new-id import that fails adds nothing. `load()` recovers a crash mid-swap (`recoverInterruptedImports`: a staging folder removed, an old folder put back when the new one is missing). Export fails with "Couldn't read games/…" on a game file it can't read instead of leaving it out.
- **The freeze, measured** (`ExportImport_500Games_Timing`, 500 games, Release; `CHESS_TEST_DATA_DIR` runs it on another disk): with a flushed `QSaveFile` per file a Replace import took 3.0–3.3 s on the internal NVMe SSD (C:, with Defender) and 1.7–1.8 s on a USB hard disk; reading and checking the games is ~20 ms, the rest was the flush after each file. The staged files are now written unflushed (`AppConfig::writeFileUnflushed`; the owner's choice): 0.67 s on C:, 0.9 s on the USB disk. Export: 0.12–0.2 s. The only extra risk: a power cut in the seconds right after an import could damage imported files; the `.chessprofile` is still there to import again.
- **Step 2 (a worker thread) not needed** at these times; reopen if a slower disk shows a freeze (the `[Timing] profile import` line in the log says).
- Tests: `UserManagerTest` new `Import_Replace_WriteFails_OldProfileKept`, `Import_Replace_FolderLocked_OldProfileKept`, `Import_New_WriteFails_NoProfileAdded`, `Load_RecoversAnInterruptedReplace`, `Export_UnreadableGameFile_Fails`, `ExportImport_500Games_Timing`; ChessQtTests 345/345 (Release); UI suite 13 files, 0 failed; Debug and Release build.
