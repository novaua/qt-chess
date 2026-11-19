---
id: TSK-01
title: "Screenshots on an unlocked screen"
type: task
status: done
priority: medium
---

# TSK-01 Screenshots on an unlocked screen

<!-- migrated from TASKS.md:38 (In Progress) -->

- [x] **Screenshots on an unlocked screen** — done 2026-11-09 (branch `bl10ScreenChecks_091126`): all 88 compared scenes match, 12 new baselines accepted after a look, the Decline… menu is a scene now (`online-challenges-decline`) (owed by LI-05 / LI-06, 2026-11-09: the screen was locked during those runs, `LogonUI` running). On a locked screen popups don't draw in the hidden window, animations freeze and text renders slightly differently, so these could not be checked. Qt's `bin` on `PATH` first (`$env:PATH = "D:\Tools\Qt\6.12.0\msvc2022_64\bin;$env:PATH"`), screen unlocked and the session not minimised:
  - [x] `.\scripts\Take-Screenshot.ps1 -Scene all` and `-Scene all -Dark`, then `.\scripts\Compare-Screenshots.ps1`: expect 0 failed. Failed while locked: the dialog scenes (`computer`, `players`, `players-many`, `create-user`, `import`, `levels`, `restore`, `leave-online`, `online-challenge`, light and dark) and two timeouts (`clock-promotion`, `clock-continue`).
  - [x] `premove-played` and `premove-dropped` (light and dark) differed by 0.06 %, only in the move-list text: they should match again; if they still differ, look at the diff in `test-results\diffs\` and re-accept them only if it is text rendering.
  - [x] `reconnecting` (new, LI-06): look at the picture (the caution bar over the board), then accept it: `.\scripts\Compare-Screenshots.ps1 -Update -Name reconnecting,reconnecting-dark`.
  - [x] `robot` in a real window (needs Stockfish; it timed out while locked, passed `-Offscreen`).
  - [x] `premove-robot` (needs Stockfish, no baseline): three or four moves, Nf3 played after the robot's first answer.
  - [x] `challenge`, `challenge-unplayable`, `challenge-ingame`, `online-challenges` (new, BL-10 steps 3–4): look at them, then accept them: `.\scripts\Compare-Screenshots.ps1 -Update -Name challenge,challenge-unplayable,challenge-ingame,online-challenges` (and `-dark` after a `-Dark` run). Also open the Online page's Decline… menu once by hand (popups don't draw on a locked screen).
