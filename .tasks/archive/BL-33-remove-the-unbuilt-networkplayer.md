---
id: BL-33
title: "Remove the unbuilt networkplayer sources"
type: task
status: done
priority: low
---

# BL-33 Remove the unbuilt networkplayer sources

Found in BL-32 step 3 (2026-11-22): `ChessGui/ChessGui/networkplayer.h` and `networkplayer.cpp` are in no project (`ChessGui.vcxproj`, `ChessGui.pro`, the test projects): an early sketch of network play (commits `0abe433`, `dec3983`: "Added non-yet-functional network player"). Network play is Lichess now (`LiChessConnector`).

To do: `git rm` both files; check that nothing includes `networkplayer.h` (`grep -rn networkplayer`); build ChessGui and the tests.

## Resolution

### 2026-11-26 — removed (branch: features/bl33RemoveNetworkPlayer_261126)
- `git rm` of `ChessGui/ChessGui/networkplayer.h` and `networkplayer.cpp`; its line in `ChessGui/ChessGui/CLAUDE.md`
  dropped. Nothing included `networkplayer.h` (only a stale `Debug/moc_networkplayer.cpp`, an ignored build
  artifact of an old qmake build).
- Checked: `ChessCore.sln` Release builds every project (ChessGui, ChessGuiTests, ChessQtTests, ChessTests);
  `Run-UiTests.ps1`: 13 files green.
