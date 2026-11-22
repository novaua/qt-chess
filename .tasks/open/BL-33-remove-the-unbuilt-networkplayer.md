---
id: BL-33
title: "Remove the unbuilt networkplayer sources"
type: task
status: open
priority: low
---

# BL-33 Remove the unbuilt networkplayer sources

Found in BL-32 step 3 (2026-11-22): `ChessGui/ChessGui/networkplayer.h` and `networkplayer.cpp` are in no project (`ChessGui.vcxproj`, `ChessGui.pro`, the test projects): an early sketch of network play (commits `0abe433`, `dec3983`: "Added non-yet-functional network player"). Network play is Lichess now (`LiChessConnector`).

To do: `git rm` both files; check that nothing includes `networkplayer.h` (`grep -rn networkplayer`); build ChessGui and the tests.
