---
id: BL-31
title: "Keep the app log from growing forever"
type: task
status: done
priority: low
---

# BL-31 Keep the app log from growing forever

Found by the async review of 2026-11-21.

**Found:** `logs/ChessPlusPlus_AppDebug.log` (`main.cpp:492`) is opened in append mode (`applog.cpp:26`) and never rotated. `qDebug` is on in Release, and every Lichess stream line is logged, keep-alives included (`LichessStream.cpp:84`): about 40–50 KB an hour per open stream (estimate), plus every request (`makeRequest`).

**Fix:** at start, a log over ~5 MB is renamed to `.1` (an older `.1` replaced); empty keep-alive lines aren't logged (or only under a debug category).

**Test:** a log over the limit is rotated when opened, and the new one starts empty (ChessQtTests).

Also seen (cleanup, no ticket): `ChessGui/ChessGui/networkplayer.cpp/.h` (UDP multicast) is in no project: dead code.

## Resolution

### 2026-11-21 (branch: features/rotateAppLog_211126)
The owner's choice: roll over at 5 MB, 10 files in all.
- `AppLog` (`applog.h/cpp`): a log opened for appending rolls over at 5 MB, at open (a log left full by the last run) and while the app runs (checked after each line, under the mutex already held): `name.log` → `name.1.log`, the older ones up one, `name.9.log` dropped; the new file starts with "Log continued from ….1.log". A log that can't be moved (another Chess++ has it open) is appended to, nothing lost, and tried again after a tenth of the limit more. Truncate mode (screenshot scenes) never rolls over. `setLimits` for the tests.
- `LichessStream`: empty keep-alive lines aren't logged (still emitted, still feed the watchdog).
- Tests: `AppLogTest` 7/7 (new: `RotatesWhenFull`, `OpenRotatesAnOldBigLog`, `OpenAppendsBelowTheLimit`, `TruncateNeverRotates`, `RotationBlockedKeepsLogging`, `ConcurrentWritesWhileRotating`); ChessQtTests 339/339 (Release); UI suite 13 files, 0 failed. By hand: the app started on a 6.4 MB log moved it to `.1.log` and began a new one with "Log continued from".
