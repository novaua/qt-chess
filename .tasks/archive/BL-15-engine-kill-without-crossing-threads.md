---
id: BL-15
title: "Engine kill without crossing threads"
type: task
status: done
priority: medium
---

# BL-15 Engine kill without crossing threads

<!-- migrated from TASKS.md:59 (In Progress / Done) -->

- [x] BL-15 Engine kill without crossing threads (branch `engineKillFix_311026`): `UciConnector::Kill()` terminates the engine through its own Win32 process handle and never touches `QProcess` from the GUI thread; `EngineWorker` sets an abandoned flag and kills via an atomic connector pointer; `stopEngineThread()` waits 5 s, terminates, and otherwise leaves a running thread instead of deleting it (the abort). The `robot` scene, which ends a game while the engine thinks, crashed in 2 of 3 offscreen runs before; 5 of 5 pass after.

## Resolution

### 2026-10-31 — BL-15 fixed: the engine thread stops without crossing threads (branch: engineKillFix_311026)

#### What was done
- The `robot` scene — it ends a game while the engine is still thinking — crashed at shutdown in **2 of 3** offscreen runs: `QThread: Destroyed while thread '' is still running`. The steps: `stopEngineThread()` called `killEngine()` → `QProcess::kill()`/`state()` **from the GUI thread** (QProcess is thread-affine), which could fail to unblock the engine thread's UCI read; after `wait(3000)` it deleted the `QThread` anyway, and Qt aborts on a running thread.
- **`UciConnector::Kill()`** now terminates the engine through its own Win32 process handle: `OpenProcess(PROCESS_TERMINATE)` after a successful start, `TerminateProcess` on Kill, `CloseHandle` on restart/destruction. The handle is owned by a `std::unique_ptr` with a `CloseHandle` deleter and sits behind a mutex; QProcess is never touched from another thread, and a failed `OpenProcess`/`TerminateProcess` is logged. A stale pid can't kill an unrelated process (the handle pins the process object).
- **`EngineWorker`:** `killEngine()` sets an `_abandoned` atomic and calls the player's thread-safe `KillEngine()` through an atomic player pointer stored when the player is created (no shared_ptr read from the GUI thread). `findMove()` gives up before creating the engine and again before searching; `ChessEnginePlayer::FindMove` checks the same flag after it restarts a dead engine, so a restart cannot slip through a stop.
- **`stopEngineThread()`:** kill → `quit()` → wait 3 s. If it doesn't stop, the thread is **left behind** (`setParent(nullptr)`, pointers dropped) — never `QThread::terminate()`: TerminateThread can kill the thread inside the heap or the log lock and leave it held. The thinking state is cleared on both paths.
- **`UciConnector::Init()`**'s `uci`/`uciok` handshake now has a 10 s deadline (it previously looped forever on an engine that starts but never answers — a thread that could not be stopped cleanly).
- New test `EngineWorkerTest.Kill_StopsAThinkingWorker_FromAnotherThread_Test`: the worker on its own `QThread`, a 2.5 s search, `killEngine()` from the test thread, the thread must exit within 2.5 s.
- `ChessEnginePlayer::KillEngine()` stays as the thread-safe wrapper the engine tests use.
- Verified: `robot` scene **5/5** offscreen runs pass (before: 1/3); Debug and Release build; ChessTests 111, ChessQtTests **111** (the new test) pass in both.

#### Code review (medium, same branch)
14 findings; fixed:
- **`QThread::terminate()` removed** — TerminateThread can leave the CRT heap or `AppLog`'s mutex locked (the GUI thread then hangs); the leave-behind path is now the only fallback, and both stop paths share one cleanup tail.
- The terminate path's worker/process leak is gone with it.
- **Stop during Init/restart**: the cancel check inside `FindMove` closes the window where a restart started after `killEngine()`.
- **A hung handshake** no longer lasts forever (10 s deadline).
- **Silent handle failures** are logged (`OpenProcess` pid + error, `TerminateProcess` error).
- **Raw `HANDLE` as `void*`** replaced by a `std::unique_ptr` with a `CloseHandle` deleter.
- **No automated test** for the cross-thread kill — added.
- **Late sweep addendum:** `IsRunning()` now reports a killed engine immediately (`_killed` flag), so `ChessEnginePlayer::FindMove`'s restart recovery cannot send the next request to the process that was just terminated; test `IsRunning_IsFalseRightAfterKill_Test`.
Not changed (with reasons): the GUI can still wait up to 3 s when the kill misses (the engine process normally dies in ms; the alternative, `terminate()`, is worse); on non-Windows `Kill()` is a no-op (the app is Windows-only); the public `Connector()` accessor was removed in favour of the atomic player pointer.

#### Where to continue
- BL-15 is merged (#104). BL-14 step 4 (`resultOnIdle_311026`) is rebased onto it and its `robot` scene re-run (3/3); next is step 5 (the Lichess opponent through the gate).
