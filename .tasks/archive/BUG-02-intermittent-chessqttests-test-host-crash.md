---
id: BUG-02
title: "Intermittent `ChessQtTests` test host crash"
type: bug
status: done
priority: medium
---

# BUG-02 Intermittent `ChessQtTests` test host crash

<!-- migrated from TASKS.md:70 (In Progress / Done) -->

- [x] Intermittent `ChessQtTests` test host crash (#71) — leaked `UciConnector` in `MainInit_IsGoood_Test` outlived the `QCoreApplication`; `.runsettings` git-ignored
