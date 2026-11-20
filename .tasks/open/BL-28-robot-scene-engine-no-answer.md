---
id: BL-28
title: "The installer smoke test's robot scene timed out once: the continued game's engine never answered"
type: bug
status: open
priority: low
---

# BL-28 The installer smoke test's robot scene timed out once

Seen once, on PR #210 (run 44757912261, 2026-11-20): `installer\Test-Installer.ps1`, scene `robot`, "FAIL robot - exit code 2: timed out waiting for the scene". The re-run of the same job passed; every installer run since the smoke test came (2026-11-12) had passed. PR #210 changed only `LichessClient`, nothing on this path.

What the scene does (`ScreenshotScene.qml`, `name === "robot"`): `startComputerGame(5, 1)`, `applyMoves(...)`, `endGame()` (the first engine is stopped, not waited for), then `continueGame(id)`: the robot (FakeUci, `FAKEUCI_MOVES`) must play Qh5#; the scene waits while `ChessConnector.EngineThinking`.

The app log (`robot.png.log`):
- `[Timing] engine stop: the old engine finished 177 ms later (not waited for)`
- `[Timing] continue vs Robot: 166 ms to the first frame (stop engine 0, load 0, start engine 0, ...)`
- then nothing for 77 s until the scene's timeout: no `[Engine] move error` (that would have ended the wait), no answer.

So the continued game's engine (started while the stopped one was still finishing) never answered and never failed: a hang in its start or a lost request. Cause not known. To look at: `ChessConnector::startEngineThread` / `stopEngineThread` (BL-16: stop without waiting), `EngineWorker`'s start with a slow runner; the engine trace (`ENGINE_TRACE`) is off in that log. A next occurrence: keep the job's `installer-test-results` artifact.
