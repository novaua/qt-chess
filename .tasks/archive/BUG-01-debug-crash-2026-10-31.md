---
id: BUG-01
title: "Debug crash 2026-10-31"
type: bug
status: done
priority: medium
---

# BUG-01 Debug crash 2026-10-31

<!-- migrated from TASKS.md:60 (In Progress / Done) -->

- [x] Debug crash 2026-10-31: the log handler wrote the same `QFile` from the GUI and engine threads; the heap corruption surfaced as a `QRingBuffer` assert on the engine thread. Fixed with a `QMutex` + flush per line (branch `logRaceFix_311026`); report, dump analysis and repro in [`docs/bugs/2026-10-31-qringbuffer-assert-crash.md`](docs/bugs/2026-10-31-qringbuffer-assert-crash.md)

## Resolution

### 2026-10-31 — Debug crash fixed: the log handler wrote from two threads (branch: logRaceFix_311026)

#### What was done
- The owner's **Debug** build crashed during a normal robot game: Qt's `ASSERT: "bytes <= bufferSize"` in `QRingBuffer::free()` (`qringbuffer.cpp:76`, Qt 6.12.0). The screenshot, dump analysis and a standalone repro are in [`docs/bugs/2026-10-31-qringbuffer-assert-crash.md`](docs/bugs/2026-10-31-qringbuffer-assert-crash.md).
- **Dump:** the faulting thread was the engine worker (`0x476c0`), inside `EngineWorker::findMove` → the Qt read path of its `QProcess`; no `stopEngineThread`/`killEngine` frames. **Log:** in 1,985 lines there was exactly one duplicated consecutive line — `ask: request 37 posted` twice — at the crash moment, i.e. the GUI and engine threads wrote the log at the same time. Missing request numbers (4, 8, 11) show the owner pressing Undo while robot answers were held.
- **Root cause:** `customLogMessageHandler()` writes the shared `g_logFile` with a `QTextStream`, and Qt calls message handlers from any thread without serializing them. Two streams writing the same `QFile` corrupt the heap; the corruption surfaced as the ring-buffer assert in the engine thread. In Release there is no assert, so it would be silent.
- **Reproduced:** a standalone Qt console program with the same handler and two logging threads dies with `STATUS_HEAP_CORRUPTION` after 4 lines, duplicate line included; with a `QMutex` + flush around the write it writes 40,001 intact lines and exits cleanly.
- **Fix:** the log writer is now `AppLog` (`ChessGui/applog.h/.cpp`): `open` / `close` / `write`, with a `QMutex` + `QMutexLocker` around each and a flush per line so a crash keeps the last lines. `main.cpp`'s message handler calls it.
- **Regression test** (`ChessQtTests/AppLogTest.cpp`): two threads write 2,000 lines each; it requires the exact line count, the documented format, and every line exactly once. It was written first against the unsynchronized writer: the test host died with no results (`vstest` exit 3) — the same crash as the app, now caught by the suite; with the mutex it passes in 0.7 s.
- Debug and Release build; ChessTests 111, ChessQtTests 110 pass; `home` and `robot` scenes unchanged.

#### Where to continue
- **The owner re-tests the Debug build**: normal robot play, pressing Undo often; the duplicated log lines and the assert should be gone. The stress program in the report is the regression test.
- **BL-15 (new, TASKS):** `stopEngineThread()` still calls `QProcess::kill()`/`state()` from the GUI thread while the engine thread uses the same `QProcess`, reads `_enginePlayer` unsynchronized, and deletes the thread even if `wait(3000)` timed out — a separate cross-thread hazard, documented in the crash report.
- BL-14 step 4 is next (PR #102 is merged).
