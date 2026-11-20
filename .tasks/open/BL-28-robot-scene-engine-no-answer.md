---
id: BL-28
title: "The installer smoke test's robot scene timed out once: the continued game's engine never answered"
type: bug
status: in_progress
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

Correction (2026-11-21): the log doesn't show that the engine never answered. A passing run's log has no line between the `continue vs Robot` timing and `Screenshot … saved` either: in Release nothing logs the robot's request, its answer, the gate (MoveGate) holding it until the board settles, the mate or the result card. The scene waits for each of them in turn (`EngineThinking`, then `AppInfo.engineState` not "checking", then the result card: `GamePage._resultShown`), and a hang in any of them leaves the same log.

## Resolution

### 2026-11-21 — step 1: the scene says where it stopped (branch: bugs/robotSceneEngineNoAnswer_211126)

Cause still not known; looked at and ruled out:
- The engine thread: every UCI wait is bounded (start 5 s, handshake 10 s, each read 10 s); a hang there ends in `[Engine] move error`, which would have been logged.
- The answer dropped: every path that moves `_engineRequest` on also clears `EngineThinking`.
- The gate shut by the old game page: its `Component.onDestruction` reports `setBoardSettled(false)`; if it came after the new page's report (1 s after it opens), the gate would hold the answer for good. Measured: the old page goes ~1 ms after the old engine's thread finishes (338 ms locally; in the failed run's log that line came at 177 ms), long before the new page's second, so not in this run.
- The engine probe: bounded at 5 s, then "missing", never left on "checking".

Done, so the next occurrence names the step:
- `ENGINE_TRACE` is now the logging category `chess.engine` (debug messages on in Debug builds, off in Release unless a rule turns them on); `--screenshot` mode turns it on (`main.cpp`), so every scene's log (also `installer\Test-Installer.ps1`'s and CI's) has the robot's path: asked, posted, worker started, answered, held until the board is idle, played; plus the board's settled reports (the gate's other half).
- A scene that times out logs `ChessConnector::robotState()` (the robot's thread, thinking, request, the gate: holding, board settled, idle; plies, result, review) and what the scene waited for (`ScreenshotScene._waitingFor`: the robot's answer, the engine probe, the result card; on the game page also its open second, settled, result pending / shown).

Checked: the `robot` scene 40 times in a row locally (Release): all passed, no repro. Its log now shows `ask: request 3 posted` → `worker: request 3 answers d1h5` → `held until the board is idle` → `board settled` → `playing` → `played`. A forced timeout (the scene's 30 s cut to 2 s for one local build, FakeUci 5 s) logged:
`Screenshot: robot: running, thinking 1, request 3, auto-play 1; gate: holding 0, board settled 0, idle 0; 4 plies, no result, review 0` and `Screenshot: the scene waited for the robot's answer (EngineThinking); page game, open a second false, settled false, …`. UI tests (Release): 13 files, 0 failed; ChessQtTests ChessEnginePlayer: 7 passed.

Left: wait for a next occurrence and read its log (keep the job's `installer-test-results` artifact), then fix the step it names.
